/*
 * Copyright 2011      Luc Verhaegen <libv@codethink.co.uk>
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
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#define u32 uint32_t
#define USING_MALI200
#include "mali_200_regs.h"
#include "mali_ioctl.h"

int dev_mali_fd;

/*
 *
 * Replay a dump of mali ioctls and memory.
 *
 */

struct mali_dumped_mem_content {
	unsigned int offset;
	unsigned int size;
	unsigned int memory[];
};

struct mali_dumped_mem_block {
	void *address;
	unsigned int physical;
	unsigned int size;
	int count;
	struct mali_dumped_mem_content *contents[];
};

struct mali_dumped_mem {
	int count;
	struct mali_dumped_mem_block *blocks[];
};

#include "dumped_stream.c"

void
mali_dumped_mem_content_load(void *address,
			     struct mali_dumped_mem_content *contents[],
			     int count)
{
	int i;

	for (i = 0; i < count; i++) {
		struct mali_dumped_mem_content *content = contents[i];

		memcpy(address + content->offset,
		       content->memory, content->size);
	}
}

_mali_uk_wait_for_notification_s wait;
pthread_mutex_t wait_mutex = PTHREAD_MUTEX_INITIALIZER;

void *
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

	pthread_mutex_unlock(&wait_mutex);

	return NULL;
}

void
wait_for_notification_start(void)
{
	pthread_t thread;

	pthread_create(&thread, NULL, wait_for_notification, NULL);
}

int
main(int argc, char *argv[])
{
	_mali_uk_init_mem_s mem_init;
	int ret, i;

	dev_mali_fd = open("/dev/mali", O_RDWR);
	if (dev_mali_fd == -1) {
		printf("Error: Failed to open /dev/mali: %s\n",
		       strerror(errno));
		return errno;
	}

	mem_init.ctx = (void *) dev_mali_fd;
	mem_init.mali_address_base = 0;
	mem_init.memory_size = 0;
	ret = ioctl(dev_mali_fd, MALI_IOC_MEM_INIT, &mem_init);
	if (ret == -1) {
		printf("Error: ioctl MALI_IOC_MEM_INIT failed: %s\n",
		       strerror(errno));
		return errno;
	}

	/* blink, and it's gone! */
	wait_for_notification_start();

	for (i = 0; i < dumped_mem.count; i++) {
		struct mali_dumped_mem_block *block = dumped_mem.blocks[i];

		block->address = mmap(NULL, block->size, PROT_READ | PROT_WRITE,
				      MAP_SHARED, dev_mali_fd, block->physical);
		if (block->address == MAP_FAILED) {
			printf("Error: failed to mmap offset 0x%x (0x%x): %s\n",
			       block->physical, block->size, strerror(errno));
			return errno;
		} else
			printf("Mapped 0x%x (0x%x) to %p\n", block->physical,
			       block->size, block->address);

		mali_dumped_mem_content_load(block->address, block->contents,
					     block->count);
	}

	gp_job.ctx = (void *) dev_mali_fd;
	ret = ioctl(dev_mali_fd, MALI_IOC_GP2_START_JOB, &gp_job);
	if (ret == -1) {
		printf("Error: ioctl MALI_IOC_GP2_START_JOB failed: %s\n",
		       strerror(errno));
		return errno;
	}

	pthread_mutex_lock(&wait_mutex);
	pthread_mutex_unlock(&wait_mutex);

	wait_for_notification_start();

	pp_job.ctx = (void *) dev_mali_fd;
	ret = ioctl(dev_mali_fd, MALI_IOC_PP_START_JOB, &pp_job);
	if (ret == -1) {
		printf("Error: ioctl MALI_IOC_PP_START_JOB failed: %s\n",
		       strerror(errno));
		return errno;
	}

	pthread_mutex_lock(&wait_mutex);

	fflush(stdout);

	return 0;
}
