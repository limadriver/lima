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

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <asm/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <inttypes.h>
#include <string.h>
#include <time.h>

#include <GLES2/gl2.h>

#define u32 uint32_t
#include "linux/mali_ioctl.h"

#include "version.h"
#include "limare.h"
#include "fb.h"
#include "plb.h"
#include "gp.h"
#include "pp.h"
#include "jobs.h"
#include "symbols.h"
#include "compiler.h"
#include "texture.h"
#include "hfloat.h"
#include "program.h"
#include "render_state.h"

#define FRAME_MEMORY_SIZE 0x180000
#define AUX_MEMORY_SIZE 0x01000000
#define FB_MEMORY_OFFSET 0x08000000

static int
limare_fd_open(struct limare_state *state)
{
	_mali_uk_get_api_version_s version = { 0 };
	int ret;

	state->fd = open("/dev/mali", O_RDWR);
	if (state->fd == -1) {
		printf("Error: Failed to open /dev/mali: %s\n",
		       strerror(errno));
		return errno;
	}

	ret = ioctl(state->fd, MALI_IOC_GET_API_VERSION, &version);
	if (ret) {
		printf("Error: %s: ioctl(GET_API_VERSION) failed: %s\n",
		       __func__, strerror(-ret));
		close(state->fd);
		state->fd = -1;
		return ret;
	}

	state->kernel_version = _GET_VERSION(version.version);
	printf("Kernel driver is version %d\n", state->kernel_version);

	return 0;
}

static int
limare_gpu_detect(struct limare_state *state)
{
	_mali_uk_get_pp_number_of_cores_s pp_number = { 0 };
	_mali_uk_get_pp_core_version_s pp_version = { 0 };
	_mali_uk_get_gp_number_of_cores_s gp_number = { 0 };
	_mali_uk_get_gp_core_version_s gp_version = { 0 };
	int ret, type;

	if (state->kernel_version < MALI_DRIVER_VERSION_R3P0) {
		ret = ioctl(state->fd, MALI_IOC_PP_NUMBER_OF_CORES_GET_R2P1,
			    &pp_number);
		if (ret) {
			printf("Error: %s: ioctl(PP_NUMBER_OF_CORES_GET) "
			       "failed: %s\n", __func__, strerror(-ret));
			return ret;
		}

		ret = ioctl(state->fd, MALI_IOC_PP_CORE_VERSION_GET_R2P1,
			    &pp_version);
		if (ret) {
			printf("Error: %s: ioctl(PP_CORE_VERSION_GET) "
			       "failed: %s\n", __func__, strerror(-ret));
			return ret;
		}

		ret = ioctl(state->fd, MALI_IOC_GP2_NUMBER_OF_CORES_GET_R2P1,
			    &gp_number);
		if (ret) {
			printf("Error: %s: ioctl(GP2_NUMBER_OF_CORES_GET) "
			       "failed: %s\n", __func__, strerror(-ret));
			return ret;
		}

		ret = ioctl(state->fd, MALI_IOC_GP2_CORE_VERSION_GET_R2P1,
			    &gp_version);
		if (ret) {
			printf("Error: %s: ioctl(GP2_CORE_VERSION_GET) "
			       "failed: %s\n", __func__, strerror(-ret));
			return ret;
		}
	} else {
		ret = ioctl(state->fd, MALI_IOC_PP_NUMBER_OF_CORES_GET_R3P0,
			    &pp_number);
		if (ret) {
			printf("Error: %s: ioctl(PP_NUMBER_OF_CORES_GET) "
			       "failed: %s\n", __func__, strerror(-ret));
			return ret;
		}

		ret = ioctl(state->fd, MALI_IOC_PP_CORE_VERSION_GET_R3P0,
			    &pp_version);
		if (ret) {
			printf("Error: %s: ioctl(PP_CORE_VERSION_GET) "
			       "failed: %s\n", __func__, strerror(-ret));
			return ret;
		}

		ret = ioctl(state->fd, MALI_IOC_GP2_NUMBER_OF_CORES_GET_R3P0,
			    &gp_number);
		if (ret) {
			printf("Error: %s: ioctl(GP2_NUMBER_OF_CORES_GET) "
			       "failed: %s\n", __func__, strerror(-ret));
			return ret;
		}

		ret = ioctl(state->fd, MALI_IOC_GP2_CORE_VERSION_GET_R3P0,
			    &gp_version);
		if (ret) {
			printf("Error: %s: ioctl(GP2_CORE_VERSION_GET) "
			       "failed: %s\n", __func__, strerror(-ret));
			return ret;
		}
	}

	switch (gp_version.version >> 16) {
	case MALI_CORE_GP_200:
		type = 200;
		break;
	case MALI_CORE_GP_300:
		type = 300;
		break;
	case MALI_CORE_GP_400:
		type = 400;
		break;
	case MALI_CORE_GP_450:
		type = 450;
		break;
	default:
		type = 0;
		break;
	}

	printf("Detected %d Mali-%03d GP Cores.\n",
	       gp_number.number_of_cores, type);

	switch (pp_version.version >> 16) {
	case MALI_CORE_PP_200:
		type = 200;
		break;
	case MALI_CORE_PP_300:
		type = 300;
		break;
	case MALI_CORE_PP_400:
		type = 400;
		break;
	case MALI_CORE_PP_450:
		type = 450;
		break;
	default:
		type = 0;
		break;
	}

	printf("Detected %d Mali-%03d PP Cores.\n",
	       pp_number.number_of_cores, type);

	if (type == 200)
		state->type = 200;
	else if (type == 400)
		state->type = 400;
	else
		fprintf(stderr, "Unhandled Mali hw!\n");

	return 0;
}

