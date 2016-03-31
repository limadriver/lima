/*
 * Copyright (c) 2011-2013 Luc Verhaegen <libv@skynet.be>
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
#include <asm/ioctl.h>
#include <sys/mman.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <inttypes.h>
#include <pthread.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>

#include <linux/fb.h>

#define u32 uint32_t
#include "linux/mali_ioctl.h"

#include "version.h"
#include "compiler.h"
#include "formats.h"
#include "linux/ioctl.h"
#include "bmp.h"

static int fb_ioctl(int request, void *data);
static int mali_ioctl(int request, void *data);
static int mali_address_add(void *address, unsigned int size,
			    unsigned int physical);
static int mali_address_remove(void *address, int size);
static int ump_id_add(unsigned int id, unsigned int size, void *address);
static int ump_physical_add(unsigned int id, unsigned int physical);
static int mali_external_add(unsigned int address, unsigned int physical,
			     unsigned int size, unsigned int cookie);
static int mali_external_remove(unsigned int cookie);
static void mali_memory_dump(void);
static void mali_wrap_bmp_dump(void);

static pthread_mutex_t serializer[1] = { PTHREAD_MUTEX_INITIALIZER };

unsigned int render_address;
int render_width;
int render_height;
int render_format;

static int mali_type = 400;
static int mali_version;

static int fb_ump_id;
static int fb_ump_size;
static void *fb_ump_address;

#if 0
/*
 * Broken with the wait_for_notification workaround.
 *
 */
static const char *first_func;
static int recursive;

static inline void
serialized_start(const char *func)
{
	if (recursive)
		printf("%s: trying to recursively call wrapper symbols (%s)!\n",
		       func, first_func);
	else {
		first_func = func;
		pthread_mutex_lock(serializer);
	}
	recursive++;
}

static inline void
serialized_stop(void)
{
	recursive--;
	if (!recursive) {
		first_func = NULL;
		pthread_mutex_unlock(serializer);
	}
}
#else
static inline void
serialized_start(const char *func)
{
	pthread_mutex_lock(serializer);
}

static inline void
serialized_stop(void)
{
	pthread_mutex_unlock(serializer);
}
#endif

/*
 *
 * Basic log writing infrastructure.
 *
 */
FILE *lima_wrap_log;
int frame_count;

void
lima_wrap_log_open(void)
{
	char *filename;
	char buffer[1024];

	if (lima_wrap_log)
		return;

	filename = getenv("LIMA_WRAP_LOG");
	if (!filename)
		filename = "/home/libv/lima/dump/lima.wrap.log";

	snprintf(buffer, sizeof(buffer), "%s.%04d", filename, frame_count);

	lima_wrap_log = fopen(buffer, "w");
	if (!lima_wrap_log) {
		printf("Error: failed to open wrap log %s: %s\n", filename,
		       strerror(errno));
		lima_wrap_log = stdout;
	}
}

int
wrap_log(const char *format, ...)
{
	va_list args;
	int ret;

	lima_wrap_log_open();

	va_start(args, format);
	ret = vfprintf(lima_wrap_log, format, args);
	va_end(args);

	return ret;
}

void
includes_print(void)
{
	wrap_log("#include <stdlib.h>\n");
	wrap_log("#include <unistd.h>\n");
	wrap_log("#include <stdio.h>\n");

	wrap_log("#include \"linux/ioctl.h\"\n");
	wrap_log("#include \"dump.h\"\n");
	wrap_log("#include \"limare.h\"\n");
	wrap_log("#include \"compiler.h\"\n");
	wrap_log("#include \"jobs.h\"\n");
	wrap_log("#include \"bmp.h\"\n");
	wrap_log("#include \"fb.h\"\n");
}

void
lima_wrap_log_next(void)
{
	if (lima_wrap_log) {
		fclose(lima_wrap_log);
		lima_wrap_log = NULL;
	}

	frame_count++;

	lima_wrap_log_open();

	includes_print();

	if (mali_type == 400)
		wrap_log("#define LIMA_M400 1\n\n");
}

/*
 * Wrap around the libc calls that are crucial for capturing our
 * command stream, namely, open, ioctl, and mmap.
 */
static void *libc_dl;

static int
libc_dlopen(void)
{
	libc_dl = dlopen("libc.so.6", RTLD_LAZY);
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
static int dev_ump_fd;
static int dev_fb_fd;

/*
 *
 */
static int (*orig_open)(const char* path, int mode, ...);

int
open(const char* path, int flags, ...)
{
	mode_t mode = 0;
	int ret;
	int mali = 0;
	int ump = 0;
	int fb = 0;

	if (!strcmp(path, "/dev/mali")) {
		mali = 1;
		serialized_start(__func__);
	} else if (!strcmp(path, "/dev/ump")) {
		ump = 1;
	    	serialized_start(__func__);
	} else if (!strncmp(path, "/dev/fb", 7)) {
		fb = 1;
	}

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

		if (ret != -1) {
			if (mali)
				dev_mali_fd = ret;
			else if (ump)
				dev_ump_fd = ret;
			else if (fb)
				dev_fb_fd = ret;
		}
	}

	if (mali || ump)
		serialized_stop();

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

	if (fd == dev_mali_fd)
	    	serialized_start(__func__);

	if (!orig_close)
		orig_close = libc_dlsym(__func__);

	if (fd == dev_mali_fd) {
		wrap_log("/* CLOSE */");
		dev_mali_fd = -1;
	}

	ret = orig_close(fd);

	if (fd == dev_mali_fd)
		serialized_stop();

	return ret;
}

/*
 * Bionic, go figure...
 */
#ifdef ANDROID
static int (*orig_ioctl)(int fd, int request, ...);
#else
static int (*orig_ioctl)(int fd, unsigned long request, ...);
#endif

int
#ifdef ANDROID
ioctl(int fd, int request, ...)
#else
ioctl(int fd, unsigned long request, ...)
#endif
{
	int ioc_size = _IOC_SIZE(request);
	int ret;
	int yield = 0;

	serialized_start(__func__);

	if (!orig_ioctl)
		orig_ioctl = libc_dlsym(__func__);

	/* hack around badly defined fbdev ioctls */
	if (ioc_size || ((request & 0xFFC8) == 0x4600)) {
		va_list args;
		void *ptr;

		va_start(args, request);
		ptr = va_arg(args, void *);
		va_end(args);

		if (fd == dev_mali_fd) {
			if ((request == MALI_IOC_WAIT_FOR_NOTIFICATION) ||
			    (request == MALI_IOC_WAIT_FOR_NOTIFICATION_R3P1))
				yield = 1;

			ret = mali_ioctl(request, ptr);
		} else if (fd == dev_fb_fd)
			ret = fb_ioctl(request, ptr);
		else
			ret = orig_ioctl(fd, request, ptr);
	} else {
		if (fd == dev_mali_fd)
			ret = mali_ioctl(request, NULL);
		else if (fd == dev_fb_fd)
			ret = fb_ioctl(request, NULL);
		else
			ret = orig_ioctl(fd, request);
	}

	serialized_stop();

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

	serialized_start(__func__);

	if (!orig_mmap)
		orig_mmap = libc_dlsym(__func__);

	ret = orig_mmap(addr, length, prot, flags, fd, offset);

	if (fd == dev_mali_fd) {
		wrap_log("/* MMAP 0x%08lx (0x%08x) = %p */\n\n", offset, length, ret);
		mali_address_add(ret, length, offset);
		memset(ret, 0, length);
	} else if (fd == dev_ump_fd)
		ump_id_add(offset >> 12, length, ret);
	else if (fd == dev_fb_fd)
		fb_ump_address = ret;

	serialized_stop();

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

	serialized_start(__func__);

	if (!orig_munmap)
		orig_munmap = libc_dlsym(__func__);

	ret = orig_munmap(addr, length);

	if (!mali_address_remove(addr, length))
		wrap_log("/* MUNMAP %p (0x%08x) */\n\n", addr, length);

	serialized_stop();

	return ret;
}

int (*orig_fflush)(FILE *stream);

int
fflush(FILE *stream)
{
	int ret;

	serialized_start(__func__);

	if (!orig_fflush)
		orig_fflush = libc_dlsym(__func__);

	ret = orig_fflush(stream);

	if (stream != lima_wrap_log)
		orig_fflush(lima_wrap_log);

	serialized_stop();

	return ret;
}

/*
 * Parse FB ioctls.
 */
static int
fb_ioctl(int request, void *data)
{
	int ret;

	if (data)
		ret = orig_ioctl(dev_fb_fd, request, data);
	else
		ret = orig_ioctl(dev_fb_fd, request);

#define GET_UMP_SECURE_ID_BUF1   _IOWR('m', 311, unsigned int)

	if (request == FBIOGET_FSCREENINFO) {
		struct fb_fix_screeninfo *fix = data;

		fb_ump_size = fix->smem_len;
	} else if (request == GET_UMP_SECURE_ID_BUF1) {
		unsigned int *id = data;

		fb_ump_id = *id;

		ump_id_add(fb_ump_id, fb_ump_size, fb_ump_address);
	}

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

	wrap_log("/* IOCTL MALI_IOC_GET_API_VERSION IN */\n");

	wrap_log("#if 0 /* API Version */\n\n");

	wrap_log("_mali_uk_get_api_version_s mali_version_in = {\n");
	wrap_log("\t.version = 0x%08x,\n", version->version);
	wrap_log("};\n\n");

	wrap_log("#endif /* API Version */\n\n");
}

