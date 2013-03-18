/*
 * Copyright 2013 Connor Abbott <cwabbott0@gmail.com>
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
 *
 */

/*
 * This files contain the binary shaders generated with Connors open-gpu-tools.
 * Repo: https://gitorious.org/~cwabbott/open-gpu-tools/cwabbotts-open-gpu-tools
 *
 */

#ifndef SHADERS_CLEAR_C
#define SHADERS_CLEAR_C 1

/*
 *
 * Fragment shader used for clears.
 *
 */

/*
 * ESSL code: the same shader as used directly.
 */
#ifdef ESSL_CODE
precision mediump float;

void main()
{
	gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);
}
#endif /* ESSL_CODE */

/*
 * OGT IR
 */
#ifdef OGT_IR
^const0 = vec4(1.0, 1.0, 1.0, 1.0),
$0 = ^const0,
#endif /* OGT_IR */

/*
 * Resulting MBS.
 */
static unsigned int
mbs_fragment_clear[] =
{
	0x3153424d, 0x00000054, 0x41524643, 0x0000004c, /* |MBS1T...CFRAL...| */
	0x00000005, 0x41545346, 0x00000008, 0x00000001, /* |....FSTA........| */
	0x00000001, 0x53494446, 0x00000004, 0x00000000, /* |....FDIS........| */
	0x55554246, 0x00000008, 0x00000100, 0x00000000, /* |FBUU............| */
	0x4e494244, 0x00000014, 0x00021025, 0x00000e4c, /* |DBIN....%...L...| */
	0x03c007cf, 0x03c003c0, 0x000003c0,		/* |............| */
};

#endif /* SHADERS_CLEAR_C */
