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
/*
 * Command definitions for the PLBU, which i believe might actuall be:
 * Primitive List Blocking Unit.
 */
#ifndef MALI_PLBU_H
#define MALI_PLBU_H 1

#define MALI_PLBU_CMD_TILE_HEAP_START    0x10000103
#define MALI_PLBU_CMD_TILE_HEAP_END      0x10000104

#define MALI_PLBU_CMD_VIEWPORT_Y         0x10000105
#define MALI_PLBU_CMD_VIEWPORT_H         0x10000106
#define MALI_PLBU_CMD_VIEWPORT_X         0x10000107
#define MALI_PLBU_CMD_VIEWPORT_W         0x10000108

/* width & height in blocks: (width - 1) << 24 | (height - 1) << 8 */
#define MALI_PLBU_CMD_TILED_DIMENSIONS   0x10000109

/*
 * bits 8-11: block scale (0x80 << block_scale = block_size)
 * bit 13: set with gles v2.
 *
 * bit 17: CW culling.
 * bit 18: CCW culling.
 *         Set these according to cull face mode, and front face value.
 */
#define MALI_PLBU_CMD_PRIMITIVE_SETUP    0x1000010B

/* Determines how to map tiles to blocks, in shifted values.
 *
 * 0x00020001 translates to:
 * ((1 << 2) tiles horizontally) * ((1 << 3) tiles vertically) per block.
 */
#define MALI_PLBU_CMD_BLOCK_STEP         0x1000010C

#define MALI_PLBU_CMD_DEPTH_RANGE_NEAR   0x1000010E
#define MALI_PLBU_CMD_DEPTH_RANGE_FAR    0x1000010F

#define MALI_PLBU_CMD_PLBU_ARRAY_ADDRESS 0x20000000

/* tiled_width / width_step */
#define MALI_PLBU_CMD_PLBU_BLOCK_STRIDE  0x30000000

#define MALI_PLBU_CMD_END                0x50000000

#define MALI_PLBU_CMD_ARRAYS_SEMAPHORE   0x60000000
#define MALI_PLBU_CMD_ARRAYS_SEMAPHORE_BEGIN       0x00010002
#define MALI_PLBU_CMD_ARRAYS_SEMAPHORE_END         0x00010001

#define MALI_PLBU_CMD_RSW_VERTEX_ARRAY   0x80000000

#endif /* MALI_PLBU_H */
