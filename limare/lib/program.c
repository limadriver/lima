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
 *
 */
/*
 * Dealing with shader programs, from compilation to linking.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "version.h"
#include "limare.h"
#include "plb.h"
#include "gp.h"
#include "program.h"
#include "compiler.h"
#include "symbols.h"

/*
 * Attribute linking:
 *
 * One single attribute per command:
 *  bits 0x1C.2 through 0x1C.5 of every instruction,
 *  bit 0x1C.6 is set when there is an attribute.
 *
 * Before linking, attribute 0 is aColor, and attribute 1 is aPosition,
 * as obviously spotted from the attribute link stream.
 * After linking, attribute 0 is aVertices, and attribute 1 is aColor.
 * This matches the way in which we attach our attributes.
 *
 * And indeed, if we swap the bits and the attributes over, things work fine,
 * and if we only swap the attributes over, then the rendering is broken in a
 * logical way.
 */
/*
 * Varying linking:
 *
 * 4 entries, linked to two varyings. If both varyings are specified, they have
 * to be the same.
 *
 * Bits 47-52, contain 4 entries, 3 bits each. 7 means invalid. 2 entries per
 * varying.
 * Bits 5A through 63 specify two varyings, 5 bits each. Top bit means
 * valid/defined, other is most likely the index in the common area.
 *
 * How this fully fits together, is not entirely clear yet, but hopefully
 * clear enough to implement linking in one of the next stages.
 */
/*
 * Uniform linking:
 *
 * Since there is just a memory blob with uniforms packed in it, it would
 * appear that there is no linking, and uniforms just have to be packed as
 * defined in the uniform link stream.
 *
 */

#define STREAM_TAG_STRI 0x49525453

#define STREAM_TAG_SUNI 0x494E5553
#define STREAM_TAG_VUNI 0x494E5556
#define STREAM_TAG_VINI 0x494E4956

#define STREAM_TAG_SATT 0x54544153
#define STREAM_TAG_VATT 0x54544156

#define STREAM_TAG_VIDX 0x58444956
#define STREAM_TAG_ITDR 0x52445449
#define STREAM_TAG_IYUV 0x56555949
#define STREAM_TAG_IGRD 0x44524749

struct stream_string {
	unsigned int tag; /* STRI */
	int size;
	char string[];
};

struct stream_uniform_table_start {
	unsigned int tag; /* SUNI */
	int size;
	int count;
	int space_needed;
};

struct stream_uniform_start {
	unsigned int tag; /* VUNI */
	int size;
};

struct stream_uniform_data { /* 0x14 */
	unsigned char type; /* 0x00 */
	unsigned char unknown01; /* 0x01 */
	unsigned short component_count; /* 0x02 */
	unsigned short component_size; /* 0x04 */
	unsigned short entry_count; /* 0x06. 0 == 1 */
	unsigned short src_stride; /* 0x08 */
	unsigned char dst_stride; /* 0x0A */
	unsigned char precision; /* 0x0B */
	unsigned short unknown0C; /* 0x0C */
	unsigned short unknown0E; /* 0x0E */
	unsigned short offset; /* offset into the uniform memory */
	unsigned short index; /* often -1 */
};

struct stream_uniform_init {
	unsigned int tag; /* VINI */
	unsigned int size;
	unsigned int count;
	unsigned int data[];
};

struct stream_uniform {
	struct stream_uniform *next;

	struct stream_uniform_start *start;
	struct stream_string *string;
	struct stream_uniform_data *data;
	struct stream_uniform_init *init;
};

struct stream_uniform_table {
	struct stream_uniform_table_start *start;

	struct stream_uniform *uniforms;
};

static int
stream_uniform_table_start_read(void *stream, struct stream_uniform_table *table)
{
	struct stream_uniform_table_start *start = stream;

	if (start->tag != STREAM_TAG_SUNI)
		return 0;

	table->start = start;
	return sizeof(struct stream_uniform_table_start);
}

static int
stream_uniform_start_read(void *stream, struct stream_uniform *uniform)
{
	struct stream_uniform_start *start = stream;

	if (start->tag != STREAM_TAG_VUNI)
		return 0;

	uniform->start = start;
	return sizeof(struct stream_uniform_start);
}

static int
stream_string_read(void *stream, struct stream_string **string)
{
	*string = stream;

	if ((*string)->tag == STREAM_TAG_STRI)
		return sizeof(struct stream_string) + (*string)->size;

	*string = NULL;
	return 0;
}

