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

#ifndef PREMALI_GP_H
#define PREMALI_GP_H 1

struct gp_common_entry {
	unsigned int physical;
	int size; /* (element_size << 11) | (element_count - 1) */
};

struct gp_common {
	struct gp_common_entry attributes[0x10];
	struct gp_common_entry varyings[0x10];
};

struct vs_info {
	void *mem_address;
	int mem_physical;
	int mem_used;
	int mem_size;

	/* for mali200: attributes and varyings are shared. */
	struct gp_common *common;
	int common_offset;
	int common_size;

	/* for mali400 */
	struct gp_common_entry *attribute_area;
	int attribute_area_offset;
	int attribute_area_size;

	/* for mali400 */
	struct gp_common_entry *varying_area;
	int varying_area_offset;
	int varying_area_size;

	int uniform_offset;
	int uniform_size;

	struct symbol *attributes[0x10];
	int attribute_count;

	/* fragment shader can only take up to 13 varyings. */
	struct symbol *varyings[13];
	int varying_count;
	int varying_element_size;

	unsigned int *shader;
	int shader_offset;
	int shader_size;
};

struct vs_info *vs_info_create(struct premali_state *sate, void *address, int physical, int size);

int vs_info_attach_uniforms(struct vs_info *info, struct symbol **uniforms,
			    int count, int size);

int vs_info_attach_attribute(struct vs_info *info, struct symbol *attribute);
int vs_info_attach_varying(struct vs_info *info, struct symbol *varying);
int vs_info_attach_shader(struct vs_info *info, unsigned int *shader, int size);

void vs_commands_create(struct premali_state *state, int vertex_count);
void vs_info_finalize(struct premali_state *state, struct vs_info *info);

struct plbu_info {
	void *mem_address;
	int mem_physical;
	int mem_used;
	int mem_size;

	struct render_state *render_state;
	int render_state_offset;
	int render_state_size;

	unsigned int *shader;
	int shader_offset;
	int shader_size;

	int uniform_array_offset;
	int uniform_array_size;

	int uniform_offset;
	int uniform_size;
};

int vs_command_queue_create(struct premali_state *state, int offset, int size);
int plbu_command_queue_create(struct premali_state *state, int offset, int size);

void plbu_commands_draw_add(struct premali_state *state, int draw_mode,
			    int start, int count);
struct plbu_info *plbu_info_create(void *address, int physical, int size);


int plbu_info_attach_shader(struct plbu_info *info, unsigned int *shader, int size);
int plbu_info_attach_uniforms(struct plbu_info *info, struct symbol **uniforms,
			    int count, int size);

int plbu_info_render_state_create(struct plbu_info *info, struct vs_info *vs);
int plbu_info_finalize(struct premali_state *state, int draw_mode,
		       int start, int count);

int premali_gp_job_start(struct premali_state *state);

#endif /* PREMALI_GP_H */
