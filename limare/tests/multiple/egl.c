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

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include "egl_common.h"

#include "esUtil.h"
#include "cube_mesh.h"
#include "companion.h"
#include "texture_milkyway.h"

int
main(int argc, char *argv[])
{
	EGLDisplay display;
	EGLSurface surface;
	GLint ret, texture_loc;
	GLint width, height;

	buffer_size(&width, &height);

	printf("Buffer dimensions %dx%d\n", width, height);
	float aspect = (float) height / width;

	display = egl_display_init();
	surface = egl_surface_init(display, 2, width, height);

	glViewport(0, 0, width, height);

	glClearColor(0.5, 0.5, 0.5, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	/*
	 *
	 * Milky way background.
	 *
	 */
	GLuint back_vertex_shader;
	GLuint back_fragment_shader;
	GLuint back_program;
	GLuint back_texture;

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

	float back_aVertices[] = {-1.0, -1, 0.9999,
				   1.0, -1, 0.9999,
				  -1.0,  1, 0.9999,
				   1.0,  1, 0.9999};
	float back_aCoords[] = {0, 0, 1, 0, 0, 1, 1, 1};

	back_vertex_shader = vertex_shader_compile(back_vertex_shader_source);
	back_fragment_shader =
		fragment_shader_compile(back_fragment_shader_source);

	back_program = glCreateProgram();
	if (!back_program) {
		printf("Error: failed to create program!\n");
		return -1;
	}

	glAttachShader(back_program, back_vertex_shader);
	glAttachShader(back_program, back_fragment_shader);

	glBindAttribLocation(back_program, 0, "in_vertex");
	glBindAttribLocation(back_program, 1, "in_coord");

	glLinkProgram(back_program);

	glGetProgramiv(back_program, GL_LINK_STATUS, &ret);
	if (!ret) {
		char *log;

		printf("Error: back_program linking failed!:\n");
		glGetProgramiv(back_program, GL_INFO_LOG_LENGTH, &ret);

		if (ret > 1) {
			log = malloc(ret);
			glGetProgramInfoLog(back_program, ret, NULL, log);
			printf("%s", log);
		}
		return -1;
	}

	glUseProgram(back_program);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, back_aVertices);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, back_aCoords);
	glEnableVertexAttribArray(1);

	glActiveTexture(GL_TEXTURE0);

	glGenTextures(1, &back_texture);
	glBindTexture(GL_TEXTURE_2D, back_texture);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
		     TEXTURE_MILKYWAY_WIDTH, TEXTURE_MILKYWAY_HEIGHT, 0,
		     GL_RGB, GL_UNSIGNED_SHORT_5_6_5, texture_milkyway);
	//glGenerateMipmap(GL_TEXTURE_2D);

	texture_loc = glGetUniformLocation(back_program, "in_texture");
	glUniform1i(texture_loc, 0);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

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

	GLuint constant_vertex_shader;
	GLuint constant_fragment_shader;
	GLuint constant_program;

	constant_vertex_shader =
		vertex_shader_compile(constant_vertex_shader_source);
	constant_fragment_shader =
		fragment_shader_compile(constant_fragment_shader_source);

	constant_program = glCreateProgram();
	if (!constant_program) {
		printf("Error: failed to create program!\n");
		return -1;
	}

	glAttachShader(constant_program, constant_vertex_shader);
	glAttachShader(constant_program, constant_fragment_shader);

	glBindAttribLocation(constant_program, 0, "in_position");
	glBindAttribLocation(constant_program, 1, "in_normal");

	glLinkProgram(constant_program);

	glGetProgramiv(constant_program, GL_LINK_STATUS, &ret);
	if (!ret) {
		char *log;

		printf("Error: constant_program linking failed!:\n");
		glGetProgramiv(constant_program, GL_INFO_LOG_LENGTH, &ret);

		if (ret > 1) {
			log = malloc(ret);
			glGetProgramInfoLog(constant_program, ret, NULL, log);
			printf("%s", log);
		}
		return -1;
	}

	glUseProgram(constant_program);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, cube_vertices);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, cube_normals);
	glEnableVertexAttribArray(1);

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

	GLint constant_modelviewmatrix_handle =
		glGetUniformLocation(constant_program, "modelviewMatrix");
	GLint constant_modelviewprojectionmatrix_handle =
		glGetUniformLocation(constant_program,
				     "modelviewprojectionMatrix");
	GLint constant_normalmatrix_handle =
		glGetUniformLocation(constant_program, "normalMatrix");

	glUniformMatrix4fv(constant_modelviewmatrix_handle,
			   1, GL_FALSE, &constant_modelview.m[0][0]);
	glUniformMatrix4fv(constant_modelviewprojectionmatrix_handle,
			   1, GL_FALSE, &constant_modelviewprojection.m[0][0]);
	glUniformMatrix3fv(constant_normalmatrix_handle,
			   1, GL_FALSE, constant_normal);

	glDrawElements(GL_TRIANGLES, CUBE_INDEX_COUNT,
		       GL_UNSIGNED_BYTE, cube_indices);
