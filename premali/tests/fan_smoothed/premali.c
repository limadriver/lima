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
 * Template file for replaying a dumped stream.
 */

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

#include <GLES2/gl2.h>

#include "premali.h"
#include "jobs.h"
#include "bmp.h"
#include "fb.h"
#include "ioctl.h"
#include "plb.h"
#include "symbols.h"
#include "gp.h"
#include "pp.h"

#define WIDTH 800
#define HEIGHT 480

#include "vs.h"

void
vs_commands_create(struct vs_info *info, int vertex_count)
{
	struct mali_cmd *cmds = info->commands;
	int i = 0;

	cmds[i].val = MALI_VS_CMD_ARRAYS_SEMAPHORE_BEGIN_1;
	cmds[i].cmd = MALI_VS_CMD_ARRAYS_SEMAPHORE;
	i++;

	cmds[i].val = MALI_VS_CMD_ARRAYS_SEMAPHORE_BEGIN_2;
	cmds[i].cmd = MALI_VS_CMD_ARRAYS_SEMAPHORE;
	i++;

	cmds[i].val = info->mem_physical + info->shader_offset;
	cmds[i].cmd = MALI_VS_CMD_SHADER_ADDRESS | (info->shader_size << 16);
	i++;

	cmds[i].val = 0x00401800; /* will become clearer when linking */
	cmds[i].cmd = 0x10000040;
	i++;

	cmds[i].val = 0x01000100; /* will become clearer when linking */
	cmds[i].cmd = 0x10000042;
	i++;

	cmds[i].val = info->mem_physical + info->uniform_offset;
	cmds[i].cmd = MALI_VS_CMD_UNIFORMS_ADDRESS |
		(ALIGN(info->uniform_used, 4) << 14);
	i++;

	cmds[i].val = info->mem_physical + info->common_offset;
	cmds[i].cmd = MALI_VS_CMD_COMMON_ADDRESS | (info->common_size << 14);
	i++;

	cmds[i].val = 0x00000003; /* always 3 */
	cmds[i].cmd = 0x10000041;
	i++;

	cmds[i].val = (vertex_count << 24);
	cmds[i].cmd = 0x00000000;
	i++;

	cmds[i].val = 0x00000000;
	cmds[i].cmd = 0x60000000;
	i++;

	cmds[i].val = MALI_VS_CMD_ARRAYS_SEMAPHORE_END;
	cmds[i].cmd = MALI_VS_CMD_ARRAYS_SEMAPHORE;
	i++;

	/* update our size so we can set the gp job properly */
	info->commands_size = i * sizeof(struct mali_cmd);
}

int
plbu_info_render_state_create(struct plbu_info *info, struct vs_info *vs)
{
	int size;

	if (info->render_state) {
		printf("%s: render_state already assigned\n", __func__);
		return -1;
	}

	size = ALIGN(0x10 * 4, 0x40);
	if (size > (info->mem_size - info->mem_used)) {
		printf("%s: no more space\n", __func__);
		return -2;
	}

	if (!info->shader) {
		printf("%s: no shader attached yet!\n", __func__);
		return -3;
	}

	info->render_state = info->mem_address + info->mem_used;
	info->render_state_offset = info->mem_used;
	info->render_state_size = size;
	info->mem_used += size;

	/* this bit still needs some figuring out :) */
	info->render_state[0x00] = 0;
	info->render_state[0x01] = 0;
	info->render_state[0x02] = 0xfc3b1ad2;
	info->render_state[0x03] = 0x3E;
	info->render_state[0x04] = 0xFFFF0000;
	info->render_state[0x05] = 7;
	info->render_state[0x06] = 7;
	info->render_state[0x07] = 0;
	info->render_state[0x08] = 0xF807;
	info->render_state[0x09] = info->mem_physical + info->shader_offset +
		info->shader_size;
	info->render_state[0x0A] = 0x00000002;
	info->render_state[0x0B] = 0x00000000;

	info->render_state[0x0C] = 0;
	info->render_state[0x0D] = 0x301;
	info->render_state[0x0E] = 0x2000;

	info->render_state[0x0F] = vs->varyings[0]->physical;

	return 0;
}

/*
 * Attribute linking:
 *
 * One single attribute per command:
 *  bits 0x1C.2 through 0x1C.5 of every instruction,
 *  bit 0x1C.6 is set when there is an attribute.
 *
 * Before linking, attribute 0 is aColor, and attribute 1 is aPosition,
 * as obviously spotted from the attribute link stream.
 * After linking, attribute 0 is aVertices, and attribute 1 is aColor.
 * This matches the way in which we attach our attributes.
 *
 * And indeed, if we swap the bits and the attributes over, things work fine,
 * and if we only swap the attributes over, then the rendering is broken in a
 * logical way.
 */
/*
 * Varying linking:
 *
 * 4 entries, linked to two varyings. If both varyings are specified, they have
 * to be the same.
 *
 * Bits 47-52, contain 4 entries, 3 bits each. 7 means invalid. 2 entries per
 * varying.
 * Bits 5A through 63 specify two varyings, 5 bits each. Top bit means
 * valid/defined, other is most likely the index in the common area.
 *
 * How this fully fits together, is not entirely clear yet, but hopefully
 * clear enough to implement linking in one of the next stages.
 */
/*
 * Uniform linking:
 *
 * Since there is just a memory blob with uniforms packed in it, it would
 * appear that there is no linking, and uniforms just have to be packed as
 * defined in the uniform link stream.
 *
 */

#include "compiler.h"

