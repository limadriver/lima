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
 */

/*
 * Fills in and creates the pp info/job structure.
 */

#ifndef LIMARE_PP_H
#define LIMARE_PP_H 1

struct pp_info
{
	int width;
	int height;
	int pitch;

	int plb_shift_w;
	int plb_shift_h;

	unsigned int clear_color;

	void *shader_address;
	unsigned int shader_physical;
	int shader_size;

	void *render_address;
	unsigned int render_physical;
	int render_size;
};

struct pp_info *pp_info_create(struct limare_state *state,
			       struct limare_frame *frame);
void pp_info_destroy(struct pp_info *pp);

int limare_pp_job_start(struct limare_state *state, struct limare_frame *frame);

#endif /* LIMARE_PP_H */
