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
#include <fcntl.h>
#include <errno.h>
#include <inttypes.h>

#define u32 uint32_t
#define USING_MALI200
#include "mali_200_regs.h"
#include "mali_ioctl.h"

#include "premali.h"

static int
premali_fd_open(struct premali_state *state)
{
	state->fd = open("/dev/mali", O_RDWR);
	if (state->fd == -1) {
		printf("Error: Failed to open /dev/mali: %s\n",
		       strerror(errno));
		return errno;
	}

	return 0;
}

static int
premali_gpu_detect(struct premali_state *state)
{
	_mali_uk_get_system_info_size_s system_info_size;
	_mali_uk_get_system_info_s system_info_ioctl;
	struct _mali_system_info *system_info;
	int ret;

	ret = ioctl(state->fd, MALI_IOC_GET_SYSTEM_INFO_SIZE,
		    &system_info_size);
	if (ret) {
		printf("Error: %s: ioctl(GET_SYSTEM_INFO_SIZE) failed: %s\n",
		       __func__, strerror(ret));
		return ret;
	}

	system_info_ioctl.size = system_info_size.size;
	system_info_ioctl.system_info = calloc(1, system_info_size.size);
	if (!system_info_ioctl.system_info) {
		printf("%s: Error: failed to allocate system info: %s\n",
		       __func__, strerror(errno));
		return errno;
	}

	ret = ioctl(state->fd, MALI_IOC_GET_SYSTEM_INFO, &system_info_ioctl);
	if (ret) {
		printf("%s: Error: ioctl(GET_SYSTEM_INFO) failed: %s\n",
		       __func__, strerror(ret));
		free(system_info_ioctl.system_info);
		return ret;
	}

	system_info = system_info_ioctl.system_info;

	switch (system_info->core_info->type) {
	case _MALI_GP2:
	case _MALI_200:
		printf("Detected Mali-200.\n");
		state->type = 200;
		break;
	case _MALI_400_GP:
	case _MALI_400_PP:
		printf("Detected Mali-400.\n");
		state->type = 400;
		break;
	default:
		break;
	}

	return 0;
}

/*
 * TODO: MEMORY MANAGEMENT!!!!!!!!!
 *
 */
static int
premali_mem_init(struct premali_state *state)
{
	_mali_uk_init_mem_s mem_init = { 0 };
	int ret;

	mem_init.ctx = (void *) state->fd;
	mem_init.mali_address_base = 0;
	mem_init.memory_size = 0;
	ret = ioctl(state->fd, MALI_IOC_MEM_INIT, &mem_init);
	if (ret == -1) {
		printf("Error: ioctl MALI_IOC_MEM_INIT failed: %s\n",
		       strerror(errno));
		return errno;
	}

	state->mem_physical = mem_init.mali_address_base;
	state->mem_size = 0x80000;
	state->mem_address = mmap(NULL, state->mem_size, PROT_READ | PROT_WRITE,
				  MAP_SHARED, state->fd, state->mem_physical);
	if (state->mem_address == MAP_FAILED) {
		printf("Error: failed to mmap offset 0x%x (0x%x): %s\n",
		       state->mem_physical, state->mem_size, strerror(errno));
		return errno;
	}

	return 0;
}

struct premali_state *
premali_init(void)
{
	struct premali_state *state;
	int ret;

	state = calloc(1, sizeof(struct premali_state));
	if (!state) {
		printf("%s: Error: failed to allocate state: %s\n",
		       __func__, strerror(errno));
		goto error;
	}

	ret = premali_fd_open(state);
	if (ret)
		goto error;

	ret = premali_gpu_detect(state);
	if (ret)
		goto error;

	ret = premali_mem_init(state);
	if (ret)
		goto error;

	return state;
 error:
	free(state);
	return NULL;
}

void
premali_dimensions_set(struct premali_state *state, int width, int height)
{
	if (!state)
		return;

	state->width = width;
	state->height = height;
}

void
premali_clear_color_set(struct premali_state *state, unsigned int clear_color)
{
	if (!state)
		return;

	state->clear_color = clear_color;
}

/*
 * Just run fflush(stdout) to give the wrapper library a chance to finish.
 */
void
premali_finish(void)
{
	fflush(stdout);
}
