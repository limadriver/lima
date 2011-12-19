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
#include <pthread.h>
#include <ctype.h>

#define u32 uint32_t
#define USING_MALI200
#include "mali_200_regs.h"
#include "mali_ioctl.h"

#include "compiler.h"

static int mali_ioctl(int request, void *data);
static int mali_address_add(void *address, unsigned int size,
			    unsigned int physical);
static int mali_address_remove(void *address, int size);
static void mali_memory_dump(void);

static pthread_mutex_t serializer[1] = { PTHREAD_MUTEX_INITIALIZER };

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

	pthread_mutex_lock(serializer);

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
			printf("OPEN;\n");
		}
	}

	pthread_mutex_unlock(serializer);

	return ret;
}

/*
 *
 */
static int (*orig_close)(int fd);

int
close(int fd)
{
	int ret;

	pthread_mutex_lock(serializer);

	if (!orig_close)
		orig_close = libc_dlsym(__func__);

	if (fd == dev_mali_fd) {
		printf("CLOSE;");
		dev_mali_fd = -1;
	}

	ret = orig_close(fd);

	pthread_mutex_unlock(serializer);

	return ret;
}

/*
 *
 */
static int (*orig_ioctl)(int fd, int request, ...);

int
ioctl(int fd, int request, ...)
{
	int ioc_size = _IOC_SIZE(request);
	int ret;
	int yield = 0;

	pthread_mutex_lock(serializer);

	if (!orig_ioctl)
		orig_ioctl = libc_dlsym(__func__);

	if (ioc_size) {
		va_list args;
		void *ptr;

		va_start(args, request);
		ptr = va_arg(args, void *);
		va_end(args);

		if (fd == dev_mali_fd) {
			if (request == MALI_IOC_WAIT_FOR_NOTIFICATION)
				yield = 1;

			ret = mali_ioctl(request, ptr);
		} else
			ret = orig_ioctl(fd, request, ptr);
	} else {
		if (fd == dev_mali_fd)
			ret = mali_ioctl(request, NULL);
		else
			ret = orig_ioctl(fd, request);
	}

	pthread_mutex_unlock(serializer);

	if (yield)
		sched_yield();

	return ret;
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

	pthread_mutex_lock(serializer);

	if (!orig_mmap)
		orig_mmap = libc_dlsym(__func__);

        ret = orig_mmap(addr, length, prot, flags, fd, offset);

	if (fd == dev_mali_fd) {
		printf("MMAP 0x%08lx (0x%08x) = %p;\n", offset, length, ret);
		mali_address_add(ret, length, offset);
		memset(ret, 0, length);
	}

	pthread_mutex_unlock(serializer);

	return ret;
}

/*
 *
 */
int (*orig_munmap)(void *addr, size_t length);

int
munmap(void *addr, size_t length)
{
	int ret;

	pthread_mutex_lock(serializer);

	if (!orig_munmap)
		orig_munmap = libc_dlsym(__func__);

	ret = orig_munmap(addr, length);

	if (!mali_address_remove(addr, length))
		printf("MUNMAP %p (0x%08x);\n", addr, length);

	pthread_mutex_unlock(serializer);

	return ret;
}

int (*orig_fflush)(FILE *stream);

