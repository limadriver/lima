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

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include "egl_common.h"

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
		"uniform vec4 uColor;         \n"
		"                             \n"
		"void main()                  \n"
		"{                            \n"
		"    gl_FragColor = uColor;   \n"
		"}                            \n";

	GLfloat vertices[] = {
		/* triangle */
		-0.8, -0.50, 0.0,
		-0.2, -0.50, 0.0,
		-0.5,  0.50, 0.0,
		/* quad */
		 0.2, -0.50, 0.0,
		 0.8, -0.50, 0.0,
		 0.2,  0.50, 0.0,
		 0.8,  0.50, 0.0};
	GLfloat triangle_color[] = {0.0, 1.0, 0.0, 1.0};
	GLfloat quad_color[] = {1.0, 0.0, 0.0, 1.0};

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

	glBindAttribLocation(program, 0, "aPosition");

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

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vertices);
	glEnableVertexAttribArray(0);

	int uniform_location = glGetUniformLocation(program, "uColor");

	glUniform4fv(uniform_location, 1, triangle_color);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	glUniform4fv(uniform_location, 1, quad_color);
	glDrawArrays(GL_TRIANGLE_STRIP, 3, 4);

	eglSwapBuffers(display, surface);

	usleep(1000000);

	fflush(stdout);

	return 0;
}
