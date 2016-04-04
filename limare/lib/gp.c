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

/*
 *
 * Code to help deal with setting up command streams for GP vs/plbu.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>

#include <GLES2/gl2.h>

#include "ioctl_registers.h"
#include "limare.h"
#include "compiler.h"
#include "plb.h"
#include "symbols.h"
#include "gp.h"
#include "jobs.h"
#include "vs.h"
#include "plbu.h"
#include "render_state.h"
#include "hfloat.h"
#include "from_float.h"
#include "texture.h"
#include "program.h"

int
vs_command_queue_create(struct limare_frame *frame, int size)
{
	size = ALIGN(size, 0x40);

	if ((frame->mem_size - frame->mem_used) < size) {
		printf("%s: no space for vs queue\n", __func__);
		return -1;
	}

	frame->vs_commands = frame->mem_address + frame->mem_used;
	frame->vs_commands_physical = frame->mem_physical + frame->mem_used;
	frame->vs_commands_count = 0;
	frame->vs_commands_size = size / 8;

	frame->mem_used += size;

	return 0;
}

void
plbu_viewport_set(struct limare_frame *frame,
		  float x, float y, float w, float h)
{
	struct lima_cmd *cmds = frame->plbu_commands;
	int i = frame->plbu_commands_count;

	cmds[i].val = from_float(x);
	cmds[i].cmd = LIMA_PLBU_CMD_VIEWPORT_X;
	i++;

	cmds[i].val = from_float(x + w);
	cmds[i].cmd = LIMA_PLBU_CMD_VIEWPORT_W;
	i++;

	cmds[i].val = from_float(y);
	cmds[i].cmd = LIMA_PLBU_CMD_VIEWPORT_Y;
	i++;

	cmds[i].val = from_float(y + h);
	cmds[i].cmd = LIMA_PLBU_CMD_VIEWPORT_H;
	i++;

	frame->plbu_commands_count = i;
}

void
plbu_scissor(struct limare_state *state, struct limare_frame *frame)
{
	struct lima_cmd *cmds = frame->plbu_commands;
	int i = frame->plbu_commands_count;
	int x, y, w, h;

	if (state->scissor) {
		x = state->scissor_x;
		y = state->scissor_y;
		w = state->scissor_w;
		h = state->scissor_h;
	} else {
		x = state->viewport_x;
		y = state->viewport_y;
		w = state->viewport_w;
		h = state->viewport_h;
	}

	cmds[i].val = (x << 30) | (y + h - 1) << 15 | y;
	cmds[i].cmd = LIMA_PLBU_CMD_SCISSORS |
		(x + w  -1) << 13 | (x >> 2);

	frame->plbu_commands_count++;
}

int
plbu_command_queue_create(struct limare_state *state,
			  struct limare_frame *frame, int size, int heap_size)
{
	struct plb_info *plb = state->plb;
	struct lima_cmd *cmds;
	int i = 0;

	size = ALIGN(size, 0x40);
	heap_size = ALIGN(heap_size, 0x40);

	if ((frame->mem_size - frame->mem_used) < (size + heap_size)) {
		printf("%s: no space for plbu queue and tile heap\n", __func__);
		return -1;
	}

	frame->tile_heap_offset = frame->mem_used;
	frame->tile_heap_size = heap_size;
	frame->mem_used += heap_size;

	frame->plbu_commands = frame->mem_address + frame->mem_used;
	frame->plbu_commands_physical = frame->mem_physical + frame->mem_used;
	frame->plbu_commands_count = 0;
	frame->plbu_commands_size = size / 8;
	frame->mem_used += size;

	cmds = frame->plbu_commands;

	cmds[i].val = 0x0000200;
	cmds[i].cmd = LIMA_PLBU_CMD_PRIMITIVE_SETUP;
	i++;

	cmds[i].val = plb->shift_w | (plb->shift_h << 16);
	if (state->type == LIMARE_TYPE_M400)
		cmds[i].val |= plb->shift_max << 28;
	cmds[i].cmd = LIMA_PLBU_CMD_BLOCK_STEP;
	i++;

	cmds[i].val = ((plb->tiled_w - 1) << 24) | ((plb->tiled_h - 1) << 8);
	cmds[i].cmd = LIMA_PLBU_CMD_TILED_DIMENSIONS;
	i++;

	cmds[i].val = plb->block_w;
	cmds[i].cmd = LIMA_PLBU_CMD_PLBU_BLOCK_STRIDE;
	i++;

	cmds[i].val = frame->mem_physical + frame->plb_plbu_offset;
	if (state->type == LIMARE_TYPE_M200)
		cmds[i].cmd = LIMA_M200_PLBU_CMD_PLBU_ARRAY_ADDRESS;
	else if (state->type == LIMARE_TYPE_M400) {
		cmds[i].cmd = LIMA_M400_PLBU_CMD_PLBU_ARRAY_ADDRESS;
		cmds[i].cmd |= plb->block_w * plb->block_h - 1;
	}
	i++;

	frame->plbu_commands_count = i;

	plbu_viewport_set(frame, 0.0, 0.0, state->width, state->height);

	i = frame->plbu_commands_count;

	cmds[i].val = frame->mem_physical + frame->tile_heap_offset;
	cmds[i].cmd = LIMA_PLBU_CMD_TILE_HEAP_START;

	i++;

	cmds[i].val = frame->mem_physical + frame->tile_heap_offset +
		frame->tile_heap_size;
	cmds[i].cmd = LIMA_PLBU_CMD_TILE_HEAP_END;
	i++;

	frame->plbu_commands_count = i;

	return 0;
}

int
vs_info_setup(struct limare_state *state, struct limare_frame *frame,
	      struct draw_info *draw)
{
	struct vs_info *info = draw->vs;
	int i;

	if (state->type == LIMARE_TYPE_M200) {
		if ((frame->mem_size - frame->mem_used) <
		    sizeof(struct gp_common)) {
			printf("%s: no space for vs common\n", __func__);
			return -1;
		}

		/* lima200 has a common area for attributes and varyings. */
		info->common = frame->mem_address + frame->mem_used;
		info->common_offset = frame->mem_used;
		info->common_size = sizeof(struct gp_common);
		frame->mem_used += ALIGN(info->common_size, 0x40);

		/* initialize common */
		for (i = 0; i < 0x10; i++) {
			info->common->attributes[i].physical = 0;
			info->common->attributes[i].size = 0x3F;
		}
		for (i = 0; i < 0x10; i++) {
			info->common->varyings[i].physical = 0;
			info->common->varyings[i].size = 0x3F;
		}
	} else if (state->type == LIMARE_TYPE_M400) {
		if ((frame->mem_size - frame->mem_used) <
		    (0x20 * sizeof(struct gp_common_entry))) {
			printf("%s: no space for vs attribute/varying area\n",
			       __func__);
			return -1;
		}

		info->attribute_area = frame->mem_address + frame->mem_used;
		info->attribute_area_size = 0x10 * sizeof(struct gp_common_entry);
		info->attribute_area_offset = frame->mem_used;
		frame->mem_used += ALIGN(info->attribute_area_size, 0x40);

		info->varying_area = frame->mem_address + frame->mem_used;
		info->varying_area_size = 0x10 * sizeof(struct gp_common_entry);
		info->varying_area_offset = frame->mem_used;
		frame->mem_used += ALIGN(info->varying_area_size, 0x40);
	}

	return 0;
}

