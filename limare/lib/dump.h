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
#ifndef LIMARE_DUMP_H
#define LIMARE_DUMP_H

struct lima_dumped_mem_content {
	unsigned int offset;
	unsigned int size;
	unsigned int memory[];
};

struct lima_dumped_mem_block {
	void *address;
	unsigned int physical;
	unsigned int size;
	int count;
	struct lima_dumped_mem_content *contents[];
};

struct lima_dumped_mem {
	int count;
	struct lima_dumped_mem_block *blocks[];
};

int dumped_mem_load(int fd, struct lima_dumped_mem *dump);

#endif /* LIMARE_DUMP_H */
