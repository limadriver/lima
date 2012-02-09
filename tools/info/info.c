/*
 * Copyright (c) 2011 Luc Verhaegen <libv@codethink.co.uk>
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

#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#define u32 uint32_t
#include "linux/mali_ioctl.h"

static char *
mali_core_type_to_name(_mali_core_type type)
{
	switch (type) {
	case _MALI_GP2:
		return "MaliGP2 Programmable Vertex Processor (GP)";
	case _MALI_200:
		return "Mali200 Programmable Fragment Processor (PP)";
	case _MALI_400_GP:
		return "Mali400 Programmable Vertex Processor (GP)";
	case _MALI_400_PP:
		return "Mali400 Programmable Fragment Processor (PP)";
	default:
		return "Unknown";
	}
}

int
main(int argc, char *argv[])
{
	int fd;
	_mali_uk_get_api_version_s api_version;
	_mali_uk_get_system_info_size_s system_info_size;
	_mali_uk_get_system_info_s system_info_ioctl;
	struct _mali_system_info *system_info;
	_mali_core_info *core;
	_mali_uk_init_mem_s mem_init;
	int ret;

	fd = open("/dev/mali", O_RDONLY);
	if (fd == -1) {
		printf("Error: Failed to open /dev/mali: %s\n",
		       strerror(errno));
		return errno;
	}

	ret = ioctl(fd, MALI_IOC_GET_API_VERSION, &api_version);
	if (ret) {
		printf("Error: ioctl(GET_API_VERSION) failed: %s\n",
		       strerror(ret));
		return ret;
	}

	ret = ioctl(fd, MALI_IOC_GET_SYSTEM_INFO_SIZE, &system_info_size);
	if (ret) {
		printf("Error: ioctl(GET_SYSTEM_INFO_SIZE) failed: %s\n",
		       strerror(ret));
		return ret;
	}

	system_info_ioctl.size = system_info_size.size;
	system_info_ioctl.system_info = calloc(1, system_info_size.size);
	if (!system_info_ioctl.system_info) {
		printf("Error: failed to allocate system info.\n");
		return ENOMEM;
	}

	ret = ioctl(fd, MALI_IOC_GET_SYSTEM_INFO, &system_info_ioctl);
	if (ret) {
		printf("Error: ioctl(GET_SYSTEM_INFO) failed: %s\n",
		       strerror(ret));
		return ret;
	}

	ret = ioctl(fd, MALI_IOC_MEM_INIT, &mem_init);
	if (ret) {
		printf("Error: ioctl(MEM_INIT) failed: %s\n",
		       strerror(ret));
		return ret;
	}

	system_info = system_info_ioctl.system_info;

	if (system_info->has_mmu)
		printf("Mali MMU at 0x%08X, can address 0x%08X bytes\n",
		       mem_init.mali_address_base, mem_init.memory_size);
	else
		printf("Mali lacks an MMU!!!\n");

	printf("Mali Core(s) Info:\n");
	core = system_info->core_info;
	do {
		printf("  %s #%d (V%08X) at 0x%08X\n",
		       mali_core_type_to_name(core->type), core->core_nr,
		       core->version, core->reg_address);
		core = core->next;
	} while (core);

	printf("Mali Kernel Driver API version is %d\n",
	       api_version.version & 0xFFFF);

	return 0;
}
