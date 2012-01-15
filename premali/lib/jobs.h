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
 * Simplistic job handling.
 */

#ifndef PREMALI_JOBS_H
#define PREMALI_JOBS_H 1

void premali_jobs_wait(void);
void wait_for_notification_start(void);

int premali_gp_job_start_direct(struct mali_gp_job_start *job);
int premali_m200_pp_job_start_direct(struct mali200_pp_job_start *job);
int premali_m400_pp_job_start_direct(struct mali400_pp_job_start *job);

#endif /* PREMALI_JOBS_H */
