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

#include "plb.h"
#include "fb.h"

static pthread_mutex_t gp_job_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t gp_job_cond = PTHREAD_COND_INITIALIZER;
static int gp_job_done;

static pthread_mutex_t pp_job_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t pp_job_cond = PTHREAD_COND_INITIALIZER;
static int pp_job_done;

static void
limare_gp_job_done(void)
{
	int ret;

	ret = pthread_mutex_lock(&gp_job_mutex);
	if (ret)
		printf("%s: error locking mutex: %s\n", __func__,
		       strerror(ret));

        gp_job_done = 1;

        pthread_cond_broadcast(&gp_job_cond);

        ret = pthread_mutex_unlock(&gp_job_mutex);
	if (ret)
		printf("%s: error unlocking mutex: %s\n", __func__,
		       strerror(ret));
}

static void
limare_gp_job_wait(void)
{
	int ret;

	ret = pthread_mutex_lock(&gp_job_mutex);
	if (ret)
		printf("%s: error locking mutex: %s\n", __func__,
		       strerror(ret));

	while (!gp_job_done)
		pthread_cond_wait(&gp_job_cond, &gp_job_mutex);

	gp_job_done = 0;

	ret = pthread_mutex_unlock(&gp_job_mutex);
	if (ret)
		printf("%s: error unlocking mutex: %s\n", __func__,
		       strerror(ret));
}

static void
limare_pp_job_done(void)
{
	pthread_mutex_lock(&pp_job_mutex);

        pp_job_done = 1;

        pthread_cond_broadcast(&pp_job_cond);

        pthread_mutex_unlock(&pp_job_mutex);
}

static void
limare_pp_job_wait(void)
{
	pthread_mutex_lock(&pp_job_mutex);

	while (!pp_job_done)
		pthread_cond_wait(&pp_job_cond, &pp_job_mutex);

	pp_job_done = 0;

        pthread_mutex_unlock(&pp_job_mutex);
}

static void *
limare_notification_thread(void *arg)
{
	struct limare_state *state = arg;
	static _mali_uk_wait_for_notification_s wait = { 0 };
	int ret;

	while (1) {
		while (1) {
			do {
				wait.code.timeout = 500;
				ret = ioctl(state->fd,
					    MALI_IOC_WAIT_FOR_NOTIFICATION,
					    &wait);
				if (ret == -1) {
					printf("%s: Error: wait failed: %s\n",
					       __func__, strerror(errno));
					exit(-1);
				}
			} while (wait.code.type ==
				 _MALI_NOTIFICATION_CORE_TIMEOUT);

			if ((wait.code.type & 0xFF) == 0x10)
				break;

			printf("%s: %x: %x\n", __func__, wait.code.type,
			       wait.data.gp_job_suspended.reason);
		}

		if (wait.code.type == _MALI_NOTIFICATION_PP_FINISHED) {
			_mali_uk_job_status status =
				wait.data.pp_job_finished.status;

			if (status != _MALI_UK_JOB_STATUS_END_SUCCESS)
				printf("pp job returned 0x%08X\n", status);

			limare_pp_job_done();
		} else if (wait.code.type == _MALI_NOTIFICATION_GP_FINISHED) {
			_mali_uk_job_status status =
				wait.data.gp_job_finished.status;

			if (status != _MALI_UK_JOB_STATUS_END_SUCCESS)
				printf("gp job returned 0x%08X\n", status);

			limare_gp_job_done();
		}
	}

	return NULL;
}

static pthread_t limare_notification_pthread;

static int
limare_gp_job_start_r2p1(struct limare_state *state,
			 struct limare_frame *frame,
			 struct lima_gp_frame_registers *frame_regs)
{
	struct lima_gp_job_start_r2p1 job = { 0 };
	int ret;

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

	return 0;
}

static int
limare_gp_job_start_r3p0(struct limare_state *state,
			 struct limare_frame *frame,
			 struct lima_gp_frame_registers *frame_regs)
{
	struct lima_gp_job_start_r3p0 job = { 0 };
	int ret;

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

	if (ret)
		return ret;

	limare_gp_job_wait();

	return 0;
}

int
limare_m200_pp_job_start_direct(struct limare_state *state,
				struct limare_frame *frame,
				struct lima_m200_pp_frame_registers *frame_regs,
				struct lima_pp_wb_registers *wb_regs)
{
	struct lima_m200_pp_job_start job = { 0 };
	int ret;

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

	limare_pp_job_wait();

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

	limare_pp_job_wait();

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

	limare_pp_job_wait();

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

void
limare_jobs_init(struct limare_state *state)
{
	int ret;

	ret = pthread_create(&limare_notification_pthread, NULL,
			     limare_notification_thread, state);
	if (ret)
		printf("%s: error starting thread: %s\n", __func__,
		       strerror(ret));
}
