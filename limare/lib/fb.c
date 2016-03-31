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
#include "version.h"
#include "fb.h"

static char *fbdev_dev;

void
fb_destroy(struct limare_state *state)
{
	struct limare_fb *fb = state->fb;
	int ret;

	if (fb->ump_id != -1) {
		_mali_uk_release_ump_mem_s release = { 0 };

		release.cookie = fb->mali_handle;

		if (state->kernel_version < MALI_DRIVER_VERSION_R3P1)
			ret = ioctl(state->fd, MALI_IOC_MEM_RELEASE_UMP,
				    &release);
		else
			ret = ioctl(state->fd, MALI_IOC_MEM_RELEASE_UMP_R3P1,
				    &release);
		if (ret)
			printf("Error: failed to release UMP memory: %s\n",
			       strerror(errno));

	} else {
		_mali_uk_unmap_external_mem_s unmap = { 0 };

		unmap.cookie = fb->mali_handle;

		if (state->kernel_version < MALI_DRIVER_VERSION_R3P1)
			ret = ioctl(state->fd, MALI_IOC_MEM_UNMAP_EXT, &unmap);
		else
			ret = ioctl(state->fd, MALI_IOC_MEM_UNMAP_EXT_R3P1,
				    &unmap);

		if (ret)
			printf("Error: failed to unmap external memory: %s\n",
			       strerror(errno));
	}

	munmap(fb->map, fb->map_size);

	close(fb->fd);

	free(fb->fb_var);
	free(fb);
	state->fb = NULL;
}

int
fb_open(struct limare_state *state)
{
	struct limare_fb *fb = calloc(1, sizeof(struct limare_fb));
	struct fb_var_screeninfo *fb_var =
		calloc(1, sizeof(struct fb_var_screeninfo));
	struct fb_fix_screeninfo fix;

	if (!fb) {
		printf("Error: failed to alloc limare_fb: %s\n",
		       strerror(errno));
		return errno;
	}

	if (!fb_var) {
		printf("Error: failed to alloc fb_var_vscreeninfo: %s\n",
		       strerror(errno));
		free(fb);
		return errno;
	}

	fb->ump_id = -1;

#ifndef ANDROID
	if ((state->kernel_version == MALI_DRIVER_VERSION_R3P2) &&
	    (state->pp_core_count == 4))
		/* Assume that we are an odroid running over HDMI */
		fbdev_dev = "/dev/fb6";
	else
		fbdev_dev = "/dev/fb0";
#else
	fbdev_dev = "/dev/graphics/fb0";
#endif /* ANDROID */

	fb->fd = open(fbdev_dev, O_RDWR);
	if (fb->fd == -1) {
		printf("Error: failed to open %s: %s\n",
		       fbdev_dev, strerror(errno));
		free(fb);
		return errno;
	}

	if (ioctl(fb->fd, FBIOGET_VSCREENINFO, fb_var) ||
	    ioctl(fb->fd, FBIOGET_FSCREENINFO, &fix)) {
		printf("Error: failed to run ioctl on %s: %s\n",
			fbdev_dev, strerror(errno));
		close(fb->fd);
		free(fb);
		return errno;
	}

	printf("FB: %dx%d@%dbpp at 0x%08lX (0x%08X)\n",
	       fb_var->xres, fb_var->yres, fb_var->bits_per_pixel,
	       fix.smem_start, fix.smem_len);

	fb->width = fb_var->xres;
	fb->height = fb_var->yres;
	fb->bpp = fb_var->bits_per_pixel;
	if (fb->bpp == 16)
		fb->size = fb_var->xres * fb_var->yres * 2;
	else
		fb->size = fb_var->xres * fb_var->yres * 4;

	fb->fb_physical = fix.smem_start;
	fb->map_size = fix.smem_len;

	fb->map = mmap(0, fb->map_size, PROT_READ | PROT_WRITE,
		       MAP_SHARED, fb->fd, 0);
	if (fb->map == MAP_FAILED) {
		printf("Error: failed to run mmap on %s: %s (%d)\n",
		       fbdev_dev, strerror(errno), errno);
		close(fb->fd);
		free(fb);
		return errno;
	}

	fb_var->activate = FB_ACTIVATE_VBL;

	if (fb->map_size >= (2 * fb->size))
		fb->dual_buffer = 1;
	else
		fb->dual_buffer = 0;

	if (fb->dual_buffer) {
		fb_var->yoffset = 0;

		if (ioctl(fb->fd, FBIOPAN_DISPLAY, fb_var))
			printf("Error: failed to run pan ioctl on %s: %s\n",
			       fbdev_dev, strerror(errno));
	}

	fb->fb_var = fb_var;
	state->fb = fb;

	return 0;
}

