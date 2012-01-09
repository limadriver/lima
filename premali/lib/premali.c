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

int dev_mali_fd;
int mem_physical;
void *mem_address;

/*
 * TODO: MEMORY MANAGEMENT!!!!!!!!!
 *
 */
int
premali_init(void)
{
	_mali_uk_init_mem_s mem_init;
	int ret;

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

	return 0;
}

/*
 * Just run fflush(stdout) to give the wrapper library a chance to finish.
 */
void
premali_finish(void)
{
	fflush(stdout);
}
