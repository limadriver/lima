/*
 * Copyright (c) 2011 Luc Verhaegen <libv@codethink.co.uk>
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
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <errno.h>
#include <sys/mman.h>
#include <linux/fb.h>

#include "fb.h"

void
fb_clear(void)
{
	int fd = open("/dev/graphics/fb0", O_RDWR);
	struct fb_var_screeninfo info;
	unsigned char *fb;

	if (fd == -1) {
		fprintf(stderr, "Error: failed to open %s: %s\n",
			"/dev/graphics/fb0", strerror(errno));
		return;
	}

	if (ioctl(fd, FBIOGET_VSCREENINFO, &info)) {
		fprintf(stderr, "Error: failed to run ioctl on %s: %s\n",
			"/dev/graphics/fb0", strerror(errno));
		close(fd);
		return;
	}

	fb = mmap(0, 2 * info.xres * info.yres * 4, PROT_READ | PROT_WRITE,
		  MAP_SHARED, fd, 0);
	if (!fb) {
		fprintf(stderr, "Error: failed to run mmap on %s: %s\n",
			"/dev/graphics/fb0", strerror(errno));
		close(fd);
		return;
	}

	memset(fb, 0xFF, 2 * info.xres * info.yres * 4);

	close(fd);
	return;
}

void
fb_dump(unsigned char *buffer, int size, int width, int height)
{
	int fd = open("/dev/graphics/fb0", O_RDWR);
	struct fb_var_screeninfo info;
	unsigned char *fb;
	int i;

	if (fd == -1) {
		fprintf(stderr, "Error: failed to open %s: %s\n",
			"/dev/graphics/fb0", strerror(errno));
		return;
	}

	if (ioctl(fd, FBIOGET_VSCREENINFO, &info)) {
		fprintf(stderr, "Error: failed to run ioctl on %s: %s\n",
			"/dev/graphics/fb0", strerror(errno));
		close(fd);
		return;
	}

	fb = mmap(0, 2 * info.xres * info.yres * 4, PROT_READ | PROT_WRITE,
		  MAP_SHARED, fd, 0);
	if (!fb) {
		fprintf(stderr, "Error: failed to run mmap on %s: %s\n",
			"/dev/graphics/fb0", strerror(errno));
		close(fd);
		return;
	}

	if ((info.xres == width) && (info.yres == height)) {
		memcpy(fb, buffer, width * height * 4);
		memcpy(fb + 4 * info.xres * info.yres, buffer, width * height * 4);
	} else if ((info.xres >= width) && (info.yres >= height)) {
		int fb_offset, buf_offset;

		/* landscape */
		for (i = 0, fb_offset = 0, buf_offset = 0;
		     i < height;
		     i++, fb_offset += 4 * info.xres, buf_offset += 4 * width) {
			memcpy(fb + fb_offset, buffer + buf_offset, 4 * width);
			memset(fb + fb_offset + 4 * width, 0xFF, 4 * (info.xres - width));
		}

		memset(fb + fb_offset, 0xFF, 4 * (info.xres * (info.yres - height)));

#if 1
		/* portrait */
		for (i = 0, fb_offset = 4 * info.xres * info.yres, buf_offset = 0;
		     i < height;
		     i++, fb_offset += 4 * info.xres, buf_offset += 4 * width) {
			memcpy(fb + fb_offset, buffer + buf_offset, 4 * width);
			memset(fb + fb_offset + 4 * width, 0xFF, 4 * (info.xres - width));
		}

		memset(fb + fb_offset, 0xFF, 4 * (info.xres * (info.yres - height)));
#endif
	} else
		printf("%s: dimensions not implemented\n", __func__);


	munmap(fb, 2 * info.xres * info.yres * 4);
	close(fd);
}
