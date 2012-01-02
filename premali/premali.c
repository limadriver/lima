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

#include "bmp.h"
#include "fb.h"

#include "formats.h"
#include "registers.h"

#define u32 uint32_t
#define USING_MALI200
#include "mali_200_regs.h"
#include "mali_ioctl.h"

static inline unsigned int
from_float(float x)
{
	return *((unsigned int *) &x);
}

#define ALIGN(x, y) (((x) + ((y) - 1)) & ~((y) - 1))

int dev_mali_fd;
void *image_address;
#define WIDTH 800
#define HEIGHT 480

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

struct plb {
	int block_size; /* 0x200 */

	int width; /* aligned already */
	int height;

	int shift_w;
	int shift_h;

	/* holds the actual primitives */
	int plb_offset;
	int plb_size; /* 0x200 * (width >> (shift_w - 1)) * (height >> (shift_h - 1))) */

	/* holds the addresses so the plbu knows where to store the primitives */
	int plbu_offset;
	int plbu_size; /* 4 * width * height */

	/* holds the coordinates and addresses of the primitives for the PP */
	int pp_offset;
	int pp_size; /* 16 * (width * height + 1) */

	void *mem_address;
	int mem_physical;
	int mem_size;
};

struct plb *
plb_create(int width, int height)
{
	struct plb *plb = calloc(1, sizeof(struct plb));

	width = ALIGN(width, 16) >> 4;
	height = ALIGN(height, 16) >> 4;

	/* limit the amount of plb's the pp has to chew through */
	while ((width * height) > 320) {
		if (width >= height) {
			width = (width + 1) >> 1;
			plb->shift_w++;
		} else {
			height = (height + 1) >> 1;
			plb->shift_h++;
		}
	}

	plb->block_size = 0x200;

	plb->width = width << plb->shift_w;
	plb->height = height << plb->shift_h;

	printf("%s: (%dx%d) == (%d << %d, %d << %d);\n", __func__,
	       plb->width, plb->height, width, plb->shift_w, height, plb->shift_h);

	plb->plb_size = plb->block_size * width * height;
	plb->plb_offset = 0;

	plb->plbu_size = 4 * plb->width * plb->height;
	plb->plbu_offset = ALIGN(plb->plb_size, 0x40);

	plb->pp_size = 16 * (plb->width * plb->height + 1);
	plb->pp_offset = ALIGN(plb->plbu_offset + plb->plbu_size, 0x40);

	plb->mem_address = mem_0x40000000.address + 0x80000;
	plb->mem_physical = 0x40080000;
	/* just align to page size for convenience */
	plb->mem_size = ALIGN(plb->pp_offset + plb->pp_size, 0x1000);

	return plb;
}

void
plb_pp_stream_create(struct plb *plb)
{
	int x, y, i, j;
	int offset = 0, index = 0;
	int step_x = 1 << plb->shift_w;
	int step_y = 1 << plb->shift_h;
	unsigned int address = plb->mem_physical + plb->plb_offset;
	unsigned int *stream = plb->mem_address + plb->pp_offset;

	for (y = 0; y < plb->height; y += step_y) {
		for (x = 0; x < plb->width; x += step_x) {
			for (j = 0; j < step_y; j++) {
				for (i = 0; i < step_x; i++) {
					stream[index + 0] = 0;
					stream[index + 1] = 0xB8000000 | (x + i) | ((y + j) << 8);
					stream[index + 2] = 0xE0000002 | (((address + offset) >> 3) & ~0xE0000003);
					stream[index + 3] = 0xB0000000;

					index += 4;
				}
			}

			offset += plb->block_size;
		}
	}

	stream[index + 0] = 0;
	stream[index + 1] = 0xBC000000;
}

void
plb_plbu_stream_create(struct plb *plb)
{
	int i, size = plb->width * plb->height;
	unsigned int address = plb->mem_physical + plb->plb_offset;
	unsigned int *stream = plb->mem_address + plb->plbu_offset;

	for (i = 0; i < size; i++)
		stream[i] = address + (i * plb->block_size);
}

struct mali_cmd {
	unsigned int val;
	unsigned int cmd;
};

#include "vs.h"

