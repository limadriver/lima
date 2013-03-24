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

struct limare_frame {
	int id;
	int index;

	unsigned int mem_physical;
	int mem_size;
	int mem_used;
	void *mem_address;

	unsigned int tile_heap_offset;
	int tile_heap_size;

#define LIMARE_DRAW_COUNT 512
	struct draw_info *draws[LIMARE_DRAW_COUNT];
	int draw_count;

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
};

struct limare_state {
	int fd;
	int kernel_version;

#define LIMARE_TYPE_M200 200
#define LIMARE_TYPE_M400 400
	int type;

	unsigned int mem_base;

	int width;
	int height;

	unsigned int clear_color;

	int frame_count;
	int frame_current;
	struct limare_frame *frames[2];

	/* space used for programs and textures */
	void *aux_mem_address;
	unsigned int aux_mem_physical;
	int aux_mem_size;
	int aux_mem_used;

	struct texture *texture;

	int program_count;
	struct limare_program **programs;
	int program_current;

	struct limare_fb *fb;
};

/*
 * Draws a quad from only 3 vertices.
 *
 * For correct drawing:
 * - 3rd vertex must be the opposite side of the 1st.
 * - 2nd vertex must be the side corner in the direction that is not culled.
 */
#define LIMA_DRAW_QUAD_DIRECT 0x0F

/* from limare.c */
struct limare_state *limare_init(void);

int limare_state_setup(struct limare_state *state, int width, int height,
			unsigned int clear_color);
int vertex_shader_attach(struct limare_state *state, const char *source);
int fragment_shader_attach(struct limare_state *state, const char *source);
int limare_link(struct limare_state *state);

int limare_uniform_attach(struct limare_state *state, char *name,
			  int count, float *data);
int limare_attribute_pointer(struct limare_state *state, char *name,
			     int component_size, int component_count,
			     int entry_count, void *data);
int limare_texture_attach(struct limare_state *state, char *uniform_name,
			  const void *pixels, int width, int height,
			  int format);
int limare_draw_arrays(struct limare_state *state, int mode,
			int vertex_start, int vertex_count);
int limare_draw_elements(struct limare_state *state, int mode, int count,
			 void *indices, int indices_type);

void limare_finish(struct limare_state *state);

int limare_frame_new(struct limare_state *state);
int limare_frame_flush(struct limare_state *state);

void limare_buffer_clear(struct limare_state *state);
void limare_buffer_swap(struct limare_state *state);
void limare_buffer_size(struct limare_state *state, int *width, int *height);

#endif /* LIMARE_LIMARE_H */