int
vs_info_attach_uniforms(struct limare_frame *frame, struct draw_info *draw,
			struct symbol **uniforms, int count, int size)
{
	struct vs_info *info = draw->vs;
	void *address;
	int i;

	if ((frame->mem_size - frame->mem_used) <
	    ALIGN(4 * size, 0x40)) {
		printf("%s: no space for uniforms\n", __func__);
		return -1;
	}

	info->uniform_offset = frame->mem_used;
	info->uniform_size = size;
	frame->mem_used += ALIGN(4 * size, 0x40);

	address = frame->mem_address + info->uniform_offset;

	for (i = 0; i < count; i++) {
		struct symbol *symbol = uniforms[i];

		if (symbol->src_stride == symbol->dst_stride)
			memcpy(address + symbol->component_size * symbol->offset,
			       symbol->data, symbol->size);
		else {
			void *symbol_address = address +
				symbol->component_size * symbol->offset;
			int j;

			for (j = 0; (j * symbol->src_stride) < symbol->size; j++)
				memcpy(symbol_address + (j * symbol->dst_stride),
				       symbol->data + (j * symbol->src_stride),
				       symbol->src_stride);
		}
	}

	return 0;
}

int
vs_info_attach_attribute(struct limare_frame *frame, struct draw_info *draw,
			 struct symbol *attribute)
{
	struct vs_info *info = draw->vs;

