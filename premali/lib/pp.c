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
 * Fills in and creates the pp info/job structure.
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <errno.h>
#include <sys/ioctl.h>

#include "formats.h"

#include "ioctl.h"

#include "premali.h"
#include "plb.h"
#include "pp.h"
#include "jobs.h"

struct pp_info *
pp_info_create(struct premali_state *state,
	       void *address, unsigned int physical, int size,
	       unsigned int frame_physical)
{
	struct plb *plb;
	struct pp_info *info;
	unsigned int quad[5] =
		{0x00020425, 0x0000000c, 0x01e007cf, 0xb0000000, 0x000005f5};
	int offset;

	if (!state->plb) {
		printf("%s: Error: member plb not assigned yet!\n", __func__);
		return NULL;
	}

	plb = state->plb;

	info = calloc(1, sizeof(struct pp_info));
	if (!info)
		return 0;

	info->width = state->width;
	info->height = state->height;
	info->pitch = state->width * 4;
	info->clear_color = state->clear_color;

	info->plb_physical = plb->mem_physical + plb->pp_offset;
	info->plb_shift_w = plb->shift_w;
	info->plb_shift_h = plb->shift_h;

	/* first, try to grab the necessary space for our image */
	info->frame_size = info->pitch * info->height;
	info->frame_physical = frame_physical;
	info->frame_address = mmap(NULL, info->frame_size, PROT_READ | PROT_WRITE,
				   MAP_SHARED, state->fd, info->frame_physical);
	if (info->frame_address == MAP_FAILED) {
		printf("Error: failed to mmap offset 0x%x (0x%x): %s\n",
		       info->frame_physical, info->frame_size, strerror(errno));
		free(info);
		return NULL;
	}

	/* now fill out our other requirements */
	info->quad_address = address;
	info->quad_physical = physical;
	info->quad_size = 5;

	memcpy(info->quad_address, quad, 4 * info->quad_size);

	offset = ALIGN(info->quad_size, 0x40);
	info->render_address = ((void *) info->quad_address) + offset;
	info->render_physical = info->quad_physical + offset;
	info->render_size = 0x40;

	memset(info->render_address, 0, 0x40);

	info->render_address[0x08] = 0xF008;
	info->render_address[0x09] = info->quad_physical | info->quad_size;
	info->render_address[0x0D] = 0x100;

	return info;
}

int
premali_m200_pp_job_start(struct premali_state *state, struct pp_info *info)
{
	struct mali200_pp_job_start *job;
	int supersampling = 1;

	job = calloc(1, sizeof(struct mali200_pp_job_start));
	if (!job) {
		printf("%s: Error: failed to allocate job: %s\n",
		       __func__, strerror(errno));
		return errno;
	}

	info->job.mali200 = job;

	/* frame registers */
	job->frame.plbu_array_address = info->plb_physical;
        job->frame.render_address = info->render_physical;

	job->frame.flags = MALI_PP_FRAME_FLAGS_ACTIVE;
	if (supersampling)
		job->frame.flags |= MALI_PP_FRAME_FLAGS_ONSCREEN;

	job->frame.clear_value_depth = 0x00FFFFFF;
	job->frame.clear_value_stencil = 0;
	job->frame.clear_value_color = info->clear_color;
	job->frame.clear_value_color_1 = info->clear_color;
	job->frame.clear_value_color_2 = info->clear_color;
	job->frame.clear_value_color_3 = info->clear_color;

	if ((info->width & 0x0F) || (info->height & 0x0F)) {
		job->frame.width = info->width;
		job->frame.height = info->height;
	} else {
		job->frame.width = 0;
		job->frame.height = 0;
	}

	/* not needed for drawing a simple triangle */
	job->frame.fragment_stack_address = 0;
	job->frame.fragment_stack_size = 0;

	job->frame.one = 1;
	if (supersampling)
		job->frame.supersampled_height = info->height - 1;
	else
		job->frame.supersampled_height = 1;
	job->frame.dubya = 0x77;
	job->frame.onscreen = supersampling;

	/* write back registers */
	job->wb[0].type = MALI_PP_WB_TYPE_COLOR;
	job->wb[0].address = info->frame_physical;
	job->wb[0].pixel_format = MALI_PIXEL_FORMAT_RGBA_8888;
	job->wb[0].downsample_factor = 0;
	job->wb[0].pixel_layout = 0;
	job->wb[0].pitch = info->pitch / 8;
	job->wb[0].mrt_bits = 0;
	job->wb[0].mrt_pitch = 0;
	job->wb[0].zero = 0;

	return premali_m200_pp_job_start_direct(state, job);
}

