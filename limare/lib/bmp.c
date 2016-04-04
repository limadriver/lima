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
 *
 */
/*
 * Quick 'n Dirty bitmap dumper, created by poking at a pre-existing .bmp.
 */
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "limare.h"
#include "bmp.h"

#define FILENAME_SIZE 1024

struct bmp_header {
	unsigned short magic;
	unsigned int size;
	unsigned int unused;
	unsigned int start;
} __attribute__((__packed__));

struct dib_header {
	unsigned int size;
	unsigned int width;
	unsigned int height;
	unsigned short planes;
	unsigned short bpp;
	unsigned int compression;
	unsigned int data_size;
	unsigned int h_res;
	unsigned int v_res;
	unsigned int colours;
	unsigned int important_colours;
	unsigned int red_mask;
	unsigned int green_mask;
	unsigned int blue_mask;
	unsigned int alpha_mask;
	unsigned int colour_space;
	unsigned int unused[12];
} __attribute__((__packed__));

static int
bmp_header_write(int fd, int width, int height, int bgra)
{
	struct bmp_header bmp_header = {
		.magic = 0x4d42,
		.size = (width * height * 4) +
		sizeof(struct bmp_header) + sizeof(struct dib_header),
		.start = sizeof(struct bmp_header) + sizeof(struct dib_header),
	};
	struct dib_header dib_header = {
		.size = sizeof(struct dib_header),
		.width = width,
		.height = height,
		.planes = 1,
		.bpp = 32,
		.compression = 3,
		.data_size = 4 * width * height,
		.h_res = 0xB13,
		.v_res = 0xB13,
		.colours = 0,
		.important_colours = 0,
		.red_mask = 0x000000FF,
		.green_mask = 0x0000FF00,
		.blue_mask = 0x00FF0000,
		.alpha_mask = 0xFF000000,
		.colour_space = 0x57696E20,
	};
	int ret;

	if (bgra) {
		dib_header.red_mask = 0x00FF0000;
		dib_header.blue_mask = 0x000000FF;
	}

	ret = write(fd, &bmp_header, sizeof(struct bmp_header));
	if (ret != sizeof(struct bmp_header)) {
		fprintf(stderr, "%s: failed to write bmp header: %s\n",
			__func__, strerror(errno));
		return errno;
	}

	ret = write(fd, &dib_header, sizeof(struct dib_header));
	if (ret != sizeof(struct dib_header)) {
		fprintf(stderr, "%s: failed to write bmp header: %s\n",
			__func__, strerror(errno));
		return errno;
	}

	return 0;
}

void
bmp_dump(unsigned char *buffer, struct limare_state *state,
	 int width, int height, int cpp, char *filename)
{
	int ret, fd;

	fd = open(filename, O_WRONLY| O_TRUNC | O_CREAT, 0644);
	if (fd == -1) {
		printf("Failed to open %s: %s\n", filename, strerror(errno));
		return;
	}

	/* HORRIBLE HACK */
	if (state && state->type == 400)
		bmp_header_write(fd, width, height, 1);
	else
		bmp_header_write(fd, width, height, 1);

	ret = write(fd, buffer, width * height * cpp);
	if (ret != (width * height * cpp)) {
		fprintf(stderr, "%s: failed to write bmp data: %s\n",
			__func__, strerror(errno));
		return;
	}
}

