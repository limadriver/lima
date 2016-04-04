/*
 * Copyright (c) 2011-2013 Luc Verhaegen <libv@skynet.be>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/*
 *
 * Pixel/Texel format and layout definitions for M200/M400.
 *
 */
#ifndef LIMA_FORMAT_H
#define LIMA_FORMAT_H 1

#define LIMA_PIXEL_FORMAT_RGB_565		0x00
#define LIMA_PIXEL_FORMAT_RGBA_5551		0x01
#define LIMA_PIXEL_FORMAT_RGBA_4444		0x02
#define LIMA_PIXEL_FORMAT_RGBA_8888		0x03
#define LIMA_PIXEL_FORMAT_LUMINANCE		0x04 /* luminance 8bits */
#define LIMA_PIXEL_FORMAT_LUMINANCE_ALPHA	0x05 /* luminance, alpha, 8bits */
#define LIMA_PIXEL_FORMAT_RGBA64		0x06 /* rgba, 16bits */
#define LIMA_PIXEL_FORMAT_LUMINANCE_16		0x07 /* luminance 16bits */
#define LIMA_PIXEL_FORMAT_LUMINANCE_ALPHA_16	0x08 /* luminance, alpha 16bits */
/* 9, 10, 11, 12 seem zero sized and undefined */
#define LIMA_PIXEL_FORMAT_STENCIL		0x0D /* stencil, 8 bits */
#define LIMA_PIXEL_FORMAT_DEPTH_STENCIL		0x0E /* depth 16 bits, stencil 8 bits */
#define LIMA_PIXEL_FORMAT_DEPTH_STENCIL_32	0x0F /* depth 24 bits, stencil 8 bits */

#define LIMA_TEXEL_FORMAT_BGR_565		0x0E
#define LIMA_TEXEL_FORMAT_RGBA_5551		0x0F
#define LIMA_TEXEL_FORMAT_RGBA_4444		0x10
#define LIMA_TEXEL_FORMAT_LA_88			0x11
#define LIMA_TEXEL_FORMAT_RGB_888		0x15
#define LIMA_TEXEL_FORMAT_RGBA_8888		0x16
//#define LIMA_TEXEL_FORMAT_BGRA_8888             0x17 /* check ordering */
#define LIMA_TEXEL_FORMAT_RGBA64		0x26
#define LIMA_TEXEL_FORMAT_DEPTH_STENCIL_32	0x2C
#define LIMA_TEXEL_FORMAT_INVALID		0x3F

#endif /* LIMA_FORMAT_H */
