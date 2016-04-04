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
#include "pp.h"

static pthread_mutex_t gp_job_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t gp_job_cond = PTHREAD_COND_INITIALIZER;
static unsigned int gp_job_done;

static pthread_mutex_t pp_job_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t pp_job_cond = PTHREAD_COND_INITIALIZER;
static unsigned int pp_job_done;

static void
limare_gp_job_done(unsigned int id)
{
	int ret;

	ret = pthread_mutex_lock(&gp_job_mutex);
	if (ret)
		printf("%s: error locking mutex: %s\n", __func__,
		       strerror(ret));

        gp_job_done = id;

        pthread_cond_broadcast(&gp_job_cond);

        ret = pthread_mutex_unlock(&gp_job_mutex);
	if (ret)
		printf("%s: error unlocking mutex: %s\n", __func__,
		       strerror(ret));
}

static void
limare_gp_job_wait(struct limare_frame *frame)
{
	unsigned int job_id = frame->id | 0x80000000;
	int ret;

	ret = pthread_mutex_lock(&gp_job_mutex);
	if (ret)
		printf("%s: error locking mutex: %s\n", __func__,
		       strerror(ret));

	while (gp_job_done < job_id)
		pthread_cond_wait(&gp_job_cond, &gp_job_mutex);

	ret = pthread_mutex_unlock(&gp_job_mutex);
	if (ret)
		printf("%s: error unlocking mutex: %s\n", __func__,
		       strerror(ret));
}

static void
limare_pp_job_done(unsigned int id)
{
	pthread_mutex_lock(&pp_job_mutex);

        pp_job_done = id;

        pthread_cond_broadcast(&pp_job_cond);

        pthread_mutex_unlock(&pp_job_mutex);
}

static void
limare_pp_job_wait(struct limare_frame *frame)
{
	unsigned int job_id = frame->id | 0xC0000000;

	pthread_mutex_lock(&pp_job_mutex);

	while (pp_job_done < job_id)
		pthread_cond_wait(&pp_job_cond, &pp_job_mutex);

        pthread_mutex_unlock(&pp_job_mutex);
}

