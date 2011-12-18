/*
 * Copyright (c) 2011 Luc Verhaegen <libv@codethink.co.uk>
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
 *
 * Pixel/Texel format and layout definitions for Mali 200/400.
 *
 */
#ifndef MALI_FORMAT_H
#define MALI_FORMAT_H 1

#define MALI_PIXEL_FORMAT_RGB_565		0x00
#define MALI_PIXEL_FORMAT_RGBA_5551		0x01
#define MALI_PIXEL_FORMAT_RGBA_4444		0x02
#define MALI_PIXEL_FORMAT_RGBA_8888		0x03
#define MALI_PIXEL_FORMAT_LUMINANCE		0x04 /* luminance 8bits */
#define MALI_PIXEL_FORMAT_LUMINANCE_ALPHA	0x05 /* luminance, alpha, 8bits */
#define MALI_PIXEL_FORMAT_RGBA64		0x06 /* rgba, 16bits */
#define MALI_PIXEL_FORMAT_LUMINANCE_16		0x07 /* luminance 16bits */
#define MALI_PIXEL_FORMAT_LUMINANCE_ALPHA_16	0x08 /* luminance, alpha 16bits */
/* 9, 10, 11, 12 seem zero sized and undefined */
#define MALI_PIXEL_FORMAT_STENCIL		0x0D /* stencil, 8 bits */
#define MALI_PIXEL_FORMAT_DEPTH_STENCIL		0x0E /* depth 16 bits, stencil 8 bits */
#define MALI_PIXEL_FORMAT_DEPTH_STENCIL_32	0x0F /* depth 24 bits, stencil 8 bits */

#define MALI_TEXEL_FORMAT_RGB_555		0x0E
#define MALI_TEXEL_FORMAT_RGBA_5551		0x0F
#define MALI_TEXEL_FORMAT_RGBA_4444		0x10
#define MALI_TEXEL_FORMAT_RGBA_8888		0x16
#define MALI_TEXEL_FORMAT_RGBA64		0x26
#define MALI_TEXEL_FORMAT_DEPTH_STENCIL_32	0x2C
#define MALI_TEXEL_FORMAT_INVALID		0x3F

#endif /* MALI_FORMAT_H */