void
vertex_shader_attributes_patch(unsigned int *shader, int size)
{
	int i;

	for (i = 0; i < size; i++) {
		int tmp = (shader[4 * i + 1] >> 26) & 0x1F;

		if (!(tmp & 0x10))
			continue;

		tmp &= 0x0F;
		shader[4 * i + 1] &= ~(0x0F << 26);

		/* program specific bit, for now */
		if (!tmp)
			tmp = 1;
		else
			tmp = 0;

		shader[4 * i + 1] |= tmp << 26;
	}
}

void
vertex_shader_varyings_patch(unsigned int *shader, int size)
{
	int i;

	for (i = 0; i < size; i++) {
		int tmp;

		/* ignore entries for now, until we know more */

		tmp = (shader[4 * i + 2] >> 26) & 0x1F;

		if (tmp & 0x10) {
			tmp &= 0x0F;
			shader[4 * i + 2] &= ~(0x0F << 26);

			/* program specific bit, for now */
			if (!tmp)
				tmp = 1;
			else
				tmp = 0;

			shader[4 * i + 2] |= tmp << 26;
		}

		tmp = ((shader[4 * i + 2] >> 31) & 0x01) |
			((shader[4 * i + 3] << 1) & 0x1E);

		if (tmp & 0x10) {
			tmp &= 0x0F;
			shader[4 * i + 2] &= ~(0x01 << 31);
			shader[4 * i + 3] &= ~0x07;

			/* program specific bit, for now */
			if (!tmp)
				tmp = 1;
			else
				tmp = 0;

			shader[4 * i + 2] |= tmp << 31;
			shader[4 * i + 3] |= tmp >> 1;
		}
	}
}

int
vertex_shader_compile(struct vs_info *info, const char *source)
{
	struct mali_shader_binary binary = {0};
	unsigned int *shader;
	int length = strlen(source);
	int ret, size;

	ret = __mali_compile_essl_shader(&binary, MALI_SHADER_VERTEX,
					 (char *) source, &length, 1);
	if (ret) {
		if (binary.error_log)
			printf("%s: compilation failed: %s\n",
			       __func__, binary.error_log);
		else
			printf("%s: compilation failed: %s\n",
			       __func__, binary.oom_log);
		return ret;
	}

	shader = binary.shader;
	size = binary.shader_size / 16;

	vertex_shader_attributes_patch(shader, size);
	vertex_shader_varyings_patch(shader, size);

	vs_info_attach_shader(info, shader, size);

	return 0;
}

int
fragment_shader_compile(struct plbu_info *info, const char *source)
{
	struct mali_shader_binary binary = {0};
	unsigned int *shader;
	int length = strlen(source);
	int ret, size;

	ret = __mali_compile_essl_shader(&binary, MALI_SHADER_FRAGMENT,
					 (char *) source, &length, 1);
	if (ret) {
		if (binary.error_log)
			printf("%s: compilation failed: %s\n",
			       __func__, binary.error_log);
		else
			printf("%s: compilation failed: %s\n",
			       __func__, binary.oom_log);
		return ret;
	}

	shader = binary.shader;
	size = binary.shader_size / 4;

	plbu_info_attach_shader(info, shader, size);

	return 0;
}

int
main(int argc, char *argv[])
{
	int ret;
	struct plb *plb;
	struct pp_info *pp_info;
	struct vs_info *vs_info;
	struct plbu_info *plbu_info;
	struct mali_gp_job_start gp_job = {0};
	float vertices[] = {  0.0,  0.8, 0.0,
			     -0.8,  0.4, 0.0,
			     -0.6, -0.5, 0.0,
			      0.0, -0.8, 0.0,
			      0.6, -0.5, 0.0,
			      0.8,  0.4, 0.0 };
	struct symbol *aPosition =
		symbol_create("aPosition", SYMBOL_ATTRIBUTE, 12, 3, 6, vertices, 0);
	float colors[] = {1.0, 1.0, 1.0, 1.0,
			  1.0, 0.0, 0.0, 1.0,
			  1.0, 1.0, 0.0, 1.0,
			  0.0, 1.0, 0.0, 1.0,
			  0.0, 1.0, 1.0, 1.0,
			  0.0, 0.0, 1.0, 1.0};
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

	// TODO: move to gp.c once more is known.
	vs_commands_create(vs_info, 6);
	vs_info_finalize(vs_info);

	printf("attribute[0].size = 0x%x\n",
	       vs_info->common->attributes[0].size);
	printf("attribute[1].size = 0x%x\n",
	       vs_info->common->attributes[1].size);

	vs_info->common->attributes[0].size = 0x6002;
	vs_info->common->attributes[1].size = 0x8003;

	plb = plb_create(WIDTH, HEIGHT, mem_physical, mem_address,
			 0x3000, 0x7D000);
	if (!plb)
		return -1;

	plbu_info = plbu_info_create(mem_address + 0x1000,
				     mem_physical + 0x1000, 0x1000);
	fragment_shader_compile(plbu_info, fragment_shader_source);
	// TODO: move to gp.c once more is known.
	plbu_info_render_state_create(plbu_info, vs_info);
	plbu_info_finalize(plbu_info, plb, vs_info, WIDTH, HEIGHT,
			   GL_TRIANGLE_FAN, 6);

	ret = premali_gp_job_start(&gp_job, vs_info, plbu_info);
	if (ret)
		return ret;

	fb_clear();

	pp_info = pp_info_create(WIDTH, HEIGHT, 0xFF505050, plb,
				 mem_address + 0x2000,
				 mem_physical + 0x2000,
				 0x1000, mem_physical + 0x80000);

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

