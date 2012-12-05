/*
 * Copyright (c) 2011 Luc Verhaegen <libv@skynet.be>
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

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <asm/ioctl.h>

#include <linux/fb.h>

#define u32 uint32_t
#include "linux/mali_ioctl.h"

#include "limare.h"
#include "fb.h"

#ifdef ANDROID
#define FBDEV_DEV "/dev/graphics/fb0"
#else
#define FBDEV_DEV "/dev/fb0"
#endif

int
fb_open(struct limare_state *state)
{
	struct fb_var_screeninfo info;
	struct fb_fix_screeninfo fix;
	struct limare_fb *fb = calloc(1, sizeof(struct limare_fb));

	state->fb = fb;

	fb->fd = open(FBDEV_DEV, O_RDWR);
	if (fb->fd == -1) {
		printf("Error: failed to open %s: %s\n",
			FBDEV_DEV, strerror(errno));
		return errno;
	}

	if (ioctl(fb->fd, FBIOGET_VSCREENINFO, &info) ||
	    ioctl(fb->fd, FBIOGET_FSCREENINFO, &fix)) {
		printf("Error: failed to run ioctl on %s: %s\n",
			FBDEV_DEV, strerror(errno));
		close(fb->fd);
		fb->fd = -1;
		return errno;
	}

	printf("FB: %dx%d@%dbpp at 0x%08lX (0x%08X)\n", info.xres, info.yres,
	       info.bits_per_pixel, fix.smem_start, fix.smem_len);

	fb->width = info.xres;
	fb->height = info.yres;
	fb->size = info.xres * info.yres * 4;

	fb->fb_physical = fix.smem_start;

	fb->map_size = fix.smem_len;
	fb->map = mmap(0, fb->map_size, PROT_READ | PROT_WRITE, MAP_SHARED,
		       fb->fd, 0);
	if (fb->map == MAP_FAILED) {
		printf("Error: failed to run mmap on %s: %s\n",
			FBDEV_DEV, strerror(errno));
		close(fb->fd);
		fb->fd = -1;
		return errno;
	}

	return 0;
}

static int
mali_map_external(struct limare_state *state)
{
	struct limare_fb *fb = state->fb;
	_mali_uk_map_external_mem_s map = { 0 };


	map.phys_addr = fb->fb_physical;
	map.size = fb->map_size;
	map.mali_address = fb->mali_physical[0];

	if (ioctl(state->fd, MALI_IOC_MEM_MAP_EXT, &map)) {
		printf("Error: failed to map external memory: %s\n",
		       strerror(errno));
		return -1;
	}

	fb->mali_handle = map.cookie;

	return 0;
}

int
fb_init(struct limare_state *state, int width, int height, int offset)
{
	struct limare_fb *fb = state->fb;

	if ((fb->width == width) && (fb->height == height)) {
		fb->direct = 1;
		if (fb->map_size >= (2 * fb->size))
			fb->dual_buffer = 1;
	}

	fb->mali_physical[0] = state->mem_base + offset;
	if (fb->direct) {
		fb->mali_physical[1] = fb->mali_physical[0] + fb->size;

		if (mali_map_external(state)) {
			fb->direct = 0;
			fb->dual_buffer = 0;
		}
	}

	if (!fb->direct) {
		fb->buffer_size = width * height * 4;
		fb->buffer = mmap(NULL, fb->size, PROT_READ | PROT_WRITE,
				  MAP_SHARED, state->fd, fb->mali_physical[0]);
		if (fb->buffer == MAP_FAILED) {
			printf("Error: failed to mmap offset 0x%X (0x%X): %s\n",
			       fb->mali_physical[0], fb->buffer_size,
			       strerror(errno));
			return -1;
		}
	}

	if (fb->dual_buffer)
		printf("Using dual buffered direct rendering to FB.\n");
	else if (fb->direct)
		printf("Using direct rendering to FB.\n");
	else
		printf("Using memcpy to FB.\n");

	return 0;
}

void
fb_clear(struct limare_state *state)
{
	struct limare_fb *fb = state->fb;

	if (fb->fd == -1)
		return;

	memset(fb->map, 0xFF, fb->map_size);
}

static void
fb_dump_memcpy(struct limare_state *state)
{
	struct limare_fb *fb = state->fb;
	int i;

	if (fb->fd == -1)
		return;

	if ((fb->width == state->width) && (fb->height == state->height)) {
		memcpy(fb->map, fb->buffer, fb->size);
	} else if ((fb->width >= state->width) &&
		   (fb->height >= state->height)) {
		int fb_offset, buf_offset;

		for (i = 0, fb_offset = 0, buf_offset = 0;
		     i < state->height;
		     i++, fb_offset += 4 * fb->width,
			     buf_offset += 4 * state->width) {
			memcpy(fb->map + fb_offset,
			       fb->buffer + buf_offset,
			       4 * state->width);
		}
	} else
		printf("%s: dimensions not implemented\n", __func__);
}

void
fb_dump(struct limare_state *state)
{
	struct limare_fb *fb = state->fb;

	if (!fb->direct)
		fb_dump_memcpy(state);
}