static void
dev_mali_get_api_version_post(void *data, int ret)
{
	_mali_uk_get_api_version_s *version = data;

	wrap_log("/* IOCTL MALI_IOC_GET_API_VERSION OUT */\n");

	wrap_log("#if 0 /* API Version */\n\n");

	wrap_log("_mali_uk_get_api_version_s mali_version_out = {\n");
	wrap_log("\t.version = 0x%08x,\n", version->version);
	wrap_log("\t.compatible = %d,\n", version->compatible);
	wrap_log("};\n\n");

	wrap_log("#endif /* API Version */\n\n");

	mali_version = version->version & 0xFFFF;

	printf("Mali version: %d\n", mali_version);
}

static void
dev_mali_get_system_info_size_post(void *data, int ret)
{
	_mali_uk_get_system_info_size_s *size = data;

	wrap_log("/* IOCTL MALI_IOC_GET_SYSTEM_INFO_SIZE OUT */\n");

	wrap_log("#if 0 /* System Info */\n\n");

	wrap_log("_mali_uk_get_system_info_size_s mali_system_info_size_out = {\n");
	wrap_log("\t.size = 0x%x,\n", size->size);
	wrap_log("};\n\n");

	wrap_log("#endif /* System Info */\n\n");
}

static void
dev_mali_get_system_info_pre(void *data)
{
	_mali_uk_get_system_info_s *info = data;

	wrap_log("/* IOCTL MALI_IOC_GET_SYSTEM_INFO IN */\n");

	wrap_log("#if 0 /* System Info */\n\n");

	wrap_log("_mali_uk_get_system_info_s mali_system_info_in = {\n");
	wrap_log("\t.size = 0x%x,\n", info->size);
	wrap_log("\t.system_info = <malloced, above size>,\n");
	wrap_log("\t.ukk_private = 0x%x,\n", info->ukk_private);
	wrap_log("};\n\n");

	wrap_log("#endif /* System Info */\n\n");
}

static void
dev_mali_get_system_info_post(void *data, int ret)
{
	_mali_uk_get_system_info_s *info = data;
	struct _mali_core_info *core;
	struct _mali_mem_info *mem;

	wrap_log("/* IOCTL MALI_IOC_GET_SYSTEM_INFO OUT */\n");

	wrap_log("#if 0 /* System Info */\n\n");

	wrap_log("_mali_uk_get_system_info_s mali_system_info_out = {\n");
	wrap_log("\t.system_info = {\n");

	core = info->system_info->core_info;
	while (core) {
		wrap_log("\t\t\t.core_info = {\n");
		wrap_log("\t\t\t.type = 0x%x,\n", core->type);
		if (core->type == 7)
			mali_type = 400;
		else if (core->type == 5)
			mali_type = 200;
		wrap_log("\t\t.version = 0x%x,\n", core->version);
		wrap_log("\t\t\t.reg_address = 0x%x,\n", core->reg_address);
		wrap_log("\t\t\t.core_nr = 0x%x,\n", core->core_nr);
		wrap_log("\t\t\t.flags = 0x%x,\n", core->flags);
		wrap_log("\t\t},\n");
		core = core->next;
	}

	mem = info->system_info->mem_info;
	while (mem) {
		wrap_log("\t\t.mem_info = {\n");
		wrap_log("\t\t\t.size = 0x%x,\n", mem->size);
		wrap_log("\t\t\t.flags = 0x%x,\n", mem->flags);
		wrap_log("\t\t\t.maximum_order_supported = 0x%x,\n", mem->maximum_order_supported);
		wrap_log("\t\t\t.identifier = 0x%x,\n", mem->identifier);
		wrap_log("\t\t},\n");
		mem = mem->next;
	}

	wrap_log("\t\t.has_mmu = %d,\n", info->system_info->has_mmu);
	wrap_log("\t\t.drivermode = 0x%x,\n", info->system_info->drivermode);
	wrap_log("\t},\n");
	wrap_log("};\n\n");

	wrap_log("#endif /* System Info */\n\n");

	if (mali_type == 400)
		wrap_log("#define LIMA_M400 1\n\n");
}

static void
dev_mali_memory_init_mem_post(void *data, int ret)
{
	_mali_uk_init_mem_s *mem = data;

	wrap_log("/* IOCTL MALI_IOC_MEM_INIT OUT */\n");

	wrap_log("#if 0 /* Memory Init */\n\n");

	wrap_log("_mali_uk_init_mem_s mali_mem_init = {\n");
	wrap_log("\t.mali_address_base = 0x%x,\n", mem->mali_address_base);
	wrap_log("\t.memory_size = 0x%x,\n", mem->memory_size);
	wrap_log("};\n");

	wrap_log("#endif /* Memory Init */\n\n");
}

static void
dev_mali_memory_attach_ump_mem_post(void *data, int ret)
{
	_mali_uk_attach_ump_mem_s *ump = data;

	ump_physical_add(ump->secure_id, ump->mali_address);
}

static void
dev_mali_memory_map_ext_mem_post(void *data, int ret)
{
	_mali_uk_map_external_mem_s *ext = data;

	printf("map_ext_mem: ctx %p, phys_addr 0x%08X, size 0x%08X, address 0x%08X, rights 0x%08X, flags 0x%08X, cookie 0x%08X\n",
	       ext->ctx, ext->phys_addr, ext->size, ext->mali_address, ext->rights, ext->flags, ext->cookie);

	if (!ret)
		mali_external_add(ext->mali_address, ext->phys_addr,
				  ext->size, ext->cookie);
}

static void
dev_mali_memory_unmap_ext_mem_post(void *data, int ret)
{
	_mali_uk_map_external_mem_s *ext = data;

	printf("unmap_ext_mem: ctx %p, cookie 0x%08X\n",
	       ext->ctx, ext->cookie);

	mali_external_remove(ext->cookie);
}

static void
dev_mali_pp_core_version_post(void *data, int ret)
{
	_mali_uk_get_pp_core_version_s *version = data;

	wrap_log("/* IOCTL MALI_IOC_PP_CORE_VERSION_GET OUT */\n");

	wrap_log("#if 0 /* PP Core Version */\n\n");

	wrap_log("_mali_uk_get_pp_core_version_s mali_pp_version = {\n");
	wrap_log("\t.version = 0x%x,\n", version->version);
	wrap_log("};\n\n");

	wrap_log("#endif /* PP Core Version */\n\n");

	if (mali_type == 400)
		wrap_log("#define LIMA_M400 1\n\n");
}

static void
dev_mali_wait_for_notification_pre(void *data)
{
	_mali_uk_wait_for_notification_s *notification = data;

	wrap_log("/* IOCTL MALI_IOC_WAIT_FOR_NOTIFICATION IN */\n");

	wrap_log("#if 0 /* Notification */\n\n");

	wrap_log("_mali_uk_wait_for_notification_s mali_notification_in = {\n");
	wrap_log("\t.code.timeout = 0x%x,\n", notification->code.timeout);
	wrap_log("};\n\n");

	wrap_log("#endif /* Notification */\n\n");

	/* some kernels wait forever otherwise */
	serialized_stop();
}

/*
 * At this point, we do not care about the performance counters.
 */
static void
dev_mali_wait_for_notification_post(void *data, int ret)
{
	_mali_uk_wait_for_notification_s *notification = data;

	/* to match the pre function */
	serialized_start(__func__);

	wrap_log("/* IOCTL MALI_IOC_WAIT_FOR_NOTIFICATION OUT */\n");

	wrap_log("#if 0 /* Notification */\n\n");

	wrap_log("_mali_uk_wait_for_notification_s wait_for_notification = {\n");
	wrap_log("\t.code.type = 0x%x,\n", notification->code.type);

	switch (notification->code.type) {
	case _MALI_NOTIFICATION_GP_FINISHED:
		{
			_mali_uk_gp_job_finished_s *info =
				&notification->data.gp_job_finished;

			wrap_log("\t.data.gp_job_finished = {\n");

			wrap_log("\t\t.user_job_ptr = 0x%x,\n", info->user_job_ptr);
			wrap_log("\t\t.status = 0x%x,\n", info->status);
			wrap_log("\t\t.irq_status = 0x%x,\n", info->irq_status);
			wrap_log("\t\t.status_reg_on_stop = 0x%x,\n",
				 info->status_reg_on_stop);
			wrap_log("\t\t.vscl_stop_addr = 0x%x,\n",
				 info->vscl_stop_addr);
			wrap_log("\t\t.plbcl_stop_addr = 0x%x,\n",
				 info->plbcl_stop_addr);
			wrap_log("\t\t.heap_current_addr = 0x%x,\n",
				 info->heap_current_addr);
			wrap_log("\t\t.render_time = 0x%x,\n", info->render_time);

			wrap_log("\t},\n");

			//mali_memory_dump();
		}
		break;
	case _MALI_NOTIFICATION_PP_FINISHED:
		{
			_mali_uk_pp_job_finished_s *info =
				&notification->data.pp_job_finished;

			wrap_log("\t.data.pp_job_finished = {\n");

			wrap_log("\t\t.user_job_ptr = 0x%x,\n", info->user_job_ptr);
			wrap_log("\t\t.status = 0x%x,\n", info->status);
			wrap_log("\t\t.irq_status = 0x%x,\n", info->irq_status);
			wrap_log("\t\t.last_tile_list_addr = 0x%x,\n",
				 info->last_tile_list_addr);
			wrap_log("\t\t.render_time = 0x%x,\n", info->render_time);

			wrap_log("\t},\n");

			//mali_memory_dump();
		}
		break;
	case _MALI_NOTIFICATION_GP_STALLED:
		{
			_mali_uk_gp_job_suspended_s *info =
				&notification->data.gp_job_suspended;

			wrap_log("\t.data.gp_job_suspended = {\n");
			wrap_log("\t\t.user_job_ptr = 0x%x,\n", info->user_job_ptr);
			wrap_log("\t\t.reason = 0x%x,\n", info->reason);
			wrap_log("\t\t.cookie = 0x%x,\n", info->cookie);
			wrap_log("\t},\n");
		}
		break;
	default:
		wrap_log("};\n\n");
		break;
	}

	wrap_log("};\n\n");
	wrap_log("#endif /* Notification */\n\n");

	/* some post-processing */
	if (notification->code.type == _MALI_NOTIFICATION_PP_FINISHED) {
		printf("Finished frame %d\n", frame_count);

		wrap_log("int dump_render_width = %d;\n", render_width);
		wrap_log("int dump_render_height = %d;\n\n", render_height);

		/* We finished a frame, we dump the result */
		mali_wrap_bmp_dump();

		lima_wrap_log_next();
	}
}