static int
mali_map_external(struct limare_state *state)
{
	struct limare_fb *fb = state->fb;
	_mali_uk_map_external_mem_s map = { 0 };
	int ret;

	map.phys_addr = fb->fb_physical;
	map.size = fb->map_size;
	map.mali_address = fb->mali_physical[0];

	if (state->kernel_version < MALI_DRIVER_VERSION_R3P1)
		ret = ioctl(state->fd, MALI_IOC_MEM_MAP_EXT, &map);
	else
		ret = ioctl(state->fd, MALI_IOC_MEM_MAP_EXT_R3P1, &map);

	if (ret)
		return ret;

	fb->mali_handle = map.cookie;

	return 0;
}

static int
mali_map_ump(struct limare_state *state)
{
	struct limare_fb *fb = state->fb;
	_mali_uk_attach_ump_mem_s ump = { 0 };
	int ret;

#define GET_UMP_SECURE_ID_BUF1   _IOWR('m', 311, unsigned int)
	ret = ioctl(fb->fd, GET_UMP_SECURE_ID_BUF1, &fb->ump_id);
	if (ret) {
		printf("Error: failed to run GET_UMP_SECURE_ID_BUF1 ioctl "
		       "on %s: %s\n",  fbdev_dev, strerror(errno));
		return ret;
	}

	ump.secure_id = fb->ump_id;
	ump.size = fb->map_size;
	ump.mali_address = fb->mali_physical[0];

	if (state->kernel_version < MALI_DRIVER_VERSION_R3P1)
		ret = ioctl(state->fd, MALI_IOC_MEM_ATTACH_UMP, &ump);
	else
		ret = ioctl(state->fd, MALI_IOC_MEM_ATTACH_UMP_R3P1, &ump);

	if (ret) {
		printf("Error: failed to attach ump memory: %s\n",
		       strerror(errno));
		return -1;
	}

	fb->mali_handle = ump.cookie;

	return 0;
}

int
fb_init(struct limare_state *state, int width, int height, int offset)
{
	struct limare_fb *fb = state->fb;
	int ret;

	if (!width || !height) {
		width = fb->width;
		height = fb->height;
	}

	if ((fb->width < width) && (fb->height < height)) {
		printf("Requested size %dx%d is larger than fb: truncating.\n",
		       width, height);
		width = fb->width;
		height = fb->height;
	} else if (fb->width < width) {
		printf("Requested width %d is wider than fb: truncating.\n",
		       width);
		width = fb->width;
	} else if (fb->height < height) {
		printf("Requested height %d is taller than fb: truncating.\n",
		       height);
		height = fb->height;
	}

	fb->mali_physical[0] = state->mem_base + offset;
	if (fb->dual_buffer)
		fb->mali_physical[1] = fb->mali_physical[0] + fb->size;

	ret = mali_map_external(state);
	if (ret == -1)
		ret = mali_map_ump(state);

	if (ret) {
		printf("Error: failed to map FB to mali: %s\n",
		       strerror(errno));
		return ret;
	}

	if (fb->dual_buffer)
		printf("Using dual buffered direct rendering to FB.\n");
	else
		printf("Using direct rendering to FB.\n");

	state->width = width;
	state->height = height;

	/* and now, unblank */
	ret = ioctl(fb->fd, FBIOBLANK, FB_BLANK_UNBLANK);
	if (ret) {
		printf("Error: failed to run unblank FB: %s\n",
		       strerror(errno));
		return ret;
	}

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

void
limare_fb_flip(struct limare_state *state, struct limare_frame *frame)
{
	struct limare_fb *fb = state->fb;
	//int sync_arg = 0;

	if (!fb->dual_buffer)
		return;

	if (frame->index)
		fb->fb_var->yoffset = fb->height;
	else
		fb->fb_var->yoffset = 0;

#if 0
	if (ioctl(fb->fd, FBIO_WAITFORVSYNC, &sync_arg))
		printf("Error: failed to run ioctl on %s: %s\n",
			fbdev_dev, strerror(errno));
#endif

	if (ioctl(fb->fd, FBIOPAN_DISPLAY, fb->fb_var))
		printf("Error: failed to run ioctl on %s: %s\n",
			fbdev_dev, strerror(errno));
}

void
fb_dump_direct(struct limare_state *state, unsigned char *buffer,
	       int width, int height)
{
	struct limare_fb *fb = state->fb;
	int i;

	if (fb->fd == -1)
		return;

	if ((fb->width == width) && (fb->height == height)) {
		memcpy(fb->map, buffer, fb->size);
	} else if ((fb->width >= width) &&
		   (fb->height >= height)) {
		int fb_offset, buf_offset;

		for (i = 0, fb_offset = 0, buf_offset = 0;
		     i < height;
		     i++, fb_offset += 4 * fb->width,
			     buf_offset += 4 * width) {
			memcpy(fb->map + fb_offset,
			       buffer + buf_offset,
			       4 * width);
		}
	} else
		printf("%s: dimensions not implemented\n", __func__);
}

void
limare_buffer_size(struct limare_state *state, int *width, int *height)
{
	if (width)
		*width = state->width;
	if (height)
		*height = state->height;
}
