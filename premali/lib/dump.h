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
 * Helpers to deal with dumped memory.
 */
#ifndef PREMALI_DUMP_H
#define PREMALI_DUMP_H

struct mali_dumped_mem_content {
	unsigned int offset;
	unsigned int size;
	unsigned int memory[];
};

struct mali_dumped_mem_block {
	void *address;
	unsigned int physical;
	unsigned int size;
	int count;
	struct mali_dumped_mem_content *contents[];
};

struct mali_dumped_mem {
	int count;
	struct mali_dumped_mem_block *blocks[];
};

int dumped_mem_load(int fd, struct mali_dumped_mem *dump);

#endif /* PREMALI_DUMP_H */