static void
dev_mali_gp_job_start_r2p1_pre(void *data)
{
	struct lima_gp_job_start_r2p1 *job = data;

	wrap_log("/* IOCTL MALI_IOC_GP2_START_JOB IN */\n");

	wrap_log("struct lima_gp_job_start_r2p1 gp_job = {\n");

	wrap_log("\t.user_job_ptr = 0x%x,\n", job->user_job_ptr);
	wrap_log("\t.priority = 0x%x,\n", job->priority);
	wrap_log("\t.watchdog_msecs = 0x%x,\n", job->watchdog_msecs);

	wrap_log("\t.frame.vs_commands_start = 0x%x,\n", job->frame.vs_commands_start);
	wrap_log("\t.frame.vs_commands_end = 0x%x,\n", job->frame.vs_commands_end);
	wrap_log("\t.frame.plbu_commands_start = 0x%x,\n", job->frame.plbu_commands_start);
	wrap_log("\t.frame.plbu_commands_end = 0x%x,\n", job->frame.plbu_commands_end);
	wrap_log("\t.frame.tile_heap_start = 0x%x,\n", job->frame.tile_heap_start);
	wrap_log("\t.frame.tile_heap_end = 0x%x,\n", job->frame.tile_heap_end);

	wrap_log("\t.abort_id = 0x%x,\n", job->abort_id);

	wrap_log("};\n");
}

static void
dev_mali_gp_job_start_r3p0_pre(void *data)
{
	struct lima_gp_job_start_r3p0 *job = data;

	wrap_log("/* IOCTL MALI_IOC_GP2_START_JOB IN; */\n");

	wrap_log("struct lima_gp_job_start_r3p0 gp_job = {\n");

	wrap_log("\t.user_job_ptr = 0x%x,\n", job->user_job_ptr);
	wrap_log("\t.priority = 0x%x,\n", job->priority);

	wrap_log("\t.frame.vs_commands_start = 0x%x,\n", job->frame.vs_commands_start);
	wrap_log("\t.frame.vs_commands_end = 0x%x,\n", job->frame.vs_commands_end);
	wrap_log("\t.frame.plbu_commands_start = 0x%x,\n", job->frame.plbu_commands_start);
	wrap_log("\t.frame.plbu_commands_end = 0x%x,\n", job->frame.plbu_commands_end);
	wrap_log("\t.frame.tile_heap_start = 0x%x,\n", job->frame.tile_heap_start);
	wrap_log("\t.frame.tile_heap_end = 0x%x,\n", job->frame.tile_heap_end);

	wrap_log("};\n\n");
}

static void
dev_mali_gp_job_start_pre(void *data)
{
	if (mali_version < MALI_DRIVER_VERSION_R3P0)
		dev_mali_gp_job_start_r2p1_pre(data);
	else
		dev_mali_gp_job_start_r3p0_pre(data);

	mali_memory_dump();
}


static void
dev_mali_gp_job_start_r2p1_post(void *data, int ret)
{
	struct lima_gp_job_start_r2p1 *job = data;

	wrap_log("/* IOCTL MALI_IOC_GP2_START_JOB OUT */\n");


	wrap_log("struct lima_gp_job_start gp_job_out = {\n");
	wrap_log("\t.returned_user_job_ptr = 0x%x,\n",
		 job->returned_user_job_ptr);
	wrap_log("\t.status = 0x%x,\n", job->status);

	wrap_log("};\n\n");
}

static void
dev_mali_gp_job_start_post(void *data, int ret)
{
	if (mali_version < MALI_DRIVER_VERSION_R3P0)
		dev_mali_gp_job_start_r2p1_post(data, ret);
}

static void
dev_mali200_pp_job_start_pre(void *data)
{
	struct lima_m200_pp_job_start *job = data;
	int i;

	wrap_log("/* IOCTL MALI_IOC_PP_START_JOB IN */\n");

	wrap_log("struct lima_m200_pp_job_start pp_job = {\n");

	wrap_log("\t.user_job_ptr = 0x%x,\n", job->user_job_ptr);
	wrap_log("\t.priority = 0x%x,\n", job->priority);
	wrap_log("\t.watchdog_msecs = 0x%x,\n", job->watchdog_msecs);

	wrap_log("\t.frame.plbu_array_address = 0x%x,\n", job->frame.plbu_array_address);
	wrap_log("\t.frame.render_address = 0x%x,\n", job->frame.render_address);
	wrap_log("\t.frame.flags = 0x%x,\n", job->frame.flags);
	wrap_log("\t.frame.clear_value_depth = 0x%x,\n", job->frame.clear_value_depth);
	wrap_log("\t.frame.clear_value_stencil = 0x%x,\n", job->frame.clear_value_stencil);
	wrap_log("\t.frame.clear_value_color = 0x%x,\n", job->frame.clear_value_color);
	wrap_log("\t.frame.clear_value_color_1 = 0x%x,\n", job->frame.clear_value_color_1);
	wrap_log("\t.frame.clear_value_color_2 = 0x%x,\n", job->frame.clear_value_color_2);
	wrap_log("\t.frame.clear_value_color_3 = 0x%x,\n", job->frame.clear_value_color_3);
	wrap_log("\t.frame.width = 0x%x,\n", job->frame.width);
	wrap_log("\t.frame.height = 0x%x,\n", job->frame.height);
	wrap_log("\t.frame.fragment_stack_address = 0x%x,\n", job->frame.fragment_stack_address);
	wrap_log("\t.frame.fragment_stack_size = 0x%x,\n", job->frame.fragment_stack_size);
	wrap_log("\t.frame.one = 0x%x,\n", job->frame.one);
	wrap_log("\t.frame.supersampled_height = 0x%x,\n", job->frame.supersampled_height);
	wrap_log("\t.frame.dubya = 0x%x,\n", job->frame.dubya);
	wrap_log("\t.frame.onscreen = 0x%x,\n", job->frame.onscreen);

	for (i = 0; i < 3; i++) {
		wrap_log("\t.wb[%d].type = 0x%x,\n", i, job->wb[i].type);
		wrap_log("\t.wb[%d].address = 0x%x,\n", i, job->wb[i].address);
		wrap_log("\t.wb[%d].pixel_format = 0x%x,\n", i, job->wb[i].pixel_format);
		wrap_log("\t.wb[%d].downsample_factor = 0x%x,\n", i, job->wb[i].downsample_factor);
		wrap_log("\t.wb[%d].pixel_layout = 0x%x,\n", i, job->wb[i].pixel_layout);
		wrap_log("\t.wb[%d].pitch = 0x%x,\n", i, job->wb[i].pitch);
		wrap_log("\t.wb[%d].mrt_bits = 0x%x,\n", i, job->wb[i].mrt_bits);
		wrap_log("\t.wb[%d].mrt_pitch = 0x%x,\n", i, job->wb[i].mrt_pitch);
		wrap_log("\t.wb[%d].zero = 0x%x,\n", i, job->wb[i].zero);
	}

	wrap_log("\t.abort_id = 0x%x,\n", job->abort_id);

	wrap_log("};\n\n");

	/*
	 * Now store where our final render is headed, and what it looks like.
	 * We will dump it as a bmp once we're done.
	 */
	render_address = job->wb[0].address;

	render_format = job->wb[0].pixel_format;
	switch (job->wb[0].pixel_format) {
	case LIMA_PIXEL_FORMAT_RGB_565:
		render_width = job->wb[0].pitch * 4;
		break;
	case LIMA_PIXEL_FORMAT_RGBA_8888:
		render_width = job->wb[0].pitch * 2;
		break;
	default:
		wrap_log("Error: unhandled wb format!\n");
	}

	if (job->frame.height)
		render_height = job->frame.height + 1;
	else
		render_height = job->frame.supersampled_height + 1;

	//mali_memory_dump();
}

