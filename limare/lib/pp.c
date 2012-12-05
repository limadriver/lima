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
 *
 */

/*
 * Fills in and creates the pp info/job structure.
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <string.h>

#include "formats.h"
#include "ioctl_registers.h"
#include "limare.h"
#include "fb.h"
#include "plb.h"
#include "pp.h"
#include "jobs.h"

struct pp_info *
pp_info_create(struct limare_state *state, struct limare_frame *frame)
{
	struct plb *plb;
	struct pp_info *info;
	unsigned int quad[5] =
		{0x00020425, 0x0000000c, 0x01e007cf, 0xb0000000, 0x000005f5};

	if (!frame->plb) {
		printf("%s: Error: member plb not assigned yet!\n", __func__);
		return NULL;
	}

	plb = frame->plb;

	if ((frame->mem_size - frame->mem_used) < 0x80) {
		printf("%s: no space for the pp\n", __func__);
		return NULL;
	}

	info = calloc(1, sizeof(struct pp_info));
	if (!info)
		return NULL;

	info->width = state->width;
	info->height = state->height;
	info->pitch = state->width * 4;
	info->clear_color = state->clear_color;

	info->plb_physical = frame->mem_physical + plb->pp_offset;
	info->plb_shift_w = plb->shift_w;
	info->plb_shift_h = plb->shift_h;

	/* now fill out our other requirements */
	info->quad_size = 5;
	info->quad_address = frame->mem_address + frame->mem_used;
	info->quad_physical = frame->mem_physical + frame->mem_used;
	frame->mem_used += 0x40;

	memcpy(info->quad_address, quad, 4 * info->quad_size);

	info->render_size = 0x40;
	info->render_address = frame->mem_address + frame->mem_used;
	info->render_physical = frame->mem_physical + frame->mem_used;
	frame->mem_used += 0x40;

	memset(info->render_address, 0, 0x40);

	info->render_address[0x08] = 0xF008;
	info->render_address[0x09] = info->quad_physical | info->quad_size;
	info->render_address[0x0D] = 0x100;

	return info;
}

void
pp_info_destroy(struct pp_info *pp)
{
	free(pp);
}

int
limare_m200_pp_job_start(struct limare_state *state, struct pp_info *info)
{
	struct lima_m200_pp_frame_registers frame = { 0 };
	struct lima_pp_wb_registers wb = { 0 };
	int supersampling = 1;

	/* frame registers */
	frame.plbu_array_address = info->plb_physical;
	frame.render_address = info->render_physical;

	frame.flags = LIMA_PP_FRAME_FLAGS_ACTIVE;
	if (supersampling)
		frame.flags |= LIMA_PP_FRAME_FLAGS_ONSCREEN;

	frame.clear_value_depth = 0x00FFFFFF;
	frame.clear_value_stencil = 0;
	frame.clear_value_color = info->clear_color;
	frame.clear_value_color_1 = info->clear_color;
	frame.clear_value_color_2 = info->clear_color;
	frame.clear_value_color_3 = info->clear_color;

	if ((info->width & 0x0F) || (info->height & 0x0F)) {
		frame.width = info->width;
		frame.height = info->height;
	} else {
		frame.width = 0;
		frame.height = 0;
	}

	/* not needed for drawing a simple triangle */
	frame.fragment_stack_address = 0;
	frame.fragment_stack_size = 0;

	frame.one = 1;
	if (supersampling)
		frame.supersampled_height = info->height - 1;
	else
		frame.supersampled_height = 1;
	frame.dubya = 0x77;
	frame.onscreen = supersampling;

	/* write back registers */
	wb.type = LIMA_PP_WB_TYPE_COLOR;
	wb.address = state->fb->mali_physical[0];
	wb.pixel_format = LIMA_PIXEL_FORMAT_RGBA_8888;
	wb.downsample_factor = 0;
	wb.pixel_layout = 0;
	wb.pitch = info->pitch / 8;
	wb.mrt_bits = 0;
	wb.mrt_pitch = 0;
	wb.zero = 0;

	return limare_m200_pp_job_start_direct(state, &frame, &wb);
}

/* 3 registers were added, and "supersampling" is disabled */
int
limare_m400_pp_job_start(struct limare_state *state, struct pp_info *info)
{
	struct lima_m400_pp_frame_registers frame = { 0 };
	struct lima_pp_wb_registers wb = { 0 };
	int supersampling = 0;
	int max_blocking = 0;

	/* frame registers */
	frame.plbu_array_address = info->plb_physical;
	frame.render_address = info->render_physical;

	frame.flags = LIMA_PP_FRAME_FLAGS_ACTIVE;
	if (supersampling)
		frame.flags |= LIMA_PP_FRAME_FLAGS_ONSCREEN;

	frame.clear_value_depth = 0x00FFFFFF;
	frame.clear_value_stencil = 0;
	frame.clear_value_color = info->clear_color;
	frame.clear_value_color_1 = info->clear_color;
	frame.clear_value_color_2 = info->clear_color;
	frame.clear_value_color_3 = info->clear_color;

	if ((info->width & 0x0F) || (info->height & 0x0F)) {
		frame.width = info->width;
		frame.height = info->height;
	} else {
		frame.width = 0;
		frame.height = 0;
	}

	/* not needed for drawing a simple triangle */
	frame.fragment_stack_address = 0;
	frame.fragment_stack_size = 0;

	frame.one = 1;
	if (supersampling)
		frame.supersampled_height = info->height - 1;
	else
		frame.supersampled_height = 1;
	frame.dubya = 0x77;
	frame.onscreen = supersampling;

	if (info->plb_shift_w > info->plb_shift_h)
		max_blocking = info->plb_shift_w;
	else
		max_blocking = info->plb_shift_h;

	if (max_blocking > 2)
		max_blocking = 2;

	frame.blocking = (max_blocking << 28) |
		(info->plb_shift_h << 16) | (info->plb_shift_w);

	frame.scale = 0x0C;
	if (supersampling)
		frame.scale |= 0xC00;
	else
		frame.scale |= 0x100;

	/* always set to this on newer drivers */
	frame.foureight = 0x8888;

	/* write back registers */
	wb.type = LIMA_PP_WB_TYPE_COLOR;
	wb.address = state->fb->mali_physical[0];
	wb.pixel_format = LIMA_PIXEL_FORMAT_RGBA_8888;
	wb.downsample_factor = 0;
	wb.pixel_layout = 0;
	wb.pitch = info->pitch / 8;
	/* todo: infrastructure to read fbdev and see whether, we need to swap R/B */
	wb.mrt_bits = 4; /* set to RGBA instead of BGRA */
	wb.mrt_pitch = 0;
	wb.zero = 0;

	return limare_m400_pp_job_start_direct(state, &frame, &wb);
}

int
limare_pp_job_start(struct limare_state *state, struct pp_info *info)
{
	if (state->type == 400)
		return limare_m400_pp_job_start(state, info);
	else
		return limare_m200_pp_job_start(state, info);
}
