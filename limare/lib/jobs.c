/*
 * Copyright (c) 2011-2013 Luc Verhaegen <libv@skynet.be>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/*
 * Simplistic job handling.
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <inttypes.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <asm/ioctl.h>

#define u32 uint32_t
#include "linux/mali_ioctl.h"

#include "version.h"
#include "linux/ioctl.h"
#include "limare.h"
#include "jobs.h"

static _mali_uk_wait_for_notification_s wait;
static pthread_mutex_t wait_mutex = PTHREAD_MUTEX_INITIALIZER;

static void *
wait_for_notification(void *arg)
{
	struct limare_state *state = arg;
	int ret;

	pthread_mutex_lock(&wait_mutex);

	do {
		do {
			wait.code.timeout = 25;
			ret = ioctl(state->fd, MALI_IOC_WAIT_FOR_NOTIFICATION,
				    &wait);
			if (ret == -1) {
				printf("%s: Error: wait failed: %s\n",
				       __func__, strerror(errno));
				exit(-1);
			}
		} while (wait.code.type == _MALI_NOTIFICATION_CORE_TIMEOUT);

		if ((wait.code.type & 0xFF) == 0x10)
			break;

		printf("%s: %x: %x\n", __func__, wait.code.type,
		       wait.data.gp_job_suspended.reason);
	} while (1);

	pthread_mutex_unlock(&wait_mutex);

	return NULL;
}

static void
wait_for_notification_start(struct limare_state *state)
{
	pthread_t thread;

	pthread_create(&thread, NULL, wait_for_notification, state);
}

static void
limare_jobs_wait(void)
{
	pthread_mutex_lock(&wait_mutex);
	pthread_mutex_unlock(&wait_mutex);
}

static int
limare_gp_job_start_r2p1(struct limare_state *state,
			 struct limare_frame *frame,
			 struct lima_gp_frame_registers *frame_regs)
{
	struct lima_gp_job_start_r2p1 job = { 0 };
	int ret;

	wait_for_notification_start(state);

	job.fd = state->fd;
	job.user_job_ptr = (unsigned int) &job;
	job.priority = 1;
	job.watchdog_msecs = 0;
	job.frame = *frame_regs;
	job.abort_id = 0;
	ret = ioctl(state->fd, LIMA_GP_START_JOB_R2P1, &job);
	if (ret == -1) {
		printf("%s: Error: failed to start job: %s\n",
		       __func__, strerror(errno));
		return errno;
	}

	limare_jobs_wait();

	/* wait for the plb to be fully written out */
	usleep(8500);

	return 0;
}

static int
limare_gp_job_start_r3p0(struct limare_state *state,
			 struct limare_frame *frame,
			 struct lima_gp_frame_registers *frame_regs)
{
	struct lima_gp_job_start_r3p0 job = { 0 };
	int ret;

	wait_for_notification_start(state);

	job.fd = state->fd;
	job.user_job_ptr = (unsigned int) &job;
	job.priority = 1;
	job.frame = *frame_regs;
	ret = ioctl(state->fd, LIMA_GP_START_JOB_R3P0, &job);
	if (ret == -1) {
		printf("%s: Error: failed to start job: %s\n",
		       __func__, strerror(errno));
		return errno;
	}

	limare_jobs_wait();

	/* wait for the plb to be fully written out */
	usleep(8500);

	return 0;
}

int
limare_gp_job_start(struct limare_state *state, struct limare_frame *frame)
{
	struct lima_gp_frame_registers frame_regs = { 0 };
	int ret;

	frame_regs.vs_commands_start = frame->vs_commands_physical;
	frame_regs.vs_commands_end =
		frame->vs_commands_physical + 8 * frame->vs_commands_count;
	frame_regs.plbu_commands_start = frame->plbu_commands_physical;
	frame_regs.plbu_commands_end =
		frame->plbu_commands_physical + 8 * frame->plbu_commands_count;
	frame_regs.tile_heap_start =
		frame->mem_physical + frame->tile_heap_offset;
	frame_regs.tile_heap_end = frame->mem_physical +
		frame->tile_heap_offset + frame->tile_heap_size;

	if (state->kernel_version < MALI_DRIVER_VERSION_R3P0)
		ret = limare_gp_job_start_r2p1(state, frame, &frame_regs);
	else
		ret = limare_gp_job_start_r3p0(state, frame, &frame_regs);

	return ret;
}

int
limare_m200_pp_job_start_direct(struct limare_state *state,
				struct limare_frame *frame,
				struct lima_m200_pp_frame_registers *frame_regs,
				struct lima_pp_wb_registers *wb_regs)
{
	struct lima_m200_pp_job_start job = { 0 };
	int ret;

	wait_for_notification_start(state);

	job.fd = state->fd;
	job.user_job_ptr = (unsigned int) &job;
	job.priority = 1;
	job.watchdog_msecs = 0;
	job.frame = *frame_regs;
	job.wb[0] = *wb_regs;
	job.abort_id = 0;

	ret = ioctl(state->fd, LIMA_M200_PP_START_JOB, &job);
	if (ret == -1) {
		printf("%s: Error: failed to start job: %s\n",
		       __func__, strerror(errno));
		return errno;
	}

	limare_jobs_wait();

	return 0;
}

static int
limare_m400_pp_job_start_r2p1(struct limare_state *state,
			      struct limare_frame *frame,
			      struct lima_m400_pp_frame_registers *frame_regs,
			      struct lima_pp_wb_registers *wb_regs)
{
	struct lima_m400_pp_job_start_r2p1 job = { 0 };
	int ret;

	wait_for_notification_start(state);

	job.fd = state->fd;
	job.user_job_ptr = (unsigned int) &job;
	job.priority = 1;
	job.watchdog_msecs = 0;
	job.frame = *frame_regs;
	job.wb[0] = *wb_regs;
	job.abort_id = 0;

	ret = ioctl(state->fd, LIMA_M400_PP_START_JOB_R2P1, &job);
	if (ret == -1) {
		printf("%s: Error: failed to start job: %s\n",
		       __func__, strerror(errno));
		return errno;
	}

	limare_jobs_wait();

	return 0;
}

static int
limare_m400_pp_job_start_r3p0(struct limare_state *state,
			      struct limare_frame *frame,
			      struct lima_m400_pp_frame_registers *frame_regs,
			      struct lima_pp_wb_registers *wb_regs)
{
	struct lima_m400_pp_job_start_r3p0 job = { 0 };
	int ret;

	wait_for_notification_start(state);

	job.fd = state->fd;
	job.user_job_ptr = (unsigned int) &job;
	job.priority = 1;
	job.frame = *frame_regs;
	job.wb0 = *wb_regs;
	job.num_cores = 1;

	ret = ioctl(state->fd, LIMA_M400_PP_START_JOB_R3P0, &job);
	if (ret == -1) {
		printf("%s: Error: failed to start job: %s\n",
		       __func__, strerror(errno));
		return errno;
	}

	limare_jobs_wait();

	return 0;
}

int
limare_m400_pp_job_start_direct(struct limare_state *state,
				struct limare_frame *frame,
				struct lima_m400_pp_frame_registers *frame_regs,
				struct lima_pp_wb_registers *wb_regs)
{
	if (state->kernel_version < MALI_DRIVER_VERSION_R3P0)
		return limare_m400_pp_job_start_r2p1(state, frame,
						     frame_regs, wb_regs);
	else
		return limare_m400_pp_job_start_r3p0(state, frame,
						     frame_regs, wb_regs);
}
