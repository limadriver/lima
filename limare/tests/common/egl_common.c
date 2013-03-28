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
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include <linux/fb.h>

#include "egl_common.h"

struct mali_native_window native_window[1];

void
buffer_size(int *width, int *height)
{
#define FBDEV_DEV "/dev/fb0"
	int fd = open(FBDEV_DEV, O_RDWR);
	struct fb_var_screeninfo info;

	/* some lame defaults */
	*width = 640;
	*height = 480;

	if (fd == -1) {
		fprintf(stderr, "Error: failed to open %s: %s\n", FBDEV_DEV,
			strerror(errno));
		return;
	}

	if (ioctl(fd, FBIOGET_VSCREENINFO, &info)) {
		fprintf(stderr, "Error: failed to run ioctl on %s: %s\n",
			FBDEV_DEV, strerror(errno));
		close(fd);
		return;
	}

	close(fd);

	if (info.xres && info.yres) {
		*width = info.xres;
		*height = info.yres;
	} else
		fprintf(stderr, "Error: FB claims 0x0 dimensions\n");
}

void
gl_error_test(void)
{
	int error = glGetError();

	if (!error || (error == EGL_SUCCESS))
		return;

	fprintf(stderr, "(E)GL Error: %d\n", error);
	exit(1);
}

static struct {
	int error;
	const char *name;
} egl_errors[] = {
	{EGL_SUCCESS, "SUCCESS"},
	{EGL_NOT_INITIALIZED, "NOT_INITIALIZED"},
	{EGL_BAD_ACCESS, "BAD_ACCESS"},
	{EGL_BAD_ALLOC, "BAD_ALLOC"},
	{EGL_BAD_ATTRIBUTE, "BAD_ATTRIBUTE"},
	{EGL_BAD_CONFIG, "BAD_CONFIG"},
	{EGL_BAD_CONTEXT, "BAD_CONTEXT"},
	{EGL_BAD_CURRENT_SURFACE, "BAD_CURRENT_SURFACE"},
	{EGL_BAD_DISPLAY, "BAD_DISPLAY"},
	{EGL_BAD_MATCH, "BAD_MATCH"},
	{EGL_BAD_NATIVE_PIXMAP, "BAD_NATIVE_PIXMAP"},
	{EGL_BAD_NATIVE_WINDOW, "BAD_NATIVE_WINDOW"},
	{EGL_BAD_PARAMETER, "BAD_PARAMETER"},
	{EGL_BAD_SURFACE, "BAD_SURFACE"},
	{EGL_CONTEXT_LOST, "CONTEXT_LOST"},
	{0, NULL},
};

void
egl_error_test(void)
{
	int i, error;

	error = eglGetError();

	for (i = 0; egl_errors[i].name; i++)
		if (egl_errors[i].error == error) {
			fprintf(stderr, "EGL Error: %s\n", egl_errors[i].name);
			exit(1);
		}

	fprintf(stderr, "EGL Error: %s (0x%04X)\n", "UNKNOWN!!!", error);
	exit(1);
}

/*
 *
 */
EGLDisplay
egl_display_init(void)
{
	EGLDisplay display;
	int major, minor;

	display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	if (display == EGL_NO_DISPLAY) {
		printf("Error: No display found!\n");
		goto error;
	}

	if (!eglInitialize(display, &major, &minor)) {
		printf("Error: eglInitialise failed!\n");
		goto error;
	}

	printf("EGL Version \"%s\"\n", eglQueryString(display, EGL_VERSION));
	printf("EGL Vendor \"%s\"\n", eglQueryString(display, EGL_VENDOR));
	printf("EGL Extensions \"%s\"\n",
	       eglQueryString(display, EGL_EXTENSIONS));

	return display;
 error:
	egl_error_test();
	return NULL;
}

#if 0
/*
 * Some code snippets for android, for future reference.
 */
#include <android/native_window.h>

ANativeWindow *android_createDisplaySurface(void);

