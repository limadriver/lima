/*
 * Copyright 2011-2013 Luc Verhaegen <libv@skynet.be>
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
#include "formats.h"

#include "esUtil.h"
#include "cube_mesh.h"
#include "companion.h"
#include "texture_milkyway.h"

int
main(int argc, char *argv[])
{
	struct limare_state *state;
	int ret, width, height;

	state = limare_init();
	if (!state)
		return -1;

	limare_buffer_clear(state);

	ret = limare_state_setup(state, 0, 0, 0xFF808080);
	if (ret)
		return ret;

	limare_buffer_size(state, &width, &height);
	float aspect = (float) height / width;

	limare_frame_new(state);

	/*
	 *
	 * Milky way background.
	 *
	 */

	float back_vertices[] = {-1.0, -1.0, 0.9999,
				  1.0, -1.0, 0.9999,
				 -1.0,  1.0, 0.9999,
				  1.0,  1.0, 0.9999};
	float back_coords[] = {0, 0, 1, 0, 0, 1, 1, 1};

	const char *back_vertex_shader_source =
		"attribute vec4 in_vertex;\n"
		"attribute vec2 in_coord;\n"
		"varying vec2 coord;\n"
		"void main()\n"
		"{\n"
		"    gl_Position = in_vertex;\n"
		"    coord = in_coord;\n"
		"}\n";

	const char *back_fragment_shader_source =
		"precision mediump float;\n"
		"varying vec2 coord;\n"
		"uniform sampler2D in_texture;\n"
		"void main()\n"
		"{\n"
		"    gl_FragColor = texture2D(in_texture, coord);\n"
		"}\n";

	int back_program = limare_program_new(state);

	vertex_shader_attach(state, back_program, back_vertex_shader_source);
	fragment_shader_attach(state, back_program,
			       back_fragment_shader_source);

	ret = limare_link(state);
	if (ret)
		return ret;

	/*
	 * we still leak these attributes on re-using programs.
	 * TODO: create vertex buffer example, and a limare equivalent. We can
	 * then move small attributes to per-frame storage.
	 */
	limare_attribute_pointer(state, "in_vertex", 4, 3, 4, back_vertices);

	limare_attribute_pointer(state, "in_coord", 4, 2, 4, back_coords);

	int back_texture =
		limare_texture_upload(state, texture_milkyway,
				      TEXTURE_MILKYWAY_WIDTH,
				      TEXTURE_MILKYWAY_HEIGHT,
				      TEXTURE_MILKYWAY_FORMAT, 0);

	limare_texture_attach(state, "in_texture", back_texture);

	ret = limare_draw_arrays(state, GL_TRIANGLE_STRIP, 0, 4);
	if (ret)
		return ret;

	/*
	 *
	 * Constant colour flat cube.
	 *
	 */
