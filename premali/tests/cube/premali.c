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

	const char *vertex_shader_source =
	  "uniform mat4 modelviewMatrix;\n"
	  "uniform mat4 modelviewprojectionMatrix;\n"
	  "uniform mat3 normalMatrix;\n"
	  "\n"
	  "attribute vec4 in_position;    \n"
	  "attribute vec3 in_normal;      \n"
	  "attribute vec4 in_color;       \n"
	  "\n"
	  "vec4 lightSource = vec4(2.0, 2.0, 20.0, 0.0);\n"
	  "                             \n"
	  "varying vec4 vVaryingColor;         \n"
	  "                             \n"
	  "void main()                  \n"
	  "{                            \n"
	  "    gl_Position = modelviewprojectionMatrix * in_position;\n"
	  "    vec3 vEyeNormal = normalMatrix * in_normal;\n"
	  "    vec4 vPosition4 = modelviewMatrix * in_position;\n"
	  "    vec3 vPosition3 = vPosition4.xyz / vPosition4.w;\n"
	  "    vec3 vLightDir = normalize(lightSource.xyz - vPosition3);\n"
	  "    float diff = max(0.0, dot(vEyeNormal, vLightDir));\n"
	  "    vVaryingColor = vec4(diff * in_color.rgb, 1.0);\n"
	  "}                            \n";

	const char *fragment_shader_source =
	  "precision mediump float;     \n"
	  "                             \n"
	  "varying vec4 vVaryingColor;         \n"
	  "                             \n"
	  "void main()                  \n"
	  "{                            \n"
	  "    gl_FragColor = vVaryingColor;   \n"
	  "}                            \n";

	GLfloat vVertices[] = {
	  // front
	  -1.0f, -1.0f, +1.0f, // point blue
	  +1.0f, -1.0f, +1.0f, // point magenta
	  -1.0f, +1.0f, +1.0f, // point cyan
	  +1.0f, +1.0f, +1.0f, // point white
	  // back
	  +1.0f, -1.0f, -1.0f, // point red
	  -1.0f, -1.0f, -1.0f, // point black
	  +1.0f, +1.0f, -1.0f, // point yellow
	  -1.0f, +1.0f, -1.0f, // point green
	  // right
	  +1.0f, -1.0f, +1.0f, // point magenta
	  +1.0f, -1.0f, -1.0f, // point red
	  +1.0f, +1.0f, +1.0f, // point white
	  +1.0f, +1.0f, -1.0f, // point yellow
	  // left
	  -1.0f, -1.0f, -1.0f, // point black
	  -1.0f, -1.0f, +1.0f, // point blue
	  -1.0f, +1.0f, -1.0f, // point green
	  -1.0f, +1.0f, +1.0f, // point cyan
	  // top
	  -1.0f, +1.0f, +1.0f, // point cyan
	  +1.0f, +1.0f, +1.0f, // point white
	  -1.0f, +1.0f, -1.0f, // point green
	  +1.0f, +1.0f, -1.0f, // point yellow
	  // bottom
	  -1.0f, -1.0f, -1.0f, // point black
	  +1.0f, -1.0f, -1.0f, // point red
	  -1.0f, -1.0f, +1.0f, // point blue
	  +1.0f, -1.0f, +1.0f  // point magenta
	};

	GLfloat vColors[] = {
	  // front
	  0.0f,  0.0f,  1.0f, // blue
	  1.0f,  0.0f,  1.0f, // magenta
	  0.0f,  1.0f,  1.0f, // cyan
	  1.0f,  1.0f,  1.0f, // white
	  // back
	  1.0f,  0.0f,  0.0f, // red
	  0.0f,  0.0f,  0.0f, // black
	  1.0f,  1.0f,  0.0f, // yellow
	  0.0f,  1.0f,  0.0f, // green
	  // right
	  1.0f,  0.0f,  1.0f, // magenta
	  1.0f,  0.0f,  0.0f, // red
	  1.0f,  1.0f,  1.0f, // white
	  1.0f,  1.0f,  0.0f, // yellow
	  // left
	  0.0f,  0.0f,  0.0f, // black
	  0.0f,  0.0f,  1.0f, // blue
	  0.0f,  1.0f,  0.0f, // green
	  0.0f,  1.0f,  1.0f, // cyan
	  // top
	  0.0f,  1.0f,  1.0f, // cyan
	  1.0f,  1.0f,  1.0f, // white
	  0.0f,  1.0f,  0.0f, // green
	  1.0f,  1.0f,  0.0f, // yellow
	  // bottom
	  0.0f,  0.0f,  0.0f, // black
	  1.0f,  0.0f,  0.0f, // red
	  0.0f,  0.0f,  1.0f, // blue
	  1.0f,  0.0f,  1.0f  // magenta
	};

	GLfloat vNormals[] = {
	  // front
	  +0.0f, +0.0f, +1.0f, // forward
	  +0.0f, +0.0f, +1.0f, // forward
	  +0.0f, +0.0f, +1.0f, // forward
	  +0.0f, +0.0f, +1.0f, // forward
	  // back
	  +0.0f, +0.0f, -1.0f, // backbard
	  +0.0f, +0.0f, -1.0f, // backbard
	  +0.0f, +0.0f, -1.0f, // backbard
	  +0.0f, +0.0f, -1.0f, // backbard
	  // right
	  +1.0f, +0.0f, +0.0f, // right
	  +1.0f, +0.0f, +0.0f, // right
	  +1.0f, +0.0f, +0.0f, // right
	  +1.0f, +0.0f, +0.0f, // right
	  // left
	  -1.0f, +0.0f, +0.0f, // left
	  -1.0f, +0.0f, +0.0f, // left
	  -1.0f, +0.0f, +0.0f, // left
	  -1.0f, +0.0f, +0.0f, // left
	  // top
	  +0.0f, +1.0f, +0.0f, // up
	  +0.0f, +1.0f, +0.0f, // up
	  +0.0f, +1.0f, +0.0f, // up
	  +0.0f, +1.0f, +0.0f, // up
	  // bottom
	  +0.0f, -1.0f, +0.0f, // down
	  +0.0f, -1.0f, +0.0f, // down
	  +0.0f, -1.0f, +0.0f, // down
	  +0.0f, -1.0f, +0.0f  // down
	};

	fb_clear();

	state = premali_init();
	if (!state)
		return -1;

	ret = premali_state_setup(state, WIDTH, HEIGHT, 0xFF505050);
	if (ret)
		return ret;

	vertex_shader_attach(state, vertex_shader_source);
	fragment_shader_attach(state, fragment_shader_source);

	premali_link(state);

	premali_attribute_pointer(state, "in_position", 4, 3, vVertices);
	premali_attribute_pointer(state, "in_color", 4, 3, vColors);
	premali_attribute_pointer(state, "in_normal", 4, 3, vNormals);

	ret = premali_draw_arrays(state, GL_TRIANGLE_STRIP, 4);
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