static int
stream_uniform_data_read(void *stream, struct stream_uniform *uniform)
{
	uniform->data = stream;

	return sizeof(struct stream_uniform_data);
}

static int
stream_uniform_init_read(void *stream, struct stream_uniform *uniform)
{
	struct stream_uniform_init *init = stream;

	if (init->tag != STREAM_TAG_VINI)
		return 0;

	uniform->init = init;

	return 8 + init->size;
}

/*
 * Tags that are currently unused.
 */
static int
stream_uniform_skip(void *stream)
{
	unsigned int *tag = stream;

	switch (*tag) {
	case STREAM_TAG_VIDX:
		return 8;
	case STREAM_TAG_ITDR:
	case STREAM_TAG_IYUV:
	case STREAM_TAG_IGRD:
		return 12;
	default:
		return 0;
	}
}

static void
stream_uniform_table_destroy(struct stream_uniform_table *table)
{
	if (table) {
		while (table->uniforms) {
			struct stream_uniform *uniform = table->uniforms;

			table->uniforms = uniform->next;
			free(uniform);
		}

		free(table);
	}
}

static struct stream_uniform_table *
stream_uniform_table_create(void *stream, int size)
{
	struct stream_uniform_table *table;
	struct stream_uniform *uniform;
	int offset = 0;
	int i;

	if (!stream || !size)
		return NULL;

	table = calloc(1, sizeof(struct stream_uniform_table));
	if (!table) {
		printf("%s: failed to allocate table\n", __func__);
		return NULL;
	}

	offset += stream_uniform_table_start_read(stream + offset, table);
	if (!table->start) {
		printf("%s: Error: missing table start at 0x%x\n",
		       __func__, offset);
		goto corrupt;
	}

	for (i = 0; i < table->start->count; i++) {
		uniform = calloc(1, sizeof(struct stream_uniform));
		if (!table) {
			printf("%s: failed to allocate uniform\n", __func__);
			goto oom;
		}

		offset += stream_uniform_start_read(stream + offset, uniform);
		if (!uniform->start) {
			printf("%s: Error: missing uniform start at 0x%x\n",
			       __func__, offset);
			goto corrupt;
		}

		offset += stream_string_read(stream + offset, &uniform->string);
		if (!uniform->string) {
			printf("%s: Error: missing string at 0x%x\n",
			       __func__, offset);
			goto corrupt;
		}

		offset += stream_uniform_data_read(stream + offset, uniform);
		if (!uniform->data) {
			printf("%s: Error: missing uniform data at 0x%x\n",
			       __func__, offset);
			goto corrupt;
		}

		/* skip some tags that might be there */
		offset += stream_uniform_skip(stream + offset);
		offset += stream_uniform_skip(stream + offset);
		offset += stream_uniform_skip(stream + offset);
		offset += stream_uniform_skip(stream + offset);

		offset += stream_uniform_init_read(stream + offset, uniform);
		/* it is legal to not have an init block */

		uniform->next = table->uniforms;
		table->uniforms = uniform;
	}

	return table;
 oom:
	stream_uniform_table_destroy(table);
	return NULL;
 corrupt:
	stream_uniform_table_destroy(table);
	return NULL;
}

void
stream_uniform_table_print(struct stream_uniform_table *table)
{
	struct stream_uniform *uniform;

	printf("%s: Uniform space needed: 0x%x\n",
	       __func__, table->start->space_needed);
	uniform = table->uniforms;
	while (uniform) {
		printf("uniform \"%s\" = {\n", uniform->string->string);
		printf("\t type 0x%02x, unknown01 0x%02x, component_count 0x%04x\n",
		       uniform->data->type, uniform->data->unknown01,
		       uniform->data->component_count);
		printf("\t component_size 0x%04x, entry_count 0x%04x\n",
		       uniform->data->component_size, uniform->data->entry_count);
		printf("\t src_stride 0x%04x, dst_stride 0x%02x, precision 0x%02x\n",
		       uniform->data->src_stride, uniform->data->dst_stride,
		       uniform->data->precision);
		printf("\t unknown0C 0x%04x, unknown0E 0x%04x\n",
		       uniform->data->unknown0C, uniform->data->unknown0E);
		printf("\t offset 0x%04x, index 0x%04x\n",
		       uniform->data->offset, uniform->data->index);
		printf("}\n");
		uniform = uniform->next;
	}
}

