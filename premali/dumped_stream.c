/*
 *
 * This file contains the dumped memory and ioctl contents from our current
 * test.c program, as submitted at GP job start time.
 *
 */

_mali_uk_gp_start_job_s gp_job = {
	.frame_registers = {
		0x40000400, /* 0x00: VS commands start */
		0x40000500, /* 0x01: VS commands end */
		0x40000500, /* 0x02: PLBU commands start */
		0x40000600, /* 0x03: PLBU commands end */
		0, /* 0x04: Tile Heap start - stays empty in this dump. */
		0, /* 0x05: Tile Heap end */
	},
};

#if 0
/* A single attribute is bits 0x1C.2 through 0x1C.5 of every instruction,
   bit 0x1C.6 is set when there is an attribute. */

unsigned int unlinked[] = {
	{
		0xad4ad463, 0x478002b5, 0x0147ff80, 0x000a8d30, /* 0x00000000 */
		/*            /\ valid attribute is here. Note that it is 1 here, and 0 after linking. */
		0xad4fda56, 0x038022ce, 0x0007ff80, 0x000ad510, /* 0x00000010 */
		0xb04b02cd, 0x43802ac2, 0xc6462180, 0x000a8d08, /* 0x00000020 */
		/*            /\ attribute 0. */
		/*                       */
		0xad490722, 0x478082b5, 0x0007ff80, 0x000d5700, /* 0x00000030 */
		0xad4a4980, 0x478002b5, 0x0007ff80, 0x000ad500, /* 0x00000040 */
		0xb5cbcafb, 0x038049d3, 0x0007ff80, 0x000ad500, /* 0x00000050 */
		0x6c8b42b5, 0x03804193, 0x4243c080, 0x000ac508, /* 0x00000060 */
	},
};
#endif


/* vertex shader */
struct mali_dumped_mem_content vertex_shader = {
	0x00000000,
	0x00000070,
	{
		0xad4ad463, 0x438002b5, 0x0147ff80, 0x000a8d30, /* 0x00000000 */
		0xad4fda56, 0x038022ce, 0x0007ff80, 0x000ad510, /* 0x00000010 */
		0xb04b02cd, 0x47802ac2, 0x42462180, 0x000a8d08, /* 0x00000020 */
		0xad490722, 0x438082b5, 0x0007ff80, 0x000d5700, /* 0x00000030 */
		0xad4a4980, 0x438002b5, 0x0007ff80, 0x000ad500, /* 0x00000040 */
		0xb5cbcafb, 0x038049d3, 0x0007ff80, 0x000ad500, /* 0x00000050 */
		0x6c8b42b5, 0x03804193, 0xc643c080, 0x000ac508, /* 0x00000060 */
	}
};

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
/* aVertices: 0x1C0 (0x24) */
/* aColors: 0x200 (0x30) */
/* vertex uniforms: 0x240, 0x40 */

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

/* common: 0x300 (0x100) */
/* vs commands: 0x400, 0x100 */
/* plbu commands 0x500, 0x100 */

struct mali_dumped_mem_block mem_0x40000000 = {
	NULL,
	0x40000000,
	0x00200000,
	3,
	{
		&vertex_shader,
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
