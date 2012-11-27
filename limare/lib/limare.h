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

#ifndef LIMARE_LIMARE_H
#define LIMARE_LIMARE_H 1

#define ALIGN(x, y) (((x) + ((y) - 1)) & ~((y) - 1))

struct lima_cmd {
	unsigned int val;
	unsigned int cmd;
};

struct varying_map {
	int offset;
	int entries;
	int entry_size;
};

struct limare_state {
	int fd;
	int kernel_version;

#define LIMARE_TYPE_M200 200
#define LIMARE_TYPE_M400 400
	int type;

	unsigned int mem_physical;
	unsigned int mem_size;
	void *mem_address;

	int width;
	int height;

	unsigned int clear_color;

	struct draw_info *draws[32];
	int draw_count;

	unsigned int draw_mem_offset;
	int draw_mem_size;

	struct plb *plb;

	struct pp_info *pp;

	struct lima_cmd *vs_commands;
	int vs_commands_physical;
	int vs_commands_count;
	int vs_commands_size;

	struct lima_cmd *plbu_commands;
	int plbu_commands_physical;
	int plbu_commands_count;
	int plbu_commands_size;

	unsigned int texture_mem_offset;
	int texture_mem_size;
	struct texture *texture;

	/* program */
	struct lima_shader_binary *vertex_binary;

	struct symbol **vertex_uniforms;
	int vertex_uniform_count;
	int vertex_uniform_size;

	struct symbol **vertex_attributes;
	int vertex_attribute_count;

	struct symbol **vertex_varyings;
	int vertex_varying_count;

	struct lima_shader_binary *fragment_binary;

	struct symbol **fragment_uniforms;
	int fragment_uniform_count;
	int fragment_uniform_size;

	struct symbol **fragment_varyings;
	int fragment_varying_count;

	struct symbol *gl_Position;
	struct symbol *gl_PointSize;

	struct sampler **fragment_samplers;
	int fragment_sampler_count;

	struct varying_map varying_map[12];
	int varying_map_count;
	int varying_map_size;
};

/* from limare.c */
struct limare_state *limare_init(void);
int limare_state_setup(struct limare_state *state, int width, int height,
			unsigned int clear_color);
int limare_uniform_attach(struct limare_state *state, char *name,
			  int count, float *data);
int limare_attribute_pointer(struct limare_state *state, char *name, int size,
			      int count, void *data);
int limare_texture_attach(struct limare_state *state, char *uniform_name,
			  const void *pixels, int width, int height,
			  int format);
int limare_draw_arrays(struct limare_state *state, int mode,
			int vertex_start, int vertex_count);
int limare_flush(struct limare_state *state);
void limare_finish(void);

#endif /* LIMARE_LIMARE_H */