static struct symbol **
stream_uniform_table_to_symbols(struct stream_uniform_table *table,
				int *count, int *size)
{
	struct stream_uniform *uniform;
	struct symbol **symbols;
	int i;

	if (!table || !count || !size)
		return NULL;

	*count = table->start->count;
	*size = table->start->space_needed;

	if (!table->start->count)
		return NULL;

	symbols = calloc(*count, sizeof(struct symbol *));
	if (!symbols) {
		printf("%s: Error: failed to allocate symbols: %s\n",
		       __func__, strerror(errno));
		goto error;
	}

	for (i = 0, uniform = table->uniforms;
	     (i < table->start->count) && uniform;
	     i++, uniform = uniform->next) {
		int component_count = uniform->data->component_count;
		int matrix = 0;

		if (uniform->data->unknown01 == 4)
			matrix = 1;

		if (uniform->init)
			symbols[i] =
				symbol_create(uniform->string->string, SYMBOL_UNIFORM,
					      uniform->data->component_size,
					      component_count,
					      uniform->data->entry_count,
					      uniform->data->src_stride,
					      uniform->data->dst_stride,
					      uniform->init->data, 1, matrix);
		else
			symbols[i] =
				symbol_create(uniform->string->string, SYMBOL_UNIFORM,
					      uniform->data->component_size,
					      component_count,
					      uniform->data->entry_count,
					      uniform->data->src_stride,
					      uniform->data->dst_stride,
					      NULL, 0, matrix);
		if (!symbols[i]) {
			printf("%s: Error: failed to create symbol %s: %s\n",
			       __func__, uniform->string->string, strerror(errno));
			goto error;
		}

		symbols[i]->offset = uniform->data->offset;
	}

	return symbols;
 error:
	if (symbols) {
		for (i = 0; i < *count; i++)
			if (symbols[i])
				symbol_destroy(symbols[i]);
	}
	free(symbols);

	*count = 0;
	*size = 0;

	return NULL;
}

/*
 * Now for Attributes.
 */
struct stream_attribute_table_start {
	unsigned int tag; /* SATT */
	int size;
	int count;
};

struct stream_attribute_start {
	unsigned int tag; /* VATT */
	int size;
};

struct stream_attribute_data { /* 0x14 */
	unsigned char type; /* 0x00 */
	unsigned char unknown01; /* 0x01 */
	unsigned short component_count; /* 0x02 */
	unsigned short component_size; /* 0x04 */
	unsigned short entry_count; /* 0x06. 0 == 1 */
	unsigned short stride; /* 0x08 */
	unsigned char unknown0A; /* 0x0A */
	unsigned char precision; /* 0x0B */
	unsigned short unknown0C; /* 0x0C */
	unsigned short offset; /* 0x0E */
};

struct stream_attribute {
	struct stream_attribute *next;

	struct stream_attribute_start *start;
	struct stream_string *string;
	struct stream_attribute_data *data;
};

struct stream_attribute_table {
	struct stream_attribute_table_start *start;

	struct stream_attribute *attributes;
};

static int
stream_attribute_table_start_read(void *stream, struct stream_attribute_table *table)
{
	struct stream_attribute_table_start *start = stream;

	if (start->tag != STREAM_TAG_SATT)
		return 0;

	table->start = start;
	return sizeof(struct stream_attribute_table_start);
}

static int
stream_attribute_start_read(void *stream, struct stream_attribute *attribute)
{
	struct stream_attribute_start *start = stream;

	if (start->tag != STREAM_TAG_VATT)
		return 0;

	attribute->start = start;
	return sizeof(struct stream_attribute_start);
}

static int
stream_attribute_data_read(void *stream, struct stream_attribute *attribute)
{
	attribute->data = stream;

	return sizeof(struct stream_attribute_data);
}

static void
stream_attribute_table_destroy(struct stream_attribute_table *table)
{
	if (table) {
		while (table->attributes) {
			struct stream_attribute *attribute = table->attributes;

			table->attributes = attribute->next;
			free(attribute);
		}

		free(table);
	}
}

