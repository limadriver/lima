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

/*
 * Some helpers to deal with the plb stream, and the plbu and pp plb
 * reference streams.
 */
#include <stdio.h>
#include <stdlib.h>

#include "limare.h"
#include "plb.h"

static void
hilbert_rotate(int n, int *x, int *y, int rx, int ry)
{
	if (ry == 0) {
		if (rx == 1) {
			*x = n-1 - *x;
			*y = n-1 - *y;
		}

		/* Swap x and y */
		int t  = *x;
		*x = *y;
		*y = t;
	}
}

static void
hilbert_coords(int n, int d, int *x, int *y)
{
	int rx, ry, i, t=d;

	*x = *y = 0;

	for (i = 0; (1 << i) < n; i++) {

		rx = 1 & (t / 2);
		ry = 1 & (t ^ rx);

		hilbert_rotate(1 << i, x, y, rx, ry);

		*x += rx << i;
		*y += ry << i;

		t /= 4;
	}
}

/*
 * Pre-generate the PLB desciptors for the PP.
 */


static void
plb_pp_template_create(struct plb_info *plb)
{
	struct pp_pattern {
		int x;
		int y;
		unsigned int offset;
	};
	struct pp_pattern *pattern;
	unsigned int *stream;
	int size = plb->tiled_w * plb->tiled_h;
	int max, dim, count, i, index;

	pattern = calloc(size, sizeof(struct pp_pattern));

	if (plb->tiled_w < plb->tiled_h)
		max = plb->tiled_h;
	else
		max = plb->tiled_w;

	for (dim = 0; (1 << dim) < max; dim++)
		;
	count = 1 << (dim + dim);

	for (i = 0, index = 0; i < count; i++) {
		int x, y;

		hilbert_coords(max, i, &x, &y);

		if ((x < plb->tiled_w) && (y < plb->tiled_h)) {
			pattern[index].x = x;
			pattern[index].y = y;
			pattern[index].offset =
				((y >> plb->shift_h) * plb->block_w +
				 (x >> plb->shift_w)) * plb->block_size;

			index++;
		}
	}

	stream = calloc(size + 1, 4 * sizeof(unsigned int));

	for (i = 0, index = 0; i < size; i++) {
		stream[index + 0] = 0;
		stream[index + 1] = 0xB8000000 |
			pattern[i].x | (pattern[i].y << 8);
		stream[index + 2] = 0xE0000002 |
			((pattern[i].offset >> 3) & ~0xE0000003);
		stream[index + 3] = 0xB0000000;

		index += 4;
	}

	stream[index + 0] = 0;
	stream[index + 1] = 0xBC000000;

	plb->pp_template = stream;
	plb->pp_size = size + 1;
}

struct plb_info *
plb_info_create(struct limare_state *state)
{
	struct plb_info *plb = calloc(1, sizeof(struct plb_info));
	int width, height, limit;
	int max;

	width = ALIGN(state->width, 16) >> 4;
	height = ALIGN(state->height, 16) >> 4;

	plb->tiled_w = width;
	plb->tiled_h = height;

	/* limit the amount of plb's the gp has to generate */
	/* For performance, 250 seems preferred on mali200.
	 * 300 is the hard limit for mali200.
	 * 512 is the hard limit for mali400.
	 */
	if (state->type == LIMARE_TYPE_M400)
		limit = 500;
	else
		limit = 250;

	while ((width * height) > limit) {
		if (width >= height) {
			width = (width + 1) >> 1;
			plb->shift_w++;
		} else {
			height = (height + 1) >> 1;
			plb->shift_h++;
		}
	}

	plb->block_size = 0x200;

	plb->block_w = width;
	plb->block_h = height;

	if (plb->shift_h > plb->shift_w)
		max = plb->shift_h;
	else
		max = plb->shift_w;

	if (max > 2)
		plb->shift_max = 2;
	else if (max)
		plb->shift_max = 1;

#if 0
	printf("%s: (%dx%d) == (%d << %d, %d << %d);\n", __func__,
	       plb->tiled_w, plb->tiled_h,
	       plb->block_w, plb->shift_w, plb->block_h, plb->shift_h);
#endif

	plb->plb_size = plb->block_size * plb->block_w * plb->block_h;

	if (state->type == LIMARE_TYPE_M400)
		plb->plbu_size = 4 * plb->block_w * plb->block_h;
	else
		/* fixed size on mali200 */
		plb->plbu_size = 4 * 300;

	plb_pp_template_create(plb);

	return plb;
}

void
plb_info_destroy(struct plb_info *plb)
{
	free(plb);
}

/*
 * Generate the plb address stream for the plbu.
 */
static void
plb_plbu_stream_create(struct limare_frame *frame, struct plb_info *plb)
{
	unsigned int address = frame->mem_physical + frame->plb_offset;
	unsigned int *stream = frame->mem_address + frame->plb_plbu_offset;
	int i, size = plb->block_w * plb->block_h;

	for (i = 0; i < size; i++)
		stream[i] = address + (i * plb->block_size);
}

/*
 * Generate the PLB desciptors for the PP.
 */
static void
plb_pp_stream_create(struct limare_frame *frame, struct plb_info *plb)
{
	unsigned int address = frame->mem_physical + frame->plb_offset;
	unsigned int *stream = frame->mem_address + frame->plb_pp_offset[0];
	int i, size = plb->tiled_w * plb->tiled_h * 4;
	unsigned int *p, *q;

	address >>= 3;
	p = stream;
	q = plb->pp_template;

	for (i = 0; i < size; i += 4) {
		p[0] = 0;
		p[1] = q[1];
		p[2] = q[2] + address;
		p[3] = 0xB0000000;
		p += 4;
		q += 4;
	}

	p[0] = 0;
	p[1] = 0xBC000000;
}

static void
plb_pp_stream_stop(struct limare_frame *frame, int index)
{
	unsigned int *stream = frame->mem_address + frame->plb_pp_offset[index];

	stream[0] = 0;
	stream[1] = 0xBC000000;
}

int
frame_plb_create(struct limare_state *state, struct limare_frame *frame)
{
	struct plb_info *plb = state->plb;
	int mem_used = 0;
	int i;

	frame->plb_offset = frame->mem_used + mem_used;
	mem_used += ALIGN(plb->plb_size, 0x40);

	frame->plb_plbu_offset = frame->mem_used + mem_used;
	mem_used += ALIGN(plb->plbu_size, 0x40);

	for (i = 0; i < state->pp_core_count; i++) {
		frame->plb_pp_offset[i] = frame->mem_used + mem_used;
		mem_used += ALIGN(0x10 * plb->pp_size, 0x40);

		if ((frame->mem_used + mem_used) > frame->mem_size) {
			printf("%s: no space for the plb areas\n", __func__);
			return -1;
		}

		frame->mem_used += mem_used;
	}

	plb_plbu_stream_create(frame, plb);

	plb_pp_stream_create(frame, plb);

	for (i = 1; i < state->pp_core_count; i++)
		plb_pp_stream_stop(frame, i);

	return 0;
}
