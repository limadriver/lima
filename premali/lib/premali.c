/*
 * Copyright (c) 2011-2012 Luc Verhaegen <libv@codethink.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */
#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <inttypes.h>

#define u32 uint32_t
#define USING_MALI200
#include "mali_200_regs.h"
#include "mali_ioctl.h"

#include "premali.h"
#include "plb.h"
#include "gp.h"
#include "pp.h"
#include "jobs.h"
#include "symbols.h"

static int
premali_fd_open(struct premali_state *state)
{
	state->fd = open("/dev/mali", O_RDWR);
	if (state->fd == -1) {
		printf("Error: Failed to open /dev/mali: %s\n",
		       strerror(errno));
		return errno;
	}

	return 0;
}

static int
premali_gpu_detect(struct premali_state *state)
{
	_mali_uk_get_system_info_size_s system_info_size;
	_mali_uk_get_system_info_s system_info_ioctl;
	struct _mali_system_info *system_info;
	int ret;

	ret = ioctl(state->fd, MALI_IOC_GET_SYSTEM_INFO_SIZE,
		    &system_info_size);
	if (ret) {
		printf("Error: %s: ioctl(GET_SYSTEM_INFO_SIZE) failed: %s\n",
		       __func__, strerror(ret));
		return ret;
	}

	system_info_ioctl.size = system_info_size.size;
	system_info_ioctl.system_info = calloc(1, system_info_size.size);
	if (!system_info_ioctl.system_info) {
		printf("%s: Error: failed to allocate system info: %s\n",
		       __func__, strerror(errno));
		return errno;
	}

	ret = ioctl(state->fd, MALI_IOC_GET_SYSTEM_INFO, &system_info_ioctl);
	if (ret) {
		printf("%s: Error: ioctl(GET_SYSTEM_INFO) failed: %s\n",
		       __func__, strerror(ret));
		free(system_info_ioctl.system_info);
		return ret;
	}

	system_info = system_info_ioctl.system_info;

	switch (system_info->core_info->type) {
	case _MALI_GP2:
	case _MALI_200:
		printf("Detected Mali-200.\n");
		state->type = 200;
		break;
	case _MALI_400_GP:
	case _MALI_400_PP:
		printf("Detected Mali-400.\n");
		state->type = 400;
		break;
	default:
		break;
	}

	return 0;
}

/*
 * TODO: MEMORY MANAGEMENT!!!!!!!!!
 *
 */
static int
premali_mem_init(struct premali_state *state)
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
	state->mem_size = 0x100000;
	state->mem_address = mmap(NULL, state->mem_size, PROT_READ | PROT_WRITE,
				  MAP_SHARED, state->fd, state->mem_physical);
	if (state->mem_address == MAP_FAILED) {
		printf("Error: failed to mmap offset 0x%x (0x%x): %s\n",
		       state->mem_physical, state->mem_size, strerror(errno));
		return errno;
	}

	return 0;
}

struct premali_state *
premali_init(void)
{
	struct premali_state *state;
	int ret;

	state = calloc(1, sizeof(struct premali_state));
	if (!state) {
		printf("%s: Error: failed to allocate state: %s\n",
		       __func__, strerror(errno));
		goto error;
	}

	ret = premali_fd_open(state);
	if (ret)
		goto error;

	ret = premali_gpu_detect(state);
	if (ret)
		goto error;

	ret = premali_mem_init(state);
	if (ret)
		goto error;

	return state;
 error:
	free(state);
	return NULL;
}

/* here we still hardcode our memory addresses. */
int
premali_state_setup(struct premali_state *state, int width, int height,
		    unsigned int clear_color)
{
	if (!state)
		return -1;

	state->width = width;
	state->height = height;

	state->clear_color = clear_color;

	/* first, set up the plb, this is unchanged between draws. */
	state->plb = plb_create(state, state->mem_physical, state->mem_address,
				0x00000, 0x80000);
	if (!state->plb)
		return -1;

	/* now add the area for the pp, again, unchanged between draws. */
	state->pp = pp_info_create(state, state->mem_address + 0x80000,
				   state->mem_physical + 0x80000,
				   0x1000, state->mem_physical + 0x100000);
	if (!state->pp)
		return -1;

	/* now the two command queues */
	if (vs_command_queue_create(state, 0x81000, 0x4000) ||
	    plbu_command_queue_create(state, 0x85000, 0x4000))
		return -1;


	state->vs = vs_info_create(state, state->mem_address + 0x89000,
				   state->mem_physical + 0x89000, 0x1000);
	if (!state->vs)
		return -1;

	state->plbu = plbu_info_create(state->mem_address + 0x8A000,
				       state->mem_physical + 0x8A000,
				       0x1000);
	if (!state->plbu)
		return -1;

	return 0;
}

