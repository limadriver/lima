/*
 * Copyright (c) 2011-2012 Luc Verhaegen <libv@codethink.co.uk>
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

#define u32 uint32_t
#include "linux/mali_ioctl.h"

#include "linux/ioctl.h"
#include "limare.h"
#include "jobs.h"

static _mali_uk_wait_for_notification_s wait;
static pthread_mutex_t wait_mutex = PTHREAD_MUTEX_INITIALIZER;

static void *
wait_for_notification(void *arg)
{
	struct premali_state *state = arg;
	int ret;

	pthread_mutex_lock(&wait_mutex);

	do {
		wait.code.timeout = 25;
		ret = ioctl(state->fd, MALI_IOC_WAIT_FOR_NOTIFICATION, &wait);
		if (ret == -1) {
			printf("%s: Error: wait failed: %s\n",
			       __func__, strerror(errno));
			exit(-1);
		}

		sched_yield();
	} while (wait.code.type == _MALI_NOTIFICATION_CORE_TIMEOUT);

	printf("%s: done!\n", __func__);

	pthread_mutex_unlock(&wait_mutex);

	return NULL;
}

void
wait_for_notification_start(struct premali_state *state)
{
	pthread_t thread;

	pthread_create(&thread, NULL, wait_for_notification, state);
}

void
premali_jobs_wait(void)
{
	pthread_mutex_lock(&wait_mutex);
	pthread_mutex_unlock(&wait_mutex);
}

int
premali_gp_job_start_direct(struct premali_state *state,
			    struct mali_gp_job_start *job)
{
	int ret;

	/* now run the job */
	premali_jobs_wait();

	wait_for_notification_start(state);

	job->fd = state->fd;
	job->user_job_ptr = (unsigned int) job;
	job->priority = 1;
	job->watchdog_msecs = 0;
	job->abort_id = 0;
	ret = ioctl(state->fd, MALI_GP_START_JOB, job);
	if (ret == -1) {
		printf("%s: Error: failed to start job: %s\n",
		       __func__, strerror(errno));
		return errno;
	}

	return 0;
}

int
premali_m200_pp_job_start_direct(struct premali_state *state,
				 struct mali200_pp_job_start *job)
{
	int ret;

	premali_jobs_wait();

	wait_for_notification_start(state);

	job->fd = state->fd;
	job->user_job_ptr = (unsigned int) job;
	job->priority = 1;
	job->watchdog_msecs = 0;
	job->abort_id = 0;

	ret = ioctl(state->fd, MALI200_PP_START_JOB, job);
	if (ret == -1) {
		printf("%s: Error: failed to start job: %s\n",
		       __func__, strerror(errno));
		return errno;
	}

	return 0;
}

int
premali_m400_pp_job_start_direct(struct premali_state *state,
				 struct mali400_pp_job_start *job)
{
	int ret;

	premali_jobs_wait();

	wait_for_notification_start(state);

	job->fd = state->fd;
	job->user_job_ptr = (unsigned int) job;
	job->priority = 1;
	job->watchdog_msecs = 0;
	job->abort_id = 0;

	ret = ioctl(state->fd, MALI400_PP_START_JOB, job);
	if (ret == -1) {
		printf("%s: Error: failed to start job: %s\n",
		       __func__, strerror(errno));
		return errno;
	}

	return 0;
}
