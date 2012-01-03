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

#include "formats.h"
#include "registers.h"

#define u32 uint32_t
#define USING_MALI200
#include "mali_200_regs.h"
#include "mali_ioctl.h"

#include "bmp.h"
#include "fb.h"
#include "plb.h"
#include "premali.h"
#include "jobs.h"
#include "dump.h"
#include "pp.h"

int dev_mali_fd;

#define WIDTH 800
#define HEIGHT 480

#include "dumped_stream.c"

struct mali_cmd {
	unsigned int val;
	unsigned int cmd;
};

#include "vs.h"

enum symbol_type {
	SYMBOL_UNIFORM,
	SYMBOL_ATTRIBUTE,
	SYMBOL_VARYING,
};

struct symbol {
	/* as referenced by the shaders and shader compiler binary streams */
	const char *name;

	enum symbol_type type;

	int element_size;
	int element_count;

	void *address;
	int physical; /* empty for uniforms */

	void *data;
};

struct symbol *
symbol_create(const char *name, enum symbol_type type, int element_size,
	      int count, void *data, int copy)
{
	struct symbol *symbol;

	if (copy)
		symbol = calloc(1, sizeof(struct symbol) + element_size * count);
	else
		symbol = calloc(1, sizeof(struct symbol));
	if (!symbol) {
		printf("%s: failed to allocate: %s\n", __func__, strerror(errno));
		return NULL;
	}

	symbol->name = name;
	symbol->type = type;

	symbol->element_size = element_size;
	symbol->element_count = count;

	if (copy) {
		symbol->data = &symbol[1];
		memcpy(symbol->data, data, element_size * count);
	} else
		symbol->data = data;

	return symbol;
}

struct symbol *
uniform_gl_mali_ViewPortTransform(float x0, float y0, float x1, float y1,
				  float depth_near, float depth_far)
{
	float viewport[8];

	viewport[0] = x1 / 2;
	viewport[1] = y1 / 2;
	viewport[2] = (depth_far - depth_near) / 2;
	viewport[3] = depth_far;
	viewport[4] = (x0 + x1) / 2;
	viewport[5] = (y0 + y1) / 2;
	viewport[6] = (depth_near + depth_far) / 2;
	viewport[7] = depth_near;

	return symbol_create("gl_mali_ViewportTransform", SYMBOL_UNIFORM,
			     4, 8, viewport, 1);
}

struct symbol *
uniform___maligp2_constant_000(void)
{
	float constant[] = {-1e+10, 1e+10, 0.0, 0.0};

	return symbol_create("__maligp2_constant_000", SYMBOL_UNIFORM,
			     4, 4, constant, 1);
}

struct vs_info {
	void *mem_address;
	int mem_physical;
	int mem_used;
	int mem_size;

	struct mali_cmd *commands;
	int commands_offset;
	int commands_size;

	struct gp_common *common;
	int common_offset;
	int common_size;

	struct symbol *uniforms[0x10];
	int uniform_count;
	int uniform_offset;
	int uniform_used;
	int uniform_size;

	struct symbol *attributes[0x10];
	int attribute_count;

	struct symbol *varyings[0x10];
	int varying_count;
	int varying_element_size;

	/* finishes off our varyings */
	int vertex_array_offset;
	int vertex_array_size;

	unsigned int *shader;
	int shader_offset;
	int shader_size;
};

struct gp_common_entry {
	unsigned int physical;
	int size; /* (element_size << 11) | (element_count - 1) */
};

struct gp_common {
	struct gp_common_entry attributes[0x10];
	struct gp_common_entry varyings[0x10];
};

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

