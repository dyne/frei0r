/* lenscorrection.c
 * Copyright (C) 2007 Richard Spindler
 * This file is a Frei0r plugin.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include "frei0r.h"
#include "frei0r/math.h"

typedef struct lenscorrection_instance
{
  unsigned int width;
  unsigned int height;
  double xcenter;
  double ycenter;
  double correctionnearcenter;
  double correctionnearedges;
  double brightness;
  int32_t *map;
  int map_dirty;
} lenscorrection_instance_t;


static void rebuild_map(lenscorrection_instance_t* inst)
{
	int w = (int)inst->width;
	int h = (int)inst->height;
	double normallise_radius_sq = 4.0 / (inst->width * inst->width + inst->height * inst->height);
	double xcenter = inst->width * inst->xcenter;
	double ycenter = inst->height * inst->ycenter;
	double mult_sq = (inst->correctionnearcenter - 0.5);
	double mult_qd = (inst->correctionnearedges - 0.5);
	int32_t *map = inst->map;
	int x, y;

	for (y = 0; y < h; y++) {
		double off_y = y - ycenter;
		double off_y2 = off_y * off_y;
		int32_t *row = map + (size_t)y * w;
		for (x = 0; x < w; x++) {
			double off_x = x - xcenter;
			double radius_sq = ((off_x * off_x) + off_y2) * normallise_radius_sq;
			double radius_mult = radius_sq * mult_sq + radius_sq * radius_sq * mult_qd + 1.0;
			int sx = (int)(xcenter + radius_mult * off_x);
			int sy = (int)(ycenter + radius_mult * off_y);
			if (sx < 0 || sy < 0 || sx >= w || sy >= h) {
				row[x] = -1;
			} else {
				row[x] = sx + sy * w;
			}
		}
	}
	inst->map_dirty = 0;
}

int f0r_init()
{
  return 1;
}

void f0r_deinit()
{ /* no initialization required */ }

void f0r_get_plugin_info(f0r_plugin_info_t* lenscorrection_info)
{
  lenscorrection_info->name = "Lens Correction";
  lenscorrection_info->author = "Richard Spindler";
  lenscorrection_info->plugin_type = F0R_PLUGIN_TYPE_FILTER;
  lenscorrection_info->color_model = F0R_COLOR_MODEL_RGBA8888;
  lenscorrection_info->frei0r_version = FREI0R_MAJOR_VERSION;
  lenscorrection_info->major_version = 0; 
  lenscorrection_info->minor_version = 2; 
  lenscorrection_info->num_params =  5; 
  lenscorrection_info->explanation = "Allows compensation of lens distortion";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
	switch(param_index)
	{
		case 0:
			info->name = "X center";
			info->type = F0R_PARAM_DOUBLE;
			info->explanation = "";
			break;
		case 1:
			info->name = "Y center";
			info->type = F0R_PARAM_DOUBLE;
			info->explanation = "";
			break;
		case 2:
			info->name = "Correction near center";
			info->type = F0R_PARAM_DOUBLE;
			info->explanation = "";
			break;
		case 3:
			info->name = "Correction near edges";
			info->type = F0R_PARAM_DOUBLE;
			info->explanation = "";
			break;
		case 4:
			info->name = "Brightness";
			info->type = F0R_PARAM_DOUBLE;
			info->explanation = "";
			break;
	}
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
  lenscorrection_instance_t* inst = (lenscorrection_instance_t*)calloc(1, sizeof(*inst));
  inst->width = width; inst->height = height;

  inst->xcenter = 0.5;
  inst->ycenter = 0.5;
  inst->correctionnearcenter = 0.5;
  inst->correctionnearedges = 0.5;
  inst->brightness = 0.5;
  inst->map = (int32_t*)malloc((size_t)width * height * sizeof(int32_t));
  inst->map_dirty = 1;
  return (f0r_instance_t)inst;
}

void f0r_destruct(f0r_instance_t instance)
{
  lenscorrection_instance_t* inst = (lenscorrection_instance_t*)instance;
  free(inst->map);
  free(instance);
}

void f0r_set_param_value(f0r_instance_t instance, 
		f0r_param_t param, int param_index)
{
	assert(instance);
	lenscorrection_instance_t* inst = (lenscorrection_instance_t*)instance;

	switch(param_index)
	{
		double val;
		case 0:
			val = *((double*)param);
			if (val != inst->xcenter) { inst->xcenter = val; inst->map_dirty = 1; }
			break;
		case 1:
			val = *((double*)param);
			if (val != inst->ycenter) { inst->ycenter = val; inst->map_dirty = 1; }
			break;
		case 2:
			val = *((double*)param);
			if (val != inst->correctionnearcenter) { inst->correctionnearcenter = val; inst->map_dirty = 1; }
			break;
		case 3:
			val = *((double*)param);
			if (val != inst->correctionnearedges) { inst->correctionnearedges = val; inst->map_dirty = 1; }
			break;
		case 4:
			val = *((double*)param);
			inst->brightness = val;
			break;
	}
}

void f0r_get_param_value(f0r_instance_t instance,
		f0r_param_t param, int param_index)
{
	assert(instance);
	lenscorrection_instance_t* inst = (lenscorrection_instance_t*)instance;

	switch(param_index)
	{
		case 0:
			*((double*)param) = inst->xcenter;
			break;
		case 1:
			*((double*)param) = inst->ycenter;
			break;
		case 2:
			*((double*)param) = inst->correctionnearcenter;
			break;
		case 3:
			*((double*)param) = inst->correctionnearedges;
			break;
		case 4:
			*((double*)param) = inst->brightness;
			break;
	}
}

void f0r_update(f0r_instance_t instance, double time,
		const uint32_t* inframe, uint32_t* outframe)
{
	assert(instance);
	lenscorrection_instance_t* inst = (lenscorrection_instance_t*)instance;

	if (inst->map_dirty) {
		rebuild_map(inst);
	}

	size_t len = (size_t)inst->width * inst->height;
	const int32_t *map = inst->map;
	size_t i;
	for (i = 0; i < len; i++) {
		int32_t idx = map[i];
		outframe[i] = (idx >= 0) ? inframe[idx] : 0x00000000;
	}
}
