/*
 * Copyright (c) 2011-2012 Luc Verhaegen <libv@skynet.be>
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

#ifndef LIMA_IOCTL_H
#define LIMA_IOCTL_H 1

/* include MIT licensed lima defined structs */
#include "ioctl_registers.h"

#define LIMA_PP_IOC_BASE 0x84
#define LIMA_PP_IOC_START_JOB 0x00

#define LIMA_M200_PP_START_JOB _IOWR(LIMA_PP_IOC_BASE, LIMA_PP_IOC_START_JOB, struct lima_m200_pp_job_start *)
#define LIMA_M400_PP_START_JOB _IOWR(LIMA_PP_IOC_BASE, LIMA_PP_IOC_START_JOB, struct lima_m400_pp_job_start *)

#define LIMA_GP_IOC_BASE 0x85
#define LIMA_GP_IOC_START_JOB 0x00

#define LIMA_GP_START_JOB _IOWR(LIMA_GP_IOC_BASE, LIMA_GP_IOC_START_JOB, struct lima_gp_job_start *)


/*
 *
 * Structures.
 *
 *
 */

enum lima_job_start_status {
    JOB_STARTED,
    JOB_RETURNED,
    JOB_NOT_STARTED_DO_REQUEUE,
};

/*
 * GP Start ioctl.
 */

struct lima_gp_job_start {
	unsigned int fd;
	unsigned int user_job_ptr;
	unsigned int priority;
	unsigned int watchdog_msecs;
	struct lima_gp_frame_registers frame;
	unsigned int perf_counter_flag;
	unsigned int perf_counter_src0;
	unsigned int perf_counter_src1;
	unsigned int returned_user_job_ptr;
	enum lima_job_start_status status;
	unsigned int abort_id;
	unsigned int perf_counter_l2_src0;
	unsigned int perf_counter_l2_src1;
};

/*
 * PP Start ioctl.
 * One for m200, one for m400.
 */

struct lima_m200_pp_job_start {
	unsigned int fd;
	unsigned int user_job_ptr;
	unsigned int priority;
	unsigned int watchdog_msecs;
	struct lima_m200_pp_frame_registers frame;
	struct lima_pp_wb_registers wb[3];
	unsigned int perf_counter_flag;
	unsigned int perf_counter_src0;
	unsigned int perf_counter_src1;
	unsigned int returned_user_job_ptr;
	enum lima_job_start_status status;
	unsigned int abort_id;
	unsigned int perf_counter_l2_src0;
	unsigned int perf_counter_l2_src1;
};

struct lima_m400_pp_job_start {
	unsigned int fd;
	unsigned int user_job_ptr;
	unsigned int priority;
	unsigned int watchdog_msecs;
	struct lima_m400_pp_frame_registers frame;
	struct lima_pp_wb_registers wb[3];
	unsigned int perf_counter_flag;
	unsigned int perf_counter_src0;
	unsigned int perf_counter_src1;
	unsigned int returned_user_job_ptr;
	enum lima_job_start_status status;
	unsigned int abort_id;
	unsigned int perf_counter_l2_src0;
	unsigned int perf_counter_l2_src1;
};

#endif /* LIMA_IOCTL_H */
