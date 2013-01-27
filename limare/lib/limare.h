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

	int render_status;

	struct limare_state *state;
	pthread_mutex_t mutex;

	unsigned int mem_physical;
	int mem_size;
	int mem_used;
	void *mem_address;

	unsigned int tile_heap_offset;
	int tile_heap_size;

#define LIMARE_DRAW_COUNT 512
	struct draw_info *draws[LIMARE_DRAW_COUNT];
	int draw_count;

	/* locations of our plb buffers and pointers in our frame memory */
	/* holds the actual polygons */
	int plb_offset;
	/* holds the addresses so the plbu knows where to store the polygons */
	int plb_plbu_offset;
	/* holds the coordinates and addresses of the polygons for the PP */
	int plb_pp_offset;

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

enum limare_attrib_type {
	LIMARE_ATTRIB_FLOAT = 0x000,
	/* todo: find out what lives here. */
	LIMARE_ATTRIB_I16   = 0x004,
	LIMARE_ATTRIB_U16   = 0x005,
	LIMARE_ATTRIB_I8    = 0x006,
	LIMARE_ATTRIB_U8    = 0x007,
	LIMARE_ATTRIB_I8N   = 0x008,
	LIMARE_ATTRIB_U8N   = 0x009,
	LIMARE_ATTRIB_I16N  = 0x00A,
	LIMARE_ATTRIB_U16N  = 0x00B,
	/* todo: find out what lives here. */
	LIMARE_ATTRIB_FIXED = 0x101
};

struct limare_attribute_buffer {
	int handle;

	enum limare_attrib_type component_type;
	int component_count;
	int entry_stride;
	int entry_count;

	/* in AUX space */
	int mem_offset;
	unsigned int mem_physical;
};

struct limare_indices_buffer {
	int handle;

	int drawing_mode;
	int indices_type;
	int count;
	int start;

	unsigned int mem_physical;
};

#define FRAME_COUNT 3

struct limare_state {
	int fd;
	int kernel_version;

#define LIMARE_TYPE_M200 200
#define LIMARE_TYPE_M400 400
	int type;

	unsigned int mem_base;

	int width;
	int height;

	struct plb_info *plb;
	struct render_state *render_state_template;

	float viewport_transform[8];

	float viewport_x;
	float viewport_y;
	float viewport_w;
	float viewport_h;
	int viewport_dirty;

	int scissor;
	int scissor_x;
	int scissor_y;
	int scissor_w;
	int scissor_h;
	int scissor_dirty;

	float depth_near;
	float depth_far;
	int depth_dirty;

	int depth_test;
	int depth_func;

	int culling;
	int cull_front;
	int cull_back;
	int cull_front_cw;

	int polygon_offset;
	int polygon_offset_factor;
	int polygon_offset_units;

	int alpha_func;
	int alpha_func_func;
	float alpha_func_alpha;

	unsigned int clear_color;
	float depth_clear_depth;

	int frame_count;
	int frame_current;
	int frame_memory_max;
	struct limare_frame *frames[FRAME_COUNT];

	void *frame_mem_address;
	unsigned int frame_mem_physical;
	int frame_mem_size;

#define LIMARE_PROGRAM_COUNT 16
#define LIMARE_PROGRAM_SIZE 0x1000
	void *program_mem_address;
	unsigned int program_mem_physical;
	int program_mem_size;

	struct limare_program *programs[LIMARE_PROGRAM_COUNT];
	struct limare_program *program_current;
	int program_handles;

	struct limare_program *depth_buffer_clear_program;

	/* space used for vertex buffers and textures */
	void *aux_mem_address;
	unsigned int aux_mem_physical;
	int aux_mem_size;
	int aux_mem_used;

#define LIMARE_TEXTURE_COUNT 512
	struct limare_texture *textures[LIMARE_TEXTURE_COUNT];
	int texture_handles;

#define LIMARE_ATTRIBUTE_BUFFER_COUNT 16
	struct limare_attribute_buffer *
		attribute_buffers[LIMARE_ATTRIBUTE_BUFFER_COUNT];
	int attribute_buffer_handles;

#define LIMARE_INDICES_BUFFER_COUNT 4
	struct limare_indices_buffer *
	indices_buffers[LIMARE_INDICES_BUFFER_COUNT];
	int indices_buffer_handles;

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

int limare_program_new(struct limare_state *state);
int limare_program_current(struct limare_state *state, int handle);

int vertex_shader_attach(struct limare_state *state, int program_handle,
			 const char *source);
int fragment_shader_attach(struct limare_state *state, int program_handle,
			   const char *source);
int limare_link(struct limare_state *state);

int limare_texture_upload(struct limare_state *state, const void *pixels,
			  int width, int height, int format, int mipmap);
int limare_texture_mipmap_upload(struct limare_state *state, int handle,
				 int level, const void *pixels);
int limare_texture_parameters(struct limare_state *state, int handle,
			      int filter_mag, int filter_min,
			      int wrap_s, int wrap_t);
int limare_texture_attach(struct limare_state *state, char *uniform_name,
			  int texture_handle);

int limare_uniform_attach(struct limare_state *state, char *name,
			  int count, float *data);
int limare_attribute_pointer(struct limare_state *state, char *name,
			     enum limare_attrib_type type, int component_count,
			     int entry_stride, int entry_count, void *data);
int limare_attribute_buffer_upload(struct limare_state *state,
				   enum limare_attrib_type type,
				   int component_count, int entry_stride,
				   int entry_count, void *data);
int limare_attribute_buffer_attach(struct limare_state *state, char *name,
				   int buffer_handle);

int limare_elements_buffer_upload(struct limare_state *state, int mode,
				  int type, int count, void *data);

int limare_draw_arrays(struct limare_state *state, int mode,
		       int vertex_start, int vertex_count);
int limare_draw_elements(struct limare_state *state, int mode, int count,
			 void *indices, int indices_type);
int limare_draw_elements_buffer(struct limare_state *state, int buffer_handle);

int limare_depth_buffer_clear(struct limare_state *state);

int limare_frame_new(struct limare_state *state);
int limare_frame_flush(struct limare_state *state);

void limare_buffer_clear(struct limare_state *state);
void limare_buffer_swap(struct limare_state *state);
void limare_buffer_size(struct limare_state *state, int *width, int *height);

void limare_finish(struct limare_state *state);

int limare_enable(struct limare_state *state, int parameter);
int limare_disable(struct limare_state *state, int parameter);
int limare_depth_func(struct limare_state *state, int value);
int limare_depth_mask(struct limare_state *state, int value);
int limare_blend_func(struct limare_state *state, int sfactor, int dfactor);
int limare_depth(struct limare_state *state, float near, float far);
int limare_viewport(struct limare_state *state, int x, int y,
		    int width, int height);
int limare_scissor(struct limare_state *state, int x, int y,
		   int width, int height);
int limare_cullface(struct limare_state *state, int face);
int limare_frontface(struct limare_state *state, int face);
int limare_polygon_offset(struct limare_state *state,
			  float factor, float units);
int limare_alpha_func(struct limare_state *state, int func, float alpha);
int limare_depth_clear_depth(struct limare_state *state, float depth);
int limare_color_mask(struct limare_state *state,
		      int red, int green, int blue, int alpha);

#endif /* LIMARE_LIMARE_H */