struct vs_info *
vs_info_create(void *address, int physical, int size)
{
	struct vs_info *info;
	int i;

	info = calloc(1, sizeof(struct vs_info));
	if (!info) {
		printf("%s: Error allocating structure: %s\n",
		       __func__, strerror(errno));
		return NULL;
	}

	info->mem_address = address;
	info->mem_physical = physical;
	info->mem_size = size;

	info->commands = info->mem_address + info->mem_used;
	info->commands_offset = info->mem_used;
	info->commands_size = 0x10 * sizeof(struct mali_cmd);
	info->mem_used += ALIGN(info->commands_size, 0x40);

	info->common = info->mem_address + info->mem_used;
	info->common_offset = info->mem_used;
	info->common_size = sizeof(struct gp_common);
	info->mem_used += ALIGN(info->common_size, 0x40);

	/* initialize common */
	for (i = 0; i < 0x10; i++) {
		info->common->attributes[i].physical = 0;
		info->common->attributes[i].size = 0x3F;
	}
	for (i = 0; i < 0x10; i++) {
		info->common->varyings[i].physical = 0;
		info->common->varyings[i].size = 0x3F;
	}

	/* predefine an area for the uniforms */
	info->uniform_offset = info->mem_used;
	info->uniform_size = 0x200;
	info->mem_used += ALIGN(info->uniform_size, 0x40);

	/* predefine an area for our vertex array */
	info->vertex_array_offset = info->mem_used;
	info->vertex_array_size = 0x40;
	info->mem_used += 0x40;

	/* leave the rest empty for now */

	if (info->mem_used > info->mem_size) {
		printf("%s: Not enough memory\n", __func__);
		free(info);
		return NULL;
	}

	return info;
}

int
vs_info_attach_uniform(struct vs_info *info, struct symbol *uniform)
{
	int size;

	if (info->uniform_count == 0x10) {
		printf("%s: No more uniforms\n", __func__);
		return -1;
	}

	size = uniform->element_size * uniform->element_count;
	if (size > (info->uniform_size - info->uniform_used)) {
		printf("%s: No more space\n", __func__);
		return -2;
	}

	uniform->address = info->mem_address + info->uniform_offset +
		info->uniform_used;

	info->uniform_used += size;
	info->uniforms[info->uniform_count] = uniform;
	info->uniform_count++;

	memcpy(uniform->address, uniform->data,
	       uniform->element_size * uniform->element_count);

	return 0;
}

int
vs_info_attach_standard_uniforms(struct vs_info *info)
{
	struct symbol *viewport =
		uniform_gl_mali_ViewPortTransform(0.0, 0.0, WIDTH, HEIGHT, 0.0, 1.0);
	struct symbol *constant = uniform___maligp2_constant_000();
	int ret;

	ret = vs_info_attach_uniform(info, viewport);
	if (ret)
		return ret;

	ret = vs_info_attach_uniform(info, constant);
	if (ret)
		return ret;

	return 0;
}

int
vs_info_attach_attribute(struct vs_info *info, struct symbol *attribute)
{
	int size;

	if (info->attribute_count == 0x10) {
		printf("%s: No more attributes\n", __func__);
		return -1;
	}

	size = ALIGN(attribute->element_size * attribute->element_count, 0x40);
	if (size > (info->mem_size - info->mem_used)) {
		printf("%s: No more space\n", __func__);
		return -2;
	}

	attribute->address = info->mem_address + info->mem_used;
	attribute->physical = info->mem_physical + info->mem_used;
	info->mem_used += size;

	info->attributes[info->attribute_count] = attribute;
	info->attribute_count++;

	memcpy(attribute->address, attribute->data,
	       attribute->element_size * attribute->element_count);

	return 0;
}

/* varyings are still a bit of black magic at this point */
int
vs_info_attach_varying(struct vs_info *info, struct symbol *varying)
{
	int size;

	/* last varying is always the vertex array */
	if (info->varying_count == 0x0F) {
		printf("%s: No more varyings\n", __func__);
		return -1;
	}

	if (varying->element_size) {
		printf("%s: invalid varying %s\n", __func__, varying->name);
		return -3;
	}

	size = ALIGN(4 * varying->element_count, 0x40);
	if (size > (info->mem_size - info->mem_used)) {
		printf("%s: No more space\n", __func__);
		return -2;
	}

	varying->physical = info->mem_physical + info->mem_used;
	info->mem_used += size;

	info->varyings[info->varying_count] = varying;
	info->varying_count++;

	/* the vertex shader fills in the varyings */

	return 0;
}

int
vs_info_attach_shader(struct vs_info *info, unsigned int *shader, int size)
{
	int mem_size;

	if (info->shader != NULL) {
		printf("%s: shader already assigned\n", __func__);
		return -1;
	}

	mem_size = ALIGN(size * 16, 0x40);
	if (mem_size > (info->mem_size - info->mem_used)) {
		printf("%s: no more space\n", __func__);
		return -2;
	}

	info->shader = info->mem_address + info->mem_used;
	info->shader_offset = info->mem_used;
	info->shader_size = size;
	info->mem_used += mem_size;

	memcpy(info->shader, shader, 16 * size);

	return 0;
}