	if (info->attribute_count == 0x10) {
		printf("%s: No more attributes\n", __func__);
		return -1;
	}

	info->attributes[attribute->offset / 4] = attribute;
	info->attribute_count++;

	return 0;
}

int
vs_info_attach_varyings(struct limare_program *program,
			struct limare_frame *frame, struct draw_info *draw)
{
	struct vs_info *info = draw->vs;

	info->varying_size = ALIGN(program->varying_map_size *
				   draw->attributes_vertex_count, 0x40);
	if (info->varying_size > (frame->mem_size - frame->mem_used)) {
		printf("%s: No more space\n", __func__);
		return -2;
	}

	info->varying_offset = frame->mem_used;
	frame->mem_used += info->varying_size;

	info->gl_Position_size =
		ALIGN(16 * draw->attributes_vertex_count, 0x40);
	if (info->gl_Position_size > (frame->mem_size - frame->mem_used)) {
		printf("%s: No more space\n", __func__);
		return -2;
	}

	info->gl_Position_offset = frame->mem_used;
	frame->mem_used += info->gl_Position_size;

	return 0;
}

void
vs_commands_draw_add(struct limare_state *state, struct limare_frame *frame,
		     struct limare_program *program, struct draw_info *draw)
{
	struct vs_info *vs = draw->vs;
	struct plbu_info *plbu = draw->plbu;
	struct lima_cmd *cmds = frame->vs_commands;
	int i = frame->vs_commands_count;

	if (!plbu->indices_mem_physical) {
		cmds[i].val = LIMA_VS_CMD_ARRAYS_SEMAPHORE_BEGIN_1;
		cmds[i].cmd = LIMA_VS_CMD_ARRAYS_SEMAPHORE;
		i++;

		cmds[i].val = LIMA_VS_CMD_ARRAYS_SEMAPHORE_BEGIN_2;
		cmds[i].cmd = LIMA_VS_CMD_ARRAYS_SEMAPHORE;
		i++;
	}

	cmds[i].val = program->mem_physical + program->vertex_mem_offset;
	cmds[i].cmd = LIMA_VS_CMD_SHADER_ADDRESS |
		((program->vertex_shader_size / 16) << 16);
	i++;

	cmds[i].val = (program->vertex_attribute_prefetch - 1) << 20;
	cmds[i].val |= ((ALIGN(program->vertex_shader_size, 16) / 16) - 1)
		<< 10;
	cmds[i].cmd = LIMA_VS_CMD_SHADER_INFO;
	i++;

	cmds[i].val = (program->varying_map_count << 8) |
		((vs->attribute_count - 1) << 24);
	cmds[i].cmd = LIMA_VS_CMD_VARYING_ATTRIBUTE_COUNT;
	i++;

	cmds[i].val = frame->mem_physical + vs->uniform_offset;
	cmds[i].cmd = LIMA_VS_CMD_UNIFORMS_ADDRESS |
		(ALIGN(vs->uniform_size, 4) << 14);
	i++;