static struct stream_attribute_table *
stream_attribute_table_create(void *stream, int size)
{
	struct stream_attribute_table *table;
	struct stream_attribute *attribute;
	int offset = 0;
	int i;

	if (!stream || !size)
		return NULL;

	table = calloc(1, sizeof(struct stream_attribute_table));
	if (!table) {
		printf("%s: failed to allocate table\n", __func__);
		return NULL;
	}

	offset += stream_attribute_table_start_read(stream + offset, table);
	if (!table->start) {
		printf("%s: Error: missing table start at 0x%x\n",
		       __func__, offset);
		goto corrupt;
	}

	for (i = 0; i < table->start->count; i++) {
		attribute = calloc(1, sizeof(struct stream_attribute));
		if (!table) {
			printf("%s: failed to allocate attribute\n", __func__);
			goto oom;
		}

		offset += stream_attribute_start_read(stream + offset, attribute);
		if (!attribute->start) {
			printf("%s: Error: missing attribute start at 0x%x\n",
			       __func__, offset);
			goto corrupt;
		}

		offset += stream_string_read(stream + offset, &attribute->string);
		if (!attribute->string) {
			printf("%s: Error: missing string at 0x%x\n",
			       __func__, offset);
			goto corrupt;
		}

		offset += stream_attribute_data_read(stream + offset, attribute);
		if (!attribute->data) {
			printf("%s: Error: missing attribute data at 0x%x\n",
			       __func__, offset);
			goto corrupt;
		}

		attribute->next = table->attributes;
		table->attributes = attribute;
	}

	return table;
 oom:
	stream_attribute_table_destroy(table);
	return NULL;
 corrupt:
	stream_attribute_table_destroy(table);
	return NULL;
}

#if 0
static void
stream_attribute_table_print(struct stream_attribute_table *table)
{
	struct stream_attribute *attribute;

	attribute = table->attributes;
	while (attribute) {
		printf("attribute \"%s\" = {\n", attribute->string->string);
		printf("\t type 0x%02x, unknown01 0x%02x, component_count 0x%04x\n",
		       attribute->data->type, attribute->data->unknown01,
		       attribute->data->component_count);
		printf("\t component_size 0x%04x, entry_count 0x%04x\n",
		       attribute->data->component_size, attribute->data->entry_count);
		printf("\t stride 0x%04x, unknown0A 0x%02x, precision 0x%02x\n",
		       attribute->data->stride, attribute->data->unknown0A,
		       attribute->data->precision);
		printf("\t unknown0C 0x%04x, offset 0x%04x\n",
		       attribute->data->unknown0C, attribute->data->offset);
		printf("}\n");
		attribute = attribute->next;
	}
}
#endif

static struct symbol **
stream_attribute_table_to_symbols(struct stream_attribute_table *table,
				int *count)
{
	struct stream_attribute *attribute;
	struct symbol **symbols;
	int i;

	if (!table || !count)
		return NULL;

	*count = table->start->count;

	if (!table->start->count)
		return NULL;

	symbols = calloc(*count, sizeof(struct symbol *));
	if (!symbols) {
		printf("%s: Error: failed to allocate symbols: %s\n",
		       __func__, strerror(errno));
		goto error;
	}

	for (i = 0, attribute = table->attributes;
	     (i < table->start->count) && attribute;
	     i++, attribute = attribute->next) {
		struct symbol *symbol;
		symbol =
			symbol_create(attribute->string->string, SYMBOL_ATTRIBUTE,
				      attribute->data->component_size,
				      attribute->data->component_count,
				      attribute->data->entry_count,
				      0, 0, NULL, 0, 0);
		if (!symbol) {
			printf("%s: Error: failed to create symbol %s: %s\n",
			       __func__, attribute->string->string, strerror(errno));
			goto error;
		}

		symbol->offset = attribute->data->offset;

		symbols[symbol->offset / 4] = symbol;
	}

	return symbols;
 error:
	if (symbols) {
		for (i = 0; i < *count; i++)
			if (symbols[i])
				symbol_destroy(symbols[i]);
	}
	free(symbols);

	*count = 0;

	return NULL;
}

/*
 * Now for Varyings.
 */
#define STREAM_TAG_SVAR 0x52415653
#define STREAM_TAG_VVAR 0x52415656

struct stream_varying_table_start {
	unsigned int tag; /* SATT */
	int size;
	int count;
};

struct stream_varying_start {
	unsigned int tag; /* VATT */
	int size;
};

struct stream_varying_data { /* 0x14 */
	unsigned char type; /* 0x00 */
	unsigned char unknown01; /* 0x01 */
	unsigned short component_count; /* 0x02 */
	unsigned short component_size; /* 0x04 */
	unsigned short entry_count; /* 0x06. 0 == 1 */
	unsigned short stride; /* 0x08 */
	unsigned char unknown0A; /* 0x0A */
	unsigned char precision; /* 0x0B */
	unsigned short unknown0C; /* 0x0C */
	unsigned short unknown0E; /* 0x0E */
	unsigned short offset; /* 0x10 */
	unsigned short index; /* 0x12 */
};

