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
