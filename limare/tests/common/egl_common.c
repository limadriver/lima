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
#include <time.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include <linux/fb.h>

#include "egl_common.h"

#ifdef _X11_XLIB_H_
#define WIDTH 480
#define HEIGHT 480
#endif /* _X11_XLIB_H_ */

#ifdef _X11_XLIB_H_
Window native_window;
#else
struct mali_native_window native_window[1];
#endif

#ifdef _X11_XLIB_H_
void
buffer_size(int *width, int *height)
{
	*width = WIDTH;
	*height = HEIGHT;
}
#else
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
#endif /* _X11_XLIB_H_ */

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

#ifdef _X11_XLIB_H_
	XSetWindowAttributes XWinAttr;
	Atom XWMDeleteMessage;
	Window XRoot;

	Display *XDisplay = XOpenDisplay(NULL);
	if (!XDisplay) {
		fprintf(stderr, "Error: failed to open X display.\n");
		return NULL;
	}

	XRoot = DefaultRootWindow(XDisplay);

	XWinAttr.event_mask  =  ExposureMask | PointerMotionMask;

	native_window = XCreateWindow(XDisplay, XRoot, 0, 0, WIDTH, HEIGHT, 0,
				CopyFromParent, InputOutput,
				CopyFromParent, CWEventMask, &XWinAttr);

	XWMDeleteMessage = XInternAtom(XDisplay, "WM_DELETE_WINDOW", False);

	XMapWindow(XDisplay, native_window);
	XStoreName(XDisplay, native_window, "lima egl test");
	XSetWMProtocols(XDisplay, native_window, &XWMDeleteMessage, 1);

	display = eglGetDisplay((EGLNativeDisplayType) XDisplay);
#else
	display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
#endif /* _X11_XLIB_H_ */

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
egl_surface_init(EGLDisplay display, int gles_version, int width, int height)
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
	EGLint context_gles1_attributes[] = {
		EGL_CONTEXT_CLIENT_VERSION, 1,
		EGL_NONE
	};
	EGLint context_gles2_attributes[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};
	int i;

#ifndef _X11_XLIB_H_
	native_window->width = width;
	native_window->height = height;
#endif

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

	if (gles_version == 2)
		context = eglCreateContext(display, configs[i], EGL_NO_CONTEXT,
					   context_gles2_attributes);
	else
		context = eglCreateContext(display, configs[i], EGL_NO_CONTEXT,
					   context_gles1_attributes);
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

/*
 * simplistic benchmarking.
 */
static struct timespec framerate_time;
static struct timespec framerate_start;

void
framerate_init(void)
{
	if (clock_gettime(CLOCK_MONOTONIC, &framerate_start))
		printf("Error: failed to get time: %s\n", strerror(errno));

	framerate_time = framerate_start;
}

void
_framerate_print(int count, int total)
{
	struct timespec new = { 0 };
	int usec;
	long long average;

	if (clock_gettime(CLOCK_MONOTONIC, &new)) {
		printf("Error: failed to get time: %s\n", strerror(errno));
		return;
	}

	usec = (new.tv_sec - framerate_time.tv_sec) * 1000000;
	usec += (new.tv_nsec - framerate_time.tv_nsec) / 1000;

	average = (new.tv_sec - framerate_start.tv_sec) * 1000000;
	average += (new.tv_nsec - framerate_start.tv_nsec) / 1000;

	framerate_time = new;

	printf("%df in %fs: %f fps (%4d at %f fps)\n", count,
	       (float) usec / 1000000, (float) (count * 1000000) / usec,
	       total, (double) (total * 1000000.0) / average);
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