/*
 * TODO: MEMORY MANAGEMENT!!!!!!!!!
 *
 */
static int
limare_mem_init(struct limare_state *state)
{
	_mali_uk_init_mem_s mem_init = { 0 };
	int ret;

	mem_init.ctx = (void *) state->fd;
	mem_init.mali_address_base = 0;
	mem_init.memory_size = 0;
	ret = ioctl(state->fd, MALI_IOC_MEM_INIT, &mem_init);
	if (ret == -1) {
		printf("Error: ioctl MALI_IOC_MEM_INIT failed: %s\n",
		       strerror(errno));
		return errno;
	}

	state->mem_base = mem_init.mali_address_base;

	return 0;
}

/*
 * simplistic benchmarking.
 */
static struct timespec framerate_time;

static void
limare_framerate_init(struct limare_state *state)
{
	if (clock_gettime(CLOCK_MONOTONIC, &framerate_time))
		printf("Error: failed to get time: %s\n", strerror(errno));
}

static void
limare_framerate_print(struct limare_state *state, int count)
{
	struct timespec new = { 0 };
	int usec;

	if (clock_gettime(CLOCK_MONOTONIC, &new)) {
		printf("Error: failed to get time: %s\n", strerror(errno));
		return;
	}

	usec = (new.tv_sec - framerate_time.tv_sec) * 1000000;
	usec += (new.tv_nsec - framerate_time.tv_nsec) / 1000;

	framerate_time = new;

	printf("%d frames in %f seconds: %f fps\n", count,
	       (float) usec / 1000000, (float) (count * 1000000) / usec);
}

struct limare_state *
limare_init(void)
{
	struct limare_state *state;
	int ret;

	state = calloc(1, sizeof(struct limare_state));
	if (!state) {
		printf("%s: Error: failed to allocate state: %s\n",
		       __func__, strerror(errno));
		goto error;
	}

	ret = limare_fd_open(state);
	if (ret)
		goto error;

	ret = limare_gpu_detect(state);
	if (ret)
		goto error;

	ret = limare_mem_init(state);
	if (ret)
		goto error;

	state->render_state_template = limare_render_state_template();
	if (!state->render_state_template)
		goto error;

	fb_open(state);

	limare_framerate_init(state);

	limare_jobs_init(state);

	return state;
 error:
	free(state);
	return NULL;
}

void
limare_frame_destroy(struct limare_frame *frame)
{
	int i;

	if (!frame)
		return;

	if (frame->mem_address)
		munmap(frame->mem_address, frame->mem_size);

	for (i = 0; i < frame->draw_count; i++)
		draw_info_destroy(frame->draws[i]);

	if (frame->pp)
		pp_info_destroy(frame->pp);

	free(frame);
}

struct limare_frame *
limare_frame_create(struct limare_state *state, int offset, int size)
{
	struct limare_frame *frame;

	frame = calloc(1, sizeof(struct limare_frame));
	if (!frame)
		return NULL;

	frame->id = state->frame_count;
	frame->index = frame->id & 0x01;

	/* space for our programs and textures. */
	frame->mem_size = size;
	frame->mem_used = 0;
	frame->mem_physical = state->mem_base + offset;
	frame->mem_address = mmap(NULL, frame->mem_size,
				  PROT_READ | PROT_WRITE,
				  MAP_SHARED, state->fd,
				  frame->mem_physical);
	if (frame->mem_address == MAP_FAILED) {
		printf("Error: failed to mmap offset 0x%x (0x%x): %s\n",
		       frame->mem_physical, frame->mem_size,
		       strerror(errno));
		limare_frame_destroy(frame);
		return NULL;
	}

	if (frame_plb_create(state, frame)) {
		limare_frame_destroy(frame);
		return NULL;
	}

	/* now add the area for the pp, again, unchanged between draws. */
	frame->pp = pp_info_create(state, frame);
	if (!frame->pp) {
		limare_frame_destroy(frame);
		return NULL;
	}

	/* now the two command queues */
	if (vs_command_queue_create(frame, 0x4000) ||
	    plbu_command_queue_create(state, frame, 0x4000, 0x50000)) {
		limare_frame_destroy(frame);
		return NULL;
	}

	state->viewport_dirty = 1;
	state->depth_dirty = 1;

	return frame;
}

static void
limare_state_init(struct limare_state *state, unsigned int clear_color)
{
	state->clear_color = clear_color;

	state->viewport_x = 0.0;
	state->viewport_y = 0.0;
	state->viewport_w = state->width;
	state->viewport_h = state->height;

	state->depth_func = GL_LESS;
	state->depth_near = 0.0;
	state->depth_far = 1.0;
}

