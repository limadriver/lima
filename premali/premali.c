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

void
vs_commands_create(struct mali_cmd *cmds)
{
	int i = 0;

	cmds[i].val = MALI_VS_CMD_ARRAYS_SEMAPHORE_BEGIN_1;
	cmds[i].cmd = MALI_VS_CMD_ARRAYS_SEMAPHORE;
	i++;

	cmds[i].val = MALI_VS_CMD_ARRAYS_SEMAPHORE_BEGIN_2;
	cmds[i].cmd = MALI_VS_CMD_ARRAYS_SEMAPHORE;
	i++;

	cmds[i].val = 0x40000000; /* vs shader address */
	cmds[i].cmd = MALI_VS_CMD_SHADER_ADDRESS | (7 << 16); /* vs shader size */
	i++;

	cmds[i].val = 0x00401800;
	cmds[i].cmd = 0x10000040;
	i++;

	cmds[i].val = 0x01000100;
	cmds[i].cmd = 0x10000042;
	i++;

	cmds[i].val = 0x40000240; /* uniforms address */
	cmds[i].cmd = MALI_VS_CMD_UNIFORMS_ADDRESS | (3 << 16); /* ALIGN(uniforms_size, 4) / 4 */
	i++;

	cmds[i].val = 0x40000300; /* shared address space for attributes and varyings, half/half */
	cmds[i].cmd = MALI_VS_CMD_COMMON_ADDRESS | (0x40 << 16); /* (guessing) common size / 4 */
	i++;

	cmds[i].val = 0x00000003;
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

void
mali_uniforms_create(unsigned int *uniforms, int size)
{
	{ 	/* gl_mali_ViewportTransform */
		float x0 = 0, x1 = WIDTH, y0 = 0, y1 = HEIGHT;
		float depth_near = 0, depth_far = 1;

		uniforms[0] = from_float(x1 / 2);
		uniforms[1] = from_float(y1 / 2);
		uniforms[2] = from_float((depth_far - depth_near) / 2);
		uniforms[3] = from_float(depth_far);
		uniforms[4] = from_float((x0 + x1) / 2);
		uniforms[5] = from_float((y0 + y1) / 2);
		uniforms[6] = from_float((depth_near + depth_far) / 2);
		uniforms[7] = from_float(depth_near);
	}

	{	/* __maligp2_constant_000 */
		uniforms[8] = from_float(-1e+10);
		uniforms[9] = from_float(1e+10);
		uniforms[10] = 0;
		uniforms[11] = 0;
	}
}

int
main(int argc, char *argv[])
{
	_mali_uk_init_mem_s mem_init;
	int ret;
	struct plb *plb;
	struct pp_info *pp_info;

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

	mali_uniforms_create(mem_0x40000000.address + 0x240, 12);

	plb = plb_create(WIDTH, HEIGHT, 0x40000000, mem_0x40000000.address,
			 0x80000, 0x80000);
	if (!plb)
		return -1;

	vs_commands_create((struct mali_cmd *) (mem_0x40000000.address + 0x400));
	plbu_commands_create((struct mali_cmd *) (mem_0x40000000.address + 0x500),
			     WIDTH, HEIGHT, plb);

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