struct stream_varying {
	struct stream_varying *next;

	struct stream_varying_start *start;
	struct stream_string *string;
	struct stream_varying_data *data;
};

struct stream_varying_table {
	struct stream_varying_table_start *start;

	struct stream_varying *varyings;
};

static int
stream_varying_table_start_read(void *stream, struct stream_varying_table *table)
{
	struct stream_varying_table_start *start = stream;

	if (start->tag != STREAM_TAG_SVAR)
		return 0;

	table->start = start;
	return sizeof(struct stream_varying_table_start);
}

static int
stream_varying_start_read(void *stream, struct stream_varying *varying)
{
	struct stream_varying_start *start = stream;

	if (start->tag != STREAM_TAG_VVAR)
		return 0;

	varying->start = start;
	return sizeof(struct stream_varying_start);
}

static int
stream_varying_data_read(void *stream, struct stream_varying *varying)
{
	varying->data = stream;

	return sizeof(struct stream_varying_data);
}

static void
stream_varying_table_destroy(struct stream_varying_table *table)
{
	if (table) {
		while (table->varyings) {
			struct stream_varying *varying = table->varyings;

			table->varyings = varying->next;
			free(varying);
		}

		free(table);
	}
}

static struct stream_varying_table *
stream_varying_table_create(void *stream, int size)
{
	struct stream_varying_table *table;
	struct stream_varying *varying;
	int offset = 0;
	int i;

	if (!stream || !size)
		return NULL;

	table = calloc(1, sizeof(struct stream_varying_table));
	if (!table) {
		printf("%s: failed to allocate table\n", __func__);
		return NULL;
	}

	offset += stream_varying_table_start_read(stream + offset, table);
	if (!table->start) {
		printf("%s: Error: missing table start at 0x%x\n",
		       __func__, offset);
		goto corrupt;
	}

	for (i = 0; i < table->start->count; i++) {
		varying = calloc(1, sizeof(struct stream_varying));
		if (!table) {
			printf("%s: failed to allocate varying\n", __func__);
			goto oom;
		}

		offset += stream_varying_start_read(stream + offset, varying);
		if (!varying->start) {
			printf("%s: Error: missing varying start at 0x%x\n",
			       __func__, offset);
			goto corrupt;
		}

		offset += stream_string_read(stream + offset, &varying->string);
		if (!varying->string) {
			printf("%s: Error: missing string at 0x%x\n",
			       __func__, offset);
			goto corrupt;
		}

		offset += stream_varying_data_read(stream + offset, varying);
		if (!varying->data) {
			printf("%s: Error: missing varying data at 0x%x\n",
			       __func__, offset);
			goto corrupt;
		}

		varying->next = table->varyings;
		table->varyings = varying;
	}

	return table;
 oom:
	stream_varying_table_destroy(table);
	return NULL;
 corrupt:
	stream_varying_table_destroy(table);
	return NULL;
}

#if 0
static void
stream_varying_table_print(struct stream_varying_table *table)
{
	struct stream_varying *varying;

	varying = table->varyings;
	while (varying) {
		printf("varying \"%s\" = {\n", varying->string->string);
		printf("\t type 0x%02x, unknown01 0x%02x, component_count 0x%04x\n",
		       varying->data->type, varying->data->unknown01,
		       varying->data->component_count);
		printf("\t component_size 0x%04x, entry_count 0x%04x\n",
		       varying->data->component_size, varying->data->entry_count);
		printf("\t stride 0x%04x, unknown0A 0x%02x, precision 0x%02x\n",
		       varying->data->stride, varying->data->unknown0A,
		       varying->data->precision);
		printf("\t unknown0C 0x%04x, offset 0x%04x\n",
		       varying->data->unknown0C, varying->data->offset);
		printf("}\n");
		varying = varying->next;
	}
}
#endif