void
vs_info_finalize(struct vs_info *info)
{
	int i;

	for (i = 0; i < info->attribute_count; i++) {
		info->common->attributes[i].physical = info->attributes[i]->physical;
		info->common->attributes[i].size =
			(info->attributes[i]->element_size << 11) |
			(info->attributes[i]->element_count - 1);
	}

	for (i = 0; i < info->varying_count; i++) {
		info->common->varyings[i].physical = info->varyings[i]->physical;
		info->common->varyings[i].size = (8 << 11) |
			(info->varyings[i]->element_count - 1);
	}

	info->common->varyings[i].physical =
		info->mem_physical + info->vertex_array_offset;
	info->common->varyings[i].size = (16 << 11) | (1 - 1) | 0x20;

	/* hardcode until plbu is complete */
	info->common->varyings[i].physical = 0x40000140;
	info->common->varyings[i].size = 0x8020;

	vs_commands_create(info);
}

#include "plbu.h"

void
plbu_commands_create(struct mali_cmd *cmds, int width, int height,
		     struct plb *plb)
{
	int i = 0;

	cmds[i].val = plb->shift_w | (plb->shift_h << 16);
	cmds[i].cmd = MALI_PLBU_CMD_BLOCK_STEP;
	i++;

	cmds[i].val = ((plb->width - 1) << 24) | ((plb->height - 1) << 8);
	cmds[i].cmd = MALI_PLBU_CMD_TILED_DIMENSIONS;
	i++;

	cmds[i].val = plb->width >> plb->shift_w;
	cmds[i].cmd = MALI_PLBU_CMD_PLBU_BLOCK_STRIDE;
	i++;

	cmds[i].val = plb->mem_physical + plb->plbu_offset;
	cmds[i].cmd = MALI_PLBU_CMD_PLBU_ARRAY_ADDRESS;
	i++;

#if 0
	cmds[i].val = 0x40100000;
	cmds[i].cmd = MALI_PLBU_CMD_TILE_HEAP_START;
	i++;

	cmds[i].val = 0x40150000;
	cmds[i].cmd = MALI_PLBU_CMD_TILE_HEAP_END;
	i++;
#endif

	cmds[i].val = from_float(0.0);
	cmds[i].cmd = MALI_PLBU_CMD_VIEWPORT_Y;
	i++;

	cmds[i].val = from_float(height);
	cmds[i].cmd = MALI_PLBU_CMD_VIEWPORT_H;
	i++;

	cmds[i].val = from_float(0.0);
	cmds[i].cmd = MALI_PLBU_CMD_VIEWPORT_X;
	i++;

	cmds[i].val = from_float(width);
	cmds[i].cmd = MALI_PLBU_CMD_VIEWPORT_W;
	i++;

	/*
	 *
	 */
	cmds[i].val = MALI_PLBU_CMD_ARRAYS_SEMAPHORE_BEGIN;
	cmds[i].cmd = MALI_PLBU_CMD_ARRAYS_SEMAPHORE;
	i++;

	cmds[i].val = 0x00002200;
	cmds[i].cmd = MALI_PLBU_CMD_PRIMITIVE_SETUP;
	i++;

	cmds[i].val = 0x40000280; /* RSW address */
	cmds[i].cmd = MALI_PLBU_CMD_RSW_VERTEX_ARRAY | (0x40000140 >> 4); /* vertex array address */
	i++;

	cmds[i].val = 0x00000000;
	cmds[i].cmd = 0x1000010a;
	i++;

	cmds[i].val = from_float(0.0);
	cmds[i].cmd = MALI_PLBU_CMD_DEPTH_RANGE_NEAR;
	i++;

	cmds[i].val = from_float(1.0);
	cmds[i].cmd = MALI_PLBU_CMD_DEPTH_RANGE_FAR;
	i++;

	/* could this be some sort of delay or wait? */
	cmds[i].val = 0x03000000;
	cmds[i].cmd = 0x00040000;
	i++;

	cmds[i].val = MALI_PLBU_CMD_ARRAYS_SEMAPHORE_END;
	cmds[i].cmd = MALI_PLBU_CMD_ARRAYS_SEMAPHORE;
	i++;

	/*
	 * Some inter-frame communication apparently.
	 */
#if 0
	cmds[i].val = 0x400e41c0;
	cmds[i].cmd = 0xa0000103;
	i++;

	cmds[i].val = 0x400e41c4;
	cmds[i].cmd = 0xa0000104;
	i++;

	cmds[i].val = 0x400e41c8;
	cmds[i].cmd = 0xa0000107;
	i++;

