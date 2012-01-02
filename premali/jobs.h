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

int premali_gp_job_start(_mali_uk_gp_start_job_s *gp_job);
int premali_pp_job_start(_mali_uk_pp_start_job_s *pp_job);
void premali_jobs_wait(void);

#endif /* PREMALI_JOBS_H */
