/*
 * Copyright 2011-2012 Luc Verhaegen <libv@codethink.co.uk>
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

/*
 * Draws a smoothed triangle strip.
 */

#include <stdlib.h>
#include <stdio.h>

#include <GLES2/gl2.h>

#include "premali.h"
#include "bmp.h"
#include "fb.h"
#include "ioctl.h"
#include "jobs.h"
#include "plb.h"
#include "symbols.h"
#include "gp.h"
#include "pp.h"
#include "program.h"

#define WIDTH 800
#define HEIGHT 480

int
main(int argc, char *argv[])
{
	int ret;
	struct plb *plb;
	struct pp_info *pp_info;
	struct vs_info *vs_info;
	struct plbu_info *plbu_info;
	struct mali_gp_job_start gp_job = {0};
	float vertices[] = { -0.7,  0.7, 0.0,
			     -0.7,  0.2, 0.0,
			      0.0, -0.2, 0.0,
			      0.0, -0.7, 0.0,
			      0.7,  0.7, 0.0,
			      0.7,  0.2, 0.0};
	struct symbol *aPosition =
		symbol_create("aPosition", SYMBOL_ATTRIBUTE, 12, 3, 6, vertices, 0);
	float colors[] = {1.0, 0.0, 0.0, 1.0,
			  1.0, 1.0, 0.0, 1.0,
			  1.0, 0.0, 1.0, 1.0,
			  0.0, 1.0, 0.0, 1.0,
			  0.0, 0.0, 1.0, 1.0,
			  0.0, 1.0, 1.0, 1.0};
	struct symbol *aColors =
		symbol_create("aColors", SYMBOL_ATTRIBUTE, 16, 4, 6, colors, 0);
	struct symbol *vColors =
		symbol_create("vColors", SYMBOL_VARYING, 0, 16, 0, NULL, 0);

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

	fb_clear();

	ret = premali_init();
	if (ret)
		return ret;

	vs_info = vs_info_create(mem_address + 0x0000,
				 mem_physical + 0x0000, 0x1000);

	vertex_shader_compile(vs_info, vertex_shader_source);

	vs_info_attach_standard_uniforms(vs_info, WIDTH, HEIGHT);

	vs_info_attach_attribute(vs_info, aPosition);
	vs_info_attach_attribute(vs_info, aColors);

	vs_info_attach_varying(vs_info, vColors);

	vs_commands_create(vs_info, 6);
	vs_info_finalize(vs_info);

	plb = plb_create(WIDTH, HEIGHT, mem_physical, mem_address,
			 0x3000, 0x7D000);
	if (!plb)
		return -1;

	plbu_info = plbu_info_create(mem_address + 0x1000,
				     mem_physical + 0x1000, 0x1000);
	fragment_shader_compile(plbu_info, fragment_shader_source);

	plbu_info_render_state_create(plbu_info, vs_info);
	plbu_info_finalize(plbu_info, plb, vs_info, WIDTH, HEIGHT,
			   GL_TRIANGLE_STRIP, 6);

	pp_info = pp_info_create(WIDTH, HEIGHT, 0xFF505050, plb,
				 mem_address + 0x2000,
				 mem_physical + 0x2000,
				 0x1000, mem_physical + 0x80000);

	ret = premali_gp_job_start(&gp_job, vs_info, plbu_info);
	if (ret)
		return ret;

	ret = premali_pp_job_start(pp_info);
	if (ret)
		return ret;

	premali_jobs_wait();

	bmp_dump(pp_info->frame_address, pp_info->frame_size,
		 pp_info->width, pp_info->height, "/sdcard/premali.bmp");

	fb_dump(pp_info->frame_address, pp_info->frame_size,
		pp_info->width, pp_info->height);

	premali_finish();

	return 0;
}
