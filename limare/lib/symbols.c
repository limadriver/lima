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
 * Some helpers to deal with symbols (uniforms, attributes, varyings).
 */
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "symbols.h"

struct symbol *
symbol_create(const char *name, enum symbol_type type,
	      int component_size, int component_count,
	      int entry_count, int src_stride, int dst_stride,
	      void *data, int copy, int matrix)
{
	struct symbol *symbol;
	int size;

	if (!entry_count)
		entry_count = 1;

	if (matrix)
		component_count *= component_count;

	size = component_size * component_count * entry_count;

	symbol = calloc(1, sizeof(struct symbol));
	if (!symbol) {
		printf("%s: failed to allocate: %s\n", __func__, strerror(errno));
		return NULL;
	}

	if (copy) {
		symbol->data = malloc(size);
		if (!symbol->data) {
			printf("%s: failed to allocate data: %s\n",
			       __func__, strerror(errno));
			free(symbol);
			return NULL;
		}
		symbol->data_allocated = 1;
	}

	strncpy(symbol->name, name, SYMBOL_STRING_SIZE);
	symbol->type = type;

	symbol->component_size = component_size;
	symbol->component_count = component_count;
	symbol->entry_count = entry_count;

	if (matrix) {
		symbol->src_stride = src_stride;
		symbol->dst_stride = dst_stride;
	}

	symbol->size = size;

	if (copy)
		memcpy(symbol->data, data, size);
	else
		symbol->data = data;

	return symbol;
}

struct symbol *
symbol_copy(struct symbol *original, int start, int count)
{
	struct symbol *symbol;

	symbol = malloc(sizeof(struct symbol));
	if (!symbol) {
		printf("%s: failed to allocate: %s\n", __func__, strerror(errno));
		return NULL;
	}

	memcpy(symbol, original, sizeof(struct symbol));

	symbol->entry_count = count;
	symbol->size = symbol->component_size * symbol->component_count *
		symbol->entry_count;

	symbol->data_allocated = 0;

	if (original->data)
		symbol->data = original->data +
			symbol->component_size * symbol->component_count * start;

	return symbol;
}


void
symbol_destroy(struct symbol *symbol)
{
	if (symbol->data_allocated)
		free(symbol->data);
	free(symbol);
}

void
symbol_print(struct symbol *symbol)
{
	char *type;
	int i;

	switch(symbol->type) {
	case SYMBOL_UNIFORM:
		type = "uniform";
		break;
	case SYMBOL_ATTRIBUTE:
		type = "attribute";
		break;
	case SYMBOL_VARYING:
		type = "varying";
		break;
	}

	printf("Symbol %s (%s) = {\n", symbol->name, type);
	printf("\t.component_size = %d,\n", symbol->component_size);
	printf("\t.component_count = %d,\n", symbol->component_count);
	printf("\t.entry_count = %d,\n", symbol->entry_count);
	printf("\t.src_stride = %d,\n", symbol->src_stride);
	printf("\t.dst_stride = %d,\n", symbol->dst_stride);
	printf("\t.size = %d,\n", symbol->size);
	printf("\t.offset = %d,\n", symbol->offset);
	printf("\t.address = %p,\n", symbol->address);
	printf("\t.physical = 0x%x,\n", symbol->physical);

	if (symbol->data) {
		float *data = symbol->data;

		for (i = 0; i < (symbol->size / 4); i++)
			printf("\t.data[0x%02x] = %f (0x%x)\n",
			       i, data[i], *((int *)(&data[i])));
	}

	printf("};\n");
}