#if 1
	const char *constant_vertex_shader_source =
	  "uniform mat4 modelviewMatrix;\n"
	  "uniform mat4 modelviewprojectionMatrix;\n"
	  "uniform mat3 normalMatrix;\n"
	  "\n"
	  "attribute vec4 in_position;    \n"
	  "attribute vec3 in_normal;      \n"
	  "\n"
	  "vec4 lightSource = vec4(10.0, 20.0, 40.0, 0.0);\n"
	  "                             \n"
	  "varying vec4 vVaryingColor;  \n"
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
	  "}                            \n";
	const char *constant_fragment_shader_source =
	  "precision mediump float;     \n"
	  "                             \n"
	  "varying vec4 vVaryingColor;  \n"
	  "                             \n"
	  "vec4 color = vec4(0.5, 0.5, 0.5, 1.0);\n"
	  "                             \n"
	  "void main()                  \n"
	  "{                            \n"
	  "    gl_FragColor = vVaryingColor * color;\n"
	  "}                            \n";

	int constant_program = limare_program_new(state);

	vertex_shader_attach(state, constant_program,
			     constant_vertex_shader_source);
	fragment_shader_attach(state, constant_program,
			       constant_fragment_shader_source);

	ret = limare_link(state);
	if (ret)
		return ret;

	limare_attribute_pointer(state, "in_position", 4,
				 3, CUBE_VERTEX_COUNT, cube_vertices);
	limare_attribute_pointer(state, "in_normal", 4,
				 3, CUBE_VERTEX_COUNT, cube_normals);

	ESMatrix constant_modelview;
	esMatrixLoadIdentity(&constant_modelview);
	esTranslate(&constant_modelview, 0.0, 0.0, -5.5);
	esRotate(&constant_modelview, -30.0, 1.0, 0.0, 0.0);
	esRotate(&constant_modelview, -45.0, 0.0, 1.0, 0.0);

	ESMatrix constant_projection;
	esMatrixLoadIdentity(&constant_projection);
	esFrustum(&constant_projection, -0.4, +1.6, -1.0 * aspect, +1.0 * aspect,
		  1.0, 10.0);

	ESMatrix constant_modelviewprojection;
	esMatrixLoadIdentity(&constant_modelviewprojection);
	esMatrixMultiply(&constant_modelviewprojection, &constant_modelview,
			 &constant_projection);

	float constant_normal[9];
	constant_normal[0] = constant_modelview.m[0][0];
	constant_normal[1] = constant_modelview.m[0][1];
	constant_normal[2] = constant_modelview.m[0][2];
	constant_normal[3] = constant_modelview.m[1][0];
	constant_normal[4] = constant_modelview.m[1][1];
	constant_normal[5] = constant_modelview.m[1][2];
	constant_normal[6] = constant_modelview.m[2][0];
	constant_normal[7] = constant_modelview.m[2][1];
	constant_normal[8] = constant_modelview.m[2][2];

	limare_uniform_attach(state, "modelviewMatrix", 16,
			      &constant_modelview.m[0][0]);
	limare_uniform_attach(state, "modelviewprojectionMatrix", 16,
			      &constant_modelviewprojection.m[0][0]);
	limare_uniform_attach(state, "normalMatrix", 9, constant_normal);

	ret = limare_draw_elements(state, GL_TRIANGLES,
				   CUBE_INDEX_COUNT, cube_indices,
				   GL_UNSIGNED_BYTE);
	if (ret)
		return ret;
#endif

	/*
	 *
	 * Textured flat cube.
	 *
	 */
#if 1
	const char *flat_vertex_shader_source =
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
	const char *flat_fragment_shader_source =
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

	int flat_program = limare_program_new(state);

	vertex_shader_attach(state, flat_program,
			     flat_vertex_shader_source);
	fragment_shader_attach(state, flat_program,
			       flat_fragment_shader_source);

	ret = limare_link(state);
	if (ret)
		return ret;

	limare_attribute_pointer(state, "in_position", 4,
				 3, CUBE_VERTEX_COUNT, cube_vertices);
	limare_attribute_pointer(state, "in_coord", 4,
				 2, CUBE_VERTEX_COUNT,
				 cube_texture_coordinates);
	limare_attribute_pointer(state, "in_normal", 4,
				 3, CUBE_VERTEX_COUNT, cube_normals);

	int flat_texture =
		limare_texture_upload(state, companion_texture_flat,
				      COMPANION_TEXTURE_WIDTH,
				      COMPANION_TEXTURE_HEIGHT,
				      COMPANION_TEXTURE_FORMAT, 0);

	limare_texture_attach(state, "in_texture", flat_texture);

	ESMatrix flat_modelview;
	esMatrixLoadIdentity(&flat_modelview);
	esTranslate(&flat_modelview, 0.0, 0.0, -5.5);
	esRotate(&flat_modelview, -30.0, 1.0, 0.0, 0.0);
	esRotate(&flat_modelview, -45.0, 0.0, 1.0, 0.0);

	ESMatrix flat_projection;
	esMatrixLoadIdentity(&flat_projection);
	esFrustum(&flat_projection, -1.0, +1.0, -1.0 * aspect, +1.0 * aspect,
		  1.0, 10.0);

	ESMatrix flat_modelviewprojection;
	esMatrixLoadIdentity(&flat_modelviewprojection);
	esMatrixMultiply(&flat_modelviewprojection, &flat_modelview,
			 &flat_projection);

	float flat_normal[9];
	flat_normal[0] = flat_modelview.m[0][0];
	flat_normal[1] = flat_modelview.m[0][1];
	flat_normal[2] = flat_modelview.m[0][2];
	flat_normal[3] = flat_modelview.m[1][0];
	flat_normal[4] = flat_modelview.m[1][1];
	flat_normal[5] = flat_modelview.m[1][2];
	flat_normal[6] = flat_modelview.m[2][0];
	flat_normal[7] = flat_modelview.m[2][1];
	flat_normal[8] = flat_modelview.m[2][2];

	limare_uniform_attach(state, "modelviewMatrix", 16,
			      &flat_modelview.m[0][0]);
	limare_uniform_attach(state, "modelviewprojectionMatrix", 16,
			      &flat_modelviewprojection.m[0][0]);
	limare_uniform_attach(state, "normalMatrix", 9, flat_normal);

	ret = limare_draw_elements(state, GL_TRIANGLES,
				   CUBE_INDEX_COUNT, cube_indices,
				   GL_UNSIGNED_BYTE);
	if (ret)
		return ret;