#endif

	/*
	 *
	 * Textured flat cube.
	 *
	 */
#if 1
	GLuint flat_program;

	GLuint flat_vertex_shader;
	GLuint flat_fragment_shader;

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

	flat_vertex_shader = vertex_shader_compile(flat_vertex_shader_source);
	flat_fragment_shader =
		fragment_shader_compile(flat_fragment_shader_source);

	flat_program = glCreateProgram();
	if (!flat_program) {
		printf("Error: failed to create program!\n");
		return -1;
	}

	glAttachShader(flat_program, flat_vertex_shader);
	glAttachShader(flat_program, flat_fragment_shader);

	glBindAttribLocation(flat_program, 0, "in_position");
	glBindAttribLocation(flat_program, 1, "in_normal");
	glBindAttribLocation(flat_program, 2, "in_coord");

	glLinkProgram(flat_program);

	glGetProgramiv(flat_program, GL_LINK_STATUS, &ret);
	if (!ret) {
		char *log;

		printf("Error: flat_program linking failed!:\n");
		glGetProgramiv(flat_program, GL_INFO_LOG_LENGTH, &ret);

		if (ret > 1) {
			log = malloc(ret);
			glGetProgramInfoLog(flat_program, ret, NULL, log);
			printf("%s", log);
		}
		return -1;
	}

	glUseProgram(flat_program);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, cube_vertices);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, cube_normals);
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0,
			      cube_texture_coordinates);
	glEnableVertexAttribArray(2);

	glActiveTexture(GL_TEXTURE1);

	GLuint flat_texture;
	glGenTextures(1, &flat_texture);
	glBindTexture(GL_TEXTURE_2D, flat_texture);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
		     COMPANION_TEXTURE_WIDTH, COMPANION_TEXTURE_HEIGHT, 0,
		     GL_RGB, GL_UNSIGNED_BYTE, companion_texture_flat);

	texture_loc = glGetUniformLocation(flat_program, "in_texture");
	glUniform1i(texture_loc, 1);

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

	GLint flat_modelviewmatrix_handle =
		glGetUniformLocation(flat_program, "modelviewMatrix");
	GLint flat_modelviewprojectionmatrix_handle =
		glGetUniformLocation(flat_program, "modelviewprojectionMatrix");
	GLint flat_normalmatrix_handle =
		glGetUniformLocation(flat_program, "normalMatrix");

	glUniformMatrix4fv(flat_modelviewmatrix_handle,
			   1, GL_FALSE, &flat_modelview.m[0][0]);
	glUniformMatrix4fv(flat_modelviewprojectionmatrix_handle,
			   1, GL_FALSE, &flat_modelviewprojection.m[0][0]);
	glUniformMatrix3fv(flat_normalmatrix_handle, 1, GL_FALSE, flat_normal);

	glDrawElements(GL_TRIANGLES, CUBE_INDEX_COUNT,
		       GL_UNSIGNED_BYTE, cube_indices);
#endif

	/*
	 *
	 * Companion cube.
	 *
	 */
#if 1
	float *comp_vertices_array = companion_vertices_array();
	float *comp_texture_coordinates_array =
		companion_texture_coordinates_array();
	float *comp_normals_array = companion_normals_array();

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, comp_vertices_array);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, comp_normals_array);
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0,
			      comp_texture_coordinates_array);
	glEnableVertexAttribArray(2);

	glActiveTexture(GL_TEXTURE2);

	GLuint comp_texture;
	glGenTextures(1, &comp_texture);
	glBindTexture(GL_TEXTURE_2D, comp_texture);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
		     COMPANION_TEXTURE_WIDTH, COMPANION_TEXTURE_HEIGHT, 0,
		     GL_RGB, GL_UNSIGNED_BYTE, companion_texture);

	texture_loc = glGetUniformLocation(flat_program, "in_texture");
	glUniform1i(texture_loc, 2); // 2 -> GL_TEXTURE2 in glActiveTexture

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

	GLint cube_modelviewmatrix_handle =
		glGetUniformLocation(flat_program, "modelviewMatrix");
	GLint cube_modelviewprojectionmatrix_handle =
		glGetUniformLocation(flat_program, "modelviewprojectionMatrix");
	GLint cube_normalmatrix_handle =
		glGetUniformLocation(flat_program, "normalMatrix");

	glUniformMatrix4fv(cube_modelviewmatrix_handle, 1, GL_FALSE,
			   &cube_modelview.m[0][0]);
	glUniformMatrix4fv(cube_modelviewprojectionmatrix_handle, 1, GL_FALSE,
			   &cube_modelviewprojection.m[0][0]);
	glUniformMatrix3fv(cube_normalmatrix_handle, 1, GL_FALSE, cube_normal);

	glDrawArrays(GL_TRIANGLES, 0, COMPANION_ARRAY_COUNT);
#endif

	eglSwapBuffers(display, surface);

	usleep(1000000);

	fflush(stdout);

	return 0;
}
