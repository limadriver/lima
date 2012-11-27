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

#ifndef LIMARE_GP_H
#define LIMARE_GP_H 1

struct gp_common_entry {
	unsigned int physical;
	int size; /* (element_size << 11) | (element_count - 1) */
};

struct gp_common {
	struct gp_common_entry attributes[0x10];
	struct gp_common_entry varyings[0x10];
};

struct vs_info {
	/* for m200: attributes and varyings are shared. */
	struct gp_common *common;
	int common_offset;
	int common_size;

	/* for m400 */
	struct gp_common_entry *attribute_area;
	int attribute_area_offset;
	int attribute_area_size;

	/* for m400 */
	struct gp_common_entry *varying_area;
	int varying_area_offset;
	int varying_area_size;

	int uniform_offset;
	int uniform_size;

	struct symbol *attributes[0x10];
	int attribute_count;

	/* fragment shader can only take up to 13 varyings. */
	int varying_offset;
	int varying_size;

	int gl_Position_offset;
	int gl_Position_size;
};

int vs_info_attach_uniforms(struct draw_info *draw, struct symbol **uniforms,
			    int count, int size);

int vs_info_attach_attribute(struct draw_info *draw, struct symbol *attribute);
int vs_info_attach_varyings(struct limare_program *program, struct draw_info *draw);

void vs_commands_draw_add(struct limare_state *state,
			  struct limare_frame *frame,
			  struct limare_program *program,
			  struct draw_info *draw);
void vs_info_finalize(struct limare_state *state, struct limare_program *program,
		      struct draw_info *draw, struct vs_info *info);

struct plbu_info {
	struct render_state *render_state;
	int render_state_offset;
	int render_state_size;

	int uniform_array_offset;
	int uniform_array_size;

	int uniform_offset;
	int uniform_size;
};

int vs_command_queue_create(struct limare_frame *frame, int offset, int size);
int plbu_command_queue_create(struct limare_state *state,
			      struct limare_frame *frame, int offset, int size);

void plbu_commands_draw_add(struct limare_frame *frame, struct draw_info *draw);
void plbu_commands_finish(struct limare_frame *frame);

int plbu_info_attach_uniforms(struct draw_info *draw, struct symbol **uniforms,
			    int count, int size);
int plbu_info_attach_textures(struct draw_info *draw, struct texture **textures,
			      int count);

int plbu_info_render_state_create(struct limare_program *program,
				  struct draw_info *draw);

struct draw_info {
	unsigned int mem_physical;
	unsigned int mem_size;
	int mem_used;
	void *mem_address;

	int draw_mode;
	int vertex_start;
	int vertex_count;

	struct vs_info vs[1];

	struct plbu_info plbu[1];

	int texture_descriptor_count;
	unsigned int texture_descriptor_list_offset;
};

struct draw_info *draw_create_new(struct limare_state *state,
				  struct limare_frame *frame,
				  int offset, int size, int draw_mode,
				  int vertex_start, int vertex_count);

void draw_info_destroy(struct draw_info *draw);

int limare_gp_job_start(struct limare_state *state,
			struct limare_frame *frame);

#endif /* LIMARE_GP_H */
