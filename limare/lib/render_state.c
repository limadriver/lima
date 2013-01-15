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

#include <GLES2/gl2.h>

#include "limare.h"
#include "gp.h"
#include "compiler.h"
#include "program.h"
#include "render_state.h"

static unsigned int blend_func;

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

	blend_func = (render->unknown08 & 0x00FFFFC0) >> 6;

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

int
limare_render_state_set(struct render_state *render,
			int parameter, int value)
{
	switch (parameter) {
	case GL_BLEND:
		render->unknown08 &= ~0x00FFFFC0;
		if (value)
			render->unknown08 |= blend_func << 6;
		else
			render->unknown08 |= 0x0000EC6B << 6;
		return 0;
	default:
		printf("%s: Error: unknown parameter: 0x%04X\n", __func__,
		       parameter);
		return -1;
	}
}

struct limare_translate {
	int gl_value;
	int lima_value;
};

int
limare_translate(struct limare_translate *table, int gl_value)
{
	int i;

	for (i = 0; table[i].gl_value != -1; i++)
		if (table[i].gl_value == gl_value)
			return table[i].lima_value;

	return -1;
}

static struct limare_translate compare_funcs[] = {
	{GL_NEVER,	0x00},
	{GL_LESS,	0x01},
	{GL_EQUAL,	0x02},
	{GL_LEQUAL,	0x03},
	{GL_GREATER,	0x04},
	{GL_NOTEQUAL,	0x05},
	{GL_GEQUAL,	0x06},
	{GL_ALWAYS,	0x07},
	{-1, -1},
};

int
limare_render_state_depth_func(struct render_state *render, int gl_value)
{
	int lima_value = limare_translate(compare_funcs, gl_value);

	if (lima_value == -1) {
	       printf("%s: Error: unknown value: 0x%04X\n", __func__,
		      gl_value);
	       return -1;
	}

	render->unknown0C &= ~0x0E;
	render->unknown0C |= lima_value << 1;

	return 0;
}

int
limare_render_state_depth_mask(struct render_state *render, int value)
{
	if ((render->unknown0C & 0x0E) == 0x0E)
		value = 0;

	if (value)
		render->unknown0C |= 0x01;
	else
		render->unknown0C &= ~0x01;

	return 0;
}

static struct limare_translate blend_funcs[] = {
	{GL_SRC_COLOR,			0x00},
	{GL_DST_COLOR,			0x01},
	{GL_CONSTANT_COLOR,		0x02},
	{GL_ZERO,			0x03},
	{GL_SRC_ALPHA_SATURATE,		0x07},
	{GL_ONE_MINUS_SRC_COLOR,	0x08},
	{GL_ONE_MINUS_DST_COLOR,	0x09},
	{GL_ONE_MINUS_CONSTANT_COLOR,	0x0A},
	{GL_ONE,			0x0B},
	{GL_SRC_ALPHA,			0x10},
	{GL_DST_ALPHA,			0x11},
	{GL_CONSTANT_ALPHA,		0x12},
	{GL_ONE_MINUS_SRC_ALPHA,	0x18},
	{GL_ONE_MINUS_DST_ALPHA,	0x19},
	{GL_ONE_MINUS_CONSTANT_ALPHA,	0x1A},
	{-1, -1},
};

int
limare_render_state_blend_func(struct render_state *render,
			       int sfactor, int dfactor)
{
	int rgb_src = limare_translate(blend_funcs, sfactor);
	int rgb_dst = limare_translate(blend_funcs, dfactor);
	int alpha_src, alpha_dst;

	if (rgb_src == -1) {
	       printf("%s: Error: unknown source: 0x%04X\n", __func__,
		      sfactor);
	       return -1;
	}

	if (rgb_dst == -1) {
		printf("%s: Error: unknown destination: 0x%04X\n", __func__,
		       dfactor);
	       return -1;
	}

	alpha_src = rgb_src & 0x0F;
	alpha_dst = rgb_dst & 0x0F;

	render->unknown08 &= ~0x00FFFFC0;
	render->unknown08 |= rgb_src << 6;
	render->unknown08 |= rgb_dst << 11;
	render->unknown08 |= alpha_src << 16;
	render->unknown08 |= alpha_dst << 20;

	blend_func = render->unknown08 >> 6;

	return 0;
}

int
limare_render_state_depth(struct render_state *render, float near, float far)
{
	unsigned short tmp;

	render->depth_range = 0;

	tmp = near * 0xFFFF;
	render->depth_range |= tmp;

	tmp = far * 0xFFFF;
	if (tmp != 0xFFFF)
		tmp += 1;
	render->depth_range |= tmp << 16;

	return 0;
}

int
limare_render_state_polygon_offset(struct render_state *render, int value)
{
	render->unknown0C &= ~0x00FF0000;

	if (value) {
		unsigned char tmp;

		if (value > 0x80)
			tmp = 0x80;
		else /* automatically sets 0x80 */
			tmp = 0x100 - value;
		render->unknown0C |= tmp << 16;
	}

	return 0;
}

int
limare_render_state_alpha_func(struct render_state *render,
			       int func, float alpha)
{
	int lima_func = limare_translate(compare_funcs, func);
	int lima_alpha = alpha * 0x100;

	if (lima_func == -1) {
	       printf("%s: Error: unknown value: 0x%04X\n", __func__,
		      func);
	       return -1;
	}

	render->unknown1C &= ~0x00FF0000;
	if (lima_alpha >= 0x100)
		render->unknown1C |= 0x00FF0000;
	else if (lima_alpha > 0)
		render->unknown1C |= lima_alpha << 16;

	render->unknown20 &= ~0x07;
	render->unknown20 |= lima_func;

	if (func == GL_ALWAYS)
		render->unknown34 |= 0x200;
	else /* disable early Z */
		render->unknown34 &= ~0x200;

	return 0;
}

int
limare_render_state_color_mask(struct render_state *render,
			       int red, int green, int blue, int alpha)
{
	render->unknown08 &= ~0xF0000000;

	if (red)
		render->unknown08 |= 0x10000000;
	if (green)
		render->unknown08 |= 0x20000000;
	if (blue)
		render->unknown08 |= 0x40000000;
	if (alpha)
		render->unknown08 |= 0x80000000;

	return 0;
}
