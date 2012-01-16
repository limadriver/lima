/*
 * Copyright (c) 2011-2012 Luc Verhaegen <libv@codethink.co.uk>
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

/*
 * Draws a single smoothed triangle.
 */

#include <stdlib.h>
#include <stdio.h>

#include <GLES2/gl2.h>

#include "premali.h"
#include "ioctl.h"
#include "bmp.h"
#include "fb.h"
#include "plb.h"
#include "symbols.h"
#include "jobs.h"
#include "gp.h"
#include "pp.h"
#include "program.h"

#define WIDTH 800
#define HEIGHT 480

int
main(int argc, char *argv[])
{
	struct premali_state *state;
	int ret;
	struct plb *plb;
	struct pp_info *pp_info;
	struct vs_info *vs_info;
	struct plbu_info *plbu_info;
	struct mali_gp_job_start gp_job = {0};
	float vertices[] = { 0.0, -0.6, 0.0,
			     0.4, 0.6, 0.0,
			     -0.4, 0.6, 0.0};
	struct symbol *aPosition =
		symbol_create("aPosition", SYMBOL_ATTRIBUTE, 12, 3, 3, vertices, 0);
	float colors[] = {0.0, 1.0, 0.0, 1.0,
			  0.0, 0.0, 1.0, 1.0,
			  1.0, 0.0, 0.0, 1.0};
	struct symbol *aColors =
		symbol_create("aColors", SYMBOL_ATTRIBUTE, 16, 4, 3, colors, 0);
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

	state = premali_init();
	if (!state)
		return -1;

	premali_dimensions_set(state, WIDTH, HEIGHT);
	premali_clear_color_set(state, 0xFF505050);


	vs_info = vs_info_create(state, state->mem_address + 0x0000,
				 state->mem_physical + 0x0000, 0x1000);

	plb = plb_create(state, state->mem_physical, state->mem_address,
			 0x3000, 0x7D000);
	if (!plb)
		return -1;

	plbu_info = plbu_info_create(state->mem_address + 0x1000,
				     state->mem_physical + 0x1000, 0x1000);

	pp_info = pp_info_create(state, plb,
				 state->mem_address + 0x2000,
				 state->mem_physical + 0x2000,
				 0x1000, state->mem_physical + 0x80000);


	vertex_shader_compile(vs_info, vertex_shader_source);
	fragment_shader_compile(plbu_info, fragment_shader_source);


	vs_info_attach_standard_uniforms(state, vs_info);

	vs_info_attach_attribute(vs_info, aPosition);
	vs_info_attach_attribute(vs_info, aColors);

	vs_info_attach_varying(vs_info, vColors);


	vs_commands_create(state, vs_info, 3);
	vs_info_finalize(state, vs_info);

	plbu_info_render_state_create(plbu_info, vs_info);
	plbu_info_finalize(state, plbu_info, plb, vs_info, GL_TRIANGLES, 3);


	ret = premali_gp_job_start(state, &gp_job, vs_info, plbu_info);
	if (ret)
		return ret;

	ret = premali_pp_job_start(state, pp_info);
	if (ret)
		return ret;

	premali_jobs_wait();

	bmp_dump(pp_info->frame_address, 0,
		 state->width, state->height, "/sdcard/premali.bmp");

	fb_dump(pp_info->frame_address, pp_info->frame_size,
		state->width, state->height);

	premali_finish();

	return 0;
}
