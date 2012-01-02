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
#include <inttypes.h>
#include <sys/mman.h>
#include <errno.h>

#include "formats.h"
#include "registers.h"

#define u32 uint32_t
#define USING_MALI200
#include "mali_200_regs.h"
#include "mali_ioctl.h"

#include "premali.h"
#include "plb.h"
#include "pp.h"

struct pp_info *
pp_info_create(int width, int height, unsigned int clear_color, struct plb *plb,
	       void *address, unsigned int physical, int size,
	       unsigned int frame_physical)
{
	struct pp_info *info;
	_mali_uk_pp_start_job_s *job;
	unsigned int quad[5] =
		{0x00020425, 0x0000000c, 0x01e007cf, 0xb0000000, 0x000005f5};
	int offset;

	info = calloc(1, sizeof(struct pp_info));
	if (!info)
		return 0;

	info->width = width;
	info->height = height;
	info->pitch = width * 4;
	info->clear_color = clear_color;

	/* first, try to grab the necessary space for our image */
	info->frame_size = info->pitch * info->height;
	info->frame_physical = frame_physical;
	info->frame_address = mmap(NULL, info->frame_size, PROT_READ | PROT_WRITE,
				   MAP_SHARED, dev_mali_fd, info->frame_physical);
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

	/* setup the actual pp job */
	job = info->job;

	/* frame registers */
	job->frame_registers[MALI_PP_PLBU_ARRAY_ADDRESS] = plb->mem_physical + plb->pp_offset;
        job->frame_registers[MALI_PP_FRAME_RENDER_ADDRESS] = info->render_physical;
	job->frame_registers[MALI_PP_FRAME_FLAGS] =
		MALI_PP_FRAME_FLAGS_ACTIVE | MALI_PP_FRAME_FLAGS_ONSCREEN;
	job->frame_registers[MALI_PP_FRAME_CLEAR_VALUE_DEPTH] = 0x00FFFFFF;
	job->frame_registers[MALI_PP_FRAME_CLEAR_VALUE_STENCIL] = 0;
	job->frame_registers[MALI_PP_FRAME_CLEAR_VALUE_COLOR] = info->clear_color;
	job->frame_registers[MALI_PP_FRAME_CLEAR_VALUE_COLOR_1] = info->clear_color;
	job->frame_registers[MALI_PP_FRAME_CLEAR_VALUE_COLOR_2] = info->clear_color;
	job->frame_registers[MALI_PP_FRAME_CLEAR_VALUE_COLOR_3] = info->clear_color;

	if ((info->width & 0x0F) || (info->height & 0x0F)) {
		job->frame_registers[MALI_PP_FRAME_WIDTH] = info->width;
		job->frame_registers[MALI_PP_FRAME_HEIGHT] = info->height;
	} else {
		job->frame_registers[MALI_PP_FRAME_WIDTH] = 0;
		job->frame_registers[MALI_PP_FRAME_HEIGHT] = 0;
	}

	/* not needed for drawing a simple triangle */
	job->frame_registers[MALI_PP_FRAME_FRAGMENT_STACK_ADDRESS] = 0;
	job->frame_registers[MALI_PP_FRAME_FRAGMENT_STACK_SIZE] = 0;

	job->frame_registers[MALI_PP_FRAME_ONE] = 1;
	job->frame_registers[MALI_PP_FRAME_SUPERSAMPLED_HEIGHT] = info->height - 1;
	job->frame_registers[MALI_PP_FRAME_DUBYA] = 0x77;
	job->frame_registers[MALI_PP_FRAME_ONSCREEN] = 1;

	/* write back registers */
	job->wb0_registers[MALI_PP_WB_TYPE] = MALI_PP_WB_TYPE_COLOR;
	job->wb0_registers[MALI_PP_WB_ADDRESS] = info->frame_physical;
	job->wb0_registers[MALI_PP_WB_PIXEL_FORMAT] = MALI_PIXEL_FORMAT_RGBA_8888;
	job->wb0_registers[MALI_PP_WB_DOWNSAMPLE_FACTOR] = 0;
	job->wb0_registers[MALI_PP_WB_PIXEL_LAYOUT] = 0;
	job->wb0_registers[MALI_PP_WB_PITCH] = info->pitch / 8;
	job->wb0_registers[MALI_PP_WB_MRT_BITS] = 0;
	job->wb0_registers[MALI_PP_WB_MRT_PITCH] = 0;
	job->wb0_registers[MALI_PP_WB_ZERO] = 0;

	return info;
}
