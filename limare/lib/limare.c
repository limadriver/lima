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

#define u32 uint32_t
#include "linux/mali_ioctl.h"

#include "version.h"
#include "limare.h"
#include "plb.h"
#include "gp.h"
#include "pp.h"
#include "jobs.h"
#include "symbols.h"
#include "compiler.h"
#include "texture.h"
#include "hfloat.h"
#include "program.h"

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

	if (frame->plb)
		plb_destroy(frame->plb);

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

	/* first, set up the plb, this is unchanged between draws. */
	frame->plb = plb_create(state, frame);
	if (!frame->plb) {
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

	return frame;
}

/* here we still hardcode our memory addresses. */
int
limare_state_setup(struct limare_state *state, int width, int height,
		    unsigned int clear_color)
{
	if (!state)
		return -1;

	state->width = width;
	state->height = height;

	state->clear_color = clear_color;

	/* space for our programs and textures. */
	state->aux_mem_size = 0x200000;
	state->aux_mem_physical = state->mem_base + 0x300000;
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


	state->programs = calloc(1, sizeof(struct program *));
	state->programs[0] = limare_program_create(state->aux_mem_address,
						   state->aux_mem_physical,
						   0, 0x1000);
	state->program_count = 1;
	state->program_current = 0;

	state->aux_mem_used += 0x1000;

	/* try to grab the necessary space for our image */
	state->dest_mem_size = state->width * state->height * 4;
	state->dest_mem_physical = state->mem_base + 0x0500000;
	state->dest_mem_address = mmap(NULL, state->dest_mem_size,
				       PROT_READ | PROT_WRITE,
				       MAP_SHARED, state->fd,
				       state->dest_mem_physical);
	if (state->dest_mem_address == MAP_FAILED) {
		printf("Error: failed to mmap offset 0x%x (0x%x): %s\n",
		       state->dest_mem_physical, state->dest_mem_size,
		       strerror(errno));
		return -1;
	}

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
	struct limare_program *program =
		state->programs[state->program_current];
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

int
limare_attribute_pointer(struct limare_state *state, char *name,
			 int component_size, int component_count,
			 int entry_count, void *data)
{
	struct limare_program *program =
		state->programs[state->program_current];
	struct symbol *symbol;
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

	if (symbol->component_size != component_size) {
		printf("%s: Error: Attribute %s has different dimensions\n",
		       __func__, name);
		return -1;
	}

	if (symbol->data && symbol->data_allocated) {
		free(symbol->data);
		symbol->data = NULL;
		symbol->data_allocated = 0;
	}

	symbol->component_count = component_count;
	symbol->entry_count = entry_count;
	symbol->size = symbol->component_size * symbol->component_count *
		symbol->entry_count;
	symbol->data = data;
	return 0;
}

int
limare_gl_mali_ViewPortTransform(struct limare_state *state,
				  struct symbol *symbol)
{
	float x0 = 0, y0 = 0, x1 = state->width, y1 = state->height;
	float depth_near = 0, depth_far = 1.0;
	float *viewport;

	if (symbol->data && symbol->data_allocated) {
		free(symbol->data);
		symbol->data = NULL;
		symbol->data_allocated = 0;
	}

	symbol->data = calloc(8, sizeof(float));
	if (!symbol->data) {
		printf("%s: Error: Failed to allocate data: %s\n",
		       __func__, strerror(errno));
		return -1;
	}

	symbol->data_allocated = 1;

	viewport = symbol->data;

	viewport[0] = x1 / 2;
	viewport[1] = y1 / 2;
	viewport[2] = (depth_far - depth_near) / 2;
	viewport[3] = depth_far;
	viewport[4] = (x0 + x1) / 2;
	viewport[5] = (y0 + y1) / 2;
	viewport[6] = (depth_near + depth_far) / 2;
	viewport[7] = depth_near;

	return 0;
}

int
attribute_upload(struct limare_state *state, struct symbol *symbol)
{
	int size;

	/* already uploaded? */
	if (symbol->physical)
		return 0;

	size = ALIGN(symbol->size, 0x40);

	if ((state->aux_mem_size - state->aux_mem_used) < size) {
		printf("%s: Not enough space for %s\n", __func__, symbol->name);
		return -1;
	}

	symbol->address = state->aux_mem_address + state->aux_mem_used;
	symbol->physical = state->aux_mem_physical + state->aux_mem_used;
	state->aux_mem_used += size;

	memcpy(symbol->address, symbol->data, symbol->size);

	return 0;
}

int
limare_texture_attach(struct limare_state *state, char *uniform_name,
		      const void *pixels, int width, int height, int format)
{
	struct limare_program *program =
		state->programs[state->program_current];
	struct symbol *symbol;
	int unit = 0, i;

	if (state->texture) {
		printf("%s: already have a texture assigned\n", __func__);
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

	state->texture = texture_create(state, pixels, width, height, format);
	if (!state->texture)
		return -1;

	symbol->data_allocated = 1;
	symbol->data = calloc(1, symbol->size);

	if (symbol->size == 4)
		*((unsigned int *) symbol->data) = unit;
	else if (symbol->size == 2)
		*((unsigned short *) symbol->data) = unit;
	else
		printf("%s: Error: unhandled size for %s is.\n",
		       __func__, symbol->name);

	return 0;
}

static int
limare_draw(struct limare_state *state, int mode, int start, int count,
	    void *indices, int indices_type)
{
	struct limare_program *program =
		state->programs[state->program_current];
	struct limare_frame *frame =
		state->frames[state->frame_current];
	struct draw_info *draw;
	int i;

	if (!frame) {
		printf("%s: Error: no frame was set up!\n", __func__);
		return -1;
	}

	if (!frame->plb) {
		printf("%s: Error: plb member is not set up yet.\n", __func__);
		return -1;
	}

	if (start && indices) {
		printf("%s: Error: start provided when drawing elements.\n",
		       __func__);
		return -1;
	}

	/* Todo, check whether attributes all have data attached! */

	for (i = 0; i < program->vertex_uniform_count; i++) {
		struct symbol *symbol = program->vertex_uniforms[i];

		if (symbol->data)
			continue;

		if (!strcmp(symbol->name, "gl_mali_ViewportTransform")) {
			if (limare_gl_mali_ViewPortTransform(state, symbol))
				return -1;
		} else {
			printf("%s: Error: vertex uniform %s is empty.\n",
			       __func__, symbol->name);

			return -1;
		}
	}

	for (i = 0; i < program->fragment_uniform_count; i++) {
		struct symbol *symbol = program->fragment_uniforms[i];

		if (!symbol->data) {
			printf("%s: Error: fragment uniform %s is empty.\n",
			       __func__, symbol->name);

			return -1;
		}
	}

	if (frame->draw_count >= 32) {
		printf("%s: Error: too many draws already!\n", __func__);
		return -1;
	}

	draw = draw_create_new(state, frame, mode, start, count);
	frame->draws[frame->draw_count] = draw;
	frame->draw_count++;

	for (i = 0; i < program->vertex_attribute_count; i++) {
		attribute_upload(state, program->vertex_attributes[i]);
		vs_info_attach_attribute(frame, draw, program->vertex_attributes[i]);
	}

	if (vs_info_attach_varyings(program, frame, draw))
		return -1;

	if (vs_info_attach_uniforms(frame, draw,
				    program->vertex_uniforms,
				    program->vertex_uniform_count,
				    program->vertex_uniform_size))
		return -1;

	if (plbu_info_attach_uniforms(frame, draw,
				      program->fragment_uniforms,
				      program->fragment_uniform_count,
				      program->fragment_uniform_size))
		return -1;

	if (indices && plbu_info_attach_indices(frame, draw,
						indices, indices_type, count))
		return -1;

	if (plbu_info_attach_textures(frame, draw, &state->texture, 1))
		return -1;

	vs_commands_draw_add(state, frame, program, draw);
	vs_info_finalize(state, frame, program, draw, draw->vs);

	plbu_info_render_state_create(program, frame, draw);
	plbu_commands_draw_add(frame, draw);

	return 0;
}

int
limare_draw_arrays(struct limare_state *state, int mode, int start, int count)
{
	return limare_draw(state, mode, start, count, NULL, 0);
}

int
limare_draw_elements(struct limare_state *state, int mode, int count,
		     void *indices, int indices_type)
{
	return limare_draw(state, mode, 0, count, indices, indices_type);
}

int
limare_flush(struct limare_state *state)
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

	ret = limare_pp_job_start(state, frame->pp);
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

int
vertex_shader_attach(struct limare_state *state, const char *source)
{
	struct limare_program *program =
		state->programs[state->program_current];

	return limare_program_vertex_shader_attach(state, program, source);
}

int
fragment_shader_attach(struct limare_state *state, const char *source)
{
	struct limare_program *program =
		state->programs[state->program_current];

	return limare_program_fragment_shader_attach(state, program, source);
}

int
limare_link(struct limare_state *state)
{
	struct limare_program *program =
		state->programs[state->program_current];

	return limare_program_link(program);
}

int
limare_new(struct limare_state *state)
{
	state->frame_current = state->frame_count & 0x01;

	if (state->frames[state->frame_current])
		limare_frame_destroy(state->frames[state->frame_current]);

	state->frames[state->frame_current] =
		limare_frame_create(state, 0x180000 * state->frame_current,
				    0x180000);
	if (!state->frames[state->frame_current])
		return -1;

	state->frame_count++;

	return 0;
}
