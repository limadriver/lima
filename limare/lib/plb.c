/*
 * Copyright (c) 2011-2012 Luc Verhaegen <libv@skynet.be>
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

/*
 * Generate the plb address stream for the plbu.
 */
static void
plb_plbu_stream_create(struct plb *plb)
{
	int i, size = plb->width * plb->height;
	unsigned int address = plb->mem_physical + plb->plb_offset;
	unsigned int *stream = plb->mem_address + plb->plbu_offset;

	for (i = 0; i < size; i++)
		stream[i] = address + (i * plb->block_size);
}

/*
 * Generate the PLB desciptors for the PP.
 */
static void
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

struct plb *
plb_create(struct limare_state *state, unsigned int physical, void *address, int offset, int size)
{
	struct plb *plb = calloc(1, sizeof(struct plb));
	int width, height;

	width = ALIGN(state->width, 16) >> 4;
	height = ALIGN(state->height, 16) >> 4;

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

	plb->mem_address = address + offset;
	plb->mem_physical = physical + offset;
	/* just align to page size for convenience */
	plb->mem_size = ALIGN(plb->pp_offset + plb->pp_size, 0x1000);

	if (plb->mem_size > size) {
		printf("Error: plb size exceeds available space.\n");
		free(plb);
		plb = NULL;
	}

	plb_plbu_stream_create(plb);
	plb_pp_stream_create(plb);

	return plb;
}
