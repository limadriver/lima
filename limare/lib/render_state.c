/*
 * Copyright (c) 2012-2013 Luc Verhaegen <libv@skynet.be>
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
#include <string.h>
#include <errno.h>

#include "limare.h"
#include "gp.h"
#include "compiler.h"
#include "program.h"
#include "render_state.h"

struct render_state *
limare_render_state_template(void)
{
	struct render_state *render = calloc(1, sizeof(struct render_state));

	if (!render) {
		printf("%s: Error: failed to allocate state: %s\n",
		       __func__, strerror(errno));
		return NULL;
	}

	render->unknown00 = 0;
	render->unknown04 = 0;
	render->unknown08 = 0xfc3b1ad2;
	render->unknown0C = 0x3E;
	render->depth_range = 0xFFFF0000;
	render->unknown14 = 0xFF000007;
	render->unknown18 = 0xFF000007;
	render->unknown1C = 0;
	render->unknown20 = 0xF807;
	render->shader_address = 0xFFFFFFFF;
	render->varying_types = 0xFFFFFFFF;
	render->uniforms_address = 0xFFFFFFFF;
	render->textures_address = 0xFFFFFFFF;
	render->unknown34 = 0x00000300;
	render->unknown38 = 0x00002000;
	render->varyings_address = 0xFFFFFFFF;

	/* enable 4x MSAA */
	render->unknown20 |= 0x68;

	/* no idea what this is yet. */
	render->unknown38 |= 0x1000;

	return render;
}


int
draw_render_state_create(struct limare_frame *frame,
			 struct limare_program *program,
			 struct draw_info *draw,
			 struct render_state *template)
{
	struct plbu_info *plbu = draw->plbu;
	struct vs_info *vs = draw->vs;
	struct render_state *render;
	int size, i;

	if (plbu->render_state) {
		printf("%s: render_state already assigned\n", __func__);
		return -1;
	}

	size = ALIGN(sizeof(struct render_state), 0x40);
	if (size > (frame->mem_size - frame->mem_used)) {
		printf("%s: no more space\n", __func__);
		return -2;
	}

	plbu->render_state = frame->mem_address + frame->mem_used;
	plbu->render_state_offset = frame->mem_used;
	plbu->render_state_size = size;
	frame->mem_used += size;

	/* this bit still needs some figuring out :) */
	render = plbu->render_state;

	render->unknown00 = template->unknown00;
	render->unknown04 = template->unknown04;
	render->unknown08 = template->unknown08;
	render->unknown0C = template->unknown0C;
	render->depth_range = template->depth_range;
	render->unknown14 = template->unknown14;
	render->unknown18 = template->unknown18;
	render->unknown1C = template->unknown1C;
	render->unknown20 = template->unknown20;

	render->shader_address =
		(program->mem_physical + program->fragment_offset) |
		program->fragment_binary->parameters.fragment.unknown04;

	render->uniforms_address = 0;
	render->textures_address = 0;

	render->unknown34 = template->unknown34;
	render->unknown38 = template->unknown38;

	if (vs->varying_size) {
		render->varyings_address = frame->mem_physical +
			vs->varying_offset;
		render->unknown34 |= program->varying_map_size >> 3;
		render->varying_types = 0;

		for (i = 0; i < program->varying_map_count; i++) {
			int val;

			if (program->varying_map[i].entry_size == 4) {
				if (program->varying_map[i].entries == 4)
					val = 0;
				else
					val = 1;
			} else {
				if (program->varying_map[i].entries == 4)
					val = 2;
				else
					val = 3;
			}

			if (i < 10)
				render->varying_types |= val << (3 * i);
			else if (i == 10) {
				render->varying_types |= val << 30;
				render->varyings_address |= val >> 2;
			} else if (i == 11)
				render->varyings_address |= val << 1;

		}
	}

	if (plbu->uniform_size) {
		render->uniforms_address =
			frame->mem_physical + plbu->uniform_array_offset;

		render->uniforms_address |=
			(ALIGN(plbu->uniform_size, 4) / 4) - 1;

		render->unknown34 |= 0x80;
		render->unknown38 |= 0x10000;
	}

	if (draw->texture_descriptor_count) {

		render->textures_address = frame->mem_physical +
			draw->texture_descriptor_list_offset;
		render->unknown34 |= draw->texture_descriptor_count << 14;

		render->unknown34 |= 0x20;
	}

	return 0;
}