static void
dev_mali400_pp_job_start_r2p1_pre(void *data)
{
	struct lima_m400_pp_job_start_r2p1 *job = data;
	int i;

	wrap_log("/* IOCTL MALI_IOC_PP_START_JOB IN */\n");

	wrap_log("struct lima_m400_pp_job_start_r2p1 pp_job = {\n");

	wrap_log("\t.user_job_ptr = 0x%x,\n", job->user_job_ptr);
	wrap_log("\t.priority = 0x%x,\n", job->priority);
	wrap_log("\t.watchdog_msecs = 0x%x,\n", job->watchdog_msecs);

	wrap_log("\t.frame.plbu_array_address = 0x%x,\n", job->frame.plbu_array_address);
	wrap_log("\t.frame.render_address = 0x%x,\n", job->frame.render_address);
	wrap_log("\t.frame.flags = 0x%x,\n", job->frame.flags);
	wrap_log("\t.frame.clear_value_depth = 0x%x,\n", job->frame.clear_value_depth);
	wrap_log("\t.frame.clear_value_stencil = 0x%x,\n", job->frame.clear_value_stencil);
	wrap_log("\t.frame.clear_value_color = 0x%x,\n", job->frame.clear_value_color);
	wrap_log("\t.frame.clear_value_color_1 = 0x%x,\n", job->frame.clear_value_color_1);
	wrap_log("\t.frame.clear_value_color_2 = 0x%x,\n", job->frame.clear_value_color_2);
	wrap_log("\t.frame.clear_value_color_3 = 0x%x,\n", job->frame.clear_value_color_3);
	wrap_log("\t.frame.width = 0x%x,\n", job->frame.width);
	wrap_log("\t.frame.height = 0x%x,\n", job->frame.height);
	wrap_log("\t.frame.fragment_stack_address = 0x%x,\n", job->frame.fragment_stack_address);
	wrap_log("\t.frame.fragment_stack_size = 0x%x,\n", job->frame.fragment_stack_size);
	wrap_log("\t.frame.one = 0x%x,\n", job->frame.one);
	wrap_log("\t.frame.supersampled_height = 0x%x,\n", job->frame.supersampled_height);
	wrap_log("\t.frame.dubya = 0x%x,\n", job->frame.dubya);
	wrap_log("\t.frame.onscreen = 0x%x,\n", job->frame.onscreen);
	wrap_log("\t.frame.blocking = 0x%x,\n", job->frame.blocking);
	wrap_log("\t.frame.scale = 0x%x,\n", job->frame.scale);
	wrap_log("\t.frame.foureight = 0x%x,\n", job->frame.foureight);

	for (i = 0; i < 3; i++) {
		wrap_log("\t.wb[%d].type = 0x%x,\n", i, job->wb[i].type);
		wrap_log("\t.wb[%d].address = 0x%x,\n", i, job->wb[i].address);
		wrap_log("\t.wb[%d].pixel_format = 0x%x,\n", i, job->wb[i].pixel_format);
		wrap_log("\t.wb[%d].downsample_factor = 0x%x,\n", i, job->wb[i].downsample_factor);
		wrap_log("\t.wb[%d].pixel_layout = 0x%x,\n", i, job->wb[i].pixel_layout);
		wrap_log("\t.wb[%d].pitch = 0x%x,\n", i, job->wb[i].pitch);
		wrap_log("\t.wb[%d].mrt_bits = 0x%x,\n", i, job->wb[i].mrt_bits);
		wrap_log("\t.wb[%d].mrt_pitch = 0x%x,\n", i, job->wb[i].mrt_pitch);
		wrap_log("\t.wb[%d].zero = 0x%x,\n", i, job->wb[i].zero);
	}

	wrap_log("\t.abort_id = 0x%x,\n", job->abort_id);

	wrap_log("};\n\n");

	/*
	 * Now store where our final render is headed, and what it looks like.
	 * We will dump it as a bmp once we're done.
	 */
	render_address = job->wb[0].address;

	render_format = job->wb[0].pixel_format;
	switch (job->wb[0].pixel_format) {
	case LIMA_PIXEL_FORMAT_RGB_565:
		render_width = job->wb[0].pitch * 4;
		break;
	case LIMA_PIXEL_FORMAT_RGBA_8888:
		render_width = job->wb[0].pitch * 2;
		break;
	default:
		wrap_log("Error: unhandled wb format!\n");
	}

	if (job->frame.height)
		render_height = job->frame.height + 1;
	else
		render_height = job->frame.supersampled_height + 1;

	//mali_memory_dump();
}

static void
dev_mali400_pp_job_start_r3p0_pre(void *data)
{
	struct lima_m400_pp_job_start_r3p0 *job = data;
	int i;

	wrap_log("/* IOCTL MALI_IOC_PP_START_JOB IN; */\n");

	wrap_log("struct lima_m400_pp_job_start_r3p0 pp_job = {\n");

	wrap_log("\t.user_job_ptr = 0x%x,\n", job->user_job_ptr);
	wrap_log("\t.priority = 0x%x,\n", job->priority);

	wrap_log("\t.frame.plbu_array_address = 0x%x,\n", job->frame.plbu_array_address);
	wrap_log("\t.frame.render_address = 0x%x,\n", job->frame.render_address);
	wrap_log("\t.frame.flags = 0x%x,\n", job->frame.flags);
	wrap_log("\t.frame.clear_value_depth = 0x%x,\n", job->frame.clear_value_depth);
	wrap_log("\t.frame.clear_value_stencil = 0x%x,\n", job->frame.clear_value_stencil);
	wrap_log("\t.frame.clear_value_color = 0x%x,\n", job->frame.clear_value_color);
	wrap_log("\t.frame.clear_value_color_1 = 0x%x,\n", job->frame.clear_value_color_1);
	wrap_log("\t.frame.clear_value_color_2 = 0x%x,\n", job->frame.clear_value_color_2);
	wrap_log("\t.frame.clear_value_color_3 = 0x%x,\n", job->frame.clear_value_color_3);
	wrap_log("\t.frame.width = 0x%x,\n", job->frame.width);
	wrap_log("\t.frame.height = 0x%x,\n", job->frame.height);
	wrap_log("\t.frame.fragment_stack_address = 0x%x,\n", job->frame.fragment_stack_address);
	wrap_log("\t.frame.fragment_stack_size = 0x%x,\n", job->frame.fragment_stack_size);
	wrap_log("\t.frame.one = 0x%x,\n", job->frame.one);
	wrap_log("\t.frame.supersampled_height = 0x%x,\n", job->frame.supersampled_height);
	wrap_log("\t.frame.dubya = 0x%x,\n", job->frame.dubya);
	wrap_log("\t.frame.onscreen = 0x%x,\n", job->frame.onscreen);
	wrap_log("\t.frame.blocking = 0x%x,\n", job->frame.blocking);
	wrap_log("\t.frame.scale = 0x%x,\n", job->frame.scale);
	wrap_log("\t.frame.foureight = 0x%x,\n", job->frame.foureight);

	for (i = 0; i < 7; i++)
		wrap_log("\t.addr_frame[%d] = 0x%x,\n", i, job->addr_frame[i]);
	for (i = 0; i < 7; i++)
		wrap_log("\t.addr_stack[%d] = 0x%x,\n", i, job->addr_stack[i]);

	wrap_log("\t.wb0.type = 0x%x,\n", job->wb0.type);
	wrap_log("\t.wb0.address = 0x%x,\n", job->wb0.address);
	wrap_log("\t.wb0.pixel_format = 0x%x,\n", job->wb0.pixel_format);
	wrap_log("\t.wb0.downsample_factor = 0x%x,\n", job->wb0.downsample_factor);
	wrap_log("\t.wb0.pixel_layout = 0x%x,\n", job->wb0.pixel_layout);
	wrap_log("\t.wb0.pitch = 0x%x,\n", job->wb0.pitch);
	wrap_log("\t.wb0.mrt_bits = 0x%x,\n", job->wb0.mrt_bits);
	wrap_log("\t.wb0.mrt_pitch = 0x%x,\n", job->wb0.mrt_pitch);
	wrap_log("\t.wb0.zero = 0x%x,\n", job->wb0.zero);

	wrap_log("\t.wb1.address = 0x%x,\n", job->wb1.address);
	wrap_log("\t.wb1.pixel_format = 0x%x,\n", job->wb1.pixel_format);
	wrap_log("\t.wb1.downsample_factor = 0x%x,\n", job->wb1.downsample_factor);
	wrap_log("\t.wb1.pixel_layout = 0x%x,\n", job->wb1.pixel_layout);
	wrap_log("\t.wb1.pitch = 0x%x,\n", job->wb1.pitch);
	wrap_log("\t.wb1.mrt_bits = 0x%x,\n", job->wb1.mrt_bits);
	wrap_log("\t.wb1.mrt_pitch = 0x%x,\n", job->wb1.mrt_pitch);
	wrap_log("\t.wb1.zero = 0x%x,\n", job->wb1.zero);

	wrap_log("\t.wb2.address = 0x%x,\n", job->wb2.address);
	wrap_log("\t.wb2.pixel_format = 0x%x,\n", job->wb2.pixel_format);
	wrap_log("\t.wb2.downsample_factor = 0x%x,\n", job->wb2.downsample_factor);
	wrap_log("\t.wb2.pixel_layout = 0x%x,\n", job->wb2.pixel_layout);
	wrap_log("\t.wb2.pitch = 0x%x,\n", job->wb2.pitch);
	wrap_log("\t.wb2.mrt_bits = 0x%x,\n", job->wb2.mrt_bits);
	wrap_log("\t.wb2.mrt_pitch = 0x%x,\n", job->wb2.mrt_pitch);
	wrap_log("\t.wb2.zero = 0x%x,\n", job->wb2.zero);

	wrap_log("\t.num_cores = 0x%x,\n", job->num_cores);

	wrap_log("\t.frame_builder_id = 0x%x,\n", job->frame_builder_id);
	wrap_log("\t.flush_id = 0x%x,\n", job->flush_id);

	wrap_log("};\n");

	/*
	 * Now store where our final render is headed, and what it looks like.
	 * We will dump it as a bmp once we're done.
	 */
	render_address = job->wb0.address;

	render_format = job->wb0.pixel_format;
	switch (job->wb0.pixel_format) {
	case LIMA_PIXEL_FORMAT_RGB_565:
		render_width = job->wb0.pitch * 4;
		break;
	case LIMA_PIXEL_FORMAT_RGBA_8888:
		render_width = job->wb0.pitch * 2;
		break;
	default:
		wrap_log("Error: unhandled wb format!\n");
	}

	if (job->frame.height)
		render_height = job->frame.height + 1;
	else
		render_height = job->frame.supersampled_height + 1;

	//mali_memory_dump();
}

