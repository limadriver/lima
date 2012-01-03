/*
 *
 * This file contains the dumped memory and ioctl contents from our current
 * test.c program, as submitted at GP job start time.
 *
 */

/* fragment shader */
struct mali_dumped_mem_content fragment_shader = {
	0x00000080,
	0x0000000C,
	{
		0x000000a3, 0xf0003c60, 0x00000000,             /* 0x00000000 */
	}
};

/* vertex_array: 0x140 (0x40) */
/* varyings: 0x180 (0x40) */

struct mali_dumped_mem_content rsw = {
	0x00000280,
	0x00000040,
	{
		/* RSW */
		0x00000000, 0x00000000, 0xfc3b1ad2, 0x0000003e, /* 0x00000000 */
		0xffff0000, 0x00000007, 0x00000007, 0x00000000, /* 0x00000010 */
		/*              \/ fragment shader address | size */
		0x0000f807, 0x40000083, 0x00000002, 0x00000000, /* 0x00000020 */
		/*                                    \/ varyings address */
		0x00000000, 0x00000301, 0x00002000, 0x40000180, /* 0x00000030 */
	}
};

/* plbu commands 0x500, 0x100 */

struct mali_dumped_mem_block mem_0x40000000 = {
	NULL,
	0x40000000,
	0x00200000,
	2,
	{
		&fragment_shader,
		&rsw,
	},
};

struct mali_dumped_mem dumped_mem = {
	0x00000001,
	{
		&mem_0x40000000,
	},
};
