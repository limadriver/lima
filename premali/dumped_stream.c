/*
 *
 * This file contains the dumped memory and ioctl contents from our current
 * test.c program, as submitted at GP job start time.
 *
 */

#define RENDER_WIDTH  384
#define RENDER_HEIGHT 256

_mali_uk_gp_start_job_s gp_job = {
	.frame_registers = {
		0x400fdcc0, /* 0x00: VS commands start */
		0x400fdd18, /* 0x01: VS commands end */
		0x400fbcc0, /* 0x02: PLBU commands start */
		0x400fbe20, /* 0x03: PLBU commands end */
		0x40100000, /* 0x04: Tile Heap start - stays empty in this dump. */
		0x40150000, /* 0x05: Tile Heap end */
	},
};

_mali_uk_pp_start_job_s pp_job = {
	.frame_registers = {
		0x400f82c0, /* 0x00: Primitive list block stream address. */
		0x400e01c0, /* 0x01: Address, 0x4000 large, only partly dumped though */
		0x00000000, /* 0x02: unused */
		0x00000022, /* 0x03: flags */
		0x00ffffff, /* 0x04: clear value depth */
		0x00000000, /* 0x05: clear value stencil */
		0xff000000, /* 0x06: clear value color */
		0xff000000, /* 0x07: -> frame[0x06] */
		0xff000000, /* 0x08: -> frame[0x06] */
		0xff000000, /* 0x09: -> frame[0x06] */
		0x00000000, /* 0x0A: width - 1 */
		0x00000000, /* 0x0B: height - 1 */
		0x40034400, /* 0x0C: fragment stack address, empty in this dump. */
		0x00010001, /* 0x0D: (fragment stack start) << 16 | (fragment stack end) */
		0x00000000, /* 0x0E: unused */
		0x00000000, /* 0x0F: unused */
		0x00000001, /* 0x10: always set to 1 */
		0x000001ff, /* 0x11: if (frame[0x13] == 1) then ((height << supersample scale factor) - 1) else 1 */
		0x00000077, /* 0x12: always set to 0x77 */
		0x00000001, /* 0x13: set to 1 */
	},
	.wb0_registers = {
		0x00000002, /* 0: type */
		0x40080000, /* 1: address */
		0x00000003, /* 2: pixel format */
		0x00000000, /* 3: downsample factor */
		0x00000000, /* 4: pixel layout */
		0x000000c0, /* 5: pitch / 8 */
		0x00000000, /* 6: Multiple Render Target flags: bits 0-3 */
		0x00000000, /* 7: MRT offset */
		0x00000000, /* 8: zeroed */
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
struct mali_dumped_mem_content mem_0x40000000_0x00000001 = {
	0x00004240, /* 0x40004240 */
	0x00000070,
	{
		0xad4ad463, 0x438002b5, 0x0147ff80, 0x000a8d30, /* 0x00004240 */
		0xad4fda56, 0x038022ce, 0x0007ff80, 0x000ad510, /* 0x00004250 */
		0xb04b02cd, 0x47802ac2, 0x42462180, 0x000a8d08, /* 0x00004260 */
		0xad490722, 0x438082b5, 0x0007ff80, 0x000d5700, /* 0x00004270 */
		0xad4a4980, 0x438002b5, 0x0007ff80, 0x000ad500, /* 0x00004280 */
		0xb5cbcafb, 0x038049d3, 0x0007ff80, 0x000ad500, /* 0x00004290 */
		0x6c8b42b5, 0x03804193, 0xc643c080, 0x000ac508, /* 0x000042A0 */
	}
};

/* fragment shader */
struct mali_dumped_mem_content mem_0x40000000_0x00000002 = {
	0x000042C0, /* 0x400042C0 */
	0x0000000C,
	{
		0x000000a3, 0xf0003c60, 0x00000000,             /* 0x000042C0 */
	}
};

struct mali_dumped_mem_block mem_0x40000000 = {
	NULL,
	0x40000000,
	0x00040000,
	2,
	{
		&mem_0x40000000_0x00000001,
		&mem_0x40000000_0x00000002,
	},
};

/* dummy quad */
struct mali_dumped_mem_content mem_0x40080000_0x00000000 = { /* 0x40 */
	0x00060000, /* 0x400e0000 */
	0x00000014,
	{
		0x00020425, 0x0000000c, 0x01e007cf, 0xb0000000, /* 0x00060000 */
		0x000005f5,                                     /* 0x00060010 */
	}
};

struct mali_dumped_mem_content mem_0x40080000_0x00000002 = {
	0x000601c0, /* 0x400e01c0 */
	0x00000040, /* of 0x4000 */
	{
		0x00000000, 0x00000000, 0x00000000, 0x00000000, /* 0x000601C0 */
		0x00000000, 0x00000000, 0x00000000, 0x00000000, /* 0x000601D0 */
		/*            \/ dummy quad address | size */
		0x0000f008, 0x400e0005, 0x00000000, 0x00000000, /* 0x000601E0 */
		0x00000000, 0x00000100, 0x00000000, 0x00000000, /* 0x000601F0 */
	}
};

struct mali_dumped_mem_content mem_0x40080000_0x00000003 = {
	0x00068340, /* 0x400e8340, starts at 0x400e82c0 */
	0x000001e4, /* of 0x10000 */
	{
		/* RSW */
		0x00000000, 0x00000000, 0xfc3b1ad2, 0x0000003e, /* 0x00068340 */
		0xffff0000, 0x00000007, 0x00000007, 0x00000000, /* 0x00068350 */
		0x0000f807, 0x400042c3, 0x00000002, 0x00000000, /* 0x00068360 */
		0x00000000, 0x00000301, 0x00002000, 0x400e8300, /* 0x00068370 */
		0, 0, 0, 0, /* 0x00068380 */
		0, 0, 0, 0, /* 0x00068390 */
		0, 0, 0, 0, /* 0x000683A0 */
		0, 0, 0, 0, /* 0x000683B0 */
		/* attributes */
		/* \/ reference to vVertices, then, ((entry_size << 11) | (count - 1)) */
		/*                          \/ reference to vColor */
		0x400e8500, 0x00006002, 0x400e84c0, 0x00008003, /* 0x000683C0 */
		0x00000000, 0x0000003f, 0x00000000, 0x0000003f, /* 0x000683D0 */
		0x00000000, 0x0000003f, 0x00000000, 0x0000003f, /* 0x000683E0 */
		0x00000000, 0x0000003f, 0x00000000, 0x0000003f, /* 0x000683F0 */
		0x00000000, 0x0000003f, 0x00000000, 0x0000003f, /* 0x00068400 */
		0x00000000, 0x0000003f, 0x00000000, 0x0000003f, /* 0x00068410 */
		0x00000000, 0x0000003f, 0x00000000, 0x0000003f, /* 0x00068420 */
		0x00000000, 0x0000003f, 0x00000000, 0x0000003f, /* 0x00068430 */
		/* \/ Varyings, then ((entry_size << 11) | (count - 1)) */
		/*                        \/ Vertex array */
		/* Could 0x8020 be: 16 * 1 and 0x20 marks the end? */
		0x400e8300, 0x0000400f, 0x400e82c0, 0x00008020, /* 0x00068440 */
		0x00000000, 0x0000003f, 0x00000000, 0x0000003f, /* 0x00068450 */
		0x00000000, 0x0000003f, 0x00000000, 0x0000003f, /* 0x00068460 */
		0x00000000, 0x0000003f, 0x00000000, 0x0000003f, /* 0x00068470 */
		0x00000000, 0x0000003f, 0x00000000, 0x0000003f, /* 0x00068480 */
		0x00000000, 0x0000003f, 0x00000000, 0x0000003f, /* 0x00068490 */
		0x00000000, 0x0000003f, 0x00000000, 0x0000003f, /* 0x000684A0 */
		0x00000000, 0x0000003f, 0x00000000, 0x0000003f, /* 0x000684B0 */
		/* vColors */
		0x3f800000, 0x00000000, 0x00000000, 0x3f800000, /* 0x000684C0 */
		0x00000000, 0x3f800000, 0x00000000, 0x3f800000, /* 0x000684D0 */
		0x00000000, 0x00000000, 0x3f800000, 0x3f800000, /* 0x000684E0 */
		0x00000000, 0x00000000, 0x00000000, 0x00000000, /* 0x000684F0 */
		/* vVertices */
		0x00000000, 0x3f000000, 0x00000000, 0xbf000000, /* 0x00068500 */
		0xbf000000, 0x00000000, 0x3f000000, 0xbf000000, /* 0x00068510 */
		0x00000000,
	}
};

struct mali_dumped_mem_block mem_0x40080000 = {
	NULL,
	0x40080000,
	0x00080000,
	3,
	{
		&mem_0x40080000_0x00000000,
		&mem_0x40080000_0x00000002,
		&mem_0x40080000_0x00000003,
	},
};

/* Tile Heap -- Empty. */
struct mali_dumped_mem_block mem_0x40100000 = {
	NULL,
	0x40100000,
	0x00080000,
	0x00000000,
	{
	},
};

struct mali_dumped_mem_block mem_0x40180000 = {
	NULL,
	0x40180000,
	0x00080000,
	0x00000000,
	{
	},
};

struct mali_dumped_mem dumped_mem = {
	0x00000004,
	{
		&mem_0x40000000,
		&mem_0x40080000,
		&mem_0x40100000,
		&mem_0x40180000,
	},
};
