/*
 * Copyright (c) 2011-2012 Luc Verhaegen <libv@codethink.co.uk>
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
 * Command definitions for the VS
 */
#ifndef MALI_VS_H
#define MALI_VS_H 1

#define MALI_VS_CMD_ARRAYS_SEMAPHORE   0x50000000
#define MALI_VS_CMD_ARRAYS_SEMAPHORE_BEGIN_1     0x00028000
#define MALI_VS_CMD_ARRAYS_SEMAPHORE_BEGIN_2     0x00000001
#define MALI_VS_CMD_ARRAYS_SEMAPHORE_END         0x00000000

#define MALI_VS_CMD_SHADER_ADDRESS     0x40000000

#define MALI_VS_CMD_UNIFORMS_ADDRESS   0x30000000
#define MALI_VS_CMD_COMMON_ADDRESS     0x20000000 /* mali200 */
#define MALI_VS_CMD_ATTRIBUTES_ADDRESS 0x20000000 /* mali400 */
#define MALI_VS_CMD_VARYINGS_ADDRESS   0x20000008 /* mali400 */

#define MALI_VS_CMD_VARYING_ATTRIBUTE_COUNT 0x10000042

#endif /* MALI_VS_H */