static struct symbol **
stream_varying_table_to_symbols(struct stream_varying_table *table,
				int *count)
{
	struct stream_varying *varying;
	struct symbol **symbols;
	int i;

	if (!table || !count)
		return NULL;

	*count = table->start->count;

	if (!table->start->count)
		return NULL;

	symbols = calloc(*count, sizeof(struct symbol *));
	if (!symbols) {
		printf("%s: Error: failed to allocate symbols: %s\n",
		       __func__, strerror(errno));
		goto error;
	}

	for (i = 0, varying = table->varyings;
	     (i < table->start->count) && varying;
	     i++, varying = varying->next) {
		struct symbol *symbol;

		if (varying->data->offset == 0xFFFF) {
			(*count)--;
			continue;
		}

		symbol = symbol_create(varying->string->string, SYMBOL_VARYING,
				       varying->data->component_size,
				       varying->data->component_count,
				       varying->data->entry_count,
				       0, 0, NULL, 0, 0);
		if (!symbol) {
			printf("%s: Error: failed to create symbol %s: %s\n",
			       __func__, varying->string->string, strerror(errno));
			goto error;
		}


		symbol->offset = varying->data->offset;

		symbols[symbol->offset / 4] = symbol;
	}

	return symbols;
 error:
	if (symbols) {
		for (i = 0; i < *count; i++)
			if (symbols[i])
				symbol_destroy(symbols[i]);
	}
	free(symbols);

	*count = 0;

	return NULL;
}

/*
 *
 */
void
vertex_shader_attributes_patch(unsigned int *shader, int size)
{
	int i;

	for (i = 0; i < size; i++) {
		int tmp = (shader[4 * i + 1] >> 26) & 0x1F;

		if (!(tmp & 0x10))
			continue;

		tmp &= 0x0F;
		shader[4 * i + 1] &= ~(0x0F << 26);

		/* program specific bit, for now */
		if (!tmp)
			tmp = 1;
		else
			tmp = 0;

		shader[4 * i + 1] |= tmp << 26;
	}
}

static int
vertex_shader_varyings_patch(unsigned int *shader, int size, int *indices)
{
	int i;

	for (i = 0; i < size; i++) {
		int tmp;

		/* ignore entries for now, until we know more */

		tmp = (shader[4 * i + 2] >> 26) & 0x1F;

		if (tmp & 0x10) {
			tmp &= 0x0F;
			if (indices[tmp] == -1) {
				printf("%s: invalid index 0x%x at line %d\n",
				       __func__, tmp, i);
				return -1;
			}
			tmp = indices[tmp] & 0x0F;

			shader[4 * i + 2] &= ~(0x0F << 26);

			shader[4 * i + 2] |= tmp << 26;
		}

		tmp = ((shader[4 * i + 2] >> 31) & 0x01) |
			((shader[4 * i + 3] << 1) & 0x1E);

		if (tmp & 0x10) {
			tmp &= 0x0F;
			if (indices[tmp] == -1) {
				printf("%s: invalid index 0x%x at line %d\n",
				       __func__, tmp, i);
				return -1;
			}
			tmp = indices[tmp] & 0x0F;

			shader[4 * i + 2] &= ~(0x01 << 31);
			shader[4 * i + 3] &= ~0x07;

			shader[4 * i + 2] |= tmp << 31;
			shader[4 * i + 3] |= tmp >> 1;
		}
	}

	return 0;
}

void
limare_shader_binary_free(struct lima_shader_binary *binary)
{
	free(binary->error_log);
	free(binary->shader);
	free(binary->varying_stream);
	free(binary->uniform_stream);
	free(binary->attribute_stream);
	free(binary);
}

struct lima_shader_binary *
limare_shader_compile(struct limare_state *state, int type, const char *source)
{
	struct lima_shader_binary *binary;
	int length = strlen(source);
	int ret;

	binary = calloc(1, sizeof(struct lima_shader_binary));
	if (!binary) {
		printf("%s: Error: allocation failed: %s\n",
		       __func__, strerror(errno));
		return NULL;
	}

	if (state->kernel_version >= MALI_DRIVER_VERSION_R3P0) {
		struct lima_shader_binary_mbs mbs_binary[1] = {{ 0 }};

		ret = __mali_compile_essl_shader((struct lima_shader_binary *)
						 mbs_binary, type, source,
						 &length, 1);
		if (ret) {
			if (mbs_binary->error_log)
				printf("%s: compilation failed: %s\n",
				       __func__, mbs_binary->error_log);
			else
				printf("%s: compilation failed: %s\n",
				       __func__, mbs_binary->oom_log);

			free(mbs_binary->error_log);
			free(mbs_binary->shader);
			free(mbs_binary->mbs_stream);
			free(mbs_binary->varying_stream);
			free(mbs_binary->uniform_stream);
			free(mbs_binary->attribute_stream);
			return NULL;
		}

		/* we are not using the mbs */
		free(mbs_binary->mbs_stream);

		binary->compile_status = mbs_binary->compile_status;
		binary->shader = mbs_binary->shader;
		binary->shader_size = mbs_binary->shader_size;
		binary->varying_stream = mbs_binary->varying_stream;
		binary->varying_stream_size = mbs_binary->varying_stream_size;
		binary->uniform_stream = mbs_binary->uniform_stream;
		binary->uniform_stream_size = mbs_binary->uniform_stream_size;
		binary->attribute_stream = mbs_binary->attribute_stream;
		binary->attribute_stream_size = mbs_binary->attribute_stream_size;

		binary->parameters.fragment = mbs_binary->parameters.fragment;
	} else {
		ret = __mali_compile_essl_shader(binary, type,
						 source, &length, 1);
		if (ret) {
			if (binary->error_log)
				printf("%s: compilation failed: %s\n",
				       __func__, binary->error_log);
			else
				printf("%s: compilation failed: %s\n",
				       __func__, binary->oom_log);

			limare_shader_binary_free(binary);
			return NULL;
		}
	}



	return binary;
}

