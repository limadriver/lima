/*
 * Copyright (c) 2011-2013 Luc Verhaegen <libv@skynet.be>
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
 *
 * Small utility to quickly compile shaders.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "compiler.h"

#define STREAM_TAG_MBS1 0x3153424d

void
usage(char *name)
{
	printf("usage: %s -[fv] shader.txt\n", name);
	printf("\n");
	printf("\t-f : fragment shader\n");
	printf("\t-v : vertex shader\n");

	exit(EINVAL);
}

void
hexdump(const void *data, int size)
{
	unsigned char *buf = (void *) data;
	char alpha[17];
	int i;

	for (i = 0; i < size; i++) {
		if (!(i % 16))
			printf("\t\t%08X", (unsigned int) buf + i);

		if (((void *) (buf + i)) < ((void *) data)) {
			printf("   ");
			alpha[i % 16] = '.';
		} else {
			printf(" %02X", buf[i]);

			if (isprint(buf[i]) && (buf[i] < 0xA0))
				alpha[i % 16] = buf[i];
			else
				alpha[i % 16] = '.';
		}

		if ((i % 16) == 15) {
			alpha[16] = 0;
			printf("\t|%s|\n", alpha);
		}
	}

	if (i % 16) {
		for (i %= 16; i < 16; i++) {
			printf("   ");
			alpha[i] = '.';

			if (i == 15) {
				alpha[16] = 0;
				printf("\t|%s|\n", alpha);
			}
		}
	}
}

void
dump_shader(unsigned int *shader, int size)
{
	int i;

	for (i = 0; i < size; i += 4)
		printf("\t\t0x%08x, 0x%08x, 0x%08x, 0x%08x, /* 0x%08x */\n",
		       shader[i + 0], shader[i + 1], shader[i + 2],
		       shader[i + 3], 4 * i);
}

static void
vertex_parameters_dump(struct lima_shader_binary_vertex_parameters *parameters)
{
	printf("\t.parameters (vertex) = {\n");
	printf("\t\t.unknown00 = 0x%x,\n", parameters->unknown00);
	printf("\t\t.unknown04 = 0x%x,\n", parameters->unknown04);
	printf("\t\t.unknown08 = 0x%x,\n", parameters->unknown08);
	printf("\t\t.unknown0C = 0x%x,\n", parameters->unknown0C);
	printf("\t\t.attribute_count = 0x%x,\n", parameters->attribute_count);
	printf("\t\t.varying_count = 0x%x,\n", parameters->varying_count);
	printf("\t\t.unknown18 = 0x%x,\n", parameters->unknown18);
	printf("\t\t.size = 0x%x,\n", parameters->size);
	printf("\t\t.varying_something = 0x%x,\n", parameters->varying_something);
	printf("\t},\n");
}

static void
fragment_parameters_dump(struct lima_shader_binary_fragment_parameters *parameters)
{
	printf("\t.parameters (fragment) = {\n");
	printf("\t\t.unknown00 = 0x%x,\n", parameters->unknown00);
	printf("\t\t.unknown04 = 0x%x,\n", parameters->unknown04);
	printf("\t\t.unknown08 = 0x%x,\n", parameters->unknown08);
	printf("\t\t.unknown0C = 0x%x,\n", parameters->unknown0C);
	printf("\t\t.unknown10 = 0x%x,\n", parameters->unknown10);
	printf("\t\t.unknown14 = 0x%x,\n", parameters->unknown14);
	printf("\t\t.unknown18 = 0x%x,\n", parameters->unknown18);
	printf("\t\t.unknown1C = 0x%x,\n", parameters->unknown1C);
	printf("\t\t.unknown20 = 0x%x,\n", parameters->unknown20);
	printf("\t\t.unknown24 = 0x%x,\n", parameters->unknown24);
	printf("\t\t.unknown28 = 0x%x,\n", parameters->unknown28);
	printf("\t\t.unknown2C = 0x%x,\n", parameters->unknown2C);
	printf("\t}\n");
}

