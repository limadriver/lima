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
 * Helpers to deal with dumped memory.
 */
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>

#include "limare.h"
#include "dump.h"

static void
lima_dumped_mem_content_load(void *address,
			     struct lima_dumped_mem_content *contents[],
			     int count)
{
	int i;

	for (i = 0; i < count; i++) {
		struct lima_dumped_mem_content *content = contents[i];

		memcpy(address + content->offset,
		       content->memory, content->size);
	}
}

int
dumped_mem_load(int fd, struct lima_dumped_mem *dump)
{
	int i;

	for (i = 0; i < dump->count; i++) {
		struct lima_dumped_mem_block *block = dump->blocks[i];

		block->address = mmap(NULL, block->size, PROT_READ | PROT_WRITE,
				      MAP_SHARED, fd, block->physical);
		if (block->address == MAP_FAILED) {
			printf("Error: failed to mmap offset 0x%x (0x%x): %s\n",
			       block->physical, block->size, strerror(errno));
			return errno;
		} else
			printf("Mapped 0x%x (0x%x) to %p\n", block->physical,
			       block->size, block->address);

		lima_dumped_mem_content_load(block->address, block->contents,
					     block->count);
	}

	return 0;
}