int
vertex_shader_attach(struct limare_state *state, const char *source)
{
	struct lima_shader_binary *binary;
	struct stream_uniform_table *uniform_table;
	struct stream_attribute_table *attribute_table;
	struct stream_varying_table *varying_table;

	binary = limare_shader_compile(state, LIMA_SHADER_VERTEX, source);
	if (!binary)
		return -1;

	uniform_table =
		stream_uniform_table_create(binary->uniform_stream,
					    binary->uniform_stream_size);
	if (uniform_table) {
		state->vertex_uniforms =
			stream_uniform_table_to_symbols(uniform_table,
							&state->vertex_uniform_count,
							&state->vertex_uniform_size);
		stream_uniform_table_destroy(uniform_table);
	}

	attribute_table =
		stream_attribute_table_create(binary->attribute_stream,
					      binary->attribute_stream_size);
	if (attribute_table) {
		state->vertex_attributes =
			stream_attribute_table_to_symbols(attribute_table,
							  &state->vertex_attribute_count);
		stream_attribute_table_destroy(attribute_table);
	}

	varying_table =
		stream_varying_table_create(binary->varying_stream,
					    binary->varying_stream_size);
	if (varying_table) {
		state->vertex_varyings =
			stream_varying_table_to_symbols(varying_table,
							&state->vertex_varying_count);
		stream_varying_table_destroy(varying_table);
	}

	state->vertex_varying_something = binary->parameters.vertex.varying_something;

	state->vertex_binary = binary;

	return 0;
}

int
fragment_shader_attach(struct limare_state *state, const char *source)
{
	struct lima_shader_binary *binary;
	struct stream_uniform_table *uniform_table;
	struct stream_varying_table *varying_table;

	binary = limare_shader_compile(state, LIMA_SHADER_FRAGMENT, source);
	if (!binary)
		return -1;

	uniform_table =
		stream_uniform_table_create(binary->uniform_stream,
					    binary->uniform_stream_size);
	if (uniform_table) {
		state->fragment_uniforms =
			stream_uniform_table_to_symbols(uniform_table,
							&state->fragment_uniform_count,
							&state->fragment_uniform_size);
		stream_uniform_table_destroy(uniform_table);
	}

	varying_table =
		stream_varying_table_create(binary->varying_stream,
					    binary->varying_stream_size);
	if (varying_table) {
		state->fragment_varyings =
			stream_varying_table_to_symbols(varying_table,
							&state->fragment_varying_count);
		stream_varying_table_destroy(varying_table);
	}

	state->fragment_binary = binary;

	return 0;
}

void
state_symbols_print(struct limare_state *state)
{
	int i;

	printf("Vertex symbols:\n");
	printf("\tAttributes: %d\n", state->vertex_attribute_count);
	for (i = 0; i < state->vertex_attribute_count; i++)
		symbol_print(state->vertex_attributes[i]);
	printf("\tVaryings: %d\n", state->vertex_varying_count);
	for (i = 0; i < state->vertex_varying_count; i++)
		symbol_print(state->vertex_varyings[i]);
	printf("\tUniforms: %d\n", state->vertex_uniform_count);
	for (i = 0; i < state->vertex_uniform_count; i++)
		symbol_print(state->vertex_uniforms[i]);

	printf("Fragment symbols:\n");
	printf("\tVaryings: %d\n", state->fragment_varying_count);
	for (i = 0; i < state->fragment_varying_count; i++)
		symbol_print(state->fragment_varyings[i]);
	printf("\tUniforms: %d\n", state->fragment_uniform_count);
	for (i = 0; i < state->fragment_uniform_count; i++)
		symbol_print(state->fragment_uniforms[i]);
}

