/*
 * Copyright (c) 2011-2012 Luc Verhaegen <libv@skynet.be>
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
#include <asm/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

#define u32 uint32_t
#include "linux/mali_ioctl.h"

struct {
	int version;
	const char *compat;
} version_compat[] = {
	{6, "r2p0 and r2p1-rel0"},
	{7, "r2p1-rel1"},
	{8, "r2p2"},
	{9, "r2p3"},
	{10, "r2p4"},
	{14, "r3p0"},
	{17, "r3p1"},
	{0, NULL},
};

static const char *
mali_version_compatibility_get(int version)
{
	int i;

	for (i = 0; version_compat[i].compat; i++)
		if (version_compat[i].version == version)
			return version_compat[i].compat;

	return "UNKNOWN";
}

static int
cores_detect(int fd, int version)
{
	_mali_uk_get_pp_number_of_cores_s pp_number = { 0 };
	_mali_uk_get_pp_core_version_s pp_version = { 0 };
	_mali_uk_get_gp_number_of_cores_s gp_number = { 0 };
	_mali_uk_get_gp_core_version_s gp_version = { 0 };
	int ret, type;

	if (version < 14) {
		ret = ioctl(fd, MALI_IOC_PP_NUMBER_OF_CORES_GET,
			    &pp_number);
		if (ret) {
			printf("Error: %s: ioctl(PP_NUMBER_OF_CORES_GET) "
			       "failed: %s\n", __func__, strerror(-ret));
			return ret;
		}

		ret = ioctl(fd, MALI_IOC_PP_CORE_VERSION_GET,
			    &pp_version);
		if (ret) {
			printf("Error: %s: ioctl(PP_CORE_VERSION_GET) "
			       "failed: %s\n", __func__, strerror(-ret));
			return ret;
		}

		ret = ioctl(fd, MALI_IOC_GP2_NUMBER_OF_CORES_GET,
			    &gp_number);
		if (ret) {
			printf("Error: %s: ioctl(GP2_NUMBER_OF_CORES_GET) "
			       "failed: %s\n", __func__, strerror(-ret));
			return ret;
		}

		ret = ioctl(fd, MALI_IOC_GP2_CORE_VERSION_GET,
			    &gp_version);
		if (ret) {
			printf("Error: %s: ioctl(GP2_CORE_VERSION_GET) "
			       "failed: %s\n", __func__, strerror(-ret));
			return ret;
		}
	} else {
		ret = ioctl(fd, MALI_IOC_PP_NUMBER_OF_CORES_GET_NEW,
			    &pp_number);
		if (ret) {
			printf("Error: %s: ioctl(PP_NUMBER_OF_CORES_GET) "
			       "failed: %s\n", __func__, strerror(-ret));
			return ret;
		}

		ret = ioctl(fd, MALI_IOC_PP_CORE_VERSION_GET_NEW,
			    &pp_version);
		if (ret) {
			printf("Error: %s: ioctl(PP_CORE_VERSION_GET) "
			       "failed: %s\n", __func__, strerror(-ret));
			return ret;
		}

		ret = ioctl(fd, MALI_IOC_GP2_NUMBER_OF_CORES_GET_NEW,
			    &gp_number);
		if (ret) {
			printf("Error: %s: ioctl(GP2_NUMBER_OF_CORES_GET) "
			       "failed: %s\n", __func__, strerror(-ret));
			return ret;
		}

		ret = ioctl(fd, MALI_IOC_GP2_CORE_VERSION_GET_NEW,
			    &gp_version);
		if (ret) {
			printf("Error: %s: ioctl(GP2_CORE_VERSION_GET) "
			       "failed: %s\n", __func__, strerror(-ret));
			return ret;
		}
	}

	switch (gp_version.version >> 16) {
	case 0xA07:
		type = 200;
		break;
	case 0xC07:
		type = 300;
		break;
	case 0xB07:
		type = 400;
		break;
	case 0xD07:
		type = 450;
		break;
	default:
		type = 0;
		break;
	}

	printf("\t%d Mali-%03d GP Core(s) (vertex shader).\n",
	       gp_number.number_of_cores, type);

	switch (pp_version.version >> 16) {
	case 0xC807:
		type = 200;
		break;
	case 0xCE07:
		type = 300;
		break;
	case 0xCD07:
		type = 400;
		break;
	case 0xCF07:
		type = 450;
		break;
	default:
		type = 0;
		break;
	}

	printf("\t%d Mali-%03d PP Core(s) (fragment shader).\n",
	       pp_number.number_of_cores, type);

	return 0;
}

int
main(int argc, char *argv[])
{
	int fd;
	_mali_uk_get_api_version_s api_version;
	_mali_uk_init_mem_s mem_init;
	int ret, version;

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

	version = api_version.version & 0xFFFF;

	printf("Mali Kernel Driver API version is %d (compatible with release "
	       "%s)\n", version, mali_version_compatibility_get(version));

	ret = ioctl(fd, MALI_IOC_MEM_INIT, &mem_init);
	if (ret) {
		printf("Error: ioctl(MEM_INIT) failed: %s\n",
		       strerror(ret));
		return ret;
	}

	printf("Mali MMU at 0x%08X, can address 0x%08X bytes\n",
	       mem_init.mali_address_base, mem_init.memory_size);


	printf("Mali Core(s) Info:\n");

	return cores_detect(fd, version);
}
