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
	      int entry_count, void *data, int copy)
{
	struct symbol *symbol;
	int size;

	if (!entry_count)
		entry_count = 1;

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

	symbol->size = size;

	if (copy)
		memcpy(symbol->data, data, size);
	else
		symbol->data = data;

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
	printf("\t.size = %d,\n", symbol->size);
	printf("\t.offset = %d,\n", symbol->offset);
	printf("\t.address = %p,\n", symbol->address);
	printf("\t.physical = 0x%x,\n", symbol->physical);

	if (symbol->data) {
		float *data = symbol->data;

		for (i = 0; i < (symbol->size / 4); i++)
			printf("\t.data[0x%02x] = %f\n",
			       i, data[i]);
	}

	printf("};\n");
}

/*
 * Standard symbols.
 */
struct symbol *
uniform_gl_mali_ViewPortTransform(float x0, float y0, float x1, float y1,
				  float depth_near, float depth_far)
{
	float viewport[8];

	viewport[0] = x1 / 2;
	viewport[1] = y1 / 2;
	viewport[2] = (depth_far - depth_near) / 2;
	viewport[3] = depth_far;
	viewport[4] = (x0 + x1) / 2;
	viewport[5] = (y0 + y1) / 2;
	viewport[6] = (depth_near + depth_far) / 2;
	viewport[7] = depth_near;

	return symbol_create("gl_mali_ViewportTransform", SYMBOL_UNIFORM,
			     4, 8, 1, viewport, 1);
}

struct symbol *
uniform___maligp2_constant_000(void)
{
	float constant[] = {-1e+10, 1e+10, 0.0, 0.0};

	return symbol_create("__maligp2_constant_000", SYMBOL_UNIFORM,
			     4, 4, 1, constant, 1);
}