/*
 * Checks whether vertex and fragment attributes match.
 */
static int
limare_link_varyings_match(struct limare_state *state)
{
	int i, j;

	/* now make sure that our varyings are present in both */
	for (i = 0; i < state->fragment_varying_count; i++) {
		struct symbol *fragment = state->fragment_varyings[i];

		for (j = 0; j < state->vertex_varying_count; j++) {
			struct symbol *vertex = state->vertex_varyings[j];

			if (!strcmp(fragment->name, vertex->name)) {
				if (fragment->component_size != vertex->component_size) {
					printf("%s: Error: component_size mismatch for varying \"%s\".\n",
					       __func__, fragment->name);
					return -1;
				}

				if (fragment->component_count != vertex->component_count) {
					printf("%s: Error: component_count mismatch for varying \"%s\".\n",
					       __func__, fragment->name);
					return -1;
				}

				if (fragment->entry_count != vertex->entry_count) {
					printf("%s: Error: entry_count mismatch for varying \"%s\".\n",
					       __func__, fragment->name);
					return -1;
				}

				if (fragment->size != vertex->size) {
					printf("%s: Error: size mismatch for varying \"%s\".\n",
					       __func__, fragment->name);
					return -1;
				}

				break;
			}
		}

		if (j == state->vertex_varying_count) {
			printf("%s: Error: vertex shader does not provide "
			       "varying \"%s\".\n", __func__, fragment->name);
			return -1;
		}
	}

	/* now check for standard varyings, which might not be defined in
	 * the fragment symbol list */
	if (state->fragment_varying_count != state->vertex_varying_count) {
		for (i = 0, j = 0; i < state->vertex_varying_count; i++) {
			struct symbol *vertex = state->vertex_varyings[i];

			if (!strncmp(vertex->name, "gl_", 3))
				j++;
		}

		if (state->fragment_varying_count >
		    (state->vertex_varying_count + j)) {
			printf("%s: superfluous vertex shader varyings detected.\n",
			       __func__);
			/* no error... yet... */
		}

	}

	return 0;
}

/*
 * TODO: when there are multiple varyings defined in the fragment shader
 * how do we order then? Most likely from symbol->offset too.
 */
static void
limare_link_varyings_indices_get(struct limare_state *state, int *varyings)
{
	int i, j;

	for (i = 0; i < state->fragment_varying_count; i++) {
		struct symbol *fragment = state->fragment_varyings[i];

		for (j = 0; j < state->vertex_varying_count; j++) {
			struct symbol *vertex = state->vertex_varyings[j];

			if (!strcmp(fragment->name, vertex->name))
				varyings[vertex->offset / 4] = i;
		}
	}

	for (j = 0; j < state->vertex_varying_count; j++) {
		if (varyings[j] == -1) {
			struct symbol *vertex = state->vertex_varyings[j];

			if (!strcmp(vertex->name, "gl_Position")) {
				varyings[vertex->offset / 4] = state->fragment_varying_count;
				break;
			}
		}
	}

#if 0
	for (i = 0; i < 16; i++)
		printf("varyings[%d] = %d\n", i, varyings[i]);
#endif
}

void
vertex_shader_varyings_reorder(struct limare_state *state, int *varyings)
{
	struct symbol *symbol;
	struct symbol *symbols[16] = { 0 };
	int i;

	for (i = 0; i < state->vertex_varying_count; i++) {
		symbol = state->vertex_varyings[i];

		symbol->offset = varyings[i] * 4;
		symbols[varyings[i]] = symbol;
	}

	memcpy(state->vertex_varyings, symbols,
	       state->vertex_varying_count * sizeof(struct symbol *));
}

int
limare_link(struct limare_state *state)
{
	int varyings[16] = { -1, -1, -1, -1, -1, -1, -1, -1,
			     -1, -1, -1, -1, -1, -1, -1, -1 };

	if (limare_link_varyings_match(state))
		return -1;

	limare_link_varyings_indices_get(state, varyings);
	vertex_shader_varyings_patch(state->vertex_binary->shader,
				     state->vertex_binary->shader_size / 16,
				     varyings);
	vertex_shader_varyings_reorder(state, varyings);

	return 0;
}
