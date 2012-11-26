/*
 * Copyright (c) 2011-2012 Luc Verhaegen <libv@skynet.be>
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
#if 0
	pthread_mutex_lock(&wait_mutex);
#else
	/*
	 * On linux-sunxi with linaro userspace, threading seems to have quite
	 * some issues. Seems that our main thread does not get woken up with
	 * pthread_mutex_lock. This workaround is a lot more expensive, but
	 * at least works.
	 */
	while (1) {
		if (pthread_mutex_trylock(&wait_mutex))
			sched_yield();
		else
			break;
	}
#endif
	pthread_mutex_unlock(&wait_mutex);
}

static int
limare_gp_job_start_r2p1(struct limare_state *state,
			 struct lima_gp_frame_registers *frame)
{
	struct lima_gp_job_start_r2p1 job = { 0 };
	int ret;

	wait_for_notification_start(state);

	job.fd = state->fd;
	job.user_job_ptr = (unsigned int) &job;
	job.priority = 1;
	job.watchdog_msecs = 0;
	job.frame = *frame;
	job.abort_id = 0;
	ret = ioctl(state->fd, LIMA_GP_START_JOB_R2P1, &job);
	if (ret == -1) {
		printf("%s: Error: failed to start job: %s\n",
		       __func__, strerror(errno));
		return errno;
	}

	limare_jobs_wait();

	return 0;
}

static int
limare_gp_job_start_r3p0(struct limare_state *state,
			 struct lima_gp_frame_registers *frame)
{
	struct lima_gp_job_start_r3p0 job = { 0 };
	int ret;

	wait_for_notification_start(state);

	job.fd = state->fd;
	job.user_job_ptr = (unsigned int) &job;
	job.priority = 1;
	job.frame = *frame;
	ret = ioctl(state->fd, LIMA_GP_START_JOB_R3P0, &job);
	if (ret == -1) {
		printf("%s: Error: failed to start job: %s\n",
		       __func__, strerror(errno));
		return errno;
	}

	limare_jobs_wait();

	return 0;
}

int
limare_gp_job_start_direct(struct limare_state *state,
			   struct lima_gp_frame_registers *frame)
{
	if (state->kernel_version < MALI_DRIVER_VERSION_R3P0)
		return limare_gp_job_start_r2p1(state, frame);
	else
		return limare_gp_job_start_r3p0(state, frame);
}

int
limare_m200_pp_job_start_direct(struct limare_state *state,
				struct lima_m200_pp_frame_registers *frame,
				struct lima_pp_wb_registers *wb)
{
	struct lima_m200_pp_job_start job = { 0 };
	int ret;

	wait_for_notification_start(state);

	job.fd = state->fd;
	job.user_job_ptr = (unsigned int) &job;
	job.priority = 1;
	job.watchdog_msecs = 0;
	job.frame = *frame;
	job.wb[0] = *wb;
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
			      struct lima_m400_pp_frame_registers *frame,
			      struct lima_pp_wb_registers *wb)
{
	struct lima_m400_pp_job_start_r2p1 job = { 0 };
	int ret;

	wait_for_notification_start(state);

	job.fd = state->fd;
	job.user_job_ptr = (unsigned int) &job;
	job.priority = 1;
	job.watchdog_msecs = 0;
	job.frame = *frame;
	job.wb[0] = *wb;
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
			      struct lima_m400_pp_frame_registers *frame,
			      struct lima_pp_wb_registers *wb)
{
	struct lima_m400_pp_job_start_r3p0 job = { 0 };
	int ret;

	wait_for_notification_start(state);

	job.fd = state->fd;
	job.user_job_ptr = (unsigned int) &job;
	job.priority = 1;
	job.frame = *frame;
	job.wb0 = *wb;
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
				struct lima_m400_pp_frame_registers *frame,
				struct lima_pp_wb_registers *wb)
{
	if (state->kernel_version < MALI_DRIVER_VERSION_R3P0)
		return limare_m400_pp_job_start_r2p1(state, frame, wb);
	else
		return limare_m400_pp_job_start_r3p0(state, frame, wb);
}
