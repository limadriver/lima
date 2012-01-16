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

/*
 * Draws a single smoothed triangle.
 */

#include <stdlib.h>
#include <stdio.h>

#include <GLES2/gl2.h>

#include "premali.h"
#include "bmp.h"
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
	struct premali_state *state;
	int ret;

	float vertices[] = { 0.0, -0.6, 0.0,
			     0.4, 0.6, 0.0,
			     -0.4, 0.6, 0.0};
	struct symbol *aPosition =
		symbol_create("aPosition", SYMBOL_ATTRIBUTE, 12, 3, 3, vertices, 0);
	float colors[] = {0.0, 1.0, 0.0, 1.0,
			  0.0, 0.0, 1.0, 1.0,
			  1.0, 0.0, 0.0, 1.0};
	struct symbol *aColors =
		symbol_create("aColors", SYMBOL_ATTRIBUTE, 16, 4, 3, colors, 0);
	struct symbol *vColors =
		symbol_create("vColors", SYMBOL_VARYING, 0, 16, 0, NULL, 0);

	const char *vertex_shader_source =
		"attribute vec4 aPosition;    \n"
		"attribute vec4 aColor;       \n"
		"                             \n"
		"varying vec4 vColor;         \n"
                "                             \n"
                "void main()                  \n"
                "{                            \n"
		"    vColor = aColor;         \n"
                "    gl_Position = aPosition; \n"
                "}                            \n";
	const char *fragment_shader_source =
		"precision mediump float;     \n"
		"                             \n"
		"varying vec4 vColor;         \n"
		"                             \n"
		"void main()                  \n"
		"{                            \n"
		"    gl_FragColor = vColor;   \n"
		"}                            \n";

	fb_clear();

	state = premali_init();
	if (!state)
		return -1;

	ret = premali_state_setup(state, WIDTH, HEIGHT, 0xFF505050);
	if (ret)
		return ret;

	vertex_shader_compile(state->vs, vertex_shader_source);
	fragment_shader_compile(state->plbu, fragment_shader_source);


	vs_info_attach_standard_uniforms(state, state->vs);

	vs_info_attach_attribute(state->vs, aPosition);
	vs_info_attach_attribute(state->vs, aColors);

	vs_info_attach_varying(state->vs, vColors);

	ret = premali_draw_arrays(state, GL_TRIANGLES, 3);
	if (ret)
		return ret;

	ret = premali_flush(state);
	if (ret)
		return ret;

	bmp_dump(state->pp->frame_address, 0,
		 state->width, state->height, "/sdcard/premali.bmp");

	fb_dump(state->pp->frame_address, 0, state->width, state->height);

	premali_finish();

	return 0;
}