/* here we still hardcode our memory addresses. */
int
limare_state_setup(struct limare_state *state, int width, int height,
		   unsigned int clear_color)
{
	if (!state)
		return -1;

	/*
	 * we have two frames, FRAME_MEMORY_SIZE large, so available memory
	 * starts at 0x300000.
	 */

	/*
	 * Statically assign a given number of blocks for a fixed number of
	 * programs. Later on diverge them one by one.
	 */
	state->program_mem_physical = state->mem_base + 2 * FRAME_MEMORY_SIZE;
	state->program_mem_size = LIMARE_PROGRAM_COUNT * LIMARE_PROGRAM_SIZE;
	state->program_mem_address =
		mmap(NULL, state->program_mem_size, PROT_READ | PROT_WRITE,
		     MAP_SHARED, state->fd, state->program_mem_physical);
	if (state->program_mem_address == MAP_FAILED) {
		printf("Error: failed to mmap offset 0x%x (0x%x): %s\n",
		       state->program_mem_physical, state->program_mem_size,
		       strerror(errno));
		return -1;
	}

	/* space for our textures. */
	state->aux_mem_size = AUX_MEMORY_SIZE;
	state->aux_mem_physical =
		state->program_mem_physical + state->program_mem_size;
	state->aux_mem_address = mmap(NULL, state->aux_mem_size,
				       PROT_READ | PROT_WRITE,
				       MAP_SHARED, state->fd,
				       state->aux_mem_physical);
	if (state->aux_mem_address == MAP_FAILED) {
		printf("Error: failed to mmap offset 0x%x (0x%x): %s\n",
		       state->aux_mem_physical, state->aux_mem_size,
		       strerror(errno));
		return -1;
	}

	/* try to grab the necessary space for our image */
	if (fb_init(state, width, height, FB_MEMORY_OFFSET))
		return -1;

	limare_state_init(state, clear_color);

	state->plb = plb_info_create(state);
	if (!state->plb)
		return -1;

	return 0;
}

int
symbol_attach_data(struct symbol *symbol, int count, float *data)
{
	if (symbol->data && symbol->data_allocated) {
		free(symbol->data);
		symbol->data = NULL;
		symbol->data_allocated = 0;
	}

	if (symbol->precision == 3) {
		symbol->data = data;
		symbol->data_allocated = 0;
	} else {
		int i;

		symbol->data = malloc(2 * count);
		if (!symbol->data)
			return -ENOMEM;

		for (i = 0; i < count; i++)
			((hfloat *) symbol->data)[i] = float_to_hfloat(data[i]);

		symbol->data_allocated = 1;
	}
	return 0;
}

int
limare_uniform_attach(struct limare_state *state, char *name, int count, float *data)
{
	struct limare_program *program = state->program_current;
	int found = 0, i, ret;

	for (i = 0; i < program->vertex_uniform_count; i++) {
		struct symbol *symbol = program->vertex_uniforms[i];

		if (!strcmp(symbol->name, name)) {
			if (symbol->component_count != count) {
				printf("%s: Error: Uniform %s has wrong dimensions\n",
				       __func__, name);
				return -1;
			}

			ret = symbol_attach_data(symbol, count, data);
			if (ret)
				return ret;

			found = 1;
			break;
		}
	}

	for (i = 0; i < program->fragment_uniform_count; i++) {
		struct symbol *symbol = program->fragment_uniforms[i];

		if (!strcmp(symbol->name, name)) {
			if (symbol->component_count != count) {
				printf("%s: Error: Uniform %s has wrong dimensions\n",
				       __func__, name);
				return -1;
			}

			ret = symbol_attach_data(symbol, count, data);
			if (ret)
				return ret;

			found = 1;
			break;
		}
	}

	if (!found) {
		printf("%s: Error: Unable to find uniform %s\n",
		       __func__, name);
		return -1;
	}

	return 0;
}

static struct {
	enum limare_attrib_type type;
	char *name;
	int size;
} limare_attrib_types[] = {
	{LIMARE_ATTRIB_FLOAT,	"LIMARE_ATTRIB_FLOAT",	 4},
	{LIMARE_ATTRIB_I16,	"LIMARE_ATTRIB_I16",	 2},
	{LIMARE_ATTRIB_U16,	"LIMARE_ATTRIB_U16",	 2},
	{LIMARE_ATTRIB_I8,	"LIMARE_ATTRIB_I8",	 1},
	{LIMARE_ATTRIB_U8,	"LIMARE_ATTRIB_U8",	 1},
	{LIMARE_ATTRIB_I8N,	"LIMARE_ATTRIB_I8N",	 1},
	{LIMARE_ATTRIB_U8N,	"LIMARE_ATTRIB_U8N",	 1},
	{LIMARE_ATTRIB_I16N,	"LIMARE_ATTRIB_I16N",	 2},
	{LIMARE_ATTRIB_U16N,	"LIMARE_ATTRIB_U16N",	 2},
	{LIMARE_ATTRIB_FIXED,	"LIMARE_ATTRIB_FIXED",	 4},
	{0, NULL, 0},
};

int
limare_attrib_type_size(enum limare_attrib_type type)
{
	int i;

	for (i = 0; limare_attrib_types[i].name; i++)
		if (limare_attrib_types[i].type == type)
			return limare_attrib_types[i].size;

	return 0;
}

