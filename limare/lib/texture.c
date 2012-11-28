/*
 * Copyright (c) 2012 Luc Verhaegen <libv@skynet.be>
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

#include "limare.h"
#include "texture.h"
#include "formats.h"

static unsigned char
swizzle_index(unsigned char x, unsigned char y)
{
	unsigned char top = y;
	unsigned char bottom = y ^ x;
	unsigned char ret;

	ret = (top & 0x8) << 4;
	ret |= (bottom & 0x8) << 3;
	ret |= (top & 0x4) << 3;
	ret |= (bottom & 0x4) << 2;
	ret |= (top & 0x2) << 2;
	ret |= (bottom & 0x2) << 1;
	ret |= (top & 0x1) << 1;
	ret |= (bottom & 0x1) << 0;

	return ret;
}

static void
texture_swizzle_rgb(unsigned char *dst, const unsigned char *src,
		    int width, int height, int pitch)
{
	int block_x, block_y, block_w = width >> 4, block_h = height >> 4;
	int x, y, offset = 0, index;
	int dst_off, src_off, y_off, block_x_off, block_y_off = 0;

	for (block_y = 0; block_y < block_h; block_y++) {
		block_x_off = block_y_off;
		for (block_x = 0; block_x < block_w; block_x++) {
			y_off = block_x_off;
			for (y = 0; y < 16; y++) {
				src_off = y_off;
				for (x = 0; x < 16; x++) {
					index = swizzle_index(x, y);

					dst_off = offset + 3 * index;

					dst[dst_off + 0] = src[src_off + 0];
					dst[dst_off + 1] = src[src_off + 1];
					dst[dst_off + 2] = src[src_off + 2];

					src_off += 3;
				}
				y_off += pitch;
			}
			offset += 768;
			block_x_off += 48;
		}
		block_y_off += 16 * pitch;
	}
}

struct texture *
texture_create(struct limare_state *state, const void *src,
	       int width, int height, int format)
{
	struct texture *texture = calloc(1, sizeof(struct texture));
	int flag0 = 0, flag1 = 1, layout = 0, size;

	texture->width = width;
	texture->height = height;
	texture->format = format;

	texture->pixels = state->aux_mem_address + state->aux_mem_used;
	texture->mem_physical = state->aux_mem_physical + state->aux_mem_used;

	switch (texture->format) {
	// case LIMA_TEXEL_FORMAT_RGB_555:
	// case LIMA_TEXEL_FORMAT_RGBA_5551:
	// case LIMA_TEXEL_FORMAT_RGBA_4444:
	// case LIMA_TEXEL_FORMAT_LA_88:
	case LIMA_TEXEL_FORMAT_RGB_888:
		texture->pitch = 3 * width;
		texture->size = texture->pitch * height;

		size = ALIGN(texture->size, 0x40);

		if ((state->aux_mem_size - state->aux_mem_used) < size) {
			printf("%s: size (0x%X) exceeds available size (0x%X)\n",
			       __func__, texture->size,
			       state->aux_mem_size - state->aux_mem_used);
			free(texture);
			return NULL;
		}

		state->aux_mem_used += size;

		flag0 = 1;
		flag1 = 0;
		layout = 3;

		texture_swizzle_rgb(texture->pixels, src, texture->width,
				    texture->height, texture->pitch);
		break;
	// case LIMA_TEXEL_FORMAT_RGBA_8888:
	// case LIMA_TEXEL_FORMAT_BGRA_8888:
	// case LIMA_TEXEL_FORMAT_RGBA64:
	// case LIMA_TEXEL_FORMAT_DEPTH_STENCIL_32:
	default:
		free(texture);
		printf("%s: unsupported format %x\n", __func__, format);
		return NULL;
	}

	texture->descriptor[0] = (flag0 << 7) | (flag1 << 6) | format;
	/* assume both min and mag filter are linear */
	/* assume that we are not a cubemap */
	texture->descriptor[1] = 0x00000400;
	texture->descriptor[2] = width << 22;
	texture->descriptor[3] = 0x10000 | (height << 3) | (width >> 10);
	texture->descriptor[6] = ((texture->mem_physical & 0xC0) << 26) | (layout << 13);
	texture->descriptor[7] = texture->mem_physical >> 8;

	return texture;
}
