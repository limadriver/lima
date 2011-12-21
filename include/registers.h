/*
 * Copyright (c) 2011 Luc Verhaegen <libv@codethink.co.uk>
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
 * Register definitions and explanation for Mali 200/400 PP and GP
 *
 */
#ifndef MALI_REGISTERS_H
#define MALI_REGISTERS_H 1

/*
 * GP registers.
 */
#define MALI_GP_VS_COMMANDS_START	0x00
#define MALI_GP_VS_COMMANDS_END		0x01
#define MALI_GP_PLBU_COMMANDS_START	0x02
#define MALI_GP_PLBU_COMMANDS_END	0x03
#define MALI_GP_TILE_HEAP_START		0x04
#define MALI_GP_TILE_HEAP_END		0x05

/*
 * PP Frame registers.
 */
#define MALI_PP_PLBU_ARRAY_ADDRESS		0x00
#define MALI_PP_FRAME_SOMETHING_ADDRESS		0x01 /* No idea yet. */
#define MALI_PP_FRAME_UNUSED_0			0x02
#define MALI_PP_FRAME_FLAGS_16BITS			0x01 /* Set when 16 bits per channel */
#define MALI_PP_FRAME_FLAGS_ACTIVE			0x02 /* always set */
#define MALI_PP_FRAME_FLAGS_ONSCREEN			0x20 /* set when not an FBO */
#define MALI_PP_FRAME_FLAGS			0x03
#define MALI_PP_FRAME_CLEAR_VALUE_DEPTH		0x04
#define MALI_PP_FRAME_CLEAR_VALUE_STENCIL	0x05
#define MALI_PP_FRAME_CLEAR_VALUE_COLOR		0x06 /* rgba */
#define MALI_PP_FRAME_CLEAR_VALUE_COLOR_1	0x07 /* copy of the above. */
#define MALI_PP_FRAME_CLEAR_VALUE_COLOR_2	0x08 /* copy of the above. */
#define MALI_PP_FRAME_CLEAR_VALUE_COLOR_3	0x09 /* copy of the above. */
/* these two are only set if width or height are not 16 aligned. */
#define MALI_PP_FRAME_WIDTH			0x0A /* width - 1; */
#define MALI_PP_FRAME_HEIGHT			0x0B /* height - 1; */
#define MALI_PP_FRAME_FRAGMENT_STACK_ADDRESS	0x0C
#define MALI_PP_FRAME_FRAGMENT_STACK_SIZE	0x0D /* start << 16 | end */
#define MALI_PP_FRAME_UNUSED_1			0x0E
#define MALI_PP_FRAME_UNUSED_2			0x0F
#define MALI_PP_FRAME_ONE			0x10 /* always set to 1 */
/* When off screen, set to 1 */
#define MALI_PP_FRAME_SUPERSAMPLED_HEIGHT	0x11 /* height << supersample factor */
#define MALI_PP_FRAME_DUBYA			0x12 /*	0x77 */
#define MALI_PP_FRAME_ONSCREEN			0x13 /*	0 for FBO's, 1 for other rendering */

/*
 * PP WB registers.
 */
#define MALI_PP_WB_TYPE_DISABLED		0x00
#define MALI_PP_WB_TYPE_OTHER			0x01 /* for depth, stencil buffers, and fbo's */
#define MALI_PP_WB_TYPE_COLOR			0x02 /* for color buffers */
#define MALI_PP_WB_TYPE			0x00
#define MALI_PP_WB_ADDRESS		0x01
#define MALI_PP_WB_PIXEL_FORMAT		0x02 /* see formats.h */
#define MALI_PP_WB_DOWNSAMPLE_FACTOR	0x03
#define MALI_PP_WB_PIXEL_LAYOUT		0x04 /* todo: see formats.h */
#define MALI_PP_WB_PITCH		0x05 /* If layout is 2, then (width +	0x0F) / 16, else pitch / 8 */
#define MALI_PP_WB_MRT_BITS		0x06 /* bits	0-3 are set for each of up to 4 render targets */
#define MALI_PP_WB_MRT_PITCH		0x07 /* address pitch between render targets */
#define MALI_PP_WB_ZERO			0x08
#define MALI_PP_WB_UNUSED0		0x09
#define MALI_PP_WB_UNUSED1		0x0A
#define MALI_PP_WB_UNUSED2		0x0B

#endif /* MALI_REGISTERS_H */