{
	ANativeWindow *window;

	/* ... */
        window = android_createDisplaySurface();
        if (window == EGL_NO_SURFACE) {
                printf("Error: android_createDisplaySurface failed: %x (%s)\n",
                       eglGetError(), eglStrError(eglGetError()));
                return -1;
        }

        surface = eglCreateWindowSurface(display, config, window, NULL);
        if (surface == EGL_NO_SURFACE) {
                printf("Error: eglCreateWindowSurface failed: %d (%s)\n",
                       eglGetError(), eglStrError(eglGetError()));
                return -1;
        }
}
#endif

/*
 *
 */
#define MAX_NUM_CONFIGS 4
EGLSurface *
egl_surface_init(EGLDisplay display, int width, int height)
{
	EGLConfig configs[MAX_NUM_CONFIGS];
	EGLint config_count;
	EGLSurface *surface;
	EGLContext context;
	EGLint config_attributes[] = {
		EGL_SAMPLES, 4,

		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,

		EGL_NONE
	};
	EGLint context_attributes[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};
	int i;

	native_window->width = width;
	native_window->height = height;

	if (!eglGetConfigs(display, configs, MAX_NUM_CONFIGS, &config_count)) {
		printf("eglGetConfigs failed!\n");
		goto error;
	}

	if (!eglChooseConfig(display, config_attributes, configs,
			     MAX_NUM_CONFIGS, &config_count)) {
		printf("eglChooseConfig failed!\n");
		goto error;
	}

	for (i = 0; i < config_count; i++) {
		surface = eglCreateWindowSurface(display, configs[i],
						 (NativeWindowType)
						 native_window,
						 NULL);
		if (surface != EGL_NO_SURFACE)
			break;
	}

	if (surface == EGL_NO_SURFACE) {
		printf("eglCreateWindowSurface failed\n");
		goto error;
	}

	context = eglCreateContext(display, configs[i], EGL_NO_CONTEXT,
				   context_attributes);
	if (context == EGL_NO_CONTEXT)
		goto error;

	if (!eglMakeCurrent(display, surface, surface, context))
		goto error;

	printf("GL_VENDOR \"%s\"\n", glGetString(GL_VENDOR));
	printf("GL_RENDERER \"%s\"\n", glGetString(GL_RENDERER));
	printf("GL_VERSION \"%s\"\n", glGetString(GL_VERSION));
	printf("GL_EXTENSIONS \"%s\"\n", glGetString(GL_EXTENSIONS));

	return surface;

 error:
	egl_error_test();
	return NULL;
}

static int
shader_compile(int type, const char *source)
{
	int shader;
	int ret;

	shader = glCreateShader(type);
	if (!shader) {
		printf("Error: glCreateShader() failed\n");
		gl_error_test();
		return -1;
	}


	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);

	glGetShaderiv(shader, GL_COMPILE_STATUS, &ret);
	if (!ret) {
		char *log;

		printf("Error: Compilation failed:\n");
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &ret);

		if (ret > 1) {
			log = malloc(ret);
			glGetShaderInfoLog(shader, ret, NULL, log);
			printf("%s", log);
		}

		return -1;
	}

	return shader;
}

int
vertex_shader_compile(const char *source)
{
	int ret;

	ret = shader_compile(GL_VERTEX_SHADER, source);
	if (ret == -1) {
		printf("Failed to create vertex shader.\n");
		gl_error_test();
		return -1;
	}

	return ret;
}

int
fragment_shader_compile(const char *source)
{
	int ret;

	ret = shader_compile(GL_FRAGMENT_SHADER, source);
	if (ret == -1) {
		printf("Failed to create fragment shader.\n");
		gl_error_test();
		return -1;
	}

	return ret;
}

#if 0 /* Sample code */
int
main(int argc, char *argv[])
{
	EGLDisplay display;
	EGLSurface *surface;
	int width, height;

	fbdev_size(&width, &height);

	printf("FB dimensions %dx%d\n", width, height);

	display = egl_display_init();
	surface = egl_surface_init(display, width, height);

	/* only a separate color buffer clear triggers depth clear */
	glClear(GL_COLOR_BUFFER_BIT);
        glClear(GL_DEPTH_BUFFER_BIT);

	eglSwapBuffers(display, surface);

	gl_error_test();

	return 0;
}
#endif
