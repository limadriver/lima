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

#include <stdlib.h>
#include <stdio.h>

#include <GLES2/gl2.h>

#include "limare.h"

int
main(int argc, char *argv[])
{
	struct limare_state *state;
	int ret;

	const char *vertex_shader_source =
		"attribute vec4 aPosition;    \n"
		"                             \n"
                "void main()                  \n"
                "{                            \n"
                "    gl_Position = aPosition; \n"
                "}                            \n";
	const char *fragment_shader_source =
		"precision mediump float;     \n"
		"                             \n"
		"void main()                  \n"
		"{                            \n"
		"    gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0); \n"
		"}                            \n";

	float vertices[] = {-0.45, -0.75, 0.0,
			     0.45, -0.75, 0.0,
			    //-0.45,  0.75, 0.0, /* culled with CCW culling. */
			     0.45,  0.75, 0.0};

	state = limare_init();
	if (!state)
		return -1;

	limare_buffer_clear(state);

	ret = limare_state_setup(state, 0, 0, 0xFF505050);
	if (ret)
		return ret;

	int program = limare_program_new(state);
	vertex_shader_attach(state, program, vertex_shader_source);
	fragment_shader_attach(state, program, fragment_shader_source);

	limare_link(state);

	limare_attribute_pointer(state, "aPosition", LIMARE_ATTRIB_FLOAT,
				 3, 0, 3, vertices);

	limare_frame_new(state);

	ret = limare_draw_arrays(state, LIMA_DRAW_QUAD_DIRECT, 0, 3);
	if (ret)
		return ret;

	ret = limare_frame_flush(state);
	if (ret)
		return ret;

	limare_buffer_swap(state);

	limare_finish(state);

	return 0;
}
