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

/*
 *
 * Code to help deal with setting up command streams for GP vs/plbu.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>

#define u32 uint32_t
#define USING_MALI200
#include "mali_200_regs.h"
#include "mali_ioctl.h"

#include "premali.h"
#include "plb.h"
#include "symbols.h"
#include "gp.h"

#include "registers.h"
#include "vs.h"
#include "plbu.h"


struct vs_info *
vs_info_create(void *address, int physical, int size)
{
	struct vs_info *info;
	int i;

	info = calloc(1, sizeof(struct vs_info));
	if (!info) {
		printf("%s: Error allocating structure: %s\n",
		       __func__, strerror(errno));
		return NULL;
	}

	info->mem_address = address;
	info->mem_physical = physical;
	info->mem_size = size;

	info->commands = info->mem_address + info->mem_used;
	info->commands_offset = info->mem_used;
	info->commands_size = 0x10 * sizeof(struct mali_cmd);
	info->mem_used += ALIGN(info->commands_size, 0x40);

	info->common = info->mem_address + info->mem_used;
	info->common_offset = info->mem_used;
	info->common_size = sizeof(struct gp_common);
	info->mem_used += ALIGN(info->common_size, 0x40);

	/* initialize common */
	for (i = 0; i < 0x10; i++) {
		info->common->attributes[i].physical = 0;
		info->common->attributes[i].size = 0x3F;
	}
	for (i = 0; i < 0x10; i++) {
		info->common->varyings[i].physical = 0;
		info->common->varyings[i].size = 0x3F;
	}

	/* predefine an area for the uniforms */
	info->uniform_offset = info->mem_used;
	info->uniform_size = 0x200;
	info->mem_used += ALIGN(info->uniform_size, 0x40);

	/* predefine an area for our vertex array */
	info->vertex_array_offset = info->mem_used;
	info->vertex_array_size = 0x40;
	info->mem_used += 0x40;

	/* leave the rest empty for now */

	if (info->mem_used > info->mem_size) {
		printf("%s: Not enough memory\n", __func__);
		free(info);
		return NULL;
	}

	return info;
}

int
vs_info_attach_uniform(struct vs_info *info, struct symbol *uniform)
{
	int size;

	if (info->uniform_count == 0x10) {
		printf("%s: No more uniforms\n", __func__);
		return -1;
	}

	size = uniform->element_size * uniform->element_count;
	if (size > (info->uniform_size - info->uniform_used)) {
		printf("%s: No more space\n", __func__);
		return -2;
	}

	uniform->address = info->mem_address + info->uniform_offset +
		info->uniform_used;

	info->uniform_used += size;
	info->uniforms[info->uniform_count] = uniform;
	info->uniform_count++;

	memcpy(uniform->address, uniform->data,
	       uniform->element_size * uniform->element_count);

	return 0;
}

int
vs_info_attach_standard_uniforms(struct vs_info *info, int width, int height)
{
	struct symbol *viewport =
		uniform_gl_mali_ViewPortTransform(0.0, 0.0, width, height, 0.0, 1.0);
	struct symbol *constant = uniform___maligp2_constant_000();
	int ret;

	ret = vs_info_attach_uniform(info, viewport);
	if (ret)
		return ret;

	ret = vs_info_attach_uniform(info, constant);
	if (ret)
		return ret;

	return 0;
}

int
vs_info_attach_attribute(struct vs_info *info, struct symbol *attribute)
{
	int size;

	if (info->attribute_count == 0x10) {
		printf("%s: No more attributes\n", __func__);
		return -1;
	}

	size = ALIGN(attribute->element_size * attribute->element_count, 0x40);
	if (size > (info->mem_size - info->mem_used)) {
		printf("%s: No more space\n", __func__);
		return -2;
	}

	attribute->address = info->mem_address + info->mem_used;
	attribute->physical = info->mem_physical + info->mem_used;
	info->mem_used += size;

	info->attributes[info->attribute_count] = attribute;
	info->attribute_count++;

	memcpy(attribute->address, attribute->data,
	       attribute->element_size * attribute->element_count);

	return 0;
}

/* varyings are still a bit of black magic at this point */
int
vs_info_attach_varying(struct vs_info *info, struct symbol *varying)
{
	int size;

	/* last varying is always the vertex array */
	if (info->varying_count == 0x0F) {
		printf("%s: No more varyings\n", __func__);
		return -1;
	}

	if (varying->element_size) {
		printf("%s: invalid varying %s\n", __func__, varying->name);
		return -3;
	}

	size = ALIGN(4 * varying->element_count, 0x40);
	if (size > (info->mem_size - info->mem_used)) {
		printf("%s: No more space\n", __func__);
		return -2;
	}

	varying->physical = info->mem_physical + info->mem_used;
	info->mem_used += size;

	info->varyings[info->varying_count] = varying;
	info->varying_count++;

	/* the vertex shader fills in the varyings */

	return 0;
}

