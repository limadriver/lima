/*
 * Copyright (c) 2011-2013 Luc Verhaegen <libv@skynet.be>
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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <GLES2/gl2.h>

#include "limare.h"
#include "formats.h"

#include "esUtil.h"
#include "companion.h"

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
		"vec4 lightSource = vec4(10.0, 20.0, 40.0, 0.0);\n"
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

	state = limare_init();
	if (!state)
		return -1;

	//limare_buffer_clear(state);

	ret = limare_state_setup(state, 0, 0, 0xFF505050);
	if (ret)
		return ret;

	int width, height;
	limare_buffer_size(state, &width, &height);
	float aspect = (float) height / width;

	limare_enable(state, GL_DEPTH_TEST);
	limare_enable(state, GL_CULL_FACE);
	limare_depth_mask(state, 1);

	int program = limare_program_new(state);
	vertex_shader_attach(state, program, vertex_shader_source);
	fragment_shader_attach(state, program, fragment_shader_source);

	limare_link(state);

	int vertices_buffer =
		limare_attribute_buffer_upload(state, LIMARE_ATTRIB_FLOAT, 3,
					       0, COMPANION_VERTEX_COUNT,
					       companion_vertices);

	int texture_coordinates_buffer =
		limare_attribute_buffer_upload(state, LIMARE_ATTRIB_FLOAT, 2,
					       0, COMPANION_VERTEX_COUNT,
					       companion_texture_coordinates);

	int normals_buffer =
		limare_attribute_buffer_upload(state, LIMARE_ATTRIB_FLOAT, 3,
					       0, COMPANION_VERTEX_COUNT,
					       companion_normals);

	limare_attribute_buffer_attach(state, "in_position",
				       vertices_buffer);
	limare_attribute_buffer_attach(state, "in_coord",
				       texture_coordinates_buffer);
	limare_attribute_buffer_attach(state, "in_normal",
				       normals_buffer);

	int elements_buffer =
		limare_elements_buffer_upload(state, GL_TRIANGLES,
					      GL_UNSIGNED_SHORT,
					      COMPANION_INDEX_COUNT,
					      companion_triangles);

	int texture = limare_texture_upload(state, companion_texture,
					    COMPANION_TEXTURE_WIDTH,
					    COMPANION_TEXTURE_HEIGHT,
					    COMPANION_TEXTURE_FORMAT, 0);
	limare_texture_attach(state, "in_texture", texture);

	int i = 0;

	while (1) {
		i++;
		if (i == 0xFFFFFFF)
			i = 0;

		float angle = 0.5 * i;

		ESMatrix modelview;
		esMatrixLoadIdentity(&modelview);
		esTranslate(&modelview, 0.0, 0.0, -4.0);
		esRotate(&modelview, angle * 0.97, 1.0, 0.0, 0.0);
		esRotate(&modelview, angle * 1.13, 0.0, 1.0, 0.0);
		esRotate(&modelview, angle * 0.73, 0.0, 0.0, 1.0);

		ESMatrix projection;
		esMatrixLoadIdentity(&projection);
		esFrustum(&projection, -1.0, +1.0, -1.0 * aspect, +1.0 * aspect,
			  1.0, 10.0);

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

		limare_uniform_attach(state, "modelviewMatrix", 16,
				      &modelview.m[0][0]);
		limare_uniform_attach(state, "modelviewprojectionMatrix", 16,
				      &modelviewprojection.m[0][0]);
		limare_uniform_attach(state, "normalMatrix", 9, normal);

		limare_frame_new(state);

		ret = limare_draw_elements_buffer(state, elements_buffer);
		if (ret)
			return ret;

		ret = limare_frame_flush(state);
		if (ret)
			return ret;

		limare_buffer_swap(state);

#if 1
		if (i >= 6400)
			break;
#endif
	}

	limare_finish(state);

	return 0;
}
