/*
 * Copyright (c) 2011-2012 Luc Verhaegen <libv@codethink.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
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
#define USING_MALI200
#include "mali_200_regs.h"
#include "mali_ioctl.h"

#include "premali.h"
#include "jobs.h"

static _mali_uk_wait_for_notification_s wait;
static pthread_mutex_t wait_mutex = PTHREAD_MUTEX_INITIALIZER;

static void *
wait_for_notification(void *ignored)
{
	int ret;

	pthread_mutex_lock(&wait_mutex);

	do {
		wait.code.timeout = 25;
		ret = ioctl(dev_mali_fd, MALI_IOC_WAIT_FOR_NOTIFICATION, &wait);
		if (ret == -1) {
			printf("Error: ioctl MALI_IOC_WAIT_FOR_NOTIFICATION failed: %s\n",
			       strerror(errno));
			exit(-1);
		}

		sched_yield();
	} while (wait.code.type == _MALI_NOTIFICATION_CORE_TIMEOUT);

	printf("%s: done!\n", __func__);

	pthread_mutex_unlock(&wait_mutex);

	return NULL;
}

static void
wait_for_notification_start(void)
{
	pthread_t thread;

	pthread_create(&thread, NULL, wait_for_notification, NULL);
}

int
premali_gp_job_start(_mali_uk_gp_start_job_s *gp_job)
{
	int ret;

	premali_jobs_wait();

	wait_for_notification_start();

	gp_job->ctx = (void *) dev_mali_fd;
	gp_job->user_job_ptr = (u32) gp_job;
	gp_job->priority = 1;
	gp_job->watchdog_msecs = 0;
	gp_job->abort_id = 0;
	ret = ioctl(dev_mali_fd, MALI_IOC_GP2_START_JOB, gp_job);
	if (ret == -1) {
		printf("Error: ioctl MALI_IOC_GP2_START_JOB failed: %s\n",
		       strerror(errno));
		return errno;
	}

	printf("%s: done!\n", __func__);

	return 0;
}

int
premali_pp_job_start(_mali_uk_pp_start_job_s *pp_job)
{
	int ret;

	premali_jobs_wait();

	wait_for_notification_start();

	pp_job->ctx = (void *) dev_mali_fd;
	pp_job->user_job_ptr = (u32) pp_job;
	pp_job->priority = 1;
	pp_job->watchdog_msecs = 0;
	pp_job->abort_id = 0;
	ret = ioctl(dev_mali_fd, MALI_IOC_PP_START_JOB, pp_job);
	if (ret == -1) {
		printf("Error: ioctl MALI_IOC_PP_START_JOB failed: %s\n",
		       strerror(errno));
		return errno;
	}

	printf("%s: done!\n", __func__);

	return 0;
}

void
premali_jobs_wait(void)
{
	pthread_mutex_lock(&wait_mutex);
	pthread_mutex_unlock(&wait_mutex);
}
