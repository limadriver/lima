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

#ifndef LIMARE_FB_H
#define LIMARE_FB_H 1

struct limare_fb {
	int fd;

	/* one single fb */
	int width;
	int height;
	int bpp;
	int size;

	int dual_buffer;

	/* mapped fb */
	void *map;
	int map_size;

	/* for when we do not have direct writing */
	void *buffer;
	int buffer_size;

	int fb_physical;
	int mali_physical[2];

	unsigned int ump_id;

	int mali_handle;

	struct fb_var_screeninfo *fb_var;
};

int fb_open(struct limare_state *state);
int fb_init(struct limare_state *state, int width, int height, int offset);
void fb_clear(struct limare_state *state);
void fb_dump_direct(struct limare_state *state, unsigned char *buffer,
		    int width, int height);
void limare_fb_flip(struct limare_state *state, struct limare_frame *frame);

#endif /* LIMARE_FB_H */
