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
 *
 */

/*
 * Command definitions for the PLBU, which i believe might actuall be:
 * Primitive List Blocking Unit.
 */
#ifndef LIMA_PLBU_H
#define LIMA_PLBU_H 1

#define LIMA_PLBU_CMD_VERTEX_COUNT       0x00000000

#define LIMA_PLBU_CMD_INDEXED_DEST       0x10000100
#define LIMA_PLBU_CMD_INDICES            0x10000101

#define LIMA_PLBU_CMD_TILE_HEAP_START    0x10000103
#define LIMA_PLBU_CMD_TILE_HEAP_END      0x10000104

#define LIMA_PLBU_CMD_VIEWPORT_Y         0x10000105
#define LIMA_PLBU_CMD_VIEWPORT_H         0x10000106
#define LIMA_PLBU_CMD_VIEWPORT_X         0x10000107
#define LIMA_PLBU_CMD_VIEWPORT_W         0x10000108

/* width & height in blocks: (width - 1) << 24 | (height - 1) << 8 */
#define LIMA_PLBU_CMD_TILED_DIMENSIONS   0x10000109

/*
 * bits 8-11: block scale (0x80 << block_scale = block_size)
 * bit 13: set with gles v2.
 *
 * bit 17: CW culling.
 * bit 18: CCW culling.
 *         Set these according to cull face mode, and front face value.
 */
#define LIMA_PLBU_CMD_PRIMITIVE_SETUP    0x1000010B
#define LIMA_PLBU_CMD_PRIMITIVE_CULL_CCW         0x00040000

/* Determines how to map tiles to blocks, in shifted values.
 *
 * 0x00020001 translates to:
 * ((1 << 2) tiles horizontally) * ((1 << 3) tiles vertically) per block.
 */
#define LIMA_PLBU_CMD_BLOCK_STEP         0x1000010C

#define LIMA_PLBU_CMD_DEPTH_RANGE_NEAR   0x1000010E
#define LIMA_PLBU_CMD_DEPTH_RANGE_FAR    0x1000010F

#define LIMA_M200_PLBU_CMD_PLBU_ARRAY_ADDRESS 0x20000000
#define LIMA_M400_PLBU_CMD_PLBU_ARRAY_ADDRESS 0x28000000

/* tiled_width / width_step */
#define LIMA_PLBU_CMD_PLBU_BLOCK_STRIDE  0x30000000

#define LIMA_PLBU_CMD_END                0x50000000

#define LIMA_PLBU_CMD_ARRAYS_SEMAPHORE   0x60000000
#define LIMA_PLBU_CMD_ARRAYS_SEMAPHORE_BEGIN       0x00010002
#define LIMA_PLBU_CMD_ARRAYS_SEMAPHORE_END         0x00010001


#define LIMA_PLBU_CMD_SCISSORS           0x70000000

#define LIMA_PLBU_CMD_RSW_VERTEX_ARRAY   0x80000000

#define LIMA_PLBU_CMD_CONTINUE           0xF0000000


#endif /* LIMA_PLBU_H */