	if (state->type == LIMARE_TYPE_M200) {
		cmds[i].val = frame->mem_physical + vs->common_offset;
		cmds[i].cmd = LIMA_VS_CMD_COMMON_ADDRESS |
			(vs->common_size << 14);
		i++;
	} else if (state->type == LIMARE_TYPE_M400) {
		cmds[i].val = frame->mem_physical + vs->attribute_area_offset;
		cmds[i].cmd = LIMA_VS_CMD_ATTRIBUTES_ADDRESS |
			(vs->attribute_count << 17);
		i++;

		cmds[i].val = frame->mem_physical + vs->varying_area_offset;
		cmds[i].cmd = LIMA_VS_CMD_VARYINGS_ADDRESS |
			((program->varying_map_count + 1) << 17);
		i++;
	}

	cmds[i].val = 0x00000003; /* always 3 */
	cmds[i].cmd = 0x10000041;
	i++;

	cmds[i].val = draw->attributes_vertex_count << 24;
	if (plbu->indices_mem_physical)
		cmds[i].val |= 0x01;
	cmds[i].cmd = LIMA_VS_CMD_DRAW | draw->attributes_vertex_count >> 8;
	i++;

	cmds[i].val = 0x00000000;
	cmds[i].cmd = 0x60000000;
	i++;

	if (plbu->indices_mem_physical)
		cmds[i].val = LIMA_VS_CMD_ARRAYS_SEMAPHORE_NEXT;
	else
		cmds[i].val = LIMA_VS_CMD_ARRAYS_SEMAPHORE_END;
	cmds[i].cmd = LIMA_VS_CMD_ARRAYS_SEMAPHORE;
	i++;

	/* update our size so we can set the gp job properly */
	frame->vs_commands_count = i;
}

void
vs_info_finalize(struct limare_state *state, struct limare_frame *frame,
		 struct limare_program *program,
		 struct draw_info *draw, struct vs_info *info)
{
	int i;

	if (state->type == LIMARE_TYPE_M200) {
		for (i = 0; i < info->attribute_count; i++) {
			struct symbol *symbol = info->attributes[i];

			info->common->attributes[i].physical =
				symbol->mem_physical;
			info->common->attributes[i].size =
				(symbol->entry_stride << 11) |
				(symbol->component_type << 2) |
				(symbol->component_count - 1);
		}

		for (i = 0; i < program->varying_map_count; i++) {
			info->common->varyings[i].physical =
				frame->mem_physical +
				info->varying_offset +
				program->varying_map[i].offset;
			info->common->varyings[i].size =
				(program->varying_map_size << 11) |
				(program->varying_map[i].entries - 1);

			if (program->varying_map[i].entry_size == 2)
				info->common->varyings[i].size |= 0x0C;
		}

		if (program->gl_Position) {
			info->common->varyings[i].physical =
				frame->mem_physical + info->gl_Position_offset;
			info->common->varyings[i].size = 0x8020;
		}

	} else if (state->type == LIMARE_TYPE_M400) {
		for (i = 0; i < info->attribute_count; i++) {
			struct symbol *symbol = info->attributes[i];


			info->attribute_area[i].physical =
				symbol->mem_physical +
				(symbol->entry_stride * draw->vertex_start);
			info->attribute_area[i].size =
				(symbol->entry_stride << 11) |
				(symbol->component_type << 2) |
				(symbol->component_count - 1);
		}

		for (i = 0; i < program->varying_map_count; i++) {
			info->varying_area[i].physical =
				frame->mem_physical +
				info->varying_offset +
				program->varying_map[i].offset;
			info->varying_area[i].size =
				(program->varying_map_size << 11) |
				(program->varying_map[i].entries - 1);

			if (program->varying_map[i].entry_size == 2)
				info->varying_area[i].size |= 0x0C;
		}

		if (program->gl_Position) {
			info->varying_area[i].physical =
				frame->mem_physical + info->gl_Position_offset;
			info->varying_area[i].size = 0x8020;
		}
	}
}