int
fflush(FILE *stream)
{
	int ret;

	pthread_mutex_lock(serializer);

	if (!orig_fflush)
		orig_fflush = libc_dlsym(__func__);

	ret = orig_fflush(stream);

	pthread_mutex_unlock(serializer);

	return ret;
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

static void
dev_mali_get_api_version_pre(void *data)
{
	_mali_uk_get_api_version_s *version = data;

	printf("IOCTL MALI_IOC_GET_API_VERSION IN = {\n");
	printf("\t.version = 0x%08x,\n", version->version);
	printf("};\n");
}

static void
dev_mali_get_api_version_post(void *data)
{
	_mali_uk_get_api_version_s *version = data;

	printf("IOCTL MALI_IOC_GET_API_VERSION OUT = {\n");
	printf("\t.version = 0x%08x,\n", version->version);
	printf("\t.compatible = %d,\n", version->compatible);
	printf("};\n");
}

static void
dev_mali_get_system_info_size_post(void *data)
{
	_mali_uk_get_system_info_size_s *size = data;

	printf("IOCTL MALI_IOC_GET_SYSTEM_INFO_SIZE OUT = {\n");
	printf("\t.size = 0x%x,\n", size->size);
	printf("};\n");
}

static void
dev_mali_get_system_info_pre(void *data)
{
	_mali_uk_get_system_info_s *info = data;

	printf("IOCTL MALI_IOC_GET_SYSTEM_INFO IN = {\n");
	printf("\t.size = 0x%x,\n", info->size);
	printf("\t.system_info = <malloced, above size>,\n");
	printf("\t.ukk_private = 0x%x,\n", info->ukk_private);
	printf("};\n");
}

static void
dev_mali_get_system_info_post(void *data)
{
	_mali_uk_get_system_info_s *info = data;
	struct _mali_core_info *core;
	struct _mali_mem_info *mem;

	printf("IOCTL MALI_IOC_GET_SYSTEM_INFO OUT = {\n");
	printf("\t.system_info = {\n");

	core = info->system_info->core_info;
	while (core) {
		printf("\t\t.core_info = {\n");
		printf("\t\t\t.type = 0x%x,\n", core->type);
		printf("\t\t\t.version = 0x%x,\n", core->version);
		printf("\t\t\t.reg_address = 0x%x,\n", core->reg_address);
		printf("\t\t\t.core_nr = 0x%x,\n", core->core_nr);
		printf("\t\t\t.flags = 0x%x,\n", core->flags);
		printf("\t\t},\n");
		core = core->next;
	}

	mem = info->system_info->mem_info;
	while (mem) {
		printf("\t\t.mem_info = {\n");
		printf("\t\t\t.size = 0x%x,\n", mem->size);
		printf("\t\t\t.flags = 0x%x,\n", mem->flags);
		printf("\t\t\t.maximum_order_supported = 0x%x,\n", mem->maximum_order_supported);
		printf("\t\t\t.identifier = 0x%x,\n", mem->identifier);
		printf("\t\t},\n");
		mem = mem->next;
	}

	printf("\t\t.has_mmu = %d,\n", info->system_info->has_mmu);
	printf("\t\t.drivermode = 0x%x,\n", info->system_info->drivermode);
	printf("\t},\n");
	printf("};\n");
}

static void
dev_mali_memory_init_mem_post(void *data)
{
	_mali_uk_init_mem_s *mem = data;

	printf("IOCTL MALI_IOC_MEM_INIT OUT = {\n");
	printf("\t.mali_address_base = 0x%x,\n", mem->mali_address_base);
	printf("\t.memory_size = 0x%x,\n", mem->memory_size);
	printf("};\n");
}

static void
dev_mali_pp_core_version_post(void *data)
{
	_mali_uk_get_pp_core_version_s *version = data;

	printf("IOCTL MALI_IOC_PP_CORE_VERSION_GET OUT = {\n");
	printf("\t.version = 0x%x,\n", version->version);
	printf("};\n");
}

static void
dev_mali_wait_for_notification_pre(void *data)
{
	_mali_uk_wait_for_notification_s *notification = data;

	printf("IOCTL MALI_IOC_WAIT_FOR_NOTIFICATION IN = {\n");
	printf("\t.code.timeout = 0x%x,\n", notification->code.timeout);
	printf("};\n");
}

/*
 * At this point, we do not care about the performance counters.
 */
static void
dev_mali_wait_for_notification_post(void *data)
{
	_mali_uk_wait_for_notification_s *notification = data;

	printf("IOCTL MALI_IOC_WAIT_FOR_NOTIFICATION OUT = {\n");
	printf("\t.code.type = 0x%x,\n", notification->code.type);

	switch (notification->code.type) {
	case _MALI_NOTIFICATION_GP_FINISHED:
		{
			_mali_uk_gp_job_finished_s *info =
				&notification->data.gp_job_finished;

			printf("\t.data.gp_job_finished = {\n");

			printf("\t\t.user_job_ptr = 0x%x,\n", info->user_job_ptr);
			printf("\t\t.status = 0x%x,\n", info->status);
			printf("\t\t.irq_status = 0x%x,\n", info->irq_status);
			printf("\t\t.status_reg_on_stop = 0x%x,\n",
			       info->status_reg_on_stop);
			printf("\t\t.vscl_stop_addr = 0x%x,\n",
			       info->vscl_stop_addr);
			printf("\t\t.plbcl_stop_addr = 0x%x,\n",
			       info->plbcl_stop_addr);
			printf("\t\t.heap_current_addr = 0x%x,\n",
			       info->heap_current_addr);
			printf("\t\t.render_time = 0x%x,\n", info->render_time);

			printf("\t},\n");

			//mali_memory_dump();
		}
		break;
	case _MALI_NOTIFICATION_PP_FINISHED:
		{
			_mali_uk_pp_job_finished_s *info =
				&notification->data.pp_job_finished;

			printf("\t.data.pp_job_finished = {\n");

			printf("\t\t.user_job_ptr = 0x%x,\n", info->user_job_ptr);
			printf("\t\t.status = 0x%x,\n", info->status);
			printf("\t\t.irq_status = 0x%x,\n", info->irq_status);
			printf("\t\t.last_tile_list_addr = 0x%x,\n",
			       info->last_tile_list_addr);
			printf("\t\t.render_time = 0x%x,\n", info->render_time);

			printf("\t},\n");

			mali_memory_dump();
		}
		break;
	case _MALI_NOTIFICATION_GP_STALLED:
		{
			_mali_uk_gp_job_suspended_s *info =
				&notification->data.gp_job_suspended;

			printf("\t.data.gp_job_suspended = {\n");
			printf("\t\t.user_job_ptr = 0x%x,\n", info->user_job_ptr);
			printf("\t\t.reason = 0x%x,\n", info->reason);
			printf("\t\t.cookie = 0x%x,\n", info->cookie);
			printf("\t},\n");
		}
		break;
	default:
		break;
	}
	printf("};\n");
}

static void
dev_mali_gp_start_job_pre(void *data)
{
	_mali_uk_gp_start_job_s *job = data;
	int i;

	printf("IOCTL MALI_IOC_GP2_START_JOB IN = {\n");

	printf("\t.user_job_ptr = 0x%x,\n", job->user_job_ptr);
	printf("\t.priority = 0x%x,\n", job->priority);
	printf("\t.watchdog_msecs = 0x%x,\n", job->watchdog_msecs);

	printf("\t.frame_registers = {\n");
	for (i = 0; i < MALIGP2_NUM_REGS_FRAME; i++)
		printf("\t\t0x%08x,\n", job->frame_registers[i]);
	printf("\t},\n");

	printf("\t.abort_id = 0x%x,\n", job->watchdog_msecs);

	printf("};\n");

	mali_memory_dump();
}

static void
dev_mali_gp_start_job_post(void *data)
{
	_mali_uk_gp_start_job_s *job = data;

	printf("IOCTL MALI_IOC_GP2_START_JOB OUT = {\n");

	printf("\t.returned_user_job_ptr = 0x%x,\n",
	       job->returned_user_job_ptr);
	printf("\t.status = 0x%x,\n", job->status);

	printf("};\n");
}

static void
dev_mali_pp_start_job_pre(void *data)
{
	_mali_uk_pp_start_job_s *job = data;
	int i;

	printf("IOCTL MALI_IOC_PP_START_JOB IN = {\n");

	printf("\t.user_job_ptr = 0x%x,\n", job->user_job_ptr);
	printf("\t.priority = 0x%x,\n", job->priority);
	printf("\t.watchdog_msecs = 0x%x,\n", job->watchdog_msecs);

	printf("\t.frame_registers = {\n");
	for (i = 0; i < MALI200_NUM_REGS_FRAME; i++)
		printf("\t\t0x%08x,\n", job->frame_registers[i]);
	printf("\t},\n");

	printf("\t.wb0_registers = {\n");
	for (i = 0; i < MALI200_NUM_REGS_WBx; i++)
		printf("\t\t0x%08x,\n", job->wb0_registers[i]);
	printf("\t},\n");

	printf("\t.wb1_registers = {\n");
	for (i = 0; i < MALI200_NUM_REGS_WBx; i++)
		printf("\t\t0x%08x,\n", job->wb1_registers[i]);
	printf("\t},\n");

	printf("\t.wb2_registers = {\n");
	for (i = 0; i < MALI200_NUM_REGS_WBx; i++)
		printf("\t\t0x%08x,\n", job->wb2_registers[i]);
	printf("\t},\n");


	printf("\t.abort_id = 0x%x,\n", job->watchdog_msecs);

	printf("};\n");

	mali_memory_dump();
}

static void
dev_mali_pp_start_job_post(void *data)
{
	_mali_uk_pp_start_job_s *job = data;

	printf("IOCTL MALI_IOC_PP_START_JOB OUT = {\n");

	printf("\t.returned_user_job_ptr = 0x%x,\n",
	       job->returned_user_job_ptr);
	printf("\t.status = 0x%x,\n", job->status);

	printf("};\n");
}

static struct dev_mali_ioctl_table {
	int type;
	int nr;
	char *name;
	void (*pre)(void *data);
	void (*post)(void *data);
} dev_mali_ioctls[] = {
	{MALI_IOC_CORE_BASE, _MALI_UK_OPEN, "CORE, OPEN", NULL, NULL},
	{MALI_IOC_CORE_BASE, _MALI_UK_CLOSE, "CORE, CLOSE", NULL, NULL},
	{MALI_IOC_CORE_BASE, _MALI_UK_GET_SYSTEM_INFO_SIZE, "CORE, GET_SYSTEM_INFO_SIZE",
	 NULL, dev_mali_get_system_info_size_post},
	{MALI_IOC_CORE_BASE, _MALI_UK_GET_SYSTEM_INFO, "CORE, GET_SYSTEM_INFO",
	 dev_mali_get_system_info_pre, dev_mali_get_system_info_post},
	{MALI_IOC_CORE_BASE, _MALI_UK_WAIT_FOR_NOTIFICATION, "CORE, WAIT_FOR_NOTIFICATION",
	 dev_mali_wait_for_notification_pre, dev_mali_wait_for_notification_post},
	{MALI_IOC_CORE_BASE, _MALI_UK_GET_API_VERSION, "CORE, GET_API_VERSION",
	 dev_mali_get_api_version_pre, dev_mali_get_api_version_post},
	{MALI_IOC_MEMORY_BASE, _MALI_UK_INIT_MEM, "MEMORY, INIT_MEM",
	 NULL, dev_mali_memory_init_mem_post},
	{MALI_IOC_PP_BASE, _MALI_UK_PP_START_JOB, "PP, START_JOB",
	 dev_mali_pp_start_job_pre, dev_mali_pp_start_job_post},
	{MALI_IOC_PP_BASE, _MALI_UK_GET_PP_CORE_VERSION, "PP, GET_CORE_VERSION",
	 NULL, dev_mali_pp_core_version_post},
	{MALI_IOC_GP_BASE, _MALI_UK_GP_START_JOB, "GP, START_JOB",
	 dev_mali_gp_start_job_pre, dev_mali_gp_start_job_post},

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

	if (ioctl && !ioctl->pre && !ioctl->post) {
		if (data)
			printf("IOCTL %s(%s) %p = %d\n",
			       ioc_string, ioctl->name, data, ret);
		else
			printf("IOCTL %s(%s) = %d\n",
			       ioc_string, ioctl->name, ret);
	}

	if (ioctl && ioctl->post)
		ioctl->post(data);

	return ret;
}

/*
 *
 * Memory dumper.
 *
 */
#define MALI_ADDRESSES 0x10

static struct mali_address {
	void *address; /* mapped address */
	unsigned int size;
	unsigned int physical; /* actual address */
} mali_addresses[MALI_ADDRESSES];

static int
mali_address_add(void *address, unsigned int size, unsigned int physical)
{
	int i;

	for (i = 0; i < MALI_ADDRESSES; i++) {
		if ((mali_addresses[i].address >= address) &&
		    (mali_addresses[i].address < (address + size)) &&
		    ((mali_addresses[i].address + size) > address) &&
		    ((mali_addresses[i].address + size) <= (address + size))) {
			printf("Error: Address %p (0x%x) is already taken!\n",
			       address, size);
			return -1;
		}
	}

	for (i = 0; i < MALI_ADDRESSES; i++)
		if (!mali_addresses[i].address) {
			mali_addresses[i].address = address;
			mali_addresses[i].size = size;
			mali_addresses[i].physical = physical;
			return 0;
		}

	printf("Error: No more free memory slots for %p (0x%x)!\n",
	       address, size);
	return -1;
}

static int
mali_address_remove(void *address, int size)
{
	int i;

	for (i = 0; i < MALI_ADDRESSES; i++)
		if ((mali_addresses[i].address == address) &&
		    (mali_addresses[i].size == size)) {
			mali_addresses[i].address = NULL;
			mali_addresses[i].size = 0;
			mali_addresses[i].physical = 0;
			return 0;
		}

	return -1;
}

static void
mali_memory_dump_block(unsigned int *address, int start, int stop,
		       unsigned physical, int count)
{
	int i;

	printf("struct mali_dumped_mem_content mem_0x%08x_0x%08x = {\n",
	       physical, count);

	printf("\t0x%08x,\n", 4 * start);
	printf("\t0x%08x,\n", 4 * (stop - start));
	printf("\t{\n");

	for (i = start; i < stop; i += 4)
		printf("\t\t\t0x%08x, 0x%08x, 0x%08x, 0x%08x, /* 0x%08X */\n",
		       address[i + 0], address[i + 1],
		       address[i + 2], address[i + 3], 4 * i);

	printf("\t}\n");
	printf("};\n");
}

static void
mali_memory_dump_address(unsigned int *address, unsigned int size,
			 unsigned int physical)
{
	int i, start = -1, stop = -1, count = 0;

	for (i = 0; i < size; i += 4) {
		if (start == -1) {
			if (address[i + 0] || address[i + 1] ||
			    address[i + 2] || address[i + 3])
				start = i;
		} else if (stop == -1) {
			if (!address[i + 0] && !address[i + 1] &&
			    !address[i + 2] && !address[i + 3])
				stop = i;
		} else if (!address[i + 0] && !address[i + 1] &&
			   !address[i + 2] && !address[i + 3]) {
			if (i > (stop + 2)) {
				mali_memory_dump_block(address, start, stop,
						       physical, count);
				count++;
				start = -1;
				stop = -1;
			}
		} else
			stop = -1;
	}

	if ((start != -1) && (stop == -1)) {
		mali_memory_dump_block(address, start, size, physical, count);
		count++;
	}

	printf("struct mali_dumped_mem_block mem_0x%08x = {\n", physical);
	printf("\tNULL,\n");
	printf("\t0x%08x,\n", physical);
	printf("\t0x%08x,\n", 4 * size);
	printf("\t0x%08x,\n", count);
	printf("\t{\n");

	for (i = 0; i < count; i++)
		printf("\t\t&mem_0x%08x_0x%08x,\n", physical, i);

	printf("\t},\n");
	printf("};\n");
}

static void
mali_memory_dump(void)
{
	int i, count = 0;

	for (i = 0; i < MALI_ADDRESSES; i++)
		if (mali_addresses[i].address) {
			mali_memory_dump_address(mali_addresses[i].address,
						 mali_addresses[i].size / 4,
						 mali_addresses[i].physical);
			count++;
		}

	printf("struct mali_dumped_mem dumped_mem = {\n");
	printf("\t0x%08x,\n", count);
	printf("\t{\n");

	for (i = 0; i < MALI_ADDRESSES; i++)
		if (mali_addresses[i].address)
			printf("\t\t&mem_0x%08x,\n", mali_addresses[i].physical);

	printf("\t},\n");
	printf("};\n");
}

/*
 *
 * Wrapper for __mali_compile_essl_shader
 *
 */
static void *libmali_dl;

static int
libmali_dlopen(void)
{
	libmali_dl = dlopen("libMali.so", RTLD_LAZY);
	if (!libmali_dl) {
		printf("Failed to dlopen %s: %s\n",
		       "libMali.so", dlerror());
		exit(-1);
	}

	return 0;
}

static void *
libmali_dlsym(const char *name)
{
	void *func;

	if (!libmali_dl)
		libmali_dlopen();

	func = dlsym(libmali_dl, name);

	if (!func) {
		printf("Failed to find %s in %s: %s\n",
		       name, "libMali.so", dlerror());
		exit(-1);
	}

	return func;
}

void
hexdump(const void *data, int size)
{
	unsigned char *buf = (void *) data;
	char alpha[17];
	int i;

	for (i = 0; i < size; i++) {
		if (!(i % 16))
			printf("\t\t%08X", (unsigned int) buf + i);

		if (((void *) (buf + i)) < ((void *) data)) {
			printf("   ");
			alpha[i % 16] = '.';
		} else {
			printf(" %02X", buf[i]);

			if (isprint(buf[i]))
				alpha[i % 16] = buf[i];
			else
				alpha[i % 16] = '.';
		}

		if ((i % 16) == 15) {
			alpha[16] = 0;
			printf("\t|%s|\n", alpha);
		}
	}

	if (i % 16) {
		for (i %= 16; i < 16; i++) {
			printf("   ");
			alpha[i] = '.';

			if (i == 15) {
				alpha[16] = 0;
				printf("\t|%s|\n", alpha);
			}
		}
	}
}

int (*orig__mali_compile_essl_shader)(struct mali_shader_binary *binary, int type,
				      char *source, int *length, int count);

int
__mali_compile_essl_shader(struct mali_shader_binary *binary, int type,
			   char *source, int *length, int count)
{
	int ret;
	int i, offset = 0;

	pthread_mutex_lock(serializer);

	if (!orig__mali_compile_essl_shader)
		orig__mali_compile_essl_shader = libmali_dlsym(__func__);

	for (i = 0, offset = 0; i < count; i++) {
		printf("%s shader source %d:\n",
		       (type == 0x8B31) ? "Vertex" : "Fragment", i);
		printf("\"%s\"\n", &source[offset]);
		offset += length[i];
	}

	ret = orig__mali_compile_essl_shader(binary, type, source, length, count);

	printf("struct mali_shader_binary %p = {\n", binary);
	printf("\t.compile_status = %d,\n", binary->compile_status);
	printf("\t.error_log = \"%s\",\n", binary->error_log);
	printf("\t.shader = {\n");
	hexdump(binary->shader, binary->shader_size);
	printf("\t},\n");
	printf("\t.shader_size = 0x%x,\n", binary->shader_size);
	printf("\t.varying_stream = {\n");
	hexdump(binary->varying_stream, binary->varying_stream_size);
	printf("\t},\n");
	printf("\t.varying_stream_size = 0x%x,\n", binary->uniform_stream_size);
	printf("\t.uniform_stream = {\n");
	hexdump(binary->uniform_stream, binary->uniform_stream_size);
	printf("\t},\n");
	printf("\t.uniform_stream_size = 0x%x,\n", binary->uniform_stream_size);
	printf("\t.attribute_stream = {\n");
	hexdump(binary->attribute_stream, binary->attribute_stream_size);
	printf("\t},\n");
	printf("\t.attribute_stream_size = 0x%x,\n", binary->attribute_stream_size);

	if (type == 0x8B31) {
		printf("\t.parameters (vertex) = {\n");
		printf("\t\t.unknown00 = 0x%x,\n", binary->parameters.vertex.unknown00);
		printf("\t\t.unknown04 = 0x%x,\n", binary->parameters.vertex.unknown04);
		printf("\t\t.unknown08 = 0x%x,\n", binary->parameters.vertex.unknown08);
		printf("\t\t.unknown0C = 0x%x,\n", binary->parameters.vertex.unknown0C);
		printf("\t\t.unknown10 = 0x%x,\n", binary->parameters.vertex.unknown10);
		printf("\t\t.unknown14 = 0x%x,\n", binary->parameters.vertex.unknown14);
		printf("\t\t.unknown18 = 0x%x,\n", binary->parameters.vertex.unknown18);
		printf("\t\t.unknown1C = 0x%x,\n", binary->parameters.vertex.unknown1C);
		printf("\t\t.unknown20 = 0x%x,\n", binary->parameters.vertex.unknown20);
		printf("\t},\n");
	} else {
		printf("\t.parameters (fragment) = {\n");
		printf("\t\t.unknown00 = 0x%x,\n", binary->parameters.fragment.unknown00);
		printf("\t\t.unknown04 = 0x%x,\n", binary->parameters.fragment.unknown04);
		printf("\t\t.unknown08 = 0x%x,\n", binary->parameters.fragment.unknown08);
		printf("\t\t.unknown0C = 0x%x,\n", binary->parameters.fragment.unknown0C);
		printf("\t\t.unknown10 = 0x%x,\n", binary->parameters.fragment.unknown10);
		printf("\t\t.unknown14 = 0x%x,\n", binary->parameters.fragment.unknown14);
		printf("\t\t.unknown18 = 0x%x,\n", binary->parameters.fragment.unknown18);
		printf("\t\t.unknown1C = 0x%x,\n", binary->parameters.fragment.unknown1C);
		printf("\t\t.unknown20 = 0x%x,\n", binary->parameters.fragment.unknown20);
		printf("\t\t.unknown24 = 0x%x,\n", binary->parameters.fragment.unknown24);
		printf("\t\t.unknown28 = 0x%x,\n", binary->parameters.fragment.unknown28);
		printf("\t\t.unknown2C = 0x%x,\n", binary->parameters.fragment.unknown2C);
		printf("\t}\n");
	}
	printf("}\n");

	pthread_mutex_unlock(serializer);

	return ret;
}
