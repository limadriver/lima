/*
 * Copyright 2011      Luc Verhaegen <libv@codethink.co.uk>
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

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <inttypes.h>

static int mali_ioctl(int request, void *data);

/*
 * First up, wrap around the libc calls that are crucial for capturing our
 * command stream, namely, open, ioctl, and mmap.
 */
static void *libc_dl;

static int
libc_dlopen(void)
{
	libc_dl = dlopen("libc.so", RTLD_LAZY);
	if (!libc_dl) {
		printf("Failed to dlopen %s: %s\n",
		       "libc.so", dlerror());
		exit(-1);
	}

	return 0;
}

static void *
libc_dlsym(const char *name)
{
	void *func;

	if (!libc_dl)
		libc_dlopen();

	func = dlsym(libc_dl, name);

	if (!func) {
		printf("Failed to find %s in %s: %s\n",
		       name, "libc.so", dlerror());
		exit(-1);
	}

	return func;
}

static int dev_mali_fd;

/*
 *
 */
static int (*orig_open)(const char* path, int mode, ...);

int
open(const char* path, int flags, ...)
{
	mode_t mode = 0;
	int ret;

	if (!orig_open)
		orig_open = libc_dlsym(__func__);

	if (flags & O_CREAT) {
		va_list  args;


		va_start(args, flags);
		mode = (mode_t) va_arg(args, int);
		va_end(args);

		ret = orig_open(path, flags, mode);
	} else {
		ret = orig_open(path, flags);

		if ((ret != -1) && !strcmp(path, "/dev/mali")) {
			dev_mali_fd = ret;
			printf("OPEN %d\n", ret);
		}
	}

	return ret;
}

/*
 *
 */
static int (*orig_close)(int fd);

int
close(int fd)
{
	if (!orig_close)
		orig_close = libc_dlsym(__func__);

	if (fd == dev_mali_fd) {
		printf("CLOSE");
		dev_mali_fd = -1;
	}

	return orig_close(fd);
}

/*
 *
 */
static int (*orig_ioctl)(int fd, int request, ...);

int
ioctl(int fd, int request, ...)
{
	int ioc_size = _IOC_SIZE(request);

	if (!orig_ioctl)
		orig_ioctl = libc_dlsym(__func__);

	if (ioc_size) {
		va_list args;
		void *ptr;

		va_start(args, request);
		ptr = va_arg(args, void *);
		va_end(args);

		if (fd == dev_mali_fd)
			return mali_ioctl(request, ptr);
		else
			return orig_ioctl(fd, request, ptr);
	} else {
		if (fd == dev_mali_fd)
			return mali_ioctl(request, NULL);
		else
			return orig_ioctl(fd, request);
	}
}

/*
 *
 */
void *(*orig_mmap)(void *addr, size_t length, int prot,
		   int flags, int fd, off_t offset);

void *
mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
{
	void *ret;

	if (!orig_mmap)
		orig_mmap = libc_dlsym(__func__);

        ret = orig_mmap(addr, length, prot, flags, fd, offset);

	if (fd == dev_mali_fd)
		printf("MMAP 0x%08lx (0x%08x) = %p\n", offset, length, ret);

	return ret;
}

/*
 *
 */
int (*orig_munmap)(void *addr, size_t length);

int
munmap(void *addr, size_t length)
{
	if (!orig_munmap)
		orig_munmap = libc_dlsym(__func__);

	return orig_munmap(addr, length);
}

/*
 *
 * Now the mali specific ioctl parsing.
 *
 */
char *
ioctl_dir_string(int request)
{
	switch (_IOC_DIR(request)) {
	default: /* cannot happen */
	case 0x00:
		return "_IO";
	case 0x01:
		return "_IOW";
	case 0x02:
		return "_IOR";
	case 0x03:
		return "_IOWR";
	}
}

#define u32 uint32_t
#define USING_MALI200
#include "mali_200_regs.h"
#include "mali_ioctl.h"

static struct dev_mali_ioctl_table {
	int type;
	int nr;
	char *name;
	void (*pre)(void *data);
	void (*post)(void *data);
} dev_mali_ioctls[] = {
	{MALI_IOC_CORE_BASE, _MALI_UK_OPEN, "CORE, OPEN", NULL, NULL},
	{MALI_IOC_CORE_BASE, _MALI_UK_CLOSE, "CORE, CLOSE", NULL, NULL},
	{MALI_IOC_CORE_BASE, _MALI_UK_GET_SYSTEM_INFO_SIZE, "CORE, GET_SYSTEM_INFO_SIZE", NULL, NULL},
	{MALI_IOC_CORE_BASE, _MALI_UK_GET_SYSTEM_INFO, "CORE, GET_SYSTEM_INFO", NULL, NULL},
	{MALI_IOC_CORE_BASE, _MALI_UK_WAIT_FOR_NOTIFICATION, "CORE, WAIT_FOR_NOTIFICATION", NULL, NULL},
	{MALI_IOC_CORE_BASE, _MALI_UK_GET_API_VERSION, "CORE, GET_API_VERSION", NULL, NULL},
	{MALI_IOC_MEMORY_BASE, _MALI_UK_INIT_MEM, "MEMORY, INIT_MEM", NULL, NULL},
	{MALI_IOC_PP_BASE, _MALI_UK_PP_START_JOB, "PP, START_JOB", NULL, NULL},
	{MALI_IOC_PP_BASE, _MALI_UK_GET_PP_CORE_VERSION, "PP, GET_CORE_VERSION", NULL, NULL},
	{MALI_IOC_GP_BASE, _MALI_UK_GP_START_JOB, "GP, START_JOB", NULL, NULL},

	{ 0, 0, NULL, NULL, NULL}
};

static int
mali_ioctl(int request, void *data)
{
	struct dev_mali_ioctl_table *ioctl = NULL;
	int ioc_type = _IOC_TYPE(request);
	int ioc_nr = _IOC_NR(request);
	char *ioc_string = ioctl_dir_string(request);
	int i;
	int ret;

	for (i = 0; dev_mali_ioctls[i].name; i++) {
		if ((dev_mali_ioctls[i].type == ioc_type) &&
		    (dev_mali_ioctls[i].nr == ioc_nr)) {
			ioctl = &dev_mali_ioctls[i];
			break;
		}
	}

	if (!ioctl)
		printf("Error: No mali ioctl wrapping implemented for %02X:%02X\n",
		       ioc_type, ioc_nr);

	if (ioctl && ioctl->pre)
		ioctl->pre(data);

	if (data)
		ret = orig_ioctl(dev_mali_fd, request, data);
	else
		ret = orig_ioctl(dev_mali_fd, request);

	if (ioctl) {
		if (data)
			printf("IOCTL %s(%s) %p = %d\n",
			       ioc_string, ioctl->name, data, ret);
		else
			printf("IOCTL %s(%s) = %d\n",
			       ioc_string, ioctl->name, ret);

		if (ioctl->post)
			ioctl->post(data);
	}

	return ret;
}