void
plbu_commands_draw_add(struct limare_state *state, struct limare_frame *frame,
		       struct draw_info *draw)
{
	struct plbu_info *plbu = draw->plbu;
	struct vs_info *vs = draw->vs;
	struct lima_cmd *cmds = frame->plbu_commands;
	int i = frame->plbu_commands_count;

	/*
	 *
	 */
	if (!plbu->indices_mem_physical) {
		cmds[i].val = LIMA_PLBU_CMD_ARRAYS_SEMAPHORE_BEGIN;
		cmds[i].cmd = LIMA_PLBU_CMD_ARRAYS_SEMAPHORE;
		i++;
	}

	cmds[i].val = LIMA_PLBU_CMD_PRIMITIVE_GLES2 | 0x0000200;
	if (plbu->indices_mem_physical) {
		if (plbu->indices_type == GL_UNSIGNED_SHORT)
			cmds[i].val |= LIMA_PLBU_CMD_PRIMITIVE_INDEX_SHORT;
		else
			cmds[i].val |= LIMA_PLBU_CMD_PRIMITIVE_INDEX_BYTE;
	}
	if (state->culling) {
		if (state->cull_front_cw) {
			if (state->cull_front)
				cmds[i].val |= LIMA_PLBU_CMD_PRIMITIVE_CULL_CW;
			if (state->cull_back)
				cmds[i].val |= LIMA_PLBU_CMD_PRIMITIVE_CULL_CCW;
		} else {
			if (state->cull_front)
				cmds[i].val |= LIMA_PLBU_CMD_PRIMITIVE_CULL_CCW;
			if (state->cull_back)
				cmds[i].val |= LIMA_PLBU_CMD_PRIMITIVE_CULL_CW;
		}
	}

	cmds[i].cmd = LIMA_PLBU_CMD_PRIMITIVE_SETUP;
	i++;

	cmds[i].val = frame->mem_physical + plbu->render_state_offset;
	cmds[i].cmd = LIMA_PLBU_CMD_RSW_VERTEX_ARRAY;
	cmds[i].cmd |= (frame->mem_physical + vs->gl_Position_offset) >> 4;
	i++;

	frame->plbu_commands_count = i;

	/* original mali driver also flushes depth in this case. */
	if (state->viewport_dirty && state->scissor_dirty)
		state->depth_dirty = 1;

	if (state->viewport_dirty) {
		plbu_viewport_set(frame, state->viewport_x, state->viewport_y,
				  state->viewport_w, state->viewport_h);
		state->viewport_dirty = 0;
	}

	if (state->scissor_dirty) {
		plbu_scissor(state, frame);
		state->scissor_dirty = 0;
	}

	i = frame->plbu_commands_count;
	if (state->depth_dirty) {
		cmds[i].val = 0x00000000;
		cmds[i].cmd = 0x1000010a;
		i++;

		cmds[i].val = from_float(state->depth_near);
		cmds[i].cmd = LIMA_PLBU_CMD_DEPTH_RANGE_NEAR;
		i++;

		cmds[i].val = from_float(state->depth_far);
		cmds[i].cmd = LIMA_PLBU_CMD_DEPTH_RANGE_FAR;
		i++;

		state->depth_dirty = 0;
	}

	if (plbu->indices_mem_physical) {
		cmds[i].val = frame->mem_physical + vs->gl_Position_offset;
		cmds[i].cmd = LIMA_PLBU_CMD_INDEXED_DEST;
		i++;

		cmds[i].val = plbu->indices_mem_physical;
		cmds[i].cmd = LIMA_PLBU_CMD_INDICES;
		i++;
	} else {
		cmds[i].val = (draw->vertex_count << 24) | draw->vertex_start;
		cmds[i].cmd = LIMA_PLBU_CMD_DRAW | LIMA_PLBU_CMD_DRAW_ARRAYS |
			((draw->draw_mode & 0x1F) << 16) |
			(draw->vertex_count >> 8);
		i++;
	}

	cmds[i].val = LIMA_PLBU_CMD_ARRAYS_SEMAPHORE_END;
	cmds[i].cmd = LIMA_PLBU_CMD_ARRAYS_SEMAPHORE;
	i++;

