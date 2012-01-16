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

#ifndef PREMALI_PP_H
#define PREMALI_PP_H 1

struct pp_info
{
	union {
		struct mali200_pp_job_start *mali200;
		struct mali400_pp_job_start *mali400;
	} job;

	int width;
	int height;
	int pitch;

	unsigned int plb_physical;
	int plb_shift_w;
	int plb_shift_h;

	unsigned int clear_color;

	unsigned int *quad_address;
	unsigned int quad_physical;
	int quad_size;

	unsigned int *render_address;
	unsigned int render_physical;
	int render_size;

	/* final render, separately mapped */
	void *frame_address;
	unsigned int frame_physical;
	int frame_size;
};

struct pp_info *pp_info_create(struct premali_state *state,
			       struct plb *plb, void *address,
			       unsigned int physical, int size,
			       unsigned int frame_physical);
int premali_pp_job_start(struct premali_state *state, struct pp_info *info);

#endif /* PREMALI_PP_H */
