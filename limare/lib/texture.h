/*
 * Copyright (c) 2012-2013 Luc Verhaegen <libv@skynet.be>
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

#ifndef LIMARE_TEXTURE_H
#define LIMARE_TEXTURE_H 1

struct texture_level {
	int level;

	int width;
	int height;

	int size;

	/* GPU-AUX space */
	unsigned char *dest;
	int mem_physical;
};

struct texture {
	int width;
	int height;
	int format;

	/* in AUX space */
	unsigned int descriptor[0x10];
	unsigned int descriptor_offset;

	int levels;
	struct texture_level level[13];
};

struct texture *texture_create(struct limare_state *state, const void *src,
			       int width, int height, int format, int mipmap);

#endif /* LIMARE_TEXTURE_H */
