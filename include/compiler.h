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
/*
 * Interface to the shader compiler of libMali.so
 */
#ifndef MALI_COMPILER_H
#define MALI_COMPILER_H 1

struct mali_shader_binary_vertex_parameters { /* 0x24 */
	int unknown00; /* 0x00 */
	int unknown04; /* 0x04 */
	int unknown08; /* 0x08 */
	int unknown0C; /* 0x0C */
	int attribute_count; /* 0x10 */
	int varying_count; /* 0x14 */
	int unknown18; /* 0x18 */
	int size; /* 0x1C, commands are in 4 dwords */
	int unknown20; /* 0x20 */
};

struct mali_shader_binary_fragment_parameters { /* 0x30 */
	int unknown00; /* 0x00 */
	int unknown04; /* 0x04 */
	int unknown08; /* 0x08 */
	int unknown0C; /* 0x0C */
	int unknown10; /* 0x10 */
	int unknown14; /* 0x14 */
	int unknown18; /* 0x18 */
	int unknown1C; /* 0x1C */
	int unknown20; /* 0x20 */
	int unknown24; /* 0x24 */
	int unknown28; /* 0x28 */
	int unknown2C; /* 0x2C */
};

struct mali_shader_binary { /* 0x5C */
#define MALI_SHADER_COMPILE_STATUS_CLEAR    0x00
#define MALI_SHADER_COMPILE_STATUS_COMPILED 0x01
	int compile_status; /* 0x00 */
	char *error_log; /* 0x04 - can be freed */
	char *oom_log; /* 0x08 - static */
	void *shader; /* 0x0C */
	int shader_size; /* 0x10 */
	void *varying_stream; /* 0x14 */
	int varying_stream_size; /* 0x18 */
	void *uniform_stream; /* 0x1C */
	int uniform_stream_size; /* 0x20 */
	void *attribute_stream; /* 0x24 */
	int attribute_stream_size; /* 0x28 */
	union {
		struct mali_shader_binary_vertex_parameters vertex;
		struct mali_shader_binary_fragment_parameters fragment;
	} parameters; /* 0x2C - 0x5C */
};

/* values from GL */
#define MALI_SHADER_FRAGMENT 0x8B30
#define MALI_SHADER_VERTEX   0x8B31

int __mali_compile_essl_shader(struct mali_shader_binary *binary, int type,
			       const char *source, int *length, int count);
#endif /* MALI_COMPILER_H */