int
limare_attribute_pointer(struct limare_state *state, char *name,
			 enum limare_attrib_type type, int component_count,
			 int entry_stride, int entry_count, void *data)
{
	struct limare_program *program = state->program_current;
	struct symbol *symbol = NULL;
	int component_size;
	int i;

	for (i = 0; i < program->vertex_attribute_count; i++) {
		symbol = program->vertex_attributes[i];

		if (!strcmp(symbol->name, name))
			break;
	}

	if (i == program->vertex_attribute_count) {
		printf("%s: Error: Unable to find attribute %s\n",
		       __func__, name);
		return -1;
	}

	if (symbol->precision != 3) {
		printf("%s: Attribute %s has unsupported precision\n",
		       __func__, name);
		return -1;
	}

	component_size = limare_attrib_type_size(type);
	if (!component_size)
		printf("%s: Invalid attribute type %d\n", __func__, type);

	if (!entry_stride)
		entry_stride = component_size * component_count;
#if 0
	if (symbol->component_size != component_size) {
		printf("%s: Error: Attribute %s has different dimensions\n",
		       __func__, name);
		return -1;
	}
#endif

	if (symbol->data && symbol->data_allocated)
		free(symbol->data);
	symbol->data = NULL;
	symbol->data_allocated = 0;
	symbol->data_handle = 0;

	symbol->component_type = type;
	symbol->component_count = component_count;
	symbol->entry_count = entry_count;
	symbol->entry_stride = entry_stride;
	symbol->size = symbol->entry_stride * symbol->entry_count;

	symbol->data = data;

	return 0;
}

static int
attribute_upload(struct limare_frame *frame, struct symbol *symbol)
{
	void *address;
	int size;

	size = ALIGN(symbol->size, 0x40);

	if ((frame->mem_size - frame->mem_used) < size) {
		printf("%s: Not enough space for %s\n", __func__, symbol->name);
		return -1;
	}

	address = frame->mem_address + frame->mem_used;
	symbol->mem_physical = frame->mem_physical + frame->mem_used;
	frame->mem_used += size;

	memcpy(address, symbol->data, symbol->size);

	return 0;
}

int
limare_attribute_buffer_upload(struct limare_state *state,
			       enum limare_attrib_type type,
			       int component_count, int entry_stride,
			       int entry_count, void *data)
{
	struct limare_attribute_buffer *buffer;
	int i, size, component_size;
	void *address;

	for (i = 0; i < LIMARE_ATTRIBUTE_BUFFER_COUNT; i++)
		if (!state->attribute_buffers[i])
			break;

	if (i == LIMARE_ATTRIBUTE_BUFFER_COUNT) {
		printf("%s: all attribute buffer slots have been taken!\n",
		       __func__);
		return -1;
	}

	buffer = calloc(1, sizeof(struct limare_attribute_buffer));
	if (!buffer) {
		printf("%s: Error: failed to allocate attribute buffer: %s\n",
		       __func__, strerror(errno));
		return -1;
	}

	component_size = limare_attrib_type_size(type);
	if (!component_size)
		printf("%s: Invalid attribute type %d\n", __func__, type);

	if (!entry_stride)
		entry_stride = component_size * component_count;

	size = entry_stride * entry_count;
	size = ALIGN(size, 0x40);

	if ((state->aux_mem_size - state->aux_mem_used) < size) {
		printf("%s: Not enough space for buffer\n", __func__);
		free(buffer);
		return -1;
	}

	address = state->aux_mem_address + state->aux_mem_used;
	buffer->mem_offset = state->aux_mem_used;
	buffer->mem_physical = state->aux_mem_physical + state->aux_mem_used;
	state->aux_mem_used += size;

	buffer->component_type = type;
	buffer->component_count = component_count;
	buffer->entry_stride = entry_stride;
	buffer->entry_count = entry_count;

	buffer->handle = 0x80000000 + state->attribute_buffer_handles;
	state->attribute_buffer_handles++;

	memcpy(address, data, size);

	state->attribute_buffers[i] = buffer;

	return buffer->handle;
}

int
limare_attribute_buffer_attach(struct limare_state *state, char *name,
			       int buffer_handle)
{
	struct limare_program *program = state->program_current;
	struct symbol *symbol = NULL;
	struct limare_attribute_buffer *buffer;
	int i;

	for (i = 0; i < LIMARE_ATTRIBUTE_BUFFER_COUNT; i++) {
		buffer = state->attribute_buffers[i];

		if (!buffer)
			continue;

		if (buffer->handle == buffer_handle)
			break;
	}

	if (i == LIMARE_ATTRIBUTE_BUFFER_COUNT) {
		printf("%s: Error: Unable to find attribute buffer 0x%08X\n",
		       __func__, buffer_handle);
		return -1;
	}

	for (i = 0; i < program->vertex_attribute_count; i++) {
		symbol = program->vertex_attributes[i];

		if (!strcmp(symbol->name, name))
			break;
	}

	if (i == program->vertex_attribute_count) {
		printf("%s: Error: Unable to find attribute %s\n",
		       __func__, name);
		return -1;
	}

	if (symbol->precision != 3) {
		printf("%s: Attribute %s has unsupported precision\n",
		       __func__, name);
		return -1;
	}

#if 0
	if (symbol->component_size != buffer->component_size) {
		printf("%s: Error: Attribute %s has different dimensions\n",
		       __func__, name);
		return -1;
	}
#endif

	if (symbol->data && symbol->data_allocated)
		free(symbol->data);
	symbol->data = NULL;
	symbol->data_allocated = 0;

	symbol->component_type = buffer->component_type;
	symbol->component_count = buffer->component_count;
	symbol->entry_count = buffer->entry_count;
	symbol->entry_stride = buffer->entry_stride;
	symbol->size = symbol->entry_stride * symbol->entry_count;

	symbol->data_handle = buffer_handle;
	symbol->mem_physical = buffer->mem_physical;

	return 0;
}