void
dump_shader_binary_mbs(struct lima_shader_binary_mbs *binary, int type)
{
	printf("struct lima_shader_binary %p = {\n", binary);
	printf("\t.compile_status = %d,\n", binary->compile_status);
	printf("\t.error_log = \"%s\",\n", binary->error_log);
	printf("\t.shader = {\n");
	dump_shader(binary->shader, binary->shader_size / 4);
	printf("\t},\n");
	printf("\t.shader_size = 0x%x,\n", binary->shader_size);
	printf("\t.varying_stream = {\n");
	hexdump(binary->varying_stream, binary->varying_stream_size);
	printf("\t},\n");
	printf("\t.varying_stream_size = 0x%x,\n", binary->varying_stream_size);
	printf("\t.uniform_stream = {\n");
	hexdump(binary->uniform_stream, binary->uniform_stream_size);
	printf("\t},\n");
	printf("\t.uniform_stream_size = 0x%x,\n", binary->uniform_stream_size);
	printf("\t.attribute_stream = {\n");
	hexdump(binary->attribute_stream, binary->attribute_stream_size);
	printf("\t},\n");
	printf("\t.attribute_stream_size = 0x%x,\n", binary->attribute_stream_size);

	if (type == LIMA_SHADER_VERTEX)
		vertex_parameters_dump(&binary->parameters.vertex);
	else
		fragment_parameters_dump(&binary->parameters.fragment);

	printf("}\n");
}

void
dump_shader_binary(struct lima_shader_binary *binary, int type)
{
	printf("struct lima_shader_binary %p = {\n", binary);
	printf("\t.compile_status = %d,\n", binary->compile_status);
	printf("\t.error_log = \"%s\",\n", binary->error_log);
	printf("\t.shader = {\n");
	dump_shader(binary->shader, binary->shader_size / 4);
	printf("\t},\n");
	printf("\t.shader_size = 0x%x,\n", binary->shader_size);
	printf("\t.varying_stream = {\n");
	hexdump(binary->varying_stream, binary->varying_stream_size);
	printf("\t},\n");
	printf("\t.varying_stream_size = 0x%x,\n", binary->varying_stream_size);
	printf("\t.uniform_stream = {\n");
	hexdump(binary->uniform_stream, binary->uniform_stream_size);
	printf("\t},\n");
	printf("\t.uniform_stream_size = 0x%x,\n", binary->uniform_stream_size);
	printf("\t.attribute_stream = {\n");
	hexdump(binary->attribute_stream, binary->attribute_stream_size);
	printf("\t},\n");
	printf("\t.attribute_stream_size = 0x%x,\n", binary->attribute_stream_size);

	if (type == LIMA_SHADER_VERTEX)
		vertex_parameters_dump(&binary->parameters.vertex);
	else
		fragment_parameters_dump(&binary->parameters.fragment);

	printf("}\n");
}


int
main(int argc, char *argv[])
{
	struct lima_shader_binary_mbs binary = { 0 };
	char *filename, *source;
	int type, fd, size, length, ret;

	if (argc != 3) {
		printf("Error: Wrong number of arguments\n");
		usage(argv[0]);
	}

	if ((argv[1][0] != '-') ||
	    ((argv[1][1] != 'f') && (argv[1][1] != 'v'))) {
		printf("Error: Wrong shader type\n");
		usage(argv[0]);
	}

	if (argv[1][1] == 'f')
		type = LIMA_SHADER_FRAGMENT;
	else
		type = LIMA_SHADER_VERTEX;

	filename = argv[2];

	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		printf("Error: failed to open %s: %s\n",
			filename, strerror(errno));
		return errno;
	}

	{
		struct stat buf;

		if (stat(filename, &buf)) {
			printf("Error: failed to fstat %s: %s\n",
			       filename, strerror(errno));
			return errno;
		}

		size = buf.st_size;
		if (!size) {
			fprintf(stderr, "Error: %s is empty.\n", filename);
			return 0;
		}
	}

	source = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
	if (source == MAP_FAILED) {
		printf("Error: failed to mmap %s: %s\n",
		       filename, strerror(errno));
		return errno;
	}

	length = strlen(source);

	ret = __mali_compile_essl_shader((struct lima_shader_binary *) &binary,
					 type, source, &length, 1);
	if (ret) {
		if (binary.error_log)
			printf("%s: compilation failed: %s\n",
			       __func__, binary.error_log);
		else
			printf("%s: compilation failed: %s\n",
			       __func__, binary.oom_log);

		return ret;
	}

	if ((((unsigned int *)binary.mbs_stream)[0]) == STREAM_TAG_MBS1)
		dump_shader_binary_mbs(&binary, type);
	else
		dump_shader_binary((struct lima_shader_binary *) &binary, type);

	return 0;
}