	if (plbu->indices_mem_physical) {
		cmds[i].val = (draw->vertex_count << 24) | draw->vertex_start;
		cmds[i].cmd = LIMA_PLBU_CMD_DRAW | LIMA_PLBU_CMD_DRAW_ELEMENTS |
			((draw->draw_mode & 0x1F) << 16) | (draw->vertex_count >> 8);
		i++;
	}

	/* update our size so we can set the gp job properly */
	frame->plbu_commands_count = i;
}

void
plbu_commands_depth_buffer_clear_draw_add(struct limare_state *state,
					  struct limare_frame *frame,
					  struct draw_info *draw, unsigned int
					  varying_vertices_physical)
{
	struct plbu_info *plbu = draw->plbu;
	struct lima_cmd *cmds = frame->plbu_commands;
	int i;

	plbu_viewport_set(frame, 0.0, 0.0, 4096.0, 4096.0);
	state->viewport_dirty = 1;

	if (state->scissor_dirty) {
		plbu_scissor(state, frame);
		state->scissor_dirty = 0;
	}

	i = frame->plbu_commands_count;
	cmds[i].val = frame->mem_physical + plbu->render_state_offset;
	cmds[i].cmd = LIMA_PLBU_CMD_RSW_VERTEX_ARRAY;
	cmds[i].cmd |= varying_vertices_physical >> 4;
	i++;

	cmds[i].val = 0x00000200;
	cmds[i].cmd = LIMA_PLBU_CMD_PRIMITIVE_SETUP;
	i++;

	cmds[i].val = from_float(state->depth_near);
	cmds[i].cmd = LIMA_PLBU_CMD_DEPTH_RANGE_NEAR;
	i++;

	cmds[i].val = from_float(state->depth_far);
	cmds[i].cmd = LIMA_PLBU_CMD_DEPTH_RANGE_FAR;
	i++;

	cmds[i].val = 0x00000000;
	cmds[i].cmd = 0x1000010a;
	i++;

	state->depth_dirty = 1;

	cmds[i].val = plbu->indices_mem_physical;
	cmds[i].cmd = LIMA_PLBU_CMD_INDICES;
	i++;

	cmds[i].val = varying_vertices_physical;
	cmds[i].cmd = LIMA_PLBU_CMD_INDEXED_DEST;
	i++;

	cmds[i].val = (draw->vertex_count << 24) | draw->vertex_start;
	cmds[i].cmd = LIMA_PLBU_CMD_DRAW | LIMA_PLBU_CMD_DRAW_ELEMENTS |
		((draw->draw_mode & 0x1F) << 16) | (draw->vertex_count >> 8);
	i++;

	/* update our size so we can set the gp job properly */
	frame->plbu_commands_count = i;
}


void
plbu_commands_finish(struct limare_frame *frame)
{
	struct lima_cmd *cmds = frame->plbu_commands;
	int i = frame->plbu_commands_count;

#if 0
	cmds[i].val = 0x00000000;
	cmds[i].cmd = 0xd0000000;
	i++;

	cmds[i].val = 0x00000000;
	cmds[i].cmd = 0xd0000000;
	i++;
#endif

	cmds[i].val = 0;
	cmds[i].cmd = LIMA_PLBU_CMD_END;
	i++;

	/* update our size so we can set the gp job properly */
	frame->plbu_commands_count = i;
}

int
plbu_info_attach_uniforms(struct limare_frame *frame, struct draw_info *draw,
			  struct symbol **uniforms, int count, int size)
{
	struct plbu_info *info = draw->plbu;
	void *address;
	unsigned int *array;
	int i;

	if (!count)
		return 0;

	/* if we have only samplers, we can shortcut this. */
	for (i = 0; i < count; i++) {
		struct symbol *symbol = uniforms[i];

		if (symbol->value_type != SYMBOL_SAMPLER)
			break;
	}

	if (i == count)
		return 0;