void
limare_viewport_transform(struct limare_state *state)
{
	float w = state->viewport_w / 2;
	float h = state->viewport_h / 2;
	float d = (state->depth_far - state->depth_near) / 2;

	state->viewport_transform[0] = w;
	state->viewport_transform[1] = -h;
	state->viewport_transform[2] = d;
	state->viewport_transform[3] = 1.0;
	state->viewport_transform[4] = state->viewport_x + w;
	state->viewport_transform[5] = state->viewport_y + h;
	state->viewport_transform[6] = state->depth_near + d;
	state->viewport_transform[7] = 0.0;
}

static struct limare_texture *
limare_texture_find(struct limare_state *state, int handle)
{
	int i;

	for (i = 0; i < LIMARE_TEXTURE_COUNT; i++) {
		struct limare_texture *texture = state->textures[i];

		if (texture && (texture->handle == handle))
			return texture;
	}

	return NULL;
}

int
limare_texture_upload(struct limare_state *state, const void *pixels,
		      int width, int height, int format, int mipmap)
{
	struct limare_texture *texture;
	int i;

	for (i = 0; i < LIMARE_TEXTURE_COUNT; i++)
		if (!state->textures[i])
			break;

	if (i == LIMARE_TEXTURE_COUNT) {
		printf("%s: all texture slots have been taken!\n", __func__);
		return -1;
	}

	texture = limare_texture_create(state, pixels, width, height, format,
					mipmap);
	if (!texture)
		return -1;

	texture->handle = state->texture_handles | 0xC0000000;
	state->texture_handles++;

	state->textures[i] = texture;

	return texture->handle;
}

int
limare_texture_mipmap_upload(struct limare_state *state, int handle, int level,
			     const void *pixels)
{
	struct limare_texture *texture = limare_texture_find(state, handle);

	if (!texture) {
		printf("%s: texture 0x%08X not found!\n", __func__, handle);
		return -1;
	}

	return limare_texture_mipmap_upload_low(state, texture, level, pixels);
}

int
limare_texture_parameters(struct limare_state *state, int handle,
			  int filter_mag, int filter_min,
			  int wrap_s, int wrap_t)
{
	struct limare_texture *texture = limare_texture_find(state, handle);

	if (!texture) {
		printf("%s: texture 0x%08X not found!\n", __func__, handle);
		return -1;
	}

	switch (filter_min) {
	case GL_NEAREST:
	case GL_LINEAR: /* default */
		break;
	default:
		printf("%s: Unsupported magnification filter value 0x%04X.\n",
		       __func__, filter_mag);
		return -1;
	}

	switch (filter_min) {
	case GL_NEAREST:
	case GL_LINEAR:
	case GL_NEAREST_MIPMAP_NEAREST: /* awkward supposed default */
	case GL_NEAREST_MIPMAP_LINEAR:
	case GL_LINEAR_MIPMAP_NEAREST:
	case GL_LINEAR_MIPMAP_LINEAR:
		break;
	default:
		printf("%s: Unsupported minification filter value 0x%04X.\n",
		       __func__, filter_min);
		return -1;
	}

	switch (wrap_s) {
	case GL_REPEAT:
	case GL_CLAMP_TO_EDGE:
	case GL_MIRRORED_REPEAT:
		break;
	default:
		printf("%s: Unsupported S wrap mode 0x%04X.\n",
		       __func__, wrap_s);
		return -1;
	}

	switch (wrap_t) {
	case GL_REPEAT:
	case GL_CLAMP_TO_EDGE:
	case GL_MIRRORED_REPEAT:
		break;
	default:
		printf("%s: Unsupported T wrap mode 0x%04X.\n",
		       __func__, wrap_t);
		return -1;
	}

	texture->filter_mag = filter_mag;
	texture->filter_min = filter_min;
	texture->wrap_s = wrap_s;
	texture->wrap_t = wrap_t;

	return limare_texture_parameters_set(texture);
}

int
limare_texture_attach(struct limare_state *state, char *uniform_name,
		      int handle)
{
	struct limare_texture *texture = limare_texture_find(state, handle);
	struct limare_program *program = state->program_current;
	struct symbol *symbol = NULL;
	int i;

	if (!texture) {
		printf("%s: texture 0x%08X not found!\n", __func__, handle);
		return -1;
	}

	if (!texture->complete) {
		printf("%s: Error: texture 0x%08X still lacks mipmaps!\n",
		       __func__, handle);
		return -1;
	}

	for (i = 0; i < program->fragment_uniform_count; i++) {
		symbol = program->fragment_uniforms[i];

		if (!strcmp(symbol->name, uniform_name))
			break;
	}

	if (symbol->data) {
		printf("%s: Error: vertex uniform %s is empty.\n",
		       __func__, symbol->name);
		return -1;
	}

	if (symbol->value_type != SYMBOL_SAMPLER) {
		printf("symbol %s is not a sampler!\n", symbol->name);
		return -1;
	}

	symbol->data_handle = handle;

	return 0;
}

static int
limare_draw(struct limare_state *state, int mode, int start, int count,
	    struct limare_indices_buffer *indices_buffer)
{
	struct limare_program *program = state->program_current;
	struct limare_frame *frame =
		state->frames[state->frame_current];
	struct draw_info *draw;
	int attributes_vertex_count = 0;
	int i;

	if (!frame) {
		printf("%s: Error: no frame was set up!\n", __func__);
		return -1;
	}

