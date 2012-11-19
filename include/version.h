/*
 * Copyright (c) 2012 Luc Verhaegen <libv@skynet.be>
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
 */

#ifndef MALI_VERSION_H
#define MALI_VERSION_H 1

/* As reported by the GET_API_VERSION ioctl. */
#define MALI_DRIVER_VERSION_R2P0	6 /* also includes r2p1-rel0 */
#define MALI_DRIVER_VERSION_R2P1	7
#define MALI_DRIVER_VERSION_R2P2	8
#define MALI_DRIVER_VERSION_R2P3	9
#define MALI_DRIVER_VERSION_R2P4	10
#define MALI_DRIVER_VERSION_R3P0	14
#define MALI_DRIVER_VERSION_R3P1	17

/* As reported by the GP2_CORE_VERSION_GET ioctl. */
#define MALI_CORE_GP_200	0x0A07
#define MALI_CORE_GP_300	0x0C07
#define MALI_CORE_GP_400	0x0B07
#define MALI_CORE_GP_450	0x0D07

/* As reported by the PP_CORE_VERSION_GET ioctl. */
#define MALI_CORE_PP_200	0xC807
#define MALI_CORE_PP_300	0xCE07
#define MALI_CORE_PP_400	0xCD07
#define MALI_CORE_PP_450	0xCF07

#endif /* MALI_VERSION_H */
