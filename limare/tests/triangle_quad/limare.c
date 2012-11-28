/*
 * Copyright (c) 2011-2012 Luc Verhaegen <libv@skynet.be>
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
 * Template file for replaying a dumped stream.
 */

#include <stdlib.h>
#include <stdio.h>

#include <GLES2/gl2.h>

#include "limare.h"
#include "fb.h"
#include "symbols.h"
#include "gp.h"
#include "pp.h"
#include "program.h"

#define WIDTH 800
#define HEIGHT 480

int
main(int argc, char *argv[])
{
	struct limare_state *state;
	int ret;

	float vertices[] = {
		/* triangle */
		-0.5, -0.50, 0.0,
		-0.2,  0.50, 0.0,
		-0.8,  0.50, 0.0,
		/* quad */
		0.2, -0.50, 0.0,
		0.8, -0.50, 0.0,
		0.2,  0.50, 0.0,
		0.8,  0.50, 0.0 };
	float triangle_color[] = {0.0, 1.0, 0.0, 1.0 };
	float quad_color[] = {1.0, 0.0, 0.0, 1.0 };

	const char *vertex_shader_source =
		//"precision mediump float;     \n"
		"attribute vec4 aPosition;    \n"
		"                             \n"
                "void main()                  \n"
                "{                            \n"
                "    gl_Position = aPosition; \n"
                "}                            \n";

	const char *fragment_shader_source =
		"precision mediump float;     \n"
		"uniform vec4 uColor;         \n"
		"                             \n"
		"void main()                  \n"
		"{                            \n"
		"    gl_FragColor = uColor;   \n"
		"}                            \n";

	fb_clear();

	state = limare_init();
	if (!state)
		return -1;

	ret = limare_state_setup(state, WIDTH, HEIGHT, 0xFF505050);
	if (ret)
		return ret;

	vertex_shader_attach(state, vertex_shader_source);
	fragment_shader_attach(state, fragment_shader_source);

	limare_link(state);

	limare_attribute_pointer(state, "aPosition", 4, 3, 7, vertices);

	limare_uniform_attach(state, "uColor", 4, triangle_color);

	limare_new(state);

	ret = limare_draw_arrays(state, GL_TRIANGLES, 0, 3);
	if (ret)
		return ret;

	limare_uniform_attach(state, "uColor", 4, quad_color);

	ret = limare_draw_arrays(state, GL_TRIANGLE_STRIP, 3, 4);
	if (ret)
		return ret;

	ret = limare_flush(state);
	if (ret)
		return ret;

	fb_dump(state->dest_mem_address, 0, state->width, state->height);

	limare_finish(state);

	return 0;
}

