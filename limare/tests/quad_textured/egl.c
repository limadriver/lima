/*
 * Copyright (c) 2012-2013 Luc Verhaegen <libv@skynet.be>
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

#include "companion.h"

int
main(int argc, char *argv[])
{
	EGLDisplay display;
	EGLSurface surface;
	GLuint vertex_shader;
	GLuint fragment_shader;
	GLuint program;
	GLint ret;
	GLint width, height;
	GLuint texture;

	const char* vertex_shader_source =
		"attribute vec4 in_vertex;\n"
		"attribute vec2 in_coord;\n"
		"\n"
		"varying vec2 coord;\n"
		"\n"
		"void main()\n"
		"{\n"
		"    gl_Position = in_vertex;\n"
		"    coord = in_coord;\n"
		"}\n";
	const char* fragment_shader_source =
		"precision mediump float;\n"
		"\n"
		"varying vec2 coord;\n"
		"\n"
		"uniform sampler2D in_texture;\n"
		"\n"
		"void main()\n"
		"{\n"
		"    gl_FragColor = texture2D(in_texture, coord);\n"
		"}\n";

	const GLfloat aVertices[4][3] = {
		{-0.6, -1,  0},
		{ 0.6, -1,  0},
		{-0.6,  1,  0},
		{ 0.6,  1,  0}
	};
	const GLfloat aCoords[4][2] = {
		{0, 1},
		{1, 1},
		{0, 0},
		{1, 0}
	};

	buffer_size(&width, &height);

	printf("Buffer dimensions %dx%d\n", width, height);

	display = egl_display_init();
	surface = egl_surface_init(display, 2, width, height);

	glViewport(0, 0, width, height);

	glClearColor(0.5, 0.5, 0.5, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	vertex_shader = vertex_shader_compile(vertex_shader_source);
	fragment_shader = fragment_shader_compile(fragment_shader_source);

	program = glCreateProgram();
	if (!program) {
		printf("Error: failed to create program!\n");
		return -1;
	}

	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);

	glBindAttribLocation(program, 0, "in_vertex");
	glBindAttribLocation(program, 1, "in_coord");

	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &ret);
	if (!ret) {
		char *log;

		printf("Error: program linking failed!:\n");
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &ret);

		if (ret > 1) {
			log = malloc(ret);
			glGetProgramInfoLog(program, ret, NULL, log);
			printf("%s", log);
		}
		return -1;
	}

	glUseProgram(program);

	glActiveTexture(GL_TEXTURE0);

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
		     COMPANION_TEXTURE_WIDTH, COMPANION_TEXTURE_HEIGHT, 0,
		     GL_RGB, GL_UNSIGNED_BYTE, companion_texture_flat);
	//glGenerateMipmap(GL_TEXTURE_2D);

	GLint texture_loc = glGetUniformLocation(program, "in_texture");
	glUniform1i(texture_loc, 0); // 0 -> GL_TEXTURE0 in glActiveTexture

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, aVertices);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, aCoords);
	glEnableVertexAttribArray(1);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	eglSwapBuffers(display, surface);

	usleep(1000000);

	fflush(stdout);

	return 0;
}
