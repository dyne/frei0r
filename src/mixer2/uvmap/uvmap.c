/* uvmap.c
 * Copyright (C) 2008 Richard Spindler (richard.spindler@gmail.com)
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
#include <assert.h>
#include <math.h>

#include "frei0r.h"

typedef struct uvmap_instance
{
  unsigned int width;
  unsigned int height;
} uvmap_instance_t;

int f0r_init()
{
  return 1;
}

void f0r_deinit()
{ /* no initialization required */ }

void f0r_get_plugin_info(f0r_plugin_info_t* uvmapInfo)
{
  uvmapInfo->name = "UV Map";
  uvmapInfo->author = "Richard Spindler";
  uvmapInfo->plugin_type = F0R_PLUGIN_TYPE_MIXER2;
  uvmapInfo->color_model = F0R_COLOR_MODEL_RGBA8888;
  uvmapInfo->frei0r_version = FREI0R_MAJOR_VERSION;
  uvmapInfo->major_version = 0; 
  uvmapInfo->minor_version = 9; 
  uvmapInfo->num_params =  0; 
  uvmapInfo->explanation = "Uses Input 1 as UV Map to distort Input 2";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
  /* no params */
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
  uvmap_instance_t* inst = (uvmap_instance_t*)calloc(1, sizeof(*inst));
  inst->width = width; inst->height = height;
  return (f0r_instance_t)inst;
}

void f0r_destruct(f0r_instance_t instance)
{
  free(instance);
}

void f0r_set_param_value(f0r_instance_t instance, 
			 f0r_param_t param, int param_index)
{ /* no params */ }

void f0r_get_param_value(f0r_instance_t instance,
			 f0r_param_t param, int param_index)
{ /* no params */ }

void f0r_update2(f0r_instance_t instance,
		 double time,
		 const uint32_t* inframe1,
		 const uint32_t* inframe2,
		 const uint32_t* inframe3,
		 uint32_t* outframe)
{
	assert(instance);
	uvmap_instance_t* inst = (uvmap_instance_t*)instance;
	unsigned int w = inst->width;
	unsigned int h = inst->height;
	unsigned int x,y;

	uint32_t* dst = outframe;
	const uint32_t* uvmap = inframe1;
	const uint32_t* src = inframe2;
	float fx, fy;
	long px, py;
	for( y = 0; y < h; ++y )
		for( x = 0; x < w; ++x ) {
			/* The coordinates start in the lower left corner:
			 *
			 * ^ +-------------+
			 * | |             |
			 * G |             |
			 *  0+-------------+
			 *   0  R ->
			 *
			 */
			unsigned char* tmpc = (unsigned char*)uvmap;
			fx = ((float)tmpc[0]) / 255.0;
			fy = ((float)tmpc[1]) / 255.0;
			fy = 1.0 - fy;

			px = lrintf( w * fx );
			py = lrintf( h * fy );
			if ( tmpc[2] > 128 ) {
				*dst++ = src[px+w*py];
			} else {
				*dst++ = 0x00000000;
			}
			uvmap++;
		}
}