	if ((frame->mem_size - frame->mem_used) < (0x40 + ALIGN(size, 0x40))) {
		printf("%s: no space for plbu uniforms\n", __func__);
		return -1;
	}

	info->uniform_array_offset = frame->mem_used;
	info->uniform_array_size = 4;
	frame->mem_used += 0x40;

	array = frame->mem_address + info->uniform_array_offset;

	info->uniform_offset = frame->mem_used;
	info->uniform_size = size;
	frame->mem_used += ALIGN(4 * size, 0x40);

	address = frame->mem_address + info->uniform_offset;
	array[0] = frame->mem_physical + info->uniform_offset;

	for (i = 0; i < count; i++) {
		struct symbol *symbol = uniforms[i];

		/*
		 * For samplers, there is place reserved, but the actual
		 * uniform never is written, and the value is actually plainly
		 * ignored.
		 */
		if (symbol->value_type == SYMBOL_SAMPLER)
			continue;

		memcpy(address +
		       symbol->component_size * symbol->offset,
		       symbol->data, symbol->size);
	}

	return 0;
}

void
plbu_info_attach_indices(struct draw_info *draw, int indices_type,
			 unsigned int mem_physical)
{
	struct plbu_info *plbu = draw->plbu;

	plbu->indices_type = indices_type;

	plbu->indices_mem_physical = mem_physical;
}

int
plbu_info_attach_textures(struct limare_state *state,
			  struct limare_frame *frame,
			  struct draw_info *draw)
{
	struct limare_program *program = state->program_current;
	unsigned int *list;
	int i;

	/* first a quick pass to see whether we need any textures */
	for (i = 0; i < program->fragment_uniform_count; i++) {
		struct symbol *symbol = program->fragment_uniforms[i];

		if (symbol->value_type == SYMBOL_SAMPLER)
			break;
	}

	if (i == program->fragment_uniform_count)
		return 0;

	/* create descriptor list */
	draw->texture_descriptor_list_offset = frame->mem_used;
	list = frame->mem_address + frame->mem_used;
	frame->mem_used += 0x40;

	for (i = 0; i < program->fragment_uniform_count; i++) {
		struct symbol *symbol = program->fragment_uniforms[i];
		struct limare_texture *texture;
		int handle, j;

		if (symbol->value_type != SYMBOL_SAMPLER)
			continue;

		handle = symbol->data_handle;

		/* first, check sanity of the offset */
		if (symbol->offset >= LIMARE_DRAW_TEXTURE_COUNT) {
			printf("%s: Error: sampler %s has wrong offset %d.\n",
			       __func__, symbol->name, symbol->offset);
			return -1;
		}

		/* find the matching texture */
		for (j = 0; j < LIMARE_TEXTURE_COUNT; j++) {
			if (!state->textures[j])
				continue;

			texture = state->textures[j];

			if (handle == texture->handle)
				break;
		}

		if (j == LIMARE_TEXTURE_COUNT) {
			printf("%s: Error: symbol %s texture handle not "
			       "found\n", __func__, symbol->name);
			return -1;
		}

		draw->texture_handles[symbol->offset] = handle;
		list[symbol->offset] = texture->descriptor_physical;

		if ((symbol->offset + 1) > draw->texture_descriptor_count)
			draw->texture_descriptor_count = symbol->offset + 1;
	}

	return 0;
}

struct draw_info *
draw_create_new(struct limare_state *state, struct limare_frame *frame,
		int draw_mode, int attributes_vertex_count, int vertex_start,
		int vertex_count)
{
	struct draw_info *draw = calloc(1, sizeof(struct draw_info));

	if (!draw)
		return NULL;

	draw->draw_mode = draw_mode;

	draw->attributes_vertex_count = attributes_vertex_count;

	draw->vertex_start = vertex_start;
	draw->vertex_count = vertex_count;

	if (vs_info_setup(state, frame, draw)) {
		free(draw);
		return NULL;
	}

	return draw;
}

void
draw_info_destroy(struct draw_info *draw)
{
	free(draw);
}
