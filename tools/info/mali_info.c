/*
 * Copyright (c) 2011 Luc Verhaegen <libv@codethink.co.uk>
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