static void
dev_mali400_pp_job_start_r3p1_pre(void *data)
{
	struct lima_m400_pp_job_start_r3p1 *job = data;
	int i;

	wrap_log("/* IOCTL MALI_IOC_PP_START_JOB IN; */\n");

	wrap_log("struct lima_m400_pp_job_start_r3p1 pp_job = {\n");

	wrap_log("\t.user_job_ptr = 0x%x,\n", job->user_job_ptr);
	wrap_log("\t.priority = 0x%x,\n", job->priority);

	wrap_log("\t.frame.plbu_array_address = 0x%x,\n", job->frame.plbu_array_address);
	wrap_log("\t.frame.render_address = 0x%x,\n", job->frame.render_address);
	wrap_log("\t.frame.flags = 0x%x,\n", job->frame.flags);
	wrap_log("\t.frame.clear_value_depth = 0x%x,\n", job->frame.clear_value_depth);
	wrap_log("\t.frame.clear_value_stencil = 0x%x,\n", job->frame.clear_value_stencil);
	wrap_log("\t.frame.clear_value_color = 0x%x,\n", job->frame.clear_value_color);
	wrap_log("\t.frame.clear_value_color_1 = 0x%x,\n", job->frame.clear_value_color_1);
	wrap_log("\t.frame.clear_value_color_2 = 0x%x,\n", job->frame.clear_value_color_2);
	wrap_log("\t.frame.clear_value_color_3 = 0x%x,\n", job->frame.clear_value_color_3);
	wrap_log("\t.frame.width = 0x%x,\n", job->frame.width);
	wrap_log("\t.frame.height = 0x%x,\n", job->frame.height);
	wrap_log("\t.frame.fragment_stack_address = 0x%x,\n", job->frame.fragment_stack_address);
	wrap_log("\t.frame.fragment_stack_size = 0x%x,\n", job->frame.fragment_stack_size);
	wrap_log("\t.frame.one = 0x%x,\n", job->frame.one);
	wrap_log("\t.frame.supersampled_height = 0x%x,\n", job->frame.supersampled_height);
	wrap_log("\t.frame.dubya = 0x%x,\n", job->frame.dubya);
	wrap_log("\t.frame.onscreen = 0x%x,\n", job->frame.onscreen);
	wrap_log("\t.frame.blocking = 0x%x,\n", job->frame.blocking);
	wrap_log("\t.frame.scale = 0x%x,\n", job->frame.scale);
	wrap_log("\t.frame.foureight = 0x%x,\n", job->frame.foureight);

	for (i = 0; i < 7; i++)
		wrap_log("\t.addr_frame[%d] = 0x%x,\n", i, job->addr_frame[i]);
	for (i = 0; i < 7; i++)
		wrap_log("\t.addr_stack[%d] = 0x%x,\n", i, job->addr_stack[i]);

	wrap_log("\t.wb0.type = 0x%x,\n", job->wb0.type);
	wrap_log("\t.wb0.address = 0x%x,\n", job->wb0.address);
	wrap_log("\t.wb0.pixel_format = 0x%x,\n", job->wb0.pixel_format);
	wrap_log("\t.wb0.downsample_factor = 0x%x,\n", job->wb0.downsample_factor);
	wrap_log("\t.wb0.pixel_layout = 0x%x,\n", job->wb0.pixel_layout);
	wrap_log("\t.wb0.pitch = 0x%x,\n", job->wb0.pitch);
	wrap_log("\t.wb0.mrt_bits = 0x%x,\n", job->wb0.mrt_bits);
	wrap_log("\t.wb0.mrt_pitch = 0x%x,\n", job->wb0.mrt_pitch);
	wrap_log("\t.wb0.zero = 0x%x,\n", job->wb0.zero);

	wrap_log("\t.wb1.address = 0x%x,\n", job->wb1.address);
	wrap_log("\t.wb1.pixel_format = 0x%x,\n", job->wb1.pixel_format);
	wrap_log("\t.wb1.downsample_factor = 0x%x,\n", job->wb1.downsample_factor);
	wrap_log("\t.wb1.pixel_layout = 0x%x,\n", job->wb1.pixel_layout);
	wrap_log("\t.wb1.pitch = 0x%x,\n", job->wb1.pitch);
	wrap_log("\t.wb1.mrt_bits = 0x%x,\n", job->wb1.mrt_bits);
	wrap_log("\t.wb1.mrt_pitch = 0x%x,\n", job->wb1.mrt_pitch);
	wrap_log("\t.wb1.zero = 0x%x,\n", job->wb1.zero);

	wrap_log("\t.wb2.address = 0x%x,\n", job->wb2.address);
	wrap_log("\t.wb2.pixel_format = 0x%x,\n", job->wb2.pixel_format);
	wrap_log("\t.wb2.downsample_factor = 0x%x,\n", job->wb2.downsample_factor);
	wrap_log("\t.wb2.pixel_layout = 0x%x,\n", job->wb2.pixel_layout);
	wrap_log("\t.wb2.pitch = 0x%x,\n", job->wb2.pitch);
	wrap_log("\t.wb2.mrt_bits = 0x%x,\n", job->wb2.mrt_bits);
	wrap_log("\t.wb2.mrt_pitch = 0x%x,\n", job->wb2.mrt_pitch);
	wrap_log("\t.wb2.zero = 0x%x,\n", job->wb2.zero);

	wrap_log("\t.num_cores = 0x%x,\n", job->num_cores);

	wrap_log("\t.frame_builder_id = 0x%x,\n", job->frame_builder_id);
	wrap_log("\t.flush_id = 0x%x,\n", job->flush_id);
	wrap_log("\t.flags = 0x%x,\n", job->flags);

	wrap_log("};\n");

	/*
	 * Now store where our final render is headed, and what it looks like.
	 * We will dump it as a bmp once we're done.
	 */
	render_address = job->wb0.address;

	render_format = job->wb0.pixel_format;
	switch (job->wb0.pixel_format) {
	case LIMA_PIXEL_FORMAT_RGB_565:
		render_width = job->wb0.pitch * 4;
		break;
	case LIMA_PIXEL_FORMAT_RGBA_8888:
		render_width = job->wb0.pitch * 2;
		break;
	default:
		wrap_log("Error: unhandled wb format!\n");
	}

	if (job->frame.height)
		render_height = job->frame.height + 1;
	else
		render_height = job->frame.supersampled_height + 1;

	//mali_memory_dump();
}


