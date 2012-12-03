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

#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <unistd.h>
#include <string.h>

#include <linux/fb.h>

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

	state->fb_fd = open(FBDEV_DEV, O_RDWR);
	if (state->fb_fd == -1) {
		printf("Error: failed to open %s: %s\n",
			FBDEV_DEV, strerror(errno));
		return errno;
	}

	if (ioctl(state->fb_fd, FBIOGET_VSCREENINFO, &info)) {
		printf("Error: failed to run ioctl on %s: %s\n",
			FBDEV_DEV, strerror(errno));
		close(state->fb_fd);
		state->fb_fd = -1;
		return errno;
	}

	printf("FB has dimensions: %dx%d@%dbpp\n",
	       info.xres, info.yres, info.bits_per_pixel);
	state->fb_width = info.xres;
	state->fb_height = info.yres;
	state->fb_size = info.xres * info.yres * 4;

	state->fb_map = mmap(0, info.xres * info.yres * 4,
			     PROT_READ | PROT_WRITE, MAP_SHARED,
			     state->fb_fd, 0);
	if (state->fb_map == MAP_FAILED) {
		printf("Error: failed to run mmap on %s: %s\n",
			FBDEV_DEV, strerror(errno));
		close(state->fb_fd);
		state->fb_fd = -1;
		return errno;
	}

	return 0;
}

void
fb_clear(struct limare_state *state)
{
	if (state->fb_fd == -1)
		return;

	memset(state->fb_map, 0xFF, state->fb_size);
}

void
fb_dump(struct limare_state *state, unsigned char *buffer, int size,
	int width, int height)
{
	int i;

	if (state->fb_fd == -1)
		return;

	if ((state->fb_width == width) && (state->fb_height == height)) {
		memcpy(state->fb_map, buffer, state->fb_size);
	} else if ((state->fb_width >= width) && (state->fb_height >= height)) {
		int fb_offset, buf_offset;

		for (i = 0, fb_offset = 0, buf_offset = 0;
		     i < height;
		     i++, fb_offset += 4 * state->fb_width,
			     buf_offset += 4 * width) {
			memcpy(state->fb_map + fb_offset,
			       buffer + buf_offset, 4 * width);
		}
	} else
		printf("%s: dimensions not implemented\n", __func__);
}
