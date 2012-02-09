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
 * Largely copied from ARM'S own ioctl definitions, and because those are GPLed,
 * the license is kept.
 */

#ifndef REMALI_IOCTL_H
#define REMALI_IOCTL_H 1

/* include MIT licensed lima defined structs */
#include "ioctl_registers.h"

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
 * GP Start ioctl.
 */

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
 * PP Start ioctl.
 * One for mali200, one for mali400.
 */

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
