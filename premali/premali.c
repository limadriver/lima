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
#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>

#define u32 uint32_t
#define USING_MALI200
#include "mali_200_regs.h"
#include "mali_ioctl.h"

#include "bmp.h"
#include "fb.h"
#include "plb.h"
#include "symbols.h"
#include "premali.h"
#include "jobs.h"
#include "gp.h"
#include "pp.h"

int dev_mali_fd;

#define WIDTH 800
#define HEIGHT 480

#include "vs.h"

void
vs_commands_create(struct vs_info *info)
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

	cmds[i].val = 0x03000000;
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

#if 0
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
unsigned int unlinked[] = {
	0xad4ad463, 0x478002b5, 0x0147ff80, 0x000a8d30, /* 0x00000000 */
	/*            ^^       */
	/*       attribute 1   */
	0xad4fda56, 0x038022ce, 0x0007ff80, 0x000ad510, /* 0x00000010 */
	0xb04b02cd, 0x43802ac2, 0xc6462180, 0x000a8d08, /* 0x00000020 */
	/*            ^^          ^^ ^^^^            ^ */
	/*       attribute 0                */
	0xad490722, 0x478082b5, 0x0007ff80, 0x000d5700, /* 0x00000030 */
	/*            ^^     */
	/*       attribute 1 */
	0xad4a4980, 0x478002b5, 0x0007ff80, 0x000ad500, /* 0x00000040 */
	/*            ^^     */
	/*       attribute 1 */
	0xb5cbcafb, 0x038049d3, 0x0007ff80, 0x000ad500, /* 0x00000050 */
	0x6c8b42b5, 0x03804193, 0x4243c080, 0x000ac508, /* 0x00000060 */
	/*                        ^^ ^^^^            ^ */
};
#endif

#define VERTEX_SHADER_SIZE 7
unsigned int
vertex_shader[VERTEX_SHADER_SIZE * 4] = {
	0xad4ad463, 0x438002b5, 0x0147ff80, 0x000a8d30, /* 0x00000000 */
	0xad4fda56, 0x038022ce, 0x0007ff80, 0x000ad510, /* 0x00000010 */
	0xb04b02cd, 0x47802ac2, 0x42462180, 0x000a8d08, /* 0x00000020 */
	0xad490722, 0x438082b5, 0x0007ff80, 0x000d5700, /* 0x00000030 */
	0xad4a4980, 0x438002b5, 0x0007ff80, 0x000ad500, /* 0x00000040 */
	0xb5cbcafb, 0x038049d3, 0x0007ff80, 0x000ad500, /* 0x00000050 */
	0x6c8b42b5, 0x03804193, 0xc643c080, 0x000ac508, /* 0x00000060 */
};

#define FRAGMENT_SHADER_SIZE 3
unsigned int
fragment_shader[FRAGMENT_SHADER_SIZE] = {0x000000a3, 0xf0003c60, 0x00000000};

int
main(int argc, char *argv[])
{
	_mali_uk_init_mem_s mem_init;
	int mem_physical;
	void *mem_address;
	int ret;
	struct plb *plb;
	struct pp_info *pp_info;
	struct vs_info *vs_info;
	struct plbu_info *plbu_info;
	_mali_uk_gp_start_job_s gp_job = {0};
	float vertices[] = { 0.0, -0.6, 0.0,
			     0.4, 0.6, 0.0,
			     -0.4, 0.6, 0.0};
	struct symbol *aPosition =
		symbol_create("aPosition", SYMBOL_ATTRIBUTE, 12, 3, vertices, 0);
	float colors[] = {0.0, 1.0, 0.0, 1.0,
			  0.0, 0.0, 1.0, 1.0,
			  1.0, 0.0, 0.0, 1.0};
	struct symbol *aColors =
		symbol_create("aColors", SYMBOL_ATTRIBUTE, 16, 3, colors, 0);
	struct symbol *vColors =
		symbol_create("vColors", SYMBOL_VARYING, 0, 16, NULL, 0);

	dev_mali_fd = open("/dev/mali", O_RDWR);
	if (dev_mali_fd == -1) {
		printf("Error: Failed to open /dev/mali: %s\n",
		       strerror(errno));
		return errno;
	}

	mem_init.ctx = (void *) dev_mali_fd;
	mem_init.mali_address_base = 0;
	mem_init.memory_size = 0;
	ret = ioctl(dev_mali_fd, MALI_IOC_MEM_INIT, &mem_init);
	if (ret == -1) {
		printf("Error: ioctl MALI_IOC_MEM_INIT failed: %s\n",
		       strerror(errno));
		return errno;
	}

	mem_physical = 0x40000000;
	mem_address = mmap(NULL, 0x80000, PROT_READ | PROT_WRITE, MAP_SHARED,
			   dev_mali_fd, mem_physical);
	if (mem_address == MAP_FAILED) {
		printf("Error: failed to mmap offset 0x%x (0x%x): %s\n",
		       mem_physical, 0x80000, strerror(errno));
		return errno;
	}

	vs_info = vs_info_create(mem_address + 0x0000,
				 mem_physical + 0x0000, 0x1000);

	vs_info_attach_standard_uniforms(vs_info, WIDTH, HEIGHT);

	vs_info_attach_attribute(vs_info, aPosition);
	vs_info_attach_attribute(vs_info, aColors);

	vs_info_attach_varying(vs_info, vColors);
	vs_info_attach_shader(vs_info, vertex_shader, VERTEX_SHADER_SIZE);
	// TODO: move to gp.c once more is known.
	vs_commands_create(vs_info);
	vs_info_finalize(vs_info);

	plb = plb_create(WIDTH, HEIGHT, mem_physical, mem_address,
			 0x3000, 0x7D000);
	if (!plb)
		return -1;

	plbu_info = plbu_info_create(mem_address + 0x1000,
				     mem_physical + 0x1000, 0x1000);
	plbu_info_attach_shader(plbu_info, fragment_shader, FRAGMENT_SHADER_SIZE);
	// TODO: move to gp.c once more is known.
	plbu_info_render_state_create(plbu_info, vs_info);
	plbu_info_finalize(plbu_info, plb, vs_info, WIDTH, HEIGHT);

	gp_job_setup(&gp_job, vs_info, plbu_info);

	ret = premali_gp_job_start(&gp_job);
	if (ret)
		return ret;

	fb_clear();

	pp_info = pp_info_create(WIDTH, HEIGHT, 0xFF505050, plb,
				 mem_address + 0x2000,
				 mem_physical + 0x2000,
				 0x1000, mem_physical + 0x80000);

	ret = premali_pp_job_start(pp_info->job);
	if (ret)
		return ret;

	premali_jobs_wait();

	bmp_dump(pp_info->frame_address, pp_info->frame_size,
		 pp_info->width, pp_info->height, "/sdcard/premali.bmp");

	fb_dump(pp_info->frame_address, pp_info->frame_size,
		pp_info->width, pp_info->height);

	fflush(stdout);

	return 0;
}
