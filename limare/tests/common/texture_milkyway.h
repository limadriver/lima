/*
 * Copyright (c) 2012-2013 Luc Verhaegen <libv@skynet.be>
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

#ifndef TEXTURE_MILKYWAY_H
#define TEXTURE_MILKYWAY_H 1

#define TEXTURE_MILKYWAY_WIDTH 1024
#define TEXTURE_MILKYWAY_HEIGHT 768

#define TEXTURE_MILKYWAY_FORMAT LIMA_TEXEL_FORMAT_BGR_565

#define TEXTURE_MILKYWAY_SIZE \
	(TEXTURE_MILKYWAY_WIDTH * TEXTURE_MILKYWAY_HEIGHT / 2)

extern unsigned int texture_milkyway[TEXTURE_MILKYWAY_SIZE];

#endif /* TEXTURE_MILKYWAY_H */
