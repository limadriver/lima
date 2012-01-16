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
#ifndef PREMALI_PREMALI_H
#define PREMALI_PREMALI_H 1

#define ALIGN(x, y) (((x) + ((y) - 1)) & ~((y) - 1))

static inline unsigned int
from_float(float x)
{
	return *((unsigned int *) &x);
}

struct mali_cmd {
	unsigned int val;
	unsigned int cmd;
};

struct premali_state {
	int fd;

#define PREMALI_TYPE_MALI200 200
#define PREMALI_TYPE_MALI400 400
	int type;

	unsigned int mem_physical;
	unsigned int mem_size;
	void *mem_address;

	int width;
	int height;

	unsigned int clear_color;

	struct mali_gp_job_start *gp_job_start;
	struct vs_info *vs;
	struct plbu_info *plbu;

	struct plb_info *plb;

	struct pp_info *pp;
};

/* from premali.c */
struct premali_state *premali_init(void);
void premali_finish(void);

void premali_dimensions_set(struct premali_state *state, int width, int height);
void premali_clear_color_set(struct premali_state *state, unsigned int clear_color);

#endif /* PREMALI_PREMALI_H */
