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
 */

/*
 * Fills in and creates the pp info/job structure.
 */

#ifndef PREMALI_PP_H
#define PREMALI_PP_H 1

struct pp_info
{
	union {
		struct mali200_pp_job_start *mali200;
		struct mali400_pp_job_start *mali400;
	} job;

	int width;
	int height;
	int pitch;

	unsigned int plb_physical;
	int plb_shift_w;
	int plb_shift_h;

	unsigned int clear_color;

	unsigned int *quad_address;
	unsigned int quad_physical;
	int quad_size;

	unsigned int *render_address;
	unsigned int render_physical;
	int render_size;

	/* final render, separately mapped */
	void *frame_address;
	unsigned int frame_physical;
	int frame_size;
};

struct pp_info *pp_info_create(struct premali_state *state, void *address,
			       unsigned int physical, int size,
			       unsigned int frame_physical);
int premali_pp_job_start(struct premali_state *state, struct pp_info *info);

#endif /* PREMALI_PP_H */