static void *
limare_notification_thread(void *arg)
{
	struct limare_state *state = arg;
	static _mali_uk_wait_for_notification_s wait = { 0 };
	int request;
	int ret;

	if (state->kernel_version < MALI_DRIVER_VERSION_R3P1)
		request = MALI_IOC_WAIT_FOR_NOTIFICATION;
	else
		request = MALI_IOC_WAIT_FOR_NOTIFICATION_R3P1;

	while (1) {
		while (1) {
			do {
				wait.code.timeout = 500;
				ret = ioctl(state->fd, request, &wait);
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
				printf("pp job 0x%08X returned 0x%08X\n",
				       wait.data.pp_job_finished.user_job_ptr,
				       status);

			limare_pp_job_done(wait.data.pp_job_finished.user_job_ptr);
		} else if (wait.code.type == _MALI_NOTIFICATION_GP_FINISHED) {
			_mali_uk_job_status status =
				wait.data.gp_job_finished.status;

			if (status != _MALI_UK_JOB_STATUS_END_SUCCESS)
				printf("gp job returned 0x%08X\n", status);

			limare_gp_job_done(wait.data.pp_job_finished.user_job_ptr);
		}
	}

	return NULL;
}

static pthread_t limare_notification_pthread;

static pthread_mutex_t gp_job_time_mutex = PTHREAD_MUTEX_INITIALIZER;
static long long gp_job_time;

void
limare_gp_job_bench_start(struct timespec *start)
{
	if (clock_gettime(CLOCK_MONOTONIC, start)) {
		printf("Error: failed to get time: %s\n", strerror(errno));
		return;
	}
}

void
limare_gp_job_bench_stop(struct timespec *start)
{
	struct timespec new = { 0 };
	long long total;

	if (clock_gettime(CLOCK_MONOTONIC, &new)) {
		printf("Error: failed to get time: %s\n", strerror(errno));
		return;
	}

	total = (new.tv_sec - start->tv_sec) * 1000000;
	total += (new.tv_nsec - start->tv_nsec) / 1000;

	pthread_mutex_lock(&gp_job_time_mutex);
	gp_job_time += total;
	pthread_mutex_unlock(&gp_job_time_mutex);
}

static pthread_mutex_t pp_job_time_mutex = PTHREAD_MUTEX_INITIALIZER;
static long long pp_job_time;

void
limare_pp_job_bench_start(struct timespec *start)
{
	if (clock_gettime(CLOCK_MONOTONIC, start)) {
		printf("Error: failed to get time: %s\n", strerror(errno));
		return;
	}
}

void
limare_pp_job_bench_stop(struct timespec *start)
{
	struct timespec new = { 0 };
	long long total;

	if (clock_gettime(CLOCK_MONOTONIC, &new)) {
		printf("Error: failed to get time: %s\n", strerror(errno));
		return;
	}

	total = (new.tv_sec - start->tv_sec) * 1000000;
	total += (new.tv_nsec - start->tv_nsec) / 1000;

	pthread_mutex_lock(&pp_job_time_mutex);
	pp_job_time += total;
	pthread_mutex_unlock(&pp_job_time_mutex);
}

static int
limare_gp_job_start_r2p1(struct limare_state *state,
			 struct limare_frame *frame,
			 struct lima_gp_frame_registers *frame_regs)
{
	struct lima_gp_job_start_r2p1 job = { 0 };
	int ret;

	job.fd = state->fd;
	job.user_job_ptr = frame->id | 0x80000000;
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

int
limare_gp_job_start_r3p0(struct limare_state *state,
			 struct limare_frame *frame,
			 struct lima_gp_frame_registers *frame_regs)
{
	struct lima_gp_job_start_r3p0 job = { 0 };
	int ret;

	job.fd = state->fd;
	job.user_job_ptr = frame->id | 0x80000000;
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

static int
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
	job.user_job_ptr = frame->id | 0xC0000000;
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

	return 0;
}

int
limare_m400_pp_job_start_r2p1(struct limare_state *state,
			      struct limare_frame *frame,
			      struct lima_m400_pp_frame_registers *frame_regs,
			      struct lima_pp_wb_registers *wb_regs)
{
	struct lima_m400_pp_job_start_r2p1 job = { 0 };
	int ret;

	job.fd = state->fd;
	job.user_job_ptr = frame->id | 0xC0000000;
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

	return 0;
}

int
limare_m400_pp_job_start_r3p0(struct limare_state *state,
			      struct limare_frame *frame,
			      struct lima_m400_pp_frame_registers *frame_regs,
			      unsigned int addr_frame[7],
			      unsigned int addr_stack[7],
			      struct lima_pp_wb_registers *wb_regs)
{
	struct lima_m400_pp_job_start_r3p0 job = { 0 };
	int ret;

	job.fd = state->fd;
	job.user_job_ptr = frame->id | 0xC0000000;
	job.priority = 1;
	job.frame = *frame_regs;

	memcpy(&job.addr_frame, addr_frame, 7 * 4);
	memcpy(&job.addr_stack, addr_stack, 7 * 4);

	job.wb0 = *wb_regs;
	job.num_cores = state->pp_core_count;

	ret = ioctl(state->fd, LIMA_M400_PP_START_JOB_R3P0, &job);
	if (ret == -1) {
		printf("%s: Error: failed to start job: %s\n",
		       __func__, strerror(errno));
		return errno;
	}

	return 0;
}

int
limare_m400_pp_job_start_r3p1(struct limare_state *state,
			      struct limare_frame *frame,
			      struct lima_m400_pp_frame_registers *frame_regs,
			      unsigned int addr_frame[7],
			      unsigned int addr_stack[7],
			      struct lima_pp_wb_registers *wb_regs)
{
	struct lima_m400_pp_job_start_r3p1 job = { 0 };
	int ret;

	job.fd = state->fd;
	job.user_job_ptr = frame->id | 0xC0000000;
	job.priority = 1;
	job.frame = *frame_regs;

	memcpy(&job.addr_frame, addr_frame, 7 * 4);
	memcpy(&job.addr_stack, addr_stack, 7 * 4);

	job.wb0 = *wb_regs;
	job.num_cores = state->pp_core_count;

	ret = ioctl(state->fd, LIMA_M400_PP_START_JOB_R3P0, &job);
	if (ret == -1) {
		printf("%s: Error: failed to start job: %s\n",
		       __func__, strerror(errno));
		return errno;
	}

	return 0;
}

int
limare_m400_pp_job_start_r3p2(struct limare_state *state,
			      struct limare_frame *frame,
			      struct lima_m400_pp_frame_registers *frame_regs,
			      unsigned int addr_frame[7],
			      unsigned int addr_stack[7],
			      struct lima_pp_wb_registers *wb_regs)
{
	struct lima_m400_pp_job_start_r3p2 job = { 0 };
	int ret;

	job.fd = state->fd;
	job.user_job_ptr = frame->id | 0xC0000000;
	job.priority = 1;
	job.frame = *frame_regs;

	memcpy(&job.addr_frame, addr_frame, 7 * 4);
	memcpy(&job.addr_stack, addr_stack, 7 * 4);

	job.wb0 = *wb_regs;
	job.num_cores = state->pp_core_count;
	job.fence = -1;
	job.stream = -1;

	ret = ioctl(state->fd, LIMA_M400_PP_START_JOB_R3P0, &job);
	if (ret == -1) {
		printf("%s: Error: failed to start job: %s\n",
		       __func__, strerror(errno));
		return errno;
	}

	return 0;
}

static void
limare_render_sequence(struct limare_state *state, struct limare_frame *frame)
{
	struct timespec start;

	pthread_mutex_lock(&frame->mutex);

	/*
	 * First, push the job through the gp.
	 */
	limare_gp_job_bench_start(&start);

	limare_gp_job_start(state, frame);

	limare_gp_job_wait(frame);

	limare_gp_job_bench_stop(&start);

	/*
	 * Now we can work on the pp.
	 */
	limare_pp_job_bench_start(&start);

	limare_pp_job_start(state, frame);

	limare_pp_job_wait(frame);

	limare_pp_job_bench_stop(&start);

        /* wait for display sync, and flip the current fb. */
	limare_fb_flip(state, frame);

	frame->render_status = 2;
	pthread_mutex_unlock(&frame->mutex);
}

static pthread_mutex_t render_0_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t render_0_cond = PTHREAD_COND_INITIALIZER;
static struct limare_frame *render_0_frame;
static int render_0_stop;

static pthread_mutex_t render_1_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t render_1_cond = PTHREAD_COND_INITIALIZER;
static struct limare_frame *render_1_frame;
static int render_1_stop;

static void *
limare_render_0_thread(void *arg)
{
	struct limare_state *state = arg;
	int ret;

	ret = pthread_mutex_lock(&render_0_mutex);
	if (ret)
		printf("%s: error locking mutex: %s\n", __func__,
		       strerror(ret));

	while (!render_0_stop) {
		while (!render_0_frame && !render_0_stop) {
			ret = pthread_cond_wait(&render_0_cond,
						&render_0_mutex);
			if (ret)
				printf("%s: cond wait error: %s\n", __func__,
				       strerror(ret));
		}

		if (render_0_frame) {
			limare_render_sequence(state, render_0_frame);

			render_0_frame = NULL;
		}
	}

	ret = pthread_mutex_unlock(&render_0_mutex);
	if (ret)
		printf("%s: error locking mutex: %s\n", __func__,
		       strerror(ret));

	return NULL;
}

static pthread_t limare_render_0_pthread;

static void
limare_render_0_start(struct limare_frame *frame)
{
	int ret, done = 0;

	while (!done) {
		ret = pthread_mutex_lock(&render_0_mutex);
		if (ret)
			printf("%s: error locking mutex: %s\n", __func__,
			       strerror(ret));

		if (!render_0_frame) {
			render_0_frame = frame;
			done = 1;
		}

		if (render_0_frame)
			pthread_cond_broadcast(&render_0_cond);

		ret = pthread_mutex_unlock(&render_0_mutex);
		if (ret)
			printf("%s: error locking mutex: %s\n", __func__,
			       strerror(ret));
	}
}

static void
limare_render_0_stop(void)
{
	void *retval;
	int ret;

	ret = pthread_mutex_lock(&render_0_mutex);
	if (ret)
		printf("%s: error locking mutex: %s\n", __func__,
		       strerror(ret));

	render_0_stop = 1;
	pthread_cond_broadcast(&render_0_cond);

	ret = pthread_mutex_unlock(&render_0_mutex);
	if (ret)
		printf("%s: error locking mutex: %s\n", __func__,
		       strerror(ret));

	ret = pthread_join(limare_render_0_pthread, &retval);
	if (ret)
		printf("%s: error joining thread: %s\n", __func__,
		       strerror(ret));
}

static void *
limare_render_1_thread(void *arg)
{
	struct limare_state *state = arg;
	int ret;

	ret = pthread_mutex_lock(&render_1_mutex);
	if (ret)
		printf("%s: error locking mutex: %s\n", __func__,
		       strerror(ret));

	while (!render_1_stop) {
		while (!render_1_frame && !render_1_stop) {
			ret = pthread_cond_wait(&render_1_cond,
						&render_1_mutex);
			if (ret)
				printf("%s: cond wait error: %s\n", __func__,
				       strerror(ret));
		}

		if (render_1_frame) {
			limare_render_sequence(state, render_1_frame);

			render_1_frame = NULL;
		}
	}

	ret = pthread_mutex_unlock(&render_1_mutex);
	if (ret)
		printf("%s: error locking mutex: %s\n", __func__,
		       strerror(ret));

	return NULL;
}

static pthread_t limare_render_1_pthread;

static void
limare_render_1_start(struct limare_frame *frame)
{
	int ret, done = 0;

	while (!done) {
		ret = pthread_mutex_lock(&render_1_mutex);
		if (ret)
			printf("%s: error locking mutex: %s\n", __func__,
			       strerror(ret));

		if (!render_1_frame) {
			render_1_frame = frame;
			done = 1;
		}

		if (render_1_frame)
			pthread_cond_broadcast(&render_1_cond);

		ret = pthread_mutex_unlock(&render_1_mutex);
		if (ret)
			printf("%s: error locking mutex: %s\n", __func__,
			       strerror(ret));
	}
}

static void
limare_render_1_stop(void)
{
	void *retval;
	int ret;

	ret = pthread_mutex_lock(&render_1_mutex);
	if (ret)
		printf("%s: error locking mutex: %s\n", __func__,
		       strerror(ret));

	render_1_stop = 1;
	pthread_cond_broadcast(&render_1_cond);

	ret = pthread_mutex_unlock(&render_1_mutex);
	if (ret)
		printf("%s: error locking mutex: %s\n", __func__,
		       strerror(ret));

	ret = pthread_join(limare_render_1_pthread, &retval);
	if (ret)
		printf("%s: error joining thread: %s\n", __func__,
		       strerror(ret));
}

void
limare_render_start(struct limare_frame *frame)
{
	if (frame->index)
		limare_render_1_start(frame);
	else
		limare_render_0_start(frame);
}

static struct timespec jobs_time;

void
limare_jobs_init(struct limare_state *state)
{
	int ret;

	ret = pthread_create(&limare_notification_pthread, NULL,
			     limare_notification_thread, state);
	if (ret)
		printf("%s: error starting thread: %s\n", __func__,
		       strerror(ret));

	ret = pthread_create(&limare_render_0_pthread, NULL,
			     limare_render_0_thread, state);
	if (ret)
		printf("%s: error starting thread: %s\n", __func__,
		       strerror(ret));

	ret = pthread_create(&limare_render_1_pthread, NULL,
			     limare_render_1_thread, state);
	if (ret)
		printf("%s: error starting thread: %s\n", __func__,
		       strerror(ret));

	if (clock_gettime(CLOCK_MONOTONIC, &jobs_time))
		printf("Error: failed to get time: %s\n", strerror(errno));
}

void
limare_jobs_end(struct limare_state *state)
{
	struct timespec new = { 0 };
	long long total;

	limare_render_0_stop();
	limare_render_1_stop();

	if (clock_gettime(CLOCK_MONOTONIC, &new)) {
		printf("Error: failed to get time: %s\n", strerror(errno));
		return;
	}

	total = (new.tv_sec - jobs_time.tv_sec) * 1000000;
	total += (new.tv_nsec - jobs_time.tv_nsec) / 1000;

	printf("Total jobs time: %f seconds\n", (float) total / 1000000);
	printf("   GP job  time: %f seconds\n", (float) gp_job_time / 1000000);
	printf("   PP job  time: %f seconds\n", (float) pp_job_time / 1000000);
}

