/*
 * Copyright (c) 2011-2012 Luc Verhaegen <libv@codethink.co.uk>
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

/*
 * Some helpers to deal with the plb stream, and the plbu and pp plb
 * reference streams.
 */
#include <stdio.h>
#include <stdlib.h>

#include "premali.h"
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
plb_create(int width, int height, unsigned int physical, void *address, int offset, int size)
{
	struct plb *plb = calloc(1, sizeof(struct plb));

	width = ALIGN(width, 16) >> 4;
	height = ALIGN(height, 16) >> 4;

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