int
vs_info_attach_shader(struct vs_info *info, unsigned int *shader, int size)
{
	int mem_size;

	if (info->shader != NULL) {
		printf("%s: shader already assigned\n", __func__);
		return -1;
	}

	mem_size = ALIGN(size * 16, 0x40);
	if (mem_size > (info->mem_size - info->mem_used)) {
		printf("%s: no more space\n", __func__);
		return -2;
	}

	info->shader = info->mem_address + info->mem_used;
	info->shader_offset = info->mem_used;
	info->shader_size = size;
	info->mem_used += mem_size;

	memcpy(info->shader, shader, 16 * size);

	return 0;
}

void
vs_info_finalize(struct vs_info *info)
{
	int i;

	for (i = 0; i < info->attribute_count; i++) {
		info->common->attributes[i].physical = info->attributes[i]->physical;
		info->common->attributes[i].size =
			(info->attributes[i]->element_size << 11) |
			(info->attributes[i]->element_entries - 1);
	}

	for (i = 0; i < info->varying_count; i++) {
		info->common->varyings[i].physical = info->varyings[i]->physical;
		info->common->varyings[i].size = (8 << 11) |
			(info->varyings[i]->element_entries - 1);
	}

	info->common->varyings[i].physical =
		info->mem_physical + info->vertex_array_offset;
	info->common->varyings[i].size = (16 << 11) | (1 - 1) | 0x20;
}

void
plbu_commands_create(struct plbu_info *info, int width, int height,
		     struct plb *plb, struct vs_info *vs,
		     int draw_mode, int vertex_count)
{
	struct mali_cmd *cmds = info->commands;
	int i = 0;

	cmds[i].val = plb->shift_w | (plb->shift_h << 16);
	cmds[i].cmd = MALI_PLBU_CMD_BLOCK_STEP;
	i++;

	cmds[i].val = ((plb->width - 1) << 24) | ((plb->height - 1) << 8);
	cmds[i].cmd = MALI_PLBU_CMD_TILED_DIMENSIONS;
	i++;

	cmds[i].val = plb->width >> plb->shift_w;
	cmds[i].cmd = MALI_PLBU_CMD_PLBU_BLOCK_STRIDE;
	i++;

	cmds[i].val = plb->mem_physical + plb->plbu_offset;
	cmds[i].cmd = MALI_PLBU_CMD_PLBU_ARRAY_ADDRESS;
	i++;

#if 0
	cmds[i].val = 0x40100000;
	cmds[i].cmd = MALI_PLBU_CMD_TILE_HEAP_START;
	i++;

	cmds[i].val = 0x40150000;
	cmds[i].cmd = MALI_PLBU_CMD_TILE_HEAP_END;
	i++;
#endif

	cmds[i].val = from_float(0.0);
	cmds[i].cmd = MALI_PLBU_CMD_VIEWPORT_Y;
	i++;

	cmds[i].val = from_float(height);
	cmds[i].cmd = MALI_PLBU_CMD_VIEWPORT_H;
	i++;

	cmds[i].val = from_float(0.0);
	cmds[i].cmd = MALI_PLBU_CMD_VIEWPORT_X;
	i++;

	cmds[i].val = from_float(width);
	cmds[i].cmd = MALI_PLBU_CMD_VIEWPORT_W;
	i++;

	/*
	 *
	 */
	cmds[i].val = MALI_PLBU_CMD_ARRAYS_SEMAPHORE_BEGIN;
	cmds[i].cmd = MALI_PLBU_CMD_ARRAYS_SEMAPHORE;
	i++;

	cmds[i].val = 0x00002200;
	cmds[i].cmd = MALI_PLBU_CMD_PRIMITIVE_SETUP;
	i++;

	cmds[i].val = info->mem_physical + info->render_state_offset;
	cmds[i].cmd = MALI_PLBU_CMD_RSW_VERTEX_ARRAY |
		((vs->mem_physical + vs->vertex_array_offset) >> 4);
	i++;

	cmds[i].val = 0x00000000;
	cmds[i].cmd = 0x1000010a;
	i++;

	cmds[i].val = from_float(0.0);
	cmds[i].cmd = MALI_PLBU_CMD_DEPTH_RANGE_NEAR;
	i++;

	cmds[i].val = from_float(1.0);
	cmds[i].cmd = MALI_PLBU_CMD_DEPTH_RANGE_FAR;
	i++;

	cmds[i].val = (vertex_count << 24);
	cmds[i].cmd = ((draw_mode & 0x1F) << 16) | (vertex_count >> 8);
	i++;

	cmds[i].val = MALI_PLBU_CMD_ARRAYS_SEMAPHORE_END;
	cmds[i].cmd = MALI_PLBU_CMD_ARRAYS_SEMAPHORE;
	i++;

	/*
	 * Some inter-frame communication apparently.
	 */
#if 0
	cmds[i].val = 0x400e41c0;
	cmds[i].cmd = 0xa0000103;
	i++;

	cmds[i].val = 0x400e41c4;
	cmds[i].cmd = 0xa0000104;
	i++;

	cmds[i].val = 0x400e41c8;
	cmds[i].cmd = 0xa0000107;
	i++;

	cmds[i].val = 0x400e41cc;
	cmds[i].cmd = 0xa0000108;
	i++;

	cmds[i].val = 0x400e41d0;
	cmds[i].cmd = 0xa0000105;
	i++;

	cmds[i].val = 0x400e41d4;
	cmds[i].cmd = 0xa0000106;
	i++;
#endif

	cmds[i].val = 0x00000000;
	cmds[i].cmd = 0xd0000000;
	i++;

	cmds[i].val = 0x00000000;
	cmds[i].cmd = 0xd0000000;
	i++;

	cmds[i].val = 0;
	cmds[i].cmd = MALI_PLBU_CMD_END;
	i++;

	/* update our size so we can set the gp job properly */
	info->commands_size = i * sizeof(struct mali_cmd);
}

