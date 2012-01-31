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

#ifndef PREMALI_SYMBOLS_H
#define PREMALI_SYMBOLS_H 1

enum symbol_type {
	SYMBOL_UNIFORM,
	SYMBOL_ATTRIBUTE,
	SYMBOL_VARYING,
};

struct symbol {
#define SYMBOL_STRING_SIZE 64
	/* as referenced by the shaders and shader compiler binary streams */
	char name[SYMBOL_STRING_SIZE + 1];

	enum symbol_type type;

	int component_size;
	int component_count;
	int entry_count;

	int src_stride;
	int dst_stride;

	int size; /* size it takes in memory. */
	short offset; /* offset from start of memory block */

	void *address;
	int physical; /* empty for uniforms */

	void *data;
	int data_allocated;
};

struct symbol *symbol_create(const char *name, enum symbol_type type,
			     int component_size, int component_count,
			     int entry_count, int src_stride, int dst_stride,
			     void *data, int copy, int matrix);

struct symbol *symbol_copy(struct symbol *original, int start, int count);

void symbol_destroy(struct symbol *symbol);
void symbol_print(struct symbol *symbol);

#endif /* PREMALI_SYMBOLS_H */