static void
dev_mali400_pp_job_start_r3p2_pre(void *data)
{
	struct lima_m400_pp_job_start_r3p2 *job = data;
	int i;

	wrap_log("/* IOCTL MALI_IOC_PP_START_JOB IN; */\n");

	wrap_log("struct lima_m400_pp_job_start_r3p2 pp_job = {\n");

	wrap_log("\t.user_job_ptr = 0x%x,\n", job->user_job_ptr);
	wrap_log("\t.priority = 0x%x,\n", job->priority);

	wrap_log("\t.frame.plbu_array_address = 0x%x,\n", job->frame.plbu_array_address);
	wrap_log("\t.frame.render_address = 0x%x,\n", job->frame.render_address);
	wrap_log("\t.frame.flags = 0x%x,\n", job->frame.flags);
	wrap_log("\t.frame.clear_value_depth = 0x%x,\n", job->frame.clear_value_depth);
	wrap_log("\t.frame.clear_value_stencil = 0x%x,\n", job->frame.clear_value_stencil);
	wrap_log("\t.frame.clear_value_color = 0x%x,\n", job->frame.clear_value_color);
	wrap_log("\t.frame.clear_value_color_1 = 0x%x,\n", job->frame.clear_value_color_1);
	wrap_log("\t.frame.clear_value_color_2 = 0x%x,\n", job->frame.clear_value_color_2);
	wrap_log("\t.frame.clear_value_color_3 = 0x%x,\n", job->frame.clear_value_color_3);
	wrap_log("\t.frame.width = 0x%x,\n", job->frame.width);
	wrap_log("\t.frame.height = 0x%x,\n", job->frame.height);
	wrap_log("\t.frame.fragment_stack_address = 0x%x,\n", job->frame.fragment_stack_address);
	wrap_log("\t.frame.fragment_stack_size = 0x%x,\n", job->frame.fragment_stack_size);
	wrap_log("\t.frame.one = 0x%x,\n", job->frame.one);
	wrap_log("\t.frame.supersampled_height = 0x%x,\n", job->frame.supersampled_height);
	wrap_log("\t.frame.dubya = 0x%x,\n", job->frame.dubya);
	wrap_log("\t.frame.onscreen = 0x%x,\n", job->frame.onscreen);
	wrap_log("\t.frame.blocking = 0x%x,\n", job->frame.blocking);
	wrap_log("\t.frame.scale = 0x%x,\n", job->frame.scale);
	wrap_log("\t.frame.foureight = 0x%x,\n", job->frame.foureight);

	for (i = 0; i < 7; i++)
		wrap_log("\t.addr_frame[%d] = 0x%x,\n", i, job->addr_frame[i]);
	for (i = 0; i < 7; i++)
		wrap_log("\t.addr_stack[%d] = 0x%x,\n", i, job->addr_stack[i]);

	wrap_log("\t.wb0.type = 0x%x,\n", job->wb0.type);
	wrap_log("\t.wb0.address = 0x%x,\n", job->wb0.address);
	wrap_log("\t.wb0.pixel_format = 0x%x,\n", job->wb0.pixel_format);
	wrap_log("\t.wb0.downsample_factor = 0x%x,\n", job->wb0.downsample_factor);
	wrap_log("\t.wb0.pixel_layout = 0x%x,\n", job->wb0.pixel_layout);
	wrap_log("\t.wb0.pitch = 0x%x,\n", job->wb0.pitch);
	wrap_log("\t.wb0.mrt_bits = 0x%x,\n", job->wb0.mrt_bits);
	wrap_log("\t.wb0.mrt_pitch = 0x%x,\n", job->wb0.mrt_pitch);
	wrap_log("\t.wb0.zero = 0x%x,\n", job->wb0.zero);

	wrap_log("\t.wb1.address = 0x%x,\n", job->wb1.address);
	wrap_log("\t.wb1.pixel_format = 0x%x,\n", job->wb1.pixel_format);
	wrap_log("\t.wb1.downsample_factor = 0x%x,\n", job->wb1.downsample_factor);
	wrap_log("\t.wb1.pixel_layout = 0x%x,\n", job->wb1.pixel_layout);
	wrap_log("\t.wb1.pitch = 0x%x,\n", job->wb1.pitch);
	wrap_log("\t.wb1.mrt_bits = 0x%x,\n", job->wb1.mrt_bits);
	wrap_log("\t.wb1.mrt_pitch = 0x%x,\n", job->wb1.mrt_pitch);
	wrap_log("\t.wb1.zero = 0x%x,\n", job->wb1.zero);

	wrap_log("\t.wb2.address = 0x%x,\n", job->wb2.address);
	wrap_log("\t.wb2.pixel_format = 0x%x,\n", job->wb2.pixel_format);
	wrap_log("\t.wb2.downsample_factor = 0x%x,\n", job->wb2.downsample_factor);
	wrap_log("\t.wb2.pixel_layout = 0x%x,\n", job->wb2.pixel_layout);
	wrap_log("\t.wb2.pitch = 0x%x,\n", job->wb2.pitch);
	wrap_log("\t.wb2.mrt_bits = 0x%x,\n", job->wb2.mrt_bits);
	wrap_log("\t.wb2.mrt_pitch = 0x%x,\n", job->wb2.mrt_pitch);
	wrap_log("\t.wb2.zero = 0x%x,\n", job->wb2.zero);

	wrap_log("\t.dlbu_regs[0] = 0x%x,\n", job->dlbu_regs[0]);
	wrap_log("\t.dlbu_regs[1] = 0x%x,\n", job->dlbu_regs[1]);
	wrap_log("\t.dlbu_regs[2] = 0x%x,\n", job->dlbu_regs[2]);
	wrap_log("\t.dlbu_regs[3] = 0x%x,\n", job->dlbu_regs[3]);

	wrap_log("\t.num_cores = 0x%x,\n", job->num_cores);

	wrap_log("\t.frame_builder_id = 0x%x,\n", job->frame_builder_id);
	wrap_log("\t.flush_id = 0x%x,\n", job->flush_id);
	wrap_log("\t.flags = 0x%x,\n", job->flags);

	wrap_log("\t.fence = 0x%x,\n", job->fence);
	wrap_log("\t.stream = 0x%x,\n", job->stream);

	wrap_log("};\n");

	/*
	 * Now store where our final render is headed, and what it looks like.
	 * We will dump it as a bmp once we're done.
	 */
	render_address = job->wb0.address;

	render_format = job->wb0.pixel_format;
	switch (job->wb0.pixel_format) {
	case LIMA_PIXEL_FORMAT_RGB_565:
		render_width = job->wb0.pitch * 4;
		break;
	case LIMA_PIXEL_FORMAT_RGBA_8888:
		render_width = job->wb0.pitch * 2;
		break;
	default:
		wrap_log("Error: unhandled wb format!\n");
	}

	if (job->frame.height)
		render_height = job->frame.height + 1;
	else
		render_height = job->frame.supersampled_height + 1;

	//mali_memory_dump();
}

static void
dev_mali_pp_job_start_pre(void *data)
{
	if (mali_type == 400) {
		if (mali_version < MALI_DRIVER_VERSION_R3P0)
			dev_mali400_pp_job_start_r2p1_pre(data);
		else if (mali_version < MALI_DRIVER_VERSION_R3P1)
			dev_mali400_pp_job_start_r3p0_pre(data);
		else if (mali_version < MALI_DRIVER_VERSION_R3P2)
			dev_mali400_pp_job_start_r3p1_pre(data);
		else
			dev_mali400_pp_job_start_r3p2_pre(data);
	} else
		dev_mali200_pp_job_start_pre(data);
}

static void
dev_mali200_pp_job_start_post(void *data, int ret)
{
	struct lima_m200_pp_job_start *job = data;

	wrap_log("/* IOCTL MALI_IOC_PP_START_JOB OUT */\n");

	wrap_log("struct lima_m200_pp_job_start pp_job_out = {\n");
	wrap_log("\t.returned_user_job_ptr = 0x%x,\n",
		 job->returned_user_job_ptr);
	wrap_log("\t.status = 0x%x,\n", job->status);

	wrap_log("};\n\n");
}

static void
dev_mali400_pp_job_start_r2p1_post(void *data, int ret)
{
	struct lima_m400_pp_job_start_r2p1 *job = data;

	wrap_log("/* IOCTL MALI_IOC_PP_START_JOB OUT */\n");

	wrap_log("struct lima_m400_pp_job_start pp_job_out = {\n");
	wrap_log("\t.returned_user_job_ptr = 0x%x,\n",
		 job->returned_user_job_ptr);
	wrap_log("\t.status = 0x%x,\n", job->status);

	wrap_log("};\n\n");
}

static void
dev_mali_pp_job_start_post(void *data, int ret)
{
	if (mali_type == 400) {
		if (mali_version < MALI_DRIVER_VERSION_R3P0)
			dev_mali400_pp_job_start_r2p1_post(data, ret);
	} else
		dev_mali200_pp_job_start_post(data, ret);
}

static struct ioc_type {
	int type;
	char *name;
} ioc_types[] = {
	{MALI_IOC_CORE_BASE, "MALI_IOC_CORE_BASE"},
	{MALI_IOC_MEMORY_BASE, "MALI_IOC_MEMORY_BASE"},
	{MALI_IOC_PP_BASE, "MALI_IOC_PP_BASE"},
	{MALI_IOC_GP_BASE, "MALI_IOC_GP_BASE"},
	{0, NULL},
};

static char *
ioc_type_name(int type)
{
	int i;

	for (i = 0; ioc_types[i].name; i++)
		if (ioc_types[i].type == type)
			break;

	return ioc_types[i].name;
}

struct dev_mali_ioctl_table {
	int type;
	int nr;
	char *name;
	void (*pre)(void *data);
	void (*post)(void *data, int ret);
};

static struct dev_mali_ioctl_table
dev_mali_ioctls[] = {
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

	{MALI_IOC_MEMORY_BASE, _MALI_UK_MAP_EXT_MEM, "MEMORY, MAP_EXT_MEM",
	 NULL, dev_mali_memory_map_ext_mem_post},
	{MALI_IOC_MEMORY_BASE, _MALI_UK_UNMAP_EXT_MEM, "MEMORY, UNMAP_EXT_MEM",
	 NULL, dev_mali_memory_unmap_ext_mem_post},
	{MALI_IOC_PP_BASE, _MALI_UK_PP_START_JOB, "PP, START_JOB",
	 dev_mali_pp_job_start_pre, dev_mali_pp_job_start_post},
	{MALI_IOC_PP_BASE, _MALI_UK_GET_PP_CORE_VERSION_R2P1, "PP, GET_CORE_VERSION_R2P1",
	 NULL, dev_mali_pp_core_version_post},
	{MALI_IOC_PP_BASE, _MALI_UK_GET_PP_CORE_VERSION_R3P0, "PP, GET_CORE_VERSION_R3P0",
	 NULL, dev_mali_pp_core_version_post},
	{MALI_IOC_GP_BASE, _MALI_UK_GP_START_JOB, "GP, START_JOB",
	 dev_mali_gp_job_start_pre, dev_mali_gp_job_start_post},

	{ 0, 0, NULL, NULL, NULL}
};

static struct dev_mali_ioctl_table
dev_mali_ioctls_r3p1[] = {
	{MALI_IOC_CORE_BASE, _MALI_UK_OPEN, "CORE, OPEN", NULL, NULL},
	{MALI_IOC_CORE_BASE, _MALI_UK_CLOSE, "CORE, CLOSE", NULL, NULL},
	{MALI_IOC_CORE_BASE, _MALI_UK_WAIT_FOR_NOTIFICATION_R3P1, "CORE, WAIT_FOR_NOTIFICATION",
	 dev_mali_wait_for_notification_pre, dev_mali_wait_for_notification_post},
	{MALI_IOC_CORE_BASE, _MALI_UK_GET_API_VERSION_R3P1, "CORE, GET_API_VERSION",
	 dev_mali_get_api_version_pre, dev_mali_get_api_version_post},
	{MALI_IOC_MEMORY_BASE, _MALI_UK_INIT_MEM, "MEMORY, INIT_MEM",
	 NULL, dev_mali_memory_init_mem_post},

	{MALI_IOC_MEMORY_BASE, _MALI_UK_ATTACH_UMP_MEM_R3P1, "MEMORY, ATTACH_UMP_MEM",
	 NULL, dev_mali_memory_attach_ump_mem_post},

	{MALI_IOC_MEMORY_BASE, _MALI_UK_MAP_EXT_MEM_R3P1, "MEMORY, MAP_EXT_MEM",
	 NULL, dev_mali_memory_map_ext_mem_post},
	{MALI_IOC_MEMORY_BASE, _MALI_UK_UNMAP_EXT_MEM_R3P1, "MEMORY, UNMAP_EXT_MEM",
	 NULL, dev_mali_memory_unmap_ext_mem_post},
	{MALI_IOC_PP_BASE, _MALI_UK_PP_START_JOB, "PP, START_JOB",
	 dev_mali_pp_job_start_pre, dev_mali_pp_job_start_post},
	{MALI_IOC_PP_BASE, _MALI_UK_GET_PP_CORE_VERSION_R3P0, "PP, GET_CORE_VERSION_R3P0",
	 NULL, dev_mali_pp_core_version_post},
	{MALI_IOC_GP_BASE, _MALI_UK_GP_START_JOB, "GP, START_JOB",
	 dev_mali_gp_job_start_pre, dev_mali_gp_job_start_post},

	{ 0, 0, NULL, NULL, NULL}
};

