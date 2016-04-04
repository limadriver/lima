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
/*
 * Interface to the shader compiler of libMali.so
 */
#ifndef LIMA_COMPILER_H
#define LIMA_COMPILER_H 1

/* values from GL */
#define LIMA_SHADER_FRAGMENT 0x8B30
#define LIMA_SHADER_VERTEX   0x8B31

struct lima_shader_binary_vertex_parameters { /* 0x24 */
	int unknown00; /* 0x00 */
	int unknown04; /* 0x04 */
	int unknown08; /* 0x08 */
	int unknown0C; /* 0x0C */
	int attribute_count; /* 0x10 */
	int varying_count; /* 0x14 */
	int unknown18; /* 0x18 */
	int size; /* 0x1C, commands are in 4 dwords */
	int attribute_prefetch; /* 0x20 */
};

struct lima_shader_binary_fragment_parameters { /* 0x30 */
	int unknown00; /* 0x00 */
	int first_instruction_size; /* 0x04 */
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

struct lima_shader_binary { /* 0x5C */
#define LIMA_SHADER_COMPILE_STATUS_CLEAR    0x00
#define LIMA_SHADER_COMPILE_STATUS_COMPILED 0x01
	int compile_status; /* 0x00 */
	const char *error_log; /* 0x04 - can be freed */
	const char *oom_log; /* 0x08 - static */
	void *shader; /* 0x0C */
	int shader_size; /* 0x10 */
	const void *varying_stream; /* 0x14 */
	int varying_stream_size; /* 0x18 */
	const void *uniform_stream; /* 0x1C */
	int uniform_stream_size; /* 0x20 */
	const void *attribute_stream; /* 0x24 */
	int attribute_stream_size; /* 0x28 */
	union {
		struct lima_shader_binary_vertex_parameters vertex;
		struct lima_shader_binary_fragment_parameters fragment;
	} parameters; /* 0x2C - 0x5C */
};

/*
 * Introduced in r3p0, adds a full mbs binary and some extra parameters.
 */
struct lima_shader_binary_mbs { /* 0x6C */
	int compile_status; /* 0x00 */
	const char *error_log; /* 0x04 - can be freed */
	const char *oom_log; /* 0x08 - static */
	void *shader; /* 0x0C */
	int shader_size; /* 0x10 */
	const void *mbs_stream; /* 0x14 */
	const int mbs_stream_size; /* 0x18 */
	const void *varying_stream; /* 0x1C */
	int varying_stream_size; /* 0x20 */
	const void *uniform_stream; /* 0x24 */
	int uniform_stream_size; /* 0x28 */
	const void *attribute_stream; /* 0x2C */
	int attribute_stream_size; /* 0x30 */
	union {
		struct lima_shader_binary_vertex_parameters vertex;
		struct lima_shader_binary_fragment_parameters fragment;
	} parameters; /* 0x34 - 0x64 */
	int unknown64; /* 0x64 */
	int unknown68; /* 0x68 */
};

/*
 * Introduced in r3p2, adds something else still.
 */
struct lima_shader_binary_r3p2 { /* 0x6C */
	int compile_status; /* 0x00 */
	const char *error_log; /* 0x04 - can be freed */
	const char *oom_log; /* 0x08 - static */
	void *shader; /* 0x0C */
	int shader_size; /* 0x10 */
	const void *mbs_stream; /* 0x14 */
	const int mbs_stream_size; /* 0x18 */
	const void *varying_stream; /* 0x1C */
	int varying_stream_size; /* 0x20 */
	const void *uniform_stream; /* 0x24 */
	int uniform_stream_size; /* 0x28 */
	const void *attribute_stream; /* 0x2C */
	int attribute_stream_size; /* 0x30 */
	const void *something_stream; /* 0x34 */
	int something_stream_size; /* 0x38 */
	union {
		struct lima_shader_binary_vertex_parameters vertex;
		struct lima_shader_binary_fragment_parameters fragment;
	} parameters; /* 0x3C - 0x6C */
	int unknown6C; /* 0x6C */
	int unknown70; /* 0x70 */
};

/* from libMali.so */
int __mali_compile_essl_shader(struct lima_shader_binary *binary, int type,
			       const char *source, int *length, int count);

#endif /* LIMA_COMPILER_H */