void
vs_commands_create(struct mali_cmd *cmds)
{
	int i = 0;

	cmds[i].val = MALI_VS_CMD_ARRAYS_SEMAPHORE_BEGIN_1;
	cmds[i].cmd = MALI_VS_CMD_ARRAYS_SEMAPHORE;
	i++;

	cmds[i].val = MALI_VS_CMD_ARRAYS_SEMAPHORE_BEGIN_2;
	cmds[i].cmd = MALI_VS_CMD_ARRAYS_SEMAPHORE;
	i++;

	cmds[i].val = 0x40000000; /* vs shader address */
	cmds[i].cmd = MALI_VS_CMD_SHADER_ADDRESS | (7 << 16); /* vs shader size */
	i++;

	cmds[i].val = 0x00401800;
	cmds[i].cmd = 0x10000040;
	i++;

	cmds[i].val = 0x01000100;
	cmds[i].cmd = 0x10000042;
	i++;

	cmds[i].val = 0x40000240; /* uniforms address */
	cmds[i].cmd = MALI_VS_CMD_UNIFORMS_ADDRESS | (3 << 16); /* ALIGN(uniforms_size, 4) / 4 */
	i++;

	cmds[i].val = 0x40000300; /* shared address space for attributes and varyings, half/half */
	cmds[i].cmd = MALI_VS_CMD_COMMON_ADDRESS | (0x40 << 16); /* (guessing) common size / 4 */
	i++;

	cmds[i].val = 0x00000003;
	cmds[i].cmd = 0x10000041;
	i++;

	cmds[i].val = 0x03000000;
	cmds[i].cmd = 0x00000000;
	i++;

	cmds[i].val = 0x00000000;
	cmds[i].cmd = 0x60000000;
	i++;

	cmds[i].val = MALI_VS_CMD_ARRAYS_SEMAPHORE_END;
	cmds[i].cmd = MALI_VS_CMD_ARRAYS_SEMAPHORE;
}

#include "plbu.h"

void
plbu_commands_create(struct mali_cmd *cmds, int width, int height,
		     struct plb *plb)
{
	int i = 0;

	cmds[i].val = plb->shift_w | (plb->shift_h << 16);
	cmds[i].cmd = MALI_PLBU_CMD_BLOCK_STEP;
	i++;

	cmds[i].val = ((plb->width - 1) << 24) | ((plb->height - 1) << 8);
	cmds[i].cmd = MALI_PLBU_CMD_TILED_DIMENSIONS;
	i++;

	cmds[i].val = plb->width >> plb->shift_w;
	cmds[i].cmd = MALI_PLBU_CMD_PLBU_BLOCK_STRIDE;
	i++;

	cmds[i].val = plb->mem_physical + plb->plbu_offset;
	cmds[i].cmd = MALI_PLBU_CMD_PLBU_ARRAY_ADDRESS;
	i++;

#if 0
	cmds[i].val = 0x40100000;
	cmds[i].cmd = MALI_PLBU_CMD_TILE_HEAP_START;
	i++;

	cmds[i].val = 0x40150000;
	cmds[i].cmd = MALI_PLBU_CMD_TILE_HEAP_END;
	i++;
#endif

	cmds[i].val = from_float(0.0);
	cmds[i].cmd = MALI_PLBU_CMD_VIEWPORT_Y;
	i++;

	cmds[i].val = from_float(height);
	cmds[i].cmd = MALI_PLBU_CMD_VIEWPORT_H;
	i++;

	cmds[i].val = from_float(0.0);
	cmds[i].cmd = MALI_PLBU_CMD_VIEWPORT_X;
	i++;

	cmds[i].val = from_float(width);
	cmds[i].cmd = MALI_PLBU_CMD_VIEWPORT_W;
	i++;

	/*
	 *
	 */
	cmds[i].val = MALI_PLBU_CMD_ARRAYS_SEMAPHORE_BEGIN;
	cmds[i].cmd = MALI_PLBU_CMD_ARRAYS_SEMAPHORE;
	i++;

	cmds[i].val = 0x00002200;
	cmds[i].cmd = MALI_PLBU_CMD_PRIMITIVE_SETUP;
	i++;

	cmds[i].val = 0x40000280; /* RSW address */
	cmds[i].cmd = MALI_PLBU_CMD_RSW_VERTEX_ARRAY | (0x40000140 >> 4); /* vertex array address */
	i++;

	cmds[i].val = 0x00000000;
	cmds[i].cmd = 0x1000010a;
	i++;

	cmds[i].val = from_float(0.0);
	cmds[i].cmd = MALI_PLBU_CMD_DEPTH_RANGE_NEAR;
	i++;

	cmds[i].val = from_float(1.0);
	cmds[i].cmd = MALI_PLBU_CMD_DEPTH_RANGE_FAR;
	i++;

	/* could this be some sort of delay or wait? */
	cmds[i].val = 0x03000000;
	cmds[i].cmd = 0x00040000;
	i++;

	cmds[i].val = MALI_PLBU_CMD_ARRAYS_SEMAPHORE_END;
	cmds[i].cmd = MALI_PLBU_CMD_ARRAYS_SEMAPHORE;
	i++;

	/*
	 * Some inter-frame communication apparently.
	 */
#if 0
	cmds[i].val = 0x400e41c0;
	cmds[i].cmd = 0xa0000103;
	i++;