/* 3 registers were added, and "supersampling" is disabled */
int
premali_m400_pp_job_start(struct premali_state *state, struct pp_info *info)
{
	struct mali400_pp_job_start *job;
	int supersampling = 0;
	int max_blocking = 0;

	job = calloc(1, sizeof(struct mali400_pp_job_start));
	if (!job) {
		printf("%s: Error: failed to allocate job: %s\n",
		       __func__, strerror(errno));
		return errno;
	}

	info->job.mali400 = job;

	/* frame registers */
	job->frame.plbu_array_address = info->plb_physical;
        job->frame.render_address = info->render_physical;

	job->frame.flags = MALI_PP_FRAME_FLAGS_ACTIVE;
	if (supersampling)
		job->frame.flags |= MALI_PP_FRAME_FLAGS_ONSCREEN;

	job->frame.clear_value_depth = 0x00FFFFFF;
	job->frame.clear_value_stencil = 0;
	job->frame.clear_value_color = info->clear_color;
	job->frame.clear_value_color_1 = info->clear_color;
	job->frame.clear_value_color_2 = info->clear_color;
	job->frame.clear_value_color_3 = info->clear_color;

	if ((info->width & 0x0F) || (info->height & 0x0F)) {
		job->frame.width = info->width;
		job->frame.height = info->height;
	} else {
		job->frame.width = 0;
		job->frame.height = 0;
	}

	/* not needed for drawing a simple triangle */
	job->frame.fragment_stack_address = 0;
	job->frame.fragment_stack_size = 0;

	job->frame.one = 1;
	if (supersampling)
		job->frame.supersampled_height = info->height - 1;
	else
		job->frame.supersampled_height = 1;
	job->frame.dubya = 0x77;
	job->frame.onscreen = supersampling;

	if (info->plb_shift_w > info->plb_shift_h)
		max_blocking = info->plb_shift_w;
	else
		max_blocking = info->plb_shift_h;

	if (max_blocking > 2)
		max_blocking = 2;

	job->frame.blocking = (max_blocking << 28) |
		(info->plb_shift_h << 16) | (info->plb_shift_w);

	job->frame.scale = 0x0C;
	if (supersampling)
		job->frame.scale |= 0xC00;
	else
		job->frame.scale |= 0x100;

	/* always set to this on newer drivers */
	job->frame.foureight = 0x8888;

	/* write back registers */
	job->wb[0].type = MALI_PP_WB_TYPE_COLOR;
	job->wb[0].address = info->frame_physical;
	job->wb[0].pixel_format = MALI_PIXEL_FORMAT_RGBA_8888;
	job->wb[0].downsample_factor = 0;
	job->wb[0].pixel_layout = 0;
	job->wb[0].pitch = info->pitch / 8;
	/* todo: infrastructure to read fbdev and see whether, we need to swap R/B */
	job->wb[0].mrt_bits = 4; /* set to RGBA instead of BGRA */
	job->wb[0].mrt_pitch = 0;
	job->wb[0].zero = 0;

	return premali_m400_pp_job_start_direct(state, job);
}

int
premali_pp_job_start(struct premali_state *state, struct pp_info *info)
{
	if (state->type == 400)
		return premali_m400_pp_job_start(state, info);
	else
		return premali_m200_pp_job_start(state, info);
}