#endif

	/*
	 *
	 * Companion cube.
	 *
	 */
#if 1
	int cube_vertices_buffer =
		limare_attribute_buffer_upload(state, 4, 3,
					       COMPANION_VERTEX_COUNT,
					       companion_vertices);

	int cube_texture_coordinates_buffer =
		limare_attribute_buffer_upload(state, 4, 2,
					       COMPANION_VERTEX_COUNT,
					       companion_texture_coordinates);

	int cube_normals_buffer =
		limare_attribute_buffer_upload(state, 4, 3,
					       COMPANION_VERTEX_COUNT,
					       companion_normals);

	limare_attribute_buffer_attach(state, "in_position",
				       cube_vertices_buffer);
	limare_attribute_buffer_attach(state, "in_coord",
				       cube_texture_coordinates_buffer);
	limare_attribute_buffer_attach(state, "in_normal",
				       cube_normals_buffer);

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

	ESMatrix cube_modelview;
	esMatrixLoadIdentity(&cube_modelview);
	esTranslate(&cube_modelview, 0.0, 0.0, -5.5);
	esRotate(&cube_modelview, -30.0, 1.0, 0.0, 0.0);
	esRotate(&cube_modelview, -45.0, 0.0, 1.0, 0.0);

	ESMatrix cube_projection;
	esMatrixLoadIdentity(&cube_projection);
	esFrustum(&cube_projection, -1.6, +0.4, -1.0 * aspect, +1.0 * aspect,
		  1.0, 10.0);

	ESMatrix cube_modelviewprojection;
	esMatrixLoadIdentity(&cube_modelviewprojection);
	esMatrixMultiply(&cube_modelviewprojection, &cube_modelview,
			 &cube_projection);

	float cube_normal[9];
	cube_normal[0] = cube_modelview.m[0][0];
	cube_normal[1] = cube_modelview.m[0][1];
	cube_normal[2] = cube_modelview.m[0][2];
	cube_normal[3] = cube_modelview.m[1][0];
	cube_normal[4] = cube_modelview.m[1][1];
	cube_normal[5] = cube_modelview.m[1][2];
	cube_normal[6] = cube_modelview.m[2][0];
	cube_normal[7] = cube_modelview.m[2][1];
	cube_normal[8] = cube_modelview.m[2][2];

	limare_uniform_attach(state, "modelviewMatrix",
			      16, &cube_modelview.m[0][0]);
	limare_uniform_attach(state, "modelviewprojectionMatrix", 16,
			      &cube_modelviewprojection.m[0][0]);
	limare_uniform_attach(state, "normalMatrix", 9, cube_normal);

	ret = limare_draw_elements_buffer(state, elements_buffer);
	if (ret)
		return ret;
#endif

	ret = limare_frame_flush(state);
	if (ret)
		return ret;

	limare_buffer_swap(state);

	limare_finish(state);

	return 0;
}
