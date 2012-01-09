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

extern int dev_mali_fd;
extern int mem_physical;
extern void *mem_address;

struct mali_cmd {
	unsigned int val;
	unsigned int cmd;
};

/* from premali.c */
int premali_init(void);
void premali_finish(void);

#endif /* PREMALI_PREMALI_H */
