/*
 * Copyright (c) 2011-2013 Luc Verhaegen <libv@skynet.be>
 * Copyright (c) 2012 Arvin Schnell <arvin.schnell@gmail.com>
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

#include "cube_mesh.h"

float cube_vertices[CUBE_VERTEX_COUNT][3] = {
	/* front */
	{-1.0, -1.0, +1.0}, /* point blue */
	{+1.0, -1.0, +1.0}, /* point magenta */
	{-1.0, +1.0, +1.0}, /* point cyan */
	{+1.0, +1.0, +1.0}, /* point white */
	/* back */
	{+1.0, -1.0, -1.0}, /* point red */
	{-1.0, -1.0, -1.0}, /* point black */
	{+1.0, +1.0, -1.0}, /* point yellow */
	{-1.0, +1.0, -1.0}, /* point green */
	/* right */
	{+1.0, -1.0, +1.0}, /* point magenta */
	{+1.0, -1.0, -1.0}, /* point red */
	{+1.0, +1.0, +1.0}, /* point white */
	{+1.0, +1.0, -1.0}, /* point yellow */
	/* left */
	{-1.0, -1.0, -1.0}, /* point black */
	{-1.0, -1.0, +1.0}, /* point blue */
	{-1.0, +1.0, -1.0}, /* point green */
	{-1.0, +1.0, +1.0}, /* point cyan */
	/* top */
	{-1.0, +1.0, +1.0}, /* point cyan */
	{+1.0, +1.0, +1.0}, /* point white */
	{-1.0, +1.0, -1.0}, /* point green */
	{+1.0, +1.0, -1.0}, /* point yellow */
	/* bottom */
	{-1.0, -1.0, -1.0}, /* point black */
	{+1.0, -1.0, -1.0}, /* point red */
	{-1.0, -1.0, +1.0}, /* point blue */
	{+1.0, -1.0, +1.0}, /* point magenta */
};

float cube_colors[CUBE_VERTEX_COUNT][3] = {
	/* front */
	{0.0, 0.0, 1.0}, /* blue */
	{1.0, 0.0, 1.0}, /* magenta */
	{0.0, 1.0, 1.0}, /* cyan */
	{1.0, 1.0, 1.0}, /* white */
	/* back */
	{1.0, 0.0, 0.0}, /* red */
	{0.0, 0.0, 0.0}, /* black */
	{1.0, 1.0, 0.0}, /* yellow */
	{0.0, 1.0, 0.0}, /* green */
	/* right */
	{1.0, 0.0, 1.0}, /* magenta */
	{1.0, 0.0, 0.0}, /* red */
	{1.0, 1.0, 1.0}, /* white */
	{1.0, 1.0, 0.0}, /* yellow */
	/* left */
	{0.0, 0.0, 0.0}, /* black */
	{0.0, 0.0, 1.0}, /* blue */
	{0.0, 1.0, 0.0}, /* green */
	{0.0, 1.0, 1.0}, /* cyan */
	/* top */
	{0.0, 1.0, 1.0}, /* cyan */
	{1.0, 1.0, 1.0}, /* white */
	{0.0, 1.0, 0.0}, /* green */
	{1.0, 1.0, 0.0}, /* yellow */
	/* bottom */
	{0.0, 0.0, 0.0}, /* black */
	{1.0, 0.0, 0.0}, /* red */
	{0.0, 0.0, 1.0}, /* blue */
	{1.0, 0.0, 1.0}, /* magenta */
};

float cube_texture_coordinates[CUBE_VERTEX_COUNT][2] = {
	/* front */
	{0.0, 1.0},
	{1.0, 1.0},
	{0.0, 0.0},
	{1.0, 0.0},
	/* back */
	{0.0, 1.0},
	{1.0, 1.0},
	{0.0, 0.0},
	{1.0, 0.0},
	/* right */
	{0.0, 1.0},
	{1.0, 1.0},
	{0.0, 0.0},
	{1.0, 0.0},
	/* left */
	{0.0, 1.0},
	{1.0, 1.0},
	{0.0, 0.0},
	{1.0, 0.0},
	/* top */
	{0.0, 1.0},
	{1.0, 1.0},
	{0.0, 0.0},
	{1.0, 0.0},
	/* bottom */
	{0.0, 1.0},
	{1.0, 1.0},
	{0.0, 0.0},
	{1.0, 0.0},
};

float cube_normals[CUBE_VERTEX_COUNT][3] = {
	/* front */
	{+0.0, +0.0, +1.0}, /* forward */
	{+0.0, +0.0, +1.0}, /* forward */
	{+0.0, +0.0, +1.0}, /* forward */
	{+0.0, +0.0, +1.0}, /* forward */
	/* back */
	{+0.0, +0.0, -1.0}, /* backward */
	{+0.0, +0.0, -1.0}, /* backward */
	{+0.0, +0.0, -1.0}, /* backward */
	{+0.0, +0.0, -1.0}, /* backward */
	/* right */
	{+1.0, +0.0, +0.0}, /* right */
	{+1.0, +0.0, +0.0}, /* right */
	{+1.0, +0.0, +0.0}, /* right */
	{+1.0, +0.0, +0.0}, /* right */
	/* left */
	{-1.0, +0.0, +0.0}, /* left */
	{-1.0, +0.0, +0.0}, /* left */
	{-1.0, +0.0, +0.0}, /* left */
	{-1.0, +0.0, +0.0}, /* left */
	/* top */
	{+0.0, +1.0, +0.0}, /* up */
	{+0.0, +1.0, +0.0}, /* up */
	{+0.0, +1.0, +0.0}, /* up */
	{+0.0, +1.0, +0.0}, /* up */
	/* bottom */
	{+0.0, -1.0, +0.0}, /* down */
	{+0.0, -1.0, +0.0}, /* down */
	{+0.0, -1.0, +0.0}, /* down */
	{+0.0, -1.0, +0.0}, /* down */
};

unsigned char cube_indices[CUBE_TRIANGLE_COUNT][3] = {
	{0,  1,  2},
	{3,  2,  1},

	{4,  5,  6},
	{7,  6,  5},

	{8,  9, 10},
	{11, 10, 9},

	{12, 13, 14},
	{15, 14, 13},

	{16, 17, 18},
	{19, 18, 17},

	{20, 21, 22},
	{23, 22, 21},
};
