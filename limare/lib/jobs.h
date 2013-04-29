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
 * Simplistic job handling.
 */

#ifndef LIMARE_JOBS_H
#define LIMARE_JOBS_H 1

int limare_m200_pp_job_start_direct(struct limare_state *state,
				    struct limare_frame *frame,
				    struct lima_m200_pp_frame_registers
				    *frame_regs,
				    struct lima_pp_wb_registers *wb_regs);

int limare_m400_pp_job_start_r2p1(struct limare_state *state,
				  struct limare_frame *frame,
				  struct lima_m400_pp_frame_registers *
				  frame_regs,
				  struct lima_pp_wb_registers *wb_regs);

int limare_m400_pp_job_start_r3p0(struct limare_state *state,
				  struct limare_frame *frame,
				  struct lima_m400_pp_frame_registers *
				  frame_regs,
				  unsigned int addr_frame[7],
				  unsigned int addr_stack[7],
				  struct lima_pp_wb_registers *wb_regs);
int limare_m400_pp_job_start_r3p1(struct limare_state *state,
				  struct limare_frame *frame,
				  struct lima_m400_pp_frame_registers *
				  frame_regs,
				  unsigned int addr_frame[7],
				  unsigned int addr_stack[7],
				  struct lima_pp_wb_registers *wb_regs);
int limare_m400_pp_job_start_r3p2(struct limare_state *state,
				  struct limare_frame *frame,
				  struct lima_m400_pp_frame_registers *
				  frame_regs,
				  unsigned int addr_frame[7],
				  unsigned int addr_stack[7],
				  struct lima_pp_wb_registers *wb_regs);

void limare_jobs_init(struct limare_state *state);
void limare_jobs_end(struct limare_state *state);

void limare_render_start(struct limare_frame *frame);

#endif /* LIMARE_JOBS_H */