struct dev_mali_ioctl_table *ioctl_table;

static int
mali_ioctl(int request, void *data)
{
	struct dev_mali_ioctl_table *ioctl = NULL;
	int ioc_type = _IOC_TYPE(request);
	int ioc_nr = _IOC_NR(request);
	char *ioc_string = ioctl_dir_string(request);
	int i;
	int ret;

	if (!ioctl_table) {
		if ((ioc_type == MALI_IOC_CORE_BASE) &&
		    (ioc_nr == _MALI_UK_GET_API_VERSION_R3P1))
			ioctl_table = dev_mali_ioctls_r3p1;
		else
			ioctl_table = dev_mali_ioctls;
	}

	for (i = 0; ioctl_table[i].name; i++) {
		if ((ioctl_table[i].type == ioc_type) &&
		    (ioctl_table[i].nr == ioc_nr)) {
			ioctl = &ioctl_table[i];
			break;
		}
	}

	if (!ioctl) {
		char *name = ioc_type_name(ioc_type);

		if (name)
			wrap_log("/* Error: No mali ioctl wrapping implemented for %s:%02X */\n",
				 name, ioc_nr);
		else
			wrap_log("/* Error: No mali ioctl wrapping implemented for %02X:%02X */\n",
				 ioc_type, ioc_nr);

	}

	if (ioctl && ioctl->pre)
		ioctl->pre(data);

	if (data)
		ret = orig_ioctl(dev_mali_fd, request, data);
	else
		ret = orig_ioctl(dev_mali_fd, request);

	if (ret == -EPERM) {
		if ((ioc_type == MALI_IOC_CORE_BASE) &&
		    (ioc_nr == _MALI_UK_GET_API_VERSION))
			ioctl_table = dev_mali_ioctls_r3p1;
	}

	if (ioctl && !ioctl->pre && !ioctl->post) {
		if (data)
			wrap_log("/* IOCTL %s(%s) %p = %d */\n",
				 ioc_string, ioctl->name, data, ret);
		else
			wrap_log("/* IOCTL %s(%s) = %d */\n",
				 ioc_string, ioctl->name, ret);
	}

	if (ioctl && ioctl->post)
		ioctl->post(data, ret);

	return ret;
}

/*
 *
 * Memory dumper.
 *
 */
#define MALI_ADDRESSES 0x40

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

#define MALI_EXTERNALS 0x10
static struct mali_external {
	unsigned int address;
	unsigned int physical; /* actual address */
	unsigned int size;
	unsigned int cookie;
} mali_externals[MALI_EXTERNALS];

static int fb_fd = -1;
static char *fbdev_name = "/dev/fb0";
static unsigned int fb_physical;
static void *fb_map = ((void *) -1);
static int fb_map_size;

static int
mali_map_fb(void)
{
	struct fb_fix_screeninfo fb_fix;

	if (!orig_open)
		orig_open = libc_dlsym(__func__);

	fb_fd = orig_open(fbdev_name, O_RDONLY);
	if (fb_fd == -1) {
		printf("Error: Failed to open %s: %s\n", fbdev_name, strerror(errno));
		return errno;
	}

	if (!orig_ioctl)
		orig_ioctl = libc_dlsym(__func__);

	if (orig_ioctl(fb_fd, FBIOGET_FSCREENINFO, &fb_fix)) {
		printf("Error: failed to run ioctl on %s: %s\n",
			fbdev_name, strerror(errno));
		close(fb_fd);
		fb_fd = -1;
		return errno;
	}

	fb_physical = fb_fix.smem_start;
	fb_map_size = fb_fix.smem_len;

	if (!orig_mmap)
		orig_mmap = libc_dlsym("mmap");

	fb_map = orig_mmap(0, fb_map_size, PROT_READ, MAP_PRIVATE, fb_fd, 0);
	if (fb_map == ((void *) -1)) {
		printf("Error: Failed to mmap %s: %s\n", fbdev_name, strerror(errno));
		return -1;
	}

	printf("Mapped %dbytes from %s (0x%08X) at %p\n",
	       fb_map_size, fbdev_name, fb_physical, fb_map);

	return 0;
}

static int
mali_external_add(unsigned int address, unsigned int physical, unsigned int size, unsigned int cookie)
{
	int i;

	if (fb_map == ((void *) -1)) {
		if (mali_map_fb())
			exit(1);
	}

	if ((physical < fb_physical) &&
	    ((physical + size) > (fb_physical + size))) {
		printf("Error: external address 0x%08X (0x%08X) is not the framebuffer\n",
		       physical, size);
		exit(1);
	}

	for (i = 0; i < MALI_EXTERNALS; i++) {
		if ((mali_externals[i].address >= address) &&
		    (mali_externals[i].address < (address + size)) &&
		    ((mali_externals[i].address + size) > address) &&
		    ((mali_externals[i].address + size) <= (address + size))) {
			printf("Error: Address 0x%08X (0x%x) is already taken!\n",
			       address, size);
			return -1;
		}
	}

	for (i = 0; i < MALI_EXTERNALS; i++)
		if (!mali_externals[i].address)
			break;

	if (i == MALI_EXTERNALS) {
		printf("Error: No more free memory slots for 0x%08X (0x%x)!\n",
		       address, size);
		return -1;
	}

	/* map memory here */
	mali_externals[i].address = address;
	mali_externals[i].physical = physical;
	mali_externals[i].size = size;
	mali_externals[i].cookie = cookie;

	return 0;
}

static int
mali_external_remove(unsigned int cookie)
{
	int i;

	for (i = 0; i < MALI_EXTERNALS; i++)
		if (mali_externals[i].cookie == cookie) {
			/* deref mapping here */

			mali_externals[i].address = 0;
			mali_externals[i].physical = 0;
			mali_externals[i].size = 0;
			mali_externals[i].cookie = 0;

			return 0;
		}

	return -1;
}

#define UMP_ADDRESSES 0x10

static struct ump_address {
	void *address; /* mapped address */
	unsigned int id;
	unsigned int size;
	unsigned int physical; /* actual address */
} ump_addresses[UMP_ADDRESSES];

static int
ump_id_add(unsigned int id, unsigned int size, void *address)
{
	int i;

	for (i = 0; i < UMP_ADDRESSES; i++)
		if (!ump_addresses[i].id) {
			ump_addresses[i].id = id;
			ump_addresses[i].size = size;
			ump_addresses[i].address = address;
			return 0;
		}

	printf("%s: No more free slots for 0x%08X (0x%x)!\n",
	       __func__, id, size);
	return -1;
}

static int
ump_physical_add(unsigned int id, unsigned int physical)
{
	int i;

	for (i = 0; i < UMP_ADDRESSES; i++)
		if (ump_addresses[i].id == id) {
			ump_addresses[i].physical = physical;
			return 0;
		}

	printf("%s: Error: id 0x%08X not found!\n", __func__, id);
	return -1;
}

#if 0
static int
ump_id_remove(unsigned int id, int size)
{
	int i;

	for (i = 0; i < UMP_ADDRESSES; i++)
		if ((ump_addresses[i].id == address) &&
		    (ump_addresses[i].size == size)) {
			ump_addresses[i].address = NULL;
			ump_addresses[i].id = 0;
			ump_addresses[i].size = 0;
			ump_addresses[i].physical = 0;
			return 0;
		}

	return -1;
}
#endif

static void *
mali_address_retrieve(unsigned int physical)
{
	int i;

	for (i = 0; i < MALI_EXTERNALS; i++)
		if ((mali_externals[i].address <= physical) &&
		    ((mali_externals[i].address + mali_externals[i].size)
		     >= physical)) {
#if 0
			printf("fb_map (%p) + (physical (0x%08X)- mali_externals[i].address (0x%08X)) + (mali_externals[i].physical (0x%08X) - fb_physical(0x%08X))\n",
			       fb_map, physical, mali_externals[i].address, mali_externals[i].physical, fb_physical);
#endif
			return fb_map +
				(physical - mali_externals[i].address) +
				(mali_externals[i].physical - fb_physical);
		}

	for (i = 0; i < MALI_ADDRESSES; i++)
		if ((mali_addresses[i].physical <= physical) &&
		    ((mali_addresses[i].physical + mali_addresses[i].size)
		     >= physical))
			return mali_addresses[i].address +
				(mali_addresses[i].physical - physical);


	for (i = 0; i < UMP_ADDRESSES; i++)
		if ((ump_addresses[i].physical <= physical) &&
		    ((ump_addresses[i].physical + ump_addresses[i].size)
		     >= physical)) {
			if (ump_addresses[i].address)
				return ump_addresses[i].address +
					(ump_addresses[i].physical - physical);
			else
				return NULL;
		}

	return NULL;
}

