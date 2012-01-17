/*
 * Copyright (c) 2012 Luc Verhaegen <libv@codethink.co.uk>
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
#ifndef PREMALI_RENDER_STATE
#define PREMALI_RENDER_STATE 1

struct render_state { /* 0x40 */
	int unknown00; /* 0x00 */
	int unknown04; /* 0x04 */
	int unknown08; /* 0x08 */
	int unknown0C; /* 0x0C */
	int depth_range; /* 0x10 */
	int unknown14; /* 0x14 */
	int unknown18; /* 0x18 */
	int unknown1C; /* 0x1C */
	int unknown20; /* 0x20 */
	int shader_address; /* 0x24 */
	int varying_types; /* 0x28 */
	int uniforms_address; /* 0x2C */
	int textures_address; /* 0x30 */
	int unknown34; /* 0x34 */
	int unknown38; /* 0x38 */
	int varyings_address; /* 0x3C */
};

#endif /* PREMALI_RENDER_STATE */
