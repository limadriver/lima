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

#define u32 uint32_t
#define USING_MALI200
#include "mali_200_regs.h"
#include "mali_ioctl.h"

#include "dump.h"
#include "premali.h"
#include "jobs.h"
#include "bmp.h"
#include "fb.h"

#include "dumped_stream.c"

int
main(int argc, char *argv[])
{
	int ret;

	ret = premali_init();
	if (ret)
		return ret;

	ret = dumped_mem_load(&dumped_mem);
	if (ret)
		return ret;

	ret = premali_gp_job_start(&gp_job);
	if (ret)
		return ret;

	fb_clear();

	ret = premali_pp_job_start(&pp_job);
	if (ret)
		return ret;

	premali_jobs_wait();

	bmp_dump(mem_0x40080000.address, 0,
		 dump_render_width, dump_render_height, "/sdcard/premali.bmp");

	fb_dump(mem_0x40080000.address, 0,
		dump_render_width, dump_render_height);

	premali_finish();

	return 0;
}
