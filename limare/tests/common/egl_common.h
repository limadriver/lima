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

#ifndef EGL_COMMON_H
#define EGL_COMMON_H 1

void buffer_size(int *width, int *height);

void gl_error_test(void);
void egl_error_test(void);
EGLDisplay egl_display_init(void);
EGLSurface *egl_surface_init(EGLDisplay display, int gles_version,
			     int width, int height);

int vertex_shader_compile(const char *source);
int fragment_shader_compile(const char *source);

void framerate_init(void);
void _framerate_print(int count, int total);

#define framerate_print(c, t) \
do { \
	if (!((t) & ((c) - 1))) \
		_framerate_print((c), (t)); \
} while (0)

#endif /* EGL_COMMON_H */