	cmds[i].val = 0x400e41cc;
	cmds[i].cmd = 0xa0000108;
	i++;

	cmds[i].val = 0x400e41d0;
	cmds[i].cmd = 0xa0000105;
	i++;

	cmds[i].val = 0x400e41d4;
	cmds[i].cmd = 0xa0000106;
	i++;
#endif

	cmds[i].val = 0x00000000;
	cmds[i].cmd = 0xd0000000;
	i++;

	cmds[i].val = 0x00000000;
	cmds[i].cmd = 0xd0000000;
	i++;

	cmds[i].val = 0;
	cmds[i].cmd = MALI_PLBU_CMD_END;
}

#if 0
/* A single attribute is bits 0x1C.2 through 0x1C.5 of every instruction,
   bit 0x1C.6 is set when there is an attribute. */

unsigned int unlinked[] = {
	{
		0xad4ad463, 0x478002b5, 0x0147ff80, 0x000a8d30, /* 0x00000000 */
		/*            /\ valid attribute is here. Note that it is 1 here, and 0 after linking. */
		0xad4fda56, 0x038022ce, 0x0007ff80, 0x000ad510, /* 0x00000010 */
		0xb04b02cd, 0x43802ac2, 0xc6462180, 0x000a8d08, /* 0x00000020 */
		/*            /\ attribute 0. */
		/*                       */
		0xad490722, 0x478082b5, 0x0007ff80, 0x000d5700, /* 0x00000030 */
		0xad4a4980, 0x478002b5, 0x0007ff80, 0x000ad500, /* 0x00000040 */
		0xb5cbcafb, 0x038049d3, 0x0007ff80, 0x000ad500, /* 0x00000050 */
		0x6c8b42b5, 0x03804193, 0x4243c080, 0x000ac508, /* 0x00000060 */
	},
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

int
main(int argc, char *argv[])
{
	_mali_uk_init_mem_s mem_init;
	int ret;
	struct plb *plb;
	struct pp_info *pp_info;
	struct vs_info *vs_info;
	_mali_uk_gp_start_job_s gp_job = {0};
	float vertices[] = { 0.0, -0.6, 0.0,
			     0.4, 0.6, 0.0,
			     -0.4, 0.6, 0.0};
	struct symbol *aVertices =
		symbol_create("aVertices", SYMBOL_ATTRIBUTE, 12, 3, vertices, 0);
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

	ret = dumped_mem_load(&dumped_mem);
	if (ret)
		return ret;

	vs_info = vs_info_create(mem_0x40000000.address + 0x2000,
				 0x40000000 + 0x2000, 0x1000);

	vs_info_attach_standard_uniforms(vs_info);
	vs_info_attach_attribute(vs_info, aVertices);
	vs_info_attach_attribute(vs_info, aColors);
	vs_info_attach_varying(vs_info, vColors);
	vs_info_attach_shader(vs_info, vertex_shader, VERTEX_SHADER_SIZE);

	/* hardcode our varyings until the plbu stage is complete */
	vColors->physical = 0x40000180;

	vs_info_finalize(vs_info);

	plb = plb_create(WIDTH, HEIGHT, 0x40000000, mem_0x40000000.address,
			 0x80000, 0x80000);
	if (!plb)
		return -1;

	plbu_commands_create((struct mali_cmd *) (mem_0x40000000.address + 0x500),
			     WIDTH, HEIGHT, plb);


	gp_job.frame_registers[MALI_GP_VS_COMMANDS_START] =
		vs_info->mem_physical + vs_info->commands_offset;
	gp_job.frame_registers[MALI_GP_VS_COMMANDS_END] = vs_info->mem_physical +
		vs_info->commands_offset + vs_info->commands_size;

	gp_job.frame_registers[MALI_GP_PLBU_COMMANDS_START] = 0x40000500;
	gp_job.frame_registers[MALI_GP_PLBU_COMMANDS_END] = 0x40000600;
	gp_job.frame_registers[MALI_GP_TILE_HEAP_START] = 0;
	gp_job.frame_registers[MALI_GP_TILE_HEAP_END] = 0;

	ret = premali_gp_job_start(&gp_job);
	if (ret)
		return ret;

	fb_clear();

	pp_info = pp_info_create(WIDTH, HEIGHT, 0xFF505050, plb,
				 mem_0x40000000.address + 0x1000,
				 0x40001000, 0x1000, 0x40200000);

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