	if (start && indices_buffer) {
		printf("%s: Error: start provided when drawing elements.\n",
		       __func__);
		return -1;
	}

	for (i = 0; i < program->vertex_attribute_count; i++) {
		struct symbol *symbol = program->vertex_attributes[i];

		if (!symbol->data && !symbol->data_handle) {
			printf("%s: Error: attribute %s is empty.\n",
			       __func__, symbol->name);
			return -1;
		}

		if (!i)
			attributes_vertex_count = symbol->entry_count;
		else if (attributes_vertex_count != symbol->entry_count) {
			printf("%s: Error: attribute %s has wrong vertex count"
			       " %d.\n", __func__, symbol->name,
			       symbol->entry_count);
			return -1;
		}
	}

	/* do we need to update our viewport transform? */
	if (state->viewport_dirty || state->depth_dirty) {
		limare_viewport_transform(state);
		/* the dirty flags will be removed in the plbu */
	}

	for (i = 0; i < program->vertex_uniform_count; i++) {
		struct symbol *symbol = program->vertex_uniforms[i];

		if (symbol->data)
			continue;

		if (!strcmp(symbol->name, "gl_mali_ViewportTransform")) {
			symbol->data = state->viewport_transform;
			symbol->data_allocated = 0;
		} else {
			printf("%s: Error: vertex uniform %s is empty.\n",
			       __func__, symbol->name);

			return -1;
		}
	}

	for (i = 0; i < program->fragment_uniform_count; i++) {
		struct symbol *symbol = program->fragment_uniforms[i];

		if (!symbol->data && !symbol->data_handle) {
			printf("%s: Error: fragment uniform %s is empty.\n",
			       __func__, symbol->name);

			return -1;
		}
	}

	if (frame->draw_count >= LIMARE_DRAW_COUNT) {
		printf("%s: Error: too many draws already!\n", __func__);
		return -1;
	}

	if (indices_buffer)
		draw = draw_create_new(state, frame, mode,
				       attributes_vertex_count, start, count);
	else
		draw = draw_create_new(state, frame, mode, count, start, count);

	frame->draws[frame->draw_count] = draw;
	frame->draw_count++;

	for (i = 0; i < program->vertex_attribute_count; i++) {
		struct symbol *symbol = program->vertex_attributes[i];

		if (symbol->data)
			attribute_upload(frame, symbol);

		vs_info_attach_attribute(frame, draw, symbol);
	}

	if (vs_info_attach_varyings(program, frame, draw))
		return -1;

	if (vs_info_attach_uniforms(frame, draw,
				    program->vertex_uniforms,
				    program->vertex_uniform_count,
				    program->vertex_uniform_size))
		return -1;

	if (plbu_info_attach_textures(state, frame, draw))
		return -1;

	if (plbu_info_attach_uniforms(frame, draw,
				      program->fragment_uniforms,
				      program->fragment_uniform_count,
				      program->fragment_uniform_size))
		return -1;

	if (indices_buffer)
		plbu_info_attach_indices(draw, indices_buffer->indices_type,
					 indices_buffer->mem_physical);

	vs_commands_draw_add(state, frame, program, draw);
	vs_info_finalize(state, frame, program, draw, draw->vs);

	draw_render_state_create(frame, program, draw,
				 state->render_state_template);
	plbu_commands_draw_add(state, frame, draw);

	return 0;
}

int
limare_draw_arrays(struct limare_state *state, int mode, int start, int count)
{
	return limare_draw(state, mode, start, count, NULL);
}

/*
 * TODO: have a quick scan through the elements, and find the lowest and
 * highest indices. Then, only upload these, and limit the vertex count to
 * this. This might significantly reduce the amount of data the vs has to
 * churn through.
 */
int
limare_draw_elements(struct limare_state *state, int mode, int count,
		     void *indices, int indices_type)
{
	struct limare_frame *frame = state->frames[state->frame_current];
	struct limare_indices_buffer buffer;
	int size;
	void *address;

	buffer.handle = 0;
	buffer.drawing_mode = mode;
	buffer.indices_type = indices_type;
	buffer.count = count;

	if (indices_type == GL_UNSIGNED_BYTE)
		size = count;
	else if (indices_type == GL_UNSIGNED_SHORT)
		size = count * 2;
	else {
		printf("%s: only bytes and shorts supported.\n", __func__);
		return -1;
	}

	if ((frame->mem_size - frame->mem_used) < (0x40 + ALIGN(size, 0x40))) {
		printf("%s: no space for indices\n", __func__);
		return -1;
	}

	address = frame->mem_address + frame->mem_used;
	buffer.mem_physical = frame->mem_physical + frame->mem_used;
	frame->mem_used += ALIGN(size, 0x40);

	memcpy(address, indices, size);

	return limare_draw(state, mode, 0, count, &buffer);
}