static void
mali_memory_dump_block(unsigned int *address, int start, int stop,
		       unsigned physical, int count)
{
	int i;

	wrap_log("static struct lima_dumped_mem_content mem_0x%08x_0x%08x = {\n",
	       physical, count);

	wrap_log("\t0x%08x,\n", 4 * start);
	wrap_log("\t0x%08x,\n", 4 * (stop - start));
	wrap_log("\t{\n");

	for (i = start; i < stop; i += 4)
		wrap_log("\t\t0x%08x, 0x%08x, 0x%08x, 0x%08x, /* 0x%08X */\n",
			 address[i + 0], address[i + 1],
			 address[i + 2], address[i + 3], 4 * (i - start));

	wrap_log("\t}\n");
	wrap_log("};\n\n");
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

	if (start != -1) {
		if (stop == -1) {
			mali_memory_dump_block(address, start, size, physical, count);
			count++;
		} else {
			mali_memory_dump_block(address, start, stop, physical, count);
			count++;
		}
	}

	wrap_log("static struct lima_dumped_mem_block mem_0x%08x = {\n", physical);
	wrap_log("\tNULL,\n");
	wrap_log("\t0x%08x,\n", physical);
	wrap_log("\t0x%08x,\n", 4 * size);
	wrap_log("\t0x%08x,\n", count);
	wrap_log("\t{\n");

	for (i = 0; i < count; i++)
		wrap_log("\t\t&mem_0x%08x_0x%08x,\n", physical, i);

	wrap_log("\t},\n");
	wrap_log("};\n\n");
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

	wrap_log("struct lima_dumped_mem dumped_mem = {\n");
	wrap_log("\t0x%08x,\n", count);
	wrap_log("\t{\n");

	for (i = 0; i < MALI_ADDRESSES; i++)
		if (mali_addresses[i].address)
			wrap_log("\t\t&mem_0x%08x,\n", mali_addresses[i].physical);

	wrap_log("\t},\n");
	wrap_log("};\n\n");
}

static void
mali_wrap_bmp_dump(void)
{
	char *filename;
	void *address = mali_address_retrieve(render_address);
	char buffer[1024];

	printf("%s: dumping frame %04d from address 0x%08X (%dx%d)\n",
	       __func__, frame_count, render_address, render_width, render_height);

	filename = getenv("LIMA_WRAP_BMP");
	if (!filename)
		filename = "/home/libv/lima/dump/lima.wrap.bmp";

	snprintf(buffer, sizeof(buffer), "%s.%04d",
		 filename, frame_count);

	if (!address) {
		printf("%s: Failed to dump bmp at 0x%x\n",
		       __func__, render_address);
		return;
	}

	switch (render_format) {
	case LIMA_PIXEL_FORMAT_RGBA_8888:
	case LIMA_PIXEL_FORMAT_RGB_565:
		break;
	default:
		printf("%s: Pixel format 0x%x is currently not implemented\n",
		       __func__, render_format);
		return;
	}

	if (render_height < 16) {
		printf("%s: invalid height: %d\n", __func__, render_height);
		render_height = 480;
	}

	if (wrap_bmp_dump(address, render_width, render_height, render_format, buffer))
		printf("Failed to dump on frame %04d (0x%08X)\n", frame_count,
		       (unsigned int) address);
}

#if 0
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
			wrap_log("\t\t/* %08X", (unsigned int) buf + i);

		if (((void *) (buf + i)) < ((void *) data)) {
			wrap_log("   ");
			alpha[i % 16] = '.';
		} else {
			wrap_log(" %02X", buf[i]);

			if (isprint(buf[i]) && (buf[i] < 0xA0))
				alpha[i % 16] = buf[i];
			else
				alpha[i % 16] = '.';
		}

		if ((i % 16) == 15) {
			alpha[16] = 0;
			wrap_log("\t |%s| */\n", alpha);
		}
	}

	if (i % 16) {
		for (i %= 16; i < 16; i++) {
			wrap_log("   ");
			alpha[i] = '.';

			if (i == 15) {
				alpha[16] = 0;
				wrap_log("\t |%s| */\n", alpha);
			}
		}
	}
}

void
wrap_dump_shader(unsigned int *shader, int size)
{
	int i;

	for (i = 0; i < size; i += 4)
		wrap_log("\t\t0x%08x, 0x%08x, 0x%08x, 0x%08x, /* 0x%08x */\n",
			 shader[i + 0], shader[i + 1], shader[i + 2], shader[i + 3],
			 4 * i);

}

int (*orig__mali_compile_essl_shader)(struct lima_shader_binary *binary, int type,
				      const char *source, int *length, int count);

int
__mali_compile_essl_shader(struct lima_shader_binary *binary, int type,
			   const char *source, int *length, int count)
{
	int ret;
	int i, offset = 0;

	serialized_start(__func__);

	if (!orig__mali_compile_essl_shader)
		orig__mali_compile_essl_shader = libmali_dlsym(__func__);

	if (mali_version >= MALI_DRIVER_VERSION_R3P0)
		wrap_log("/* WARNING: the binary structure of the shader compiler "
			 "has changed and will not be valid as is! */\n");

	wrap_log("#if 0 /* Shader */\n\n");

	for (i = 0, offset = 0; i < count; i++) {
		wrap_log("/* %s shader source %d:\n",
		       (type == 0x8B31) ? "Vertex" : "Fragment", i);
		wrap_log("\"%s\" */\n", &source[offset]);
		offset += length[i];
	}

	wrap_log("\n");

	ret = orig__mali_compile_essl_shader(binary, type, source, length, count);

	wrap_log("struct lima_shader_binary shader_%p = {\n", binary);
	wrap_log("\t.compile_status = %d,\n", binary->compile_status);
	if (binary->error_log)
		wrap_log("\t.error_log = \"%s\",\n", binary->error_log);
	else
		wrap_log("\t.error_log = NULL,\n");
	wrap_log("\t.shader = {\n");
	wrap_dump_shader(binary->shader, binary->shader_size / 4);
	wrap_log("\t},\n");
	wrap_log("\t.shader_size = 0x%x,\n", binary->shader_size);

	if (binary->varying_stream) {
		wrap_log("\t.varying_stream = (char *) {\n");
		hexdump(binary->varying_stream, binary->varying_stream_size);
		wrap_log("\t},\n");
	} else
		wrap_log("\t.varying_stream = NULL,\n");
	wrap_log("\t.varying_stream_size = 0x%x,\n", binary->varying_stream_size);

	if (binary->uniform_stream) {
		wrap_log("\t.uniform_stream = {\n");
		hexdump(binary->uniform_stream, binary->uniform_stream_size);
		wrap_log("\t},\n");
	} else
		wrap_log("\t.uniform_stream = NULL,\n");
	wrap_log("\t.uniform_stream_size = 0x%x,\n", binary->uniform_stream_size);

	if (binary->attribute_stream) {
		wrap_log("\t.attribute_stream = (char *) {\n");
		hexdump(binary->attribute_stream, binary->attribute_stream_size);
		wrap_log("\t},\n");
	} else
		wrap_log("\t.attribute_stream = NULL,\n");
	wrap_log("\t.attribute_stream_size = 0x%x,\n", binary->attribute_stream_size);

	wrap_log("\t.parameters = {\n");
	if (type == 0x8B31) {
		wrap_log("\t\t.vertex = {\n");
		wrap_log("\t\t\t.unknown00 = 0x%x,\n", binary->parameters.vertex.unknown00);
		wrap_log("\t\t\t.unknown04 = 0x%x,\n", binary->parameters.vertex.unknown04);
		wrap_log("\t\t\t.unknown08 = 0x%x,\n", binary->parameters.vertex.unknown08);
		wrap_log("\t\t\t.unknown0C = 0x%x,\n", binary->parameters.vertex.unknown0C);
		wrap_log("\t\t\t.attribute_count = 0x%x,\n", binary->parameters.vertex.attribute_count);
		wrap_log("\t\t\t.varying_count = 0x%x,\n", binary->parameters.vertex.varying_count);
		wrap_log("\t\t\t.unknown18 = 0x%x,\n", binary->parameters.vertex.unknown18);
		wrap_log("\t\t\t.size = 0x%x,\n", binary->parameters.vertex.size);
		wrap_log("\t\t\t.attribute_prefetch = 0x%x,\n", binary->parameters.vertex.attribute_prefetch);
		wrap_log("\t\t},\n");
	} else {
		wrap_log("\t\t.fragment = {\n");
		wrap_log("\t\t\t.unknown00 = 0x%x,\n", binary->parameters.fragment.unknown00);
		wrap_log("\t\t\t.first_instruction_size = 0x%x,\n",
			 binary->parameters.fragment.first_instruction_size);
		wrap_log("\t\t\t.unknown08 = 0x%x,\n", binary->parameters.fragment.unknown08);
		wrap_log("\t\t\t.unknown0C = 0x%x,\n", binary->parameters.fragment.unknown0C);
		wrap_log("\t\t\t.unknown10 = 0x%x,\n", binary->parameters.fragment.unknown10);
		wrap_log("\t\t\t.unknown14 = 0x%x,\n", binary->parameters.fragment.unknown14);
		wrap_log("\t\t\t.unknown18 = 0x%x,\n", binary->parameters.fragment.unknown18);
		wrap_log("\t\t\t.unknown1C = 0x%x,\n", binary->parameters.fragment.unknown1C);
		wrap_log("\t\t\t.unknown20 = 0x%x,\n", binary->parameters.fragment.unknown20);
		wrap_log("\t\t\t.unknown24 = 0x%x,\n", binary->parameters.fragment.unknown24);
		wrap_log("\t\t\t.unknown28 = 0x%x,\n", binary->parameters.fragment.unknown28);
		wrap_log("\t\t\t.unknown2C = 0x%x,\n", binary->parameters.fragment.unknown2C);
		wrap_log("\t\t}\n");
	}
	wrap_log("\t}\n");

	wrap_log("};\n\n");

	wrap_log("#endif /* Shader */\n\n");

	serialized_stop();

	return ret;
}
#endif
