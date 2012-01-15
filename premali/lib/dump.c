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
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <errno.h>

#include "premali.h"
#include "dump.h"

static void
mali_dumped_mem_content_load(void *address,
			     struct mali_dumped_mem_content *contents[],
			     int count)
{
	int i;

	for (i = 0; i < count; i++) {
		struct mali_dumped_mem_content *content = contents[i];

		memcpy(address + content->offset,
		       content->memory, content->size);
	}
}

int
dumped_mem_load(int fd, struct mali_dumped_mem *dump)
{
	int i;

	for (i = 0; i < dump->count; i++) {
		struct mali_dumped_mem_block *block = dump->blocks[i];

		block->address = mmap(NULL, block->size, PROT_READ | PROT_WRITE,
				      MAP_SHARED, fd, block->physical);
		if (block->address == MAP_FAILED) {
			printf("Error: failed to mmap offset 0x%x (0x%x): %s\n",
			       block->physical, block->size, strerror(errno));
			return errno;
		} else
			printf("Mapped 0x%x (0x%x) to %p\n", block->physical,
			       block->size, block->address);

		mali_dumped_mem_content_load(block->address, block->contents,
					     block->count);
	}

	return 0;
}
