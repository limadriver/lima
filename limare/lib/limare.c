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

	state->mem_physical = mem_init.mali_address_base;
	state->mem_size = 0x200000;
	state->mem_address = mmap(NULL, state->mem_size, PROT_READ | PROT_WRITE,
				  MAP_SHARED, state->fd, state->mem_physical);
	if (state->mem_address == MAP_FAILED) {
		printf("Error: failed to mmap offset 0x%x (0x%x): %s\n",
		       state->mem_physical, state->mem_size, strerror(errno));
		return errno;
	}

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

	/* first, set up the plb, this is unchanged between draws. */
	state->plb = plb_create(state, state->mem_physical, state->mem_address,
				0x00000, 0x30000);
	if (!state->plb)
		return -1;

	/* now add the area for the pp, again, unchanged between draws. */
	state->pp = pp_info_create(state,
				   state->mem_address, state->mem_physical,
				   0x30000, 0x1000,
				   state->mem_physical + 0x200000);
	if (!state->pp)
		return -1;

	/* now the two command queues */
	if (vs_command_queue_create(state, 0x31000, 0x4000) ||
	    plbu_command_queue_create(state, 0x35000, 0x4000))
		return -1;

	state->draw_mem_offset = 0x40000;
	state->draw_mem_size = 0x70000;

	state->programs = calloc(1, sizeof(struct program *));
	state->programs[0] = limare_program_create(state->mem_address,
						   state->mem_physical,
						   0xF0000, 0x10000);
	state->program_count = 1;
	state->program_current = 0;

	state->texture_mem_offset = 0x100000;
	state->texture_mem_size = 0x100000;

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
limare_attribute_pointer(struct limare_state *state, char *name, int size,
			  int count, void *data)
{
	struct limare_program *program =
		state->programs[state->program_current];
	int i;

	for (i = 0; i < program->vertex_attribute_count; i++) {
		struct symbol *symbol = program->vertex_attributes[i];

		if (!strcmp(symbol->name, name)) {
			if (symbol->precision != 3) {
				printf("%s: Attribute %s has unsupported precision\n",
				       __func__, name);
				return -1;
			}

			if (symbol->component_size == size) {
				symbol->component_count = count;
				symbol->data = data;
				return 0;
			}

			printf("%s: Error: Attribute %s has different dimensions\n",
			       __func__, name);
			return -1;
		}
	}

	printf("%s: Error: Unable to find attribute %s\n",
	       __func__, name);
	return -1;
}

int
limare_gl_mali_ViewPortTransform(struct limare_state *state,
				  struct symbol *symbol)
{
	float x0 = 0, y0 = 0, x1 = state->width, y1 = state->height;
	float depth_near = 0, depth_far = 1.0;
	float *viewport;

	if (symbol->data) {
		if (symbol->data_allocated)
			free(symbol->data);
	}

	symbol->data = calloc(8, sizeof(float));
	if (!symbol->data) {
		printf("%s: Error: Failed to allocate data: %s\n",
		       __func__, strerror(errno));
		return -1;
	}

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

int
limare_draw_arrays(struct limare_state *state, int mode, int start, int count)
{
	struct limare_program *program =
		state->programs[state->program_current];
	struct draw_info *draw;
	int i;

	if (!state->plb) {
		printf("%s: Error: plb member is not set up yet.\n", __func__);
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
			printf("%s: Error: vertex uniform %s is empty.\n",
			       __func__, symbol->name);

			return -1;
		}
	}

	if (state->draw_count >= 32) {
		printf("%s: Error: too many draws already!\n", __func__);
		return -1;
	}

	if (state->draw_mem_size < 0x1000) {
		printf("%s: Error: no more space available!\n", __func__);
		return -1;
	}

	draw = draw_create_new(state, state->draw_mem_offset, 0x1000,
			       mode, start, count);
	state->draws[state->draw_count] = draw;

	state->draw_mem_offset += 0x1000;
	state->draw_mem_size -= 0x1000;
	state->draw_count++;

	for (i = 0; i < program->vertex_attribute_count; i++) {
		struct symbol *symbol;

		symbol = symbol_copy(program->vertex_attributes[i], start, count);
		if (symbol)
			vs_info_attach_attribute(draw, symbol);

	}

	if (vs_info_attach_varyings(program, draw))
		return -1;

	if (vs_info_attach_uniforms(draw, program->vertex_uniforms,
				    program->vertex_uniform_count,
				    program->vertex_uniform_size))
		return -1;

	if (plbu_info_attach_uniforms(draw, program->fragment_uniforms,
				      program->fragment_uniform_count,
				      program->fragment_uniform_size))
		return -1;

	if (plbu_info_attach_textures(draw, &state->texture, 1))
		return -1;

	vs_commands_draw_add(state, program, draw);
	vs_info_finalize(state, program, draw, draw->vs);

	plbu_info_render_state_create(state, program, draw);
	plbu_commands_draw_add(state, draw);

	return 0;
}

int
limare_flush(struct limare_state *state)
{
	int ret;

	plbu_commands_finish(state);

	ret = limare_gp_job_start(state);
	if (ret)
		return ret;

	ret = limare_pp_job_start(state, state->pp);
	if (ret)
		return ret;

	return 0;
}

/*
 * Just run fflush(stdout) to give the wrapper library a chance to finish.
 */
void
limare_finish(void)
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
