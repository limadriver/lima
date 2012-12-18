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
#ifndef LIMARE_PROGRAM_H
#define LIMARE_PROGRAM_H 1

struct varying_map {
	int offset;
	int entries;
	int entry_size;
};

struct limare_program {
	int handle;

	unsigned int mem_physical;
	unsigned int mem_size;
	void *mem_address;

	struct lima_shader_binary *vertex_binary;
	int vertex_offset;
	int vertex_size;

	struct symbol **vertex_uniforms;
	int vertex_uniform_count;
	int vertex_uniform_size;

	struct symbol **vertex_attributes;
	int vertex_attribute_count;

	struct symbol **vertex_varyings;
	int vertex_varying_count;

	struct lima_shader_binary *fragment_binary;
	int fragment_offset;
	int fragment_size;

	struct symbol **fragment_uniforms;
	int fragment_uniform_count;
	int fragment_uniform_size;

	struct symbol **fragment_varyings;
	int fragment_varying_count;

	struct symbol *gl_Position;
	struct symbol *gl_PointSize;

	struct varying_map varying_map[12];
	int varying_map_count;
	int varying_map_size;
};

struct limare_program *
limare_program_create(void *address, unsigned int physical,
		      int offset, int size);

int
limare_program_vertex_shader_attach(struct limare_state *state,
				    struct limare_program *program,
				    const char *source);

int
limare_program_fragment_shader_attach(struct limare_state *state,
				      struct limare_program *program,
				      const char *source);

int limare_program_link(struct limare_program *program);

#endif /* LIMARE_PROGRAM_H */