struct plbu_info *
plbu_info_create(void *address, int physical, int size)
{
	struct plbu_info *info;

	info = calloc(1, sizeof(struct plbu_info));
	if (!info) {
		printf("%s: Error allocating structure: %s\n",
		       __func__, strerror(errno));
		return NULL;
	}

	info->mem_address = address;
	info->mem_physical = physical;
	info->mem_size = size;

	info->commands = info->mem_address + info->mem_used;
	info->commands_offset = info->mem_used;
	info->commands_size = 0x20 * sizeof(struct mali_cmd);
	info->mem_used += ALIGN(info->commands_size, 0x40);

	/* leave the rest empty for now */

	if (info->mem_used > info->mem_size) {
		printf("%s: Not enough memory\n", __func__);
		free(info);
		return NULL;
	}

	return info;
}

int
plbu_info_attach_shader(struct plbu_info *info, unsigned int *shader, int size)
{
	int mem_size;

	if (info->shader != NULL) {
		printf("%s: shader already assigned\n", __func__);
		return -1;
	}

	mem_size = ALIGN(size * 4, 0x40);
	if (mem_size > (info->mem_size - info->mem_used)) {
		printf("%s: no more space\n", __func__);
		return -2;
	}

	info->shader = info->mem_address + info->mem_used;
	info->shader_offset = info->mem_used;
	info->shader_size = size;
	info->mem_used += mem_size;

	memcpy(info->shader, shader, 4 * size);

	return 0;
}

int
plbu_info_finalize(struct plbu_info *info, struct plb *plb, struct vs_info *vs,
		   int width, int height, int draw_mode, int vertex_count)
{
	if (!info->render_state) {
		printf("%s: Missing render_state\n", __func__);
		return -1;
	}

	if (!info->shader) {
		printf("%s: Missing shader\n", __func__);
		return -1;
	}

	plbu_commands_create(info, width, height, plb, vs,
			     draw_mode, vertex_count);

	return 0;
}

void
gp_job_setup(_mali_uk_gp_start_job_s *gp_job, struct vs_info *vs,
	     struct plbu_info *plbu)
{
	gp_job->frame_registers[MALI_GP_VS_COMMANDS_START] =
		vs->mem_physical + vs->commands_offset;
	gp_job->frame_registers[MALI_GP_VS_COMMANDS_END] = vs->mem_physical +
		vs->commands_offset + vs->commands_size;
	gp_job->frame_registers[MALI_GP_PLBU_COMMANDS_START] =
		plbu->mem_physical + plbu->commands_offset;
	gp_job->frame_registers[MALI_GP_PLBU_COMMANDS_END] = plbu->mem_physical +
		plbu->commands_offset + plbu->commands_size;
	gp_job->frame_registers[MALI_GP_TILE_HEAP_START] = 0;
	gp_job->frame_registers[MALI_GP_TILE_HEAP_END] = 0;
}
