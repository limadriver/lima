/*
 * Copyright (c) 2012 Luc Verhaegen <libv@codethink.co.uk>
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

#ifndef LIMARE_RENDER_STATE
#define LIMARE_RENDER_STATE 1

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

#endif /* LIMARE_RENDER_STATE */
