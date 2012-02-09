/*
 * Copyright (c) 2011-2012 Luc Verhaegen <libv@codethink.co.uk>
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
 * This file contains readable expansions of ARM's indexed registers.
 *
 * The need for a separate file is because ARM's header files are GPLed, while
 * MIT is very much preferred for open source GPU drivers.
 */

#ifndef LIMA_IOCTL_REGISTERS_H
#define LIMA_IOCTL_REGISTERS_H 1

/*
 * GP Start ioctl, with register names for easy typing and compiler checking.
 */
struct lima_gp_frame_registers {
	unsigned int vs_commands_start;
	unsigned int vs_commands_end;
	unsigned int plbu_commands_start;
	unsigned int plbu_commands_end;
	unsigned int tile_heap_start;
	unsigned int tile_heap_end;
};

/*
 * PP Start ioctl, with register names for easy typing and compiler checking.
 * One for m200, one for m400.
 */
#define LIMA_PP_FRAME_FLAGS_16BITS	0x01 /* Set when 16 bits per channel */
#define LIMA_PP_FRAME_FLAGS_ACTIVE	0x02 /* always set */
#define LIMA_PP_FRAME_FLAGS_ONSCREEN	0x20 /* set when not an FBO */

struct lima_m200_pp_frame_registers {
	unsigned int plbu_array_address;
	unsigned int render_address;
	unsigned int unused_0;
	unsigned int flags;
	unsigned int clear_value_depth;
	unsigned int clear_value_stencil;
	unsigned int clear_value_color; /* rgba */
	unsigned int clear_value_color_1; /* copy of the above. */
	unsigned int clear_value_color_2; /* copy of the above. */
	unsigned int clear_value_color_3; /* copy of the above. */
	/* these two are only set if width or height are not 16 aligned. */
	unsigned int width; /* width - 1; */
	unsigned int height; /* height - 1; */
	unsigned int fragment_stack_address;
	unsigned int fragment_stack_size; /* start << 16 | end */
	unsigned int unused_1;
	unsigned int unused_2;
	unsigned int one; /* always set to 1 */
	/* When off screen, set to 1 */
	unsigned int supersampled_height; /* height << supersample factor */
	unsigned int dubya; /* 0x77 */
	unsigned int onscreen; /* 0 for FBO's, 1 for other rendering */
};

struct lima_m400_pp_frame_registers {
	unsigned int plbu_array_address;
	unsigned int render_address;
	unsigned int unused_0;
	unsigned int flags;
	unsigned int clear_value_depth;
	unsigned int clear_value_stencil;
	unsigned int clear_value_color; /* rgba */
	unsigned int clear_value_color_1; /* copy of the above. */
	unsigned int clear_value_color_2; /* copy of the above. */
	unsigned int clear_value_color_3; /* copy of the above. */
	/* these two are only set if width or height are not 16 aligned. */
	unsigned int width; /* width - 1; */
	unsigned int height; /* height - 1; */
	unsigned int fragment_stack_address;
	unsigned int fragment_stack_size; /* start << 16 | end */
	unsigned int unused_1;
	unsigned int unused_2;
	unsigned int one; /* always set to 1 */
	/* When off screen, set to 1 */
	unsigned int supersampled_height; /* height << supersample factor */
	unsigned int dubya; /* 0x77 */
	unsigned int onscreen; /* 0 for FBO's, 1 for other rendering */
	/* max_step = max(step_x, step_y) */
	/* (max_step > 2) ? max_step = 2 : ; */
	unsigned int blocking; /* (max_step << 28) | (step_y << 16) | (step_x) */
	unsigned int scale; /* without supersampling, this is always 0x10C */
	unsigned int foureight; /* always 0x8888, perhaps on 4x pp this is different? */
};

enum lima_pp_wb_type {
	LIMA_PP_WB_TYPE_DISABLED = 0,
	LIMA_PP_WB_TYPE_OTHER = 1, /* for depth, stencil buffers, and fbo's */
	LIMA_PP_WB_TYPE_COLOR = 2, /* for color buffers */
};

struct lima_pp_wb_registers {
	unsigned int type;
	unsigned int address;
	unsigned int pixel_format; /* see formats.h */
	unsigned int downsample_factor;
	unsigned int pixel_layout; /* todo: see formats.h */
	unsigned int pitch; /* If layout is 2, then (width + 0x0F) / 16, else pitch / 8 */
	unsigned int mrt_bits; /* bits 0-3 are set for each of up to 4 render targets */
	unsigned int mrt_pitch; /* address pitch between render targets */
	unsigned int zero;
	unsigned int unused0;
	unsigned int unused1;
	unsigned int unused2;
};

#endif /* LIMA_IOCTL_REGISTERS_H */
