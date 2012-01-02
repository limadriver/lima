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
#ifndef PREMALI_PLB_H
#define PREMALI_PLB_H 1

struct plb {
	int block_size; /* 0x200 */

	int width; /* aligned already */
	int height;

	int shift_w;
	int shift_h;

	/* holds the actual primitives */
	int plb_offset;
	int plb_size; /* 0x200 * (width >> (shift_w - 1)) * (height >> (shift_h - 1))) */

	/* holds the addresses so the plbu knows where to store the primitives */
	int plbu_offset;
	int plbu_size; /* 4 * width * height */

	/* holds the coordinates and addresses of the primitives for the PP */
	int pp_offset;
	int pp_size; /* 16 * (width * height + 1) */

	void *mem_address;
	int mem_physical;
	int mem_size;
};

struct plb *plb_create(int width, int height, unsigned int physical,
		       void *address, int offset, int size);

#endif /* PREMALI_PLB_H */