int
premali_uniform_attach(struct premali_state *state, char *name, int size,
		       int count, void *data)
{
	int found = 0, i;

	for (i = 0; i < state->vertex_uniform_count; i++) {
		struct symbol *symbol = state->vertex_uniforms[i];

		if (!strcmp(symbol->name, name)) {
			if ((symbol->component_size == size) &&
			    (symbol->component_count == count)) {
				symbol->data = data;
				found = 1;
				break;
			}

			printf("%s: Error: Uniform %s has wrong dimensions\n",
			       __func__, name);
			return -1;
		}
	}

	for (i = 0; i < state->fragment_uniform_count; i++) {
		struct symbol *symbol = state->fragment_uniforms[i];

		if (!strcmp(symbol->name, name)) {
			if ((symbol->component_size == size) &&
			    (symbol->component_count == count)) {
				symbol->data = data;
				found = 1;
				break;
			}

			printf("%s: Error: Uniform %s has wrong dimensions\n",
			       __func__, name);
			return -1;
		}
	}

	if (!found) {
		printf("%s: Error: Unable to find attribute %s\n",
		       __func__, name);
		return -1;
	}

	return 0;
}

int
premali_attribute_pointer(struct premali_state *state, char *name, int size,
			  int count, void *data)
{
	int i;

	for (i = 0; i < state->vertex_attribute_count; i++) {
		struct symbol *symbol = state->vertex_attributes[i];

		if (!strcmp(symbol->name, name)) {
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
premali_gl_mali_ViewPortTransform(struct premali_state *state,
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
premali_draw_arrays(struct premali_state *state, int mode, int start, int count)
{
	int i;

	if (!state->vs) {
		printf("%s: Error: vs member is not set up yet.\n", __func__);
		return -1;
	}

	if (!state->plbu) {
		printf("%s: Error: plbu member is not set up yet.\n", __func__);
		return -1;
	}

	if (!state->plb) {
		printf("%s: Error: plb member is not set up yet.\n", __func__);
		return -1;
	}

	/* Todo, check whether attributes all have data attached! */

	for (i = 0; i < state->vertex_uniform_count; i++) {
		struct symbol *symbol = state->vertex_uniforms[i];

		if (!strcmp(symbol->name, "gl_mali_ViewportTransform")) {
			if (premali_gl_mali_ViewPortTransform(state, symbol))
				return -1;
		} else if (!symbol->data) {
			printf("%s: Error: vertex uniform %s is empty.\n",
			       __func__, symbol->name);

			return -1;
		}
	}

	for (i = 0; i < state->vertex_attribute_count; i++) {
		struct symbol *symbol =
			symbol_copy(state->vertex_attributes[i], start, count);

		if (symbol)
			vs_info_attach_attribute(state->vs, symbol);

	}

	for (i = 0; i < state->vertex_varying_count; i++) {
		struct symbol *symbol =
			symbol_copy(state->vertex_varyings[i], 0, count);

		if (symbol)
			vs_info_attach_varying(state->vs, symbol);
	}

	if (vs_info_attach_uniforms(state->vs, state->vertex_uniforms,
				    state->vertex_uniform_count,
				    state->vertex_uniform_size))
		return -1;

	if (plbu_info_attach_uniforms(state->plbu, state->fragment_uniforms,
				      state->fragment_uniform_count,
				      state->fragment_uniform_size))
		return -1;

	vs_commands_create(state, count);
	vs_info_finalize(state, state->vs);

	plbu_info_render_state_create(state->plbu, state->vs);
	plbu_info_finalize(state, mode, start, count);

	return 0;
}

int
premali_flush(struct premali_state *state)
{
	int ret;

	ret = premali_gp_job_start(state);
	if (ret)
		return ret;

	ret = premali_pp_job_start(state, state->pp);
	if (ret)
		return ret;

	premali_jobs_wait();
	return 0;
}

/*
 * Just run fflush(stdout) to give the wrapper library a chance to finish.
 */
void
premali_finish(void)
{
	fflush(stdout);
}