	cmds[i].val = 0x400e41c4;
	cmds[i].cmd = 0xa0000104;
	i++;

	cmds[i].val = 0x400e41c8;
	cmds[i].cmd = 0xa0000107;
	i++;

	cmds[i].val = 0x400e41cc;
	cmds[i].cmd = 0xa0000108;
	i++;

	cmds[i].val = 0x400e41d0;
	cmds[i].cmd = 0xa0000105;
	i++;

	cmds[i].val = 0x400e41d4;
	cmds[i].cmd = 0xa0000106;
	i++;
#endif

	cmds[i].val = 0x00000000;
	cmds[i].cmd = 0xd0000000;
	i++;

	cmds[i].val = 0x00000000;
	cmds[i].cmd = 0xd0000000;
	i++;

	cmds[i].val = 0;
	cmds[i].cmd = MALI_PLBU_CMD_END;
}

void
mali_uniforms_create(unsigned int *uniforms, int size)
{
	{ 	/* gl_mali_ViewportTransform */
		float x0 = 0, x1 = WIDTH, y0 = 0, y1 = HEIGHT;
		float depth_near = 0, depth_far = 1;

		uniforms[0] = from_float(x1 / 2);
		uniforms[1] = from_float(y1 / 2);
		uniforms[2] = from_float((depth_far - depth_near) / 2);
		uniforms[3] = from_float(depth_far);
		uniforms[4] = from_float((x0 + x1) / 2);
		uniforms[5] = from_float((y0 + y1) / 2);
		uniforms[6] = from_float((depth_near + depth_far) / 2);
		uniforms[7] = from_float(depth_near);
	}

	{	/* __maligp2_constant_000 */
		uniforms[8] = from_float(-1e+10);
		uniforms[9] = from_float(1e+10);
		uniforms[10] = 0;
		uniforms[11] = 0;
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
	struct plb *plb;

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

	mali_uniforms_create(mem_0x40000000.address + 0x240, 12);

	plb = plb_create(WIDTH, HEIGHT);
	plb_plbu_stream_create(plb);
	plb_pp_stream_create(plb);

	vs_commands_create((struct mali_cmd *) (mem_0x40000000.address + 0x400));
	plbu_commands_create((struct mali_cmd *) (mem_0x40000000.address + 0x500),
			     WIDTH, HEIGHT, plb);

	pp_job.frame_registers[0x00] = plb->mem_physical + plb->pp_offset;


	gp_job.ctx = (void *) dev_mali_fd;
	gp_job.user_job_ptr = (u32) &gp_job;
	gp_job.priority = 1;
	gp_job.watchdog_msecs = 0;
	gp_job.abort_id = 0;
	ret = ioctl(dev_mali_fd, MALI_IOC_GP2_START_JOB, &gp_job);
	if (ret == -1) {
		printf("Error: ioctl MALI_IOC_GP2_START_JOB failed: %s\n",
		       strerror(errno));
		return errno;
	}

	pthread_mutex_lock(&wait_mutex);
	pthread_mutex_unlock(&wait_mutex);

	image_address = mmap(NULL, WIDTH*HEIGHT*4, PROT_READ | PROT_WRITE,
					   MAP_SHARED, dev_mali_fd, 0x40200000);

	/* create a new space for our final image */
	if (image_address == MAP_FAILED) {
		printf("Error: failed to mmap offset 0x%x (0x%x): %s\n",
		       0x40200000, WIDTH*HEIGHT*4, strerror(errno));
		return errno;
	}

	printf("Mapped 0x%x (0x%x) to %p\n",
	       0x40200000, WIDTH*HEIGHT*4, image_address);

	pp_job.frame_registers[0x11] = HEIGHT - 1;
	pp_job.wb0_registers[1] = 0x40200000;
	pp_job.wb0_registers[5] = (WIDTH * 4) / 8;

	wait_for_notification_start();

	pp_job.ctx = (void *) dev_mali_fd;
	pp_job.user_job_ptr = (u32) &pp_job;
	pp_job.priority = 1;
	pp_job.watchdog_msecs = 0;
	pp_job.abort_id = 0;
	ret = ioctl(dev_mali_fd, MALI_IOC_PP_START_JOB, &pp_job);
	if (ret == -1) {
		printf("Error: ioctl MALI_IOC_PP_START_JOB failed: %s\n",
		       strerror(errno));
		return errno;
	}

	pthread_mutex_lock(&wait_mutex);

	fflush(stdout);

	bmp_dump(image_address, WIDTH * HEIGHT * 4, WIDTH, HEIGHT, "/sdcard/premali.bmp");

	fb_dump(image_address, WIDTH * HEIGHT * 4, WIDTH, HEIGHT);

	return 0;
}
