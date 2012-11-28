/*
 * Copyright (c) 2011-2012 Luc Verhaegen <libv@skynet.be>
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
#include <unistd.h>

#include <GLES2/gl2.h>

#include "limare.h"
#include "fb.h"
#include "symbols.h"
#include "gp.h"
#include "pp.h"
#include "program.h"
#include "formats.h"

#include "esUtil.h"
#include "companion.h"

#define WIDTH 1280
#define HEIGHT 720

int
main(int argc, char *argv[])
{
	struct limare_state *state;
	int ret;

	const char *vertex_shader_source =
	  "uniform mat4 modelviewMatrix;\n"
	  "uniform mat4 modelviewprojectionMatrix;\n"
	  "uniform mat3 normalMatrix;\n"
	  "\n"
	  "attribute vec4 in_position;    \n"
	  "attribute vec3 in_normal;      \n"
	  "attribute vec2 in_coord;       \n"
	  "\n"
	  "vec4 lightSource = vec4(-2.0, -2.0, 20.0, 0.0);\n"
	  "                             \n"
	  "varying vec4 vVaryingColor;  \n"
	  "varying vec2 coord;          \n"
	  "                             \n"
	  "void main()                  \n"
	  "{                            \n"
	  "    gl_Position = modelviewprojectionMatrix * in_position;\n"
	  "    vec3 vEyeNormal = normalMatrix * in_normal;\n"
	  "    vec4 vPosition4 = modelviewMatrix * in_position;\n"
	  "    vec3 vPosition3 = vPosition4.xyz / vPosition4.w;\n"
	  "    vec3 vLightDir = normalize(lightSource.xyz - vPosition3);\n"
	  "    float diff = max(0.0, dot(vEyeNormal, vLightDir));\n"
	  "    vVaryingColor = vec4(diff * vec3(1.0, 1.0, 1.0), 1.0);\n"
	  "    coord = in_coord;        \n"
	  "}                            \n";

	const char *fragment_shader_source =
	  "precision mediump float;     \n"
	  "                             \n"
	  "varying vec4 vVaryingColor;  \n"
	  "varying vec2 coord;          \n"
	  "                             \n"
	  "uniform sampler2D in_texture; \n"
	  "                             \n"
	  "void main()                  \n"
	  "{                            \n"
	  "    gl_FragColor = vVaryingColor * texture2D(in_texture, coord);\n"
	  "}                            \n";

	float *vertices_array = companion_vertices_array();
	float *texture_coordinates_array =
		companion_texture_coordinates_array();
	float *normals_array = companion_normals_array();

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

	limare_attribute_pointer(state, "in_position", 4, 3,
				 COMPANION_ARRAY_COUNT, vertices_array);
	limare_attribute_pointer(state, "in_coord", 4, 2,
				 COMPANION_ARRAY_COUNT,
				 texture_coordinates_array);
	limare_attribute_pointer(state, "in_normal", 4, 3,
				 COMPANION_ARRAY_COUNT, normals_array);

	limare_texture_attach(state, "in_texture", companion_texture,
			      COMPANION_TEXTURE_WIDTH, COMPANION_TEXTURE_HEIGHT,
			      COMPANION_TEXTURE_FORMAT);

	int i = 0;

	while (1) {
		i++;
		if (i == 0xFFFFFFF)
			i = 0;

		float angle = 0.5 * i;

		ESMatrix modelview;
		esMatrixLoadIdentity(&modelview);
		esTranslate(&modelview, 0.0f, -0.75f, -8.0f);
		esRotate(&modelview, angle * 0.97f, 1.0f, 0.0f, 0.0f);
		esRotate(&modelview, angle * 1.13f, 0.0f, 1.0f, 0.0f);
		esRotate(&modelview, angle * 0.73f, 0.0f, 0.0f, 1.0f);
		//esScale(&modelview, 0.47f, 0.47f, 0.47f);

		GLfloat aspect = (GLfloat)(HEIGHT) / (GLfloat)(WIDTH);

		ESMatrix projection;
		esMatrixLoadIdentity(&projection);
		esFrustum(&projection, -2.2f, +2.2f, -2.2f * aspect, +2.2f * aspect,
			  2.0f, 10.0f);

		ESMatrix modelviewprojection;
		esMatrixLoadIdentity(&modelviewprojection);
		esMatrixMultiply(&modelviewprojection, &modelview, &projection);

		float normal[9];
		normal[0] = modelview.m[0][0];
		normal[1] = modelview.m[0][1];
		normal[2] = modelview.m[0][2];
		normal[3] = modelview.m[1][0];
		normal[4] = modelview.m[1][1];
		normal[5] = modelview.m[1][2];
		normal[6] = modelview.m[2][0];
		normal[7] = modelview.m[2][1];
		normal[8] = modelview.m[2][2];

		limare_uniform_attach(state, "modelviewMatrix", 16, &modelview.m[0][0]);
		limare_uniform_attach(state, "modelviewprojectionMatrix", 16,
				      &modelviewprojection.m[0][0]);
		limare_uniform_attach(state, "normalMatrix", 9, normal);

		limare_new(state);

		ret = limare_draw_arrays(state, GL_TRIANGLES, 0, COMPANION_ARRAY_COUNT);
		if (ret)
			return ret;

		ret = limare_flush(state);
		if (ret)
			return ret;

		fb_dump(state->dest_mem_address, 0, state->width, state->height);
	}

	limare_finish(state);

	return 0;
}
