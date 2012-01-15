/*
 * Copyright (c) 2011-2012 Luc Verhaegen <libv@codethink.co.uk>
 *
 * Copyright (C) 2010 ARM Limited. All rights reserved.
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
 * IOCTL definitions which allow for compatibility across API and mali versions.
 *
 */

#ifndef REMALI_IOCTL_H
#define REMALI_IOCTL_H 1

#define MALI_PP_IOC_BASE 0x84
#define MALI_PP_IOC_START_JOB 0x00

#define MALI200_PP_START_JOB _IOWR(MALI_PP_IOC_BASE, MALI_PP_IOC_START_JOB, struct mali200_pp_job_start *)
#define MALI400_PP_START_JOB _IOWR(MALI_PP_IOC_BASE, MALI_PP_IOC_START_JOB, struct mali400_pp_job_start *)

#define MALI_GP_IOC_BASE 0x85
#define MALI_GP_IOC_START_JOB 0x00

#define MALI_GP_START_JOB _IOWR(MALI_GP_IOC_BASE, MALI_GP_IOC_START_JOB, struct mali_gp_job_start *)


/*
 *
 * Structures.
 *
 *
 */

enum mali_job_start_status {
    JOB_STARTED,
    JOB_RETURNED,
    JOB_NOT_STARTED_DO_REQUEUE,
};

/*
 * GP Start ioctl, with register names for easy typing and compiler checking.
 */
struct mali_gp_frame_registers {
	unsigned int vs_commands_start;
	unsigned int vs_commands_end;
	unsigned int plbu_commands_start;
	unsigned int plbu_commands_end;
	unsigned int tile_heap_start;
	unsigned int tile_heap_end;
};

struct mali_gp_job_start {
	unsigned int fd;
	unsigned int user_job_ptr;
	unsigned int priority;
	unsigned int watchdog_msecs;
	struct mali_gp_frame_registers frame;
	unsigned int perf_counter_flag;
	unsigned int perf_counter_src0;
	unsigned int perf_counter_src1;
	unsigned int returned_user_job_ptr;
	enum mali_job_start_status status;
	unsigned int abort_id;
	unsigned int perf_counter_l2_src0;
	unsigned int perf_counter_l2_src1;
};

/*
 * PP Start ioctl, with register names for easy typing and compiler checking.
 * One for mali200, one for mali400.
 */
#define MALI_PP_FRAME_FLAGS_16BITS	0x01 /* Set when 16 bits per channel */
#define MALI_PP_FRAME_FLAGS_ACTIVE	0x02 /* always set */
#define MALI_PP_FRAME_FLAGS_ONSCREEN	0x20 /* set when not an FBO */

struct mali200_pp_frame_registers {
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

struct mali400_pp_frame_registers {
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

enum mali_pp_wb_type {
	MALI_PP_WB_TYPE_DISABLED = 0,
	MALI_PP_WB_TYPE_OTHER = 1, /* for depth, stencil buffers, and fbo's */
	MALI_PP_WB_TYPE_COLOR = 2, /* for color buffers */
};

struct mali_pp_wb_registers {
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

struct mali200_pp_job_start {
	unsigned int fd;
	unsigned int user_job_ptr;
	unsigned int priority;
	unsigned int watchdog_msecs;
	struct mali200_pp_frame_registers frame;
	struct mali_pp_wb_registers wb[3];
	unsigned int perf_counter_flag;
	unsigned int perf_counter_src0;
	unsigned int perf_counter_src1;
	unsigned int returned_user_job_ptr;
	enum mali_job_start_status status;
	unsigned int abort_id;
	unsigned int perf_counter_l2_src0;
	unsigned int perf_counter_l2_src1;
};

struct mali400_pp_job_start {
	unsigned int fd;
	unsigned int user_job_ptr;
	unsigned int priority;
	unsigned int watchdog_msecs;
	struct mali400_pp_frame_registers frame;
	struct mali_pp_wb_registers wb[3];
	unsigned int perf_counter_flag;
	unsigned int perf_counter_src0;
	unsigned int perf_counter_src1;
	unsigned int returned_user_job_ptr;
	enum mali_job_start_status status;
	unsigned int abort_id;
	unsigned int perf_counter_l2_src0;
	unsigned int perf_counter_l2_src1;
};

#endif /* REMALI_IOCTL_H */
