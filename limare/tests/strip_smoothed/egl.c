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
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>

static EGLint const config_attribute_list[] = {
	EGL_RED_SIZE, 8,
	EGL_GREEN_SIZE, 8,
	EGL_BLUE_SIZE, 8,
	EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
	EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
	EGL_DEPTH_SIZE, 8,
	EGL_NONE
};

static EGLint const pbuffer_attribute_list[] = {
//	EGL_WIDTH, 1024,
//	EGL_HEIGHT, 768,
	EGL_WIDTH, 384,
	EGL_HEIGHT, 256,
	EGL_LARGEST_PBUFFER, EGL_TRUE,
    EGL_NONE
};

static const EGLint context_attribute_list[] = {
	EGL_CONTEXT_CLIENT_VERSION, 2,
	EGL_NONE
};

char *
eglStrError(EGLint error)
{
	printf("Error: %d\n", error);

	switch (error) {
	case EGL_SUCCESS:
		return "EGL_SUCCESS";
	case EGL_BAD_ALLOC:
		return "EGL_BAD_ALLOC";
	case EGL_BAD_CONFIG:
		return "EGL_BAD_CONFIG";
	case EGL_BAD_PARAMETER:
		return "EGL_BAD_PARAMETER";
	case EGL_BAD_MATCH:
		return "EGL_BAD_MATCH";
	case EGL_BAD_ATTRIBUTE:
		return "EGL_BAD_ATTRIBUTE";
	default:
		return "UNKNOWN";
	}
}

int
main(int argc, char *argv[])
{
	EGLDisplay display;
	EGLint egl_major, egl_minor;
	EGLConfig config;
	EGLint num_config;
	EGLContext context;
	EGLSurface surface;
	GLuint vertex_shader;
	GLuint fragment_shader;
	GLuint program;
	GLint ret;
	GLint width, height;
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
	GLfloat vVertices[] = { -0.7,  0.7, -0.7,
			        -0.7,  0.2, -0.4,
			         0.0,  0.3, -0.5,
				-0.2, -0.3,  0.3,
				 0.5, -0.2,  0.4,
				 0.7, -0.7, 0.7 };
	GLfloat vColors[] = {0.1, 0.1, 0.1, 1.0,
			     1.0, 0.0, 0.0, 1.0,
			     0.0, 0.0, 1.0, 1.0,
			     1.0, 1.0, 0.0, 1.0,
			     0.0, 1.0, 1.0, 1.0,
			     0.9, 0.9, 0.9, 1.0};

	display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	if (display == EGL_NO_DISPLAY) {
		printf("Error: No display found!\n");
		return -1;
	}

	if (!eglInitialize(display, &egl_major, &egl_minor)) {
		printf("Error: eglInitialise failed!\n");
		return -1;
	}

	printf("Using display %p with EGL version %d.%d\n",
	       display, egl_major, egl_minor);

	printf("EGL Version \"%s\"\n", eglQueryString(display, EGL_VERSION));
	printf("EGL Vendor \"%s\"\n", eglQueryString(display, EGL_VENDOR));
	printf("EGL Extensions \"%s\"\n", eglQueryString(display, EGL_EXTENSIONS));

	/* get an appropriate EGL frame buffer configuration */
	eglChooseConfig(display, config_attribute_list, &config, 1, &num_config);

	/* create an EGL rendering context */
	context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_attribute_list);
	if (context == EGL_NO_CONTEXT) {
		printf("Error: eglCreateContext failed: %d\n", eglGetError());
		return -1;
	}

	surface = eglCreatePbufferSurface(display, config, pbuffer_attribute_list);
	if (surface == EGL_NO_SURFACE) {
		printf("Error: eglCreatePbufferSurface failed: %d (%s)\n",
		       eglGetError(), eglStrError(eglGetError()));
		return -1;
	}

	if (!eglQuerySurface(display, surface, EGL_WIDTH, &width) ||
	    !eglQuerySurface(display, surface, EGL_HEIGHT, &height)) {
		printf("Error: eglQuerySurface failed: %d (%s)\n",
		       eglGetError(), eglStrError(eglGetError()));
		return -1;
	}
	printf("PBuffer: %dx%d\n", width, height);

	/* connect the context to the surface */
	if (!eglMakeCurrent(display, surface, surface, context)) {
		printf("Error: eglMakeCurrent() failed: %d (%s)\n",
		       eglGetError(), eglStrError(eglGetError()));
		return -1;
	}

	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	if (!vertex_shader) {
		printf("Error: glCreateShader(GL_VERTEX_SHADER) failed: %d (%s)\n",
		       eglGetError(), eglStrError(eglGetError()));
		return -1;
	}


	glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
	glCompileShader(vertex_shader);

	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &ret);
	if (!ret) {
		char *log;

		printf("Error: vertex shader compilation failed!:\n");
		glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &ret);

		if (ret > 1) {
			log = malloc(ret);
			glGetShaderInfoLog(vertex_shader, ret, NULL, log);
			printf("%s", log);
		}
		return -1;
	} else
		printf("Vertex shader compilation succeeded!\n");

	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	if (!fragment_shader) {
		printf("Error: glCreateShader(GL_FRAGMENT_SHADER) failed: %d (%s)\n",
		       eglGetError(), eglStrError(eglGetError()));
		return -1;
	}


	glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
	glCompileShader(fragment_shader);

	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &ret);
	if (!ret) {
		char *log;

		printf("Error: fragment shader compilation failed!:\n");
		glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &ret);

		if (ret > 1) {
			log = malloc(ret);
			glGetShaderInfoLog(fragment_shader, ret, NULL, log);
			printf("%s", log);
		}
		return -1;
	} else
		printf("Fragment shader compilation succeeded!\n");

	program = glCreateProgram();
	if (!program) {
		printf("Error: failed to create program!\n");
		return -1;
	}

	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);

	glBindAttribLocation(program, 0, "aPosition");
	glBindAttribLocation(program, 1, "aColor");

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
	} else
		printf("program linking succeeded!\n");

	glUseProgram(program);

	glViewport(0, 0, width, height);

	/* clear the color buffer */
	glClearColor(0.3125, 0.3125, 0.3125, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vVertices);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, vColors);
	glEnableVertexAttribArray(1);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 6);

	glFlush();

	usleep(1000000);

	fflush(stdout);

	return 0;
}