int
limare_elements_buffer_upload(struct limare_state *state, int mode, int type,
			      int count, void *data)
{
	struct limare_indices_buffer *buffer;
	int i, size;
	void *address;

	for (i = 0; i < LIMARE_INDICES_BUFFER_COUNT; i++)
		if (!state->indices_buffers[i])
			break;

	if (i == LIMARE_INDICES_BUFFER_COUNT) {
		printf("%s: all indices buffer slots have been taken!\n",
		       __func__);
		return -1;
	}

	buffer = calloc(1, sizeof(struct limare_indices_buffer));
	if (!buffer) {
		printf("%s: Error: failed to allocate indices buffer: %s\n",
		       __func__, strerror(errno));
		return -1;
	}

	buffer->drawing_mode = mode;
	buffer->indices_type = type;
	buffer->count = count;

	if (type == GL_UNSIGNED_BYTE)
		size = count;
	else if (type == GL_UNSIGNED_SHORT)
		size = count * 2;
	else {
		printf("%s: only bytes and shorts supported.\n", __func__);
		free(buffer);
		return -1;
	}

	if ((state->aux_mem_size - state->aux_mem_used) <
	    (0x40 + ALIGN(size, 0x40))) {
		printf("%s: no space for indices\n", __func__);
		free(buffer);
		return -1;
	}

	address = state->aux_mem_address + state->aux_mem_used;
	buffer->mem_physical = state->aux_mem_physical + state->aux_mem_used;
	state->aux_mem_used += ALIGN(size, 0x40);

	memcpy(address, data, size);

	buffer->handle = 0x40000000 + state->indices_buffer_handles;
	state->indices_buffer_handles++;

	state->indices_buffers[i] = buffer;

	return buffer->handle;
}

int
limare_draw_elements_buffer(struct limare_state *state, int buffer_handle)
{
	struct limare_indices_buffer *buffer;
	int i;

	for (i = 0; i < LIMARE_INDICES_BUFFER_COUNT; i++) {
		if (!state->indices_buffers[i])
			continue;

		buffer = state->indices_buffers[i];
		if (buffer->handle == buffer_handle)
			break;
	}

	if (i == LIMARE_INDICES_BUFFER_COUNT) {
		printf("%s: Error: unable to fine handle 0x%08X\n",
		       __func__, buffer_handle);
		return -1;
	}

	return limare_draw(state, buffer->drawing_mode, 0, buffer->count,
			   buffer);
}

int
limare_frame_flush(struct limare_state *state)
{
	struct limare_frame *frame = state->frames[state->frame_current];
	int ret;

	if (!frame) {
		printf("%s: Error: no frame was set up!\n", __func__);
		return -1;
	}

	plbu_commands_finish(frame);

	ret = limare_gp_job_start(state, frame);
	if (ret)
		return ret;

	ret = limare_pp_job_start(state, frame);
	if (ret)
		return ret;

	return 0;
}

/*
 * Just run fflush(stdout) to give the wrapper library a chance to finish.
 */
void
limare_finish(struct limare_state *state)
{
	fflush(stdout);
	sleep(1);
}

static struct limare_program *
limare_program_find(struct limare_state *state, int handle)
{
	int i;

	for (i = 0; i < LIMARE_PROGRAM_COUNT; i++) {
		struct limare_program *program = state->programs[i];

		if (program && (program->handle == handle))
			return program;
	}

	return NULL;
}

int
limare_program_current(struct limare_state *state, int handle)
{
	struct limare_program *program = limare_program_find(state, handle);

	if (!program) {
		printf("%s: unable to find program with handle 0x%08X\n",
		       __func__, handle);

		return -1;
	}

	state->program_current = program;

	return 0;
}

int
limare_program_new(struct limare_state *state)
{
	struct limare_program *program;
	int i;

	for (i = 0; i < LIMARE_PROGRAM_COUNT; i++)
		if (!state->programs[i])
			break;

	if (i == LIMARE_PROGRAM_COUNT) {
		printf("%s: Error: no more program slots available!\n",
		       __func__);
		return -1;
	}

	program = limare_program_create(state->program_mem_address,
					state->program_mem_physical,
					i * LIMARE_PROGRAM_SIZE,
					LIMARE_PROGRAM_SIZE);
	if (!program)
		return -ENOMEM;

	program->handle = state->program_handles;
	state->program_handles++;

	state->program_current = program;
	state->programs[i] = program;

	return program->handle;
}

int
vertex_shader_attach(struct limare_state *state, int handle,
		     const char *source)
{
	struct limare_program *program = limare_program_find(state, handle);

	if (!program) {
		printf("%s: unable to find program with handle 0x%08X\n",
		       __func__, handle);

		return -1;
	}

	return limare_program_vertex_shader_attach(state, program, source);
}

int
fragment_shader_attach(struct limare_state *state, int handle,
		       const char *source)
{
	struct limare_program *program = limare_program_find(state, handle);

	if (!program) {
		printf("%s: unable to find program with handle 0x%08X\n",
		       __func__, handle);

		return -1;
	}

	return limare_program_fragment_shader_attach(state, program, source);
}

int
limare_link(struct limare_state *state)
{
	struct limare_program *program = state->program_current;

	return limare_program_link(program);
}

