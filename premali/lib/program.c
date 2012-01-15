/*
 * Copyright 2011-2012 Luc Verhaegen <libv@codethink.co.uk>
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
 * Dealing with shader programs, from compilation to linking.
 */

#include <stdio.h>
#include <string.h>

#include "plb.h"
#include "ioctl.h"
#include "gp.h"
#include "program.h"
#include "compiler.h"

/*
 * Attribute linking:
 *
 * One single attribute per command:
 *  bits 0x1C.2 through 0x1C.5 of every instruction,
 *  bit 0x1C.6 is set when there is an attribute.
 *
 * Before linking, attribute 0 is aColor, and attribute 1 is aPosition,
 * as obviously spotted from the attribute link stream.
 * After linking, attribute 0 is aVertices, and attribute 1 is aColor.
 * This matches the way in which we attach our attributes.
 *
 * And indeed, if we swap the bits and the attributes over, things work fine,
 * and if we only swap the attributes over, then the rendering is broken in a
 * logical way.
 */
/*
 * Varying linking:
 *
 * 4 entries, linked to two varyings. If both varyings are specified, they have
 * to be the same.
 *
 * Bits 47-52, contain 4 entries, 3 bits each. 7 means invalid. 2 entries per
 * varying.
 * Bits 5A through 63 specify two varyings, 5 bits each. Top bit means
 * valid/defined, other is most likely the index in the common area.
 *
 * How this fully fits together, is not entirely clear yet, but hopefully
 * clear enough to implement linking in one of the next stages.
 */
/*
 * Uniform linking:
 *
 * Since there is just a memory blob with uniforms packed in it, it would
 * appear that there is no linking, and uniforms just have to be packed as
 * defined in the uniform link stream.
 *
 */

static void
vertex_shader_attributes_patch(unsigned int *shader, int size)
{
	int i;

	for (i = 0; i < size; i++) {
		int tmp = (shader[4 * i + 1] >> 26) & 0x1F;

		if (!(tmp & 0x10))
			continue;

		tmp &= 0x0F;
		shader[4 * i + 1] &= ~(0x0F << 26);

		/* program specific bit, for now */
		if (!tmp)
			tmp = 1;
		else
			tmp = 0;

		shader[4 * i + 1] |= tmp << 26;
	}
}

static void
vertex_shader_varyings_patch(unsigned int *shader, int size)
{
	int i;

	for (i = 0; i < size; i++) {
		int tmp;

		/* ignore entries for now, until we know more */

		tmp = (shader[4 * i + 2] >> 26) & 0x1F;

		if (tmp & 0x10) {
			tmp &= 0x0F;
			shader[4 * i + 2] &= ~(0x0F << 26);

			/* program specific bit, for now */
			if (!tmp)
				tmp = 1;
			else
				tmp = 0;

			shader[4 * i + 2] |= tmp << 26;
		}

		tmp = ((shader[4 * i + 2] >> 31) & 0x01) |
			((shader[4 * i + 3] << 1) & 0x1E);

		if (tmp & 0x10) {
			tmp &= 0x0F;
			shader[4 * i + 2] &= ~(0x01 << 31);
			shader[4 * i + 3] &= ~0x07;

			/* program specific bit, for now */
			if (!tmp)
				tmp = 1;
			else
				tmp = 0;

			shader[4 * i + 2] |= tmp << 31;
			shader[4 * i + 3] |= tmp >> 1;
		}
	}
}

int
vertex_shader_compile(struct vs_info *info, const char *source)
{
	struct mali_shader_binary binary = {0};
	unsigned int *shader;
	int length = strlen(source);
	int ret, size;

	ret = __mali_compile_essl_shader(&binary, MALI_SHADER_VERTEX,
					 (char *) source, &length, 1);
	if (ret) {
		if (binary.error_log)
			printf("%s: compilation failed: %s\n",
			       __func__, binary.error_log);
		else
			printf("%s: compilation failed: %s\n",
			       __func__, binary.oom_log);
		return ret;
	}

	shader = binary.shader;
	size = binary.shader_size / 16;

	vertex_shader_attributes_patch(shader, size);
	vertex_shader_varyings_patch(shader, size);

	vs_info_attach_shader(info, shader, size);

	return 0;
}

int
fragment_shader_compile(struct plbu_info *info, const char *source)
{
	struct mali_shader_binary binary = {0};
	unsigned int *shader;
	int length = strlen(source);
	int ret, size;

	ret = __mali_compile_essl_shader(&binary, MALI_SHADER_FRAGMENT,
					 (char *) source, &length, 1);
	if (ret) {
		if (binary.error_log)
			printf("%s: compilation failed: %s\n",
			       __func__, binary.error_log);
		else
			printf("%s: compilation failed: %s\n",
			       __func__, binary.oom_log);
		return ret;
	}

	shader = binary.shader;
	size = binary.shader_size / 4;

	plbu_info_attach_shader(info, shader, size);

	return 0;
}