static int
limare_depth_clear_init(struct limare_state *state)
{
	struct limare_program *program;
	const char *source =
		"precision mediump float;\n"
		"\n"
		"void main()\n"
		"{\n"
		"	gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);\n"
		"}\n";
	float vertices[12] = {
		   0.0,    0.0, 1.0, 1.0,
		4096.0,    0.0, 1.0, 1.0,
		   0.0, 4096.0, 1.0, 1.0,
	};
	unsigned char *indices;
	int ret;

	/*
	 * We need three blocks:
	 * 0: vertices (in a varying).
	 * 1: fragment shader.
	 * 2: indices.
	 */
	if ((state->aux_mem_size - state->aux_mem_used) < 0xC0) {
		printf("%s: no space left!\n", __func__);
		return -ENOMEM;
	}

	/*
	 * let this waste space for the vertex shader, we re-use it
	 * for our verticesaryings later on.
	 */
	program = limare_program_create(state->aux_mem_address,
					state->aux_mem_physical,
					state->aux_mem_used, 0x80);
	if (!program)
		return -ENOMEM;

	ret = limare_program_fragment_shader_attach(state, program, source);
	if (ret) {
		free(program);
		return ret;
	}

	ret = limare_depth_clear_link(state, program);
	if (ret) {
		/* TODO: clean up properly here. */
		return ret;
	}

	state->depth_clear_program = program;

	/* now get our vertices uploaded */
	state->depth_clear_vertices_physical =
		state->aux_mem_physical + state->aux_mem_used;

	memcpy(state->aux_mem_address + state->aux_mem_used,
	       vertices, 12 * sizeof(float));

	state->aux_mem_used += 0x80;

	/* now upload the indices */
	state->depth_clear_indices_physical =
		state->aux_mem_physical + state->aux_mem_used;
	indices = state->aux_mem_address + state->aux_mem_used;
	indices[0] = 0;
	indices[1] = 1;
	indices[2] = 2;
	state->aux_mem_used += 0x40;

	return 0;
}

int
limare_depth_clear(struct limare_state *state)
{
	struct limare_frame *frame = state->frames[state->frame_current];
	struct render_state template = {
		0x00000000, 0x00000000, 0x0C321892, 0x0000003F,
		0xFFFF0000, 0x00000007, 0x00000007, 0x00000000,
		0x0000F007, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000
	};
	struct draw_info *draw;
	int ret;

	if (!state->depth_clear_program) {
		ret = limare_depth_clear_init(state);
		if (ret)
			return ret;
	}

	if (frame->draw_count >= LIMARE_DRAW_COUNT) {
		printf("%s: Error: too many draws already!\n", __func__);
		return -1;
	}

	draw = draw_create_new(state, frame, LIMA_DRAW_QUAD_DIRECT, 3, 0, 3);
	frame->draws[frame->draw_count] = draw;
	frame->draw_count++;

	plbu_info_attach_indices(draw, GL_UNSIGNED_BYTE,
				 state->depth_clear_indices_physical);

	draw_render_state_create(frame, state->depth_clear_program, draw,
				 &template);
	plbu_commands_depth_clear_draw_add(state, frame, draw,
					   state->depth_clear_vertices_physical);

	return 0;
}

int
limare_frame_new(struct limare_state *state)
{
	state->frame_current = state->frame_count & 0x01;

	if (state->frames[state->frame_current])
		limare_frame_destroy(state->frames[state->frame_current]);

	state->frames[state->frame_current] =
		limare_frame_create(state,
				    FRAME_MEMORY_SIZE * state->frame_current,
				    FRAME_MEMORY_SIZE);
	if (!state->frames[state->frame_current])
		return -1;

	state->frame_count++;

	if (!(state->frame_count & 0x3F))
		limare_framerate_print(state, 64);

	return 0;
}

void
limare_buffer_clear(struct limare_state *state)
{
	fb_clear(state);
}

void
limare_buffer_swap(struct limare_state *state)
{
	limare_fb_flip(state, state->frames[state->frame_current]);
}

int
limare_enable(struct limare_state *state, int parameter)
{
	if (parameter == GL_DEPTH_TEST) {
		state->depth_test = 1;
		return limare_render_state_depth_func(state->
						      render_state_template,
						      state->depth_func);
	} else
		return limare_render_state_set(state->render_state_template,
					       parameter, 1);
}

int
limare_disable(struct limare_state *state, int parameter)
{
	if (parameter == GL_DEPTH_TEST) {
		state->depth_test = 0;
		return limare_render_state_depth_func(state->
						      render_state_template,
						      GL_ALWAYS);
	} else
		return limare_render_state_set(state->render_state_template,
					       parameter, 0);
}

int
limare_depth_func(struct limare_state *state, int value)
{
	int ret = 0;

	state->depth_func = value;

	if (state->depth_test)
		ret = limare_render_state_depth_func(state->
						     render_state_template,
						     value);
	return ret;
}

int
limare_depth_mask(struct limare_state *state, int value)
{
	return limare_render_state_depth_mask(state->render_state_template,
					      value);
}

int
limare_depth(struct limare_state *state, float near, float far)
{
	if (near < 0.0)
		near = 0.0;
	else if (near > 1.0)
		near = 1.0;

	if (far < 0.0)
		far = 0.0;
	else if (far > 1.0)
		far = 1.0;

	state->depth_near = near;
	state->depth_far = far;
	state->depth_dirty = 1;

	return limare_render_state_depth(state->render_state_template,
					 near, far);
}

int
limare_viewport(struct limare_state *state, int x, int y,
		int width, int height)
{
	/*
	 * TODO: fix some of this with scissoring.
	 */
	if ((x < 0) || (y < 0) || (width < 0) || (height < 0) ||
	    ((x + width) > state->width) || ((y + height) > state->height)) {
		printf("%s: Error: dimensions outside window not supported.\n",
		       __func__);
		return -1;
	}

	if ((x + width) > state->width)
		width = state->width - x;
	if ((y + height) > state->height)
		height = state->height - y;

	state->viewport_x = x;
	state->viewport_y = state->height - (y + height);
	state->viewport_w = width;
	state->viewport_h = height;

	state->viewport_dirty = 1;

	return 0;
}
