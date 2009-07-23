/* G.c
 * Copyright (C) 2007 Richard Spindler (richard.spindler@gmail.com)
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

#include "frei0r.h"

typedef struct rgb_instance
{
  unsigned int width;
  unsigned int height;
} rgb_instance_t;

int f0r_init()
{
  return 1;
}

void f0r_deinit()
{ /* no initialization required */ }

void f0r_get_plugin_info(f0r_plugin_info_t* rgbInfo)
{
  rgbInfo->name = "G";
  rgbInfo->author = "Richard Spindler";
  rgbInfo->plugin_type = F0R_PLUGIN_TYPE_FILTER;
  rgbInfo->color_model = F0R_COLOR_MODEL_RGBA8888;
  rgbInfo->frei0r_version = FREI0R_MAJOR_VERSION;
  rgbInfo->major_version = 0; 
  rgbInfo->minor_version = 9; 
  rgbInfo->num_params =  0; 
  rgbInfo->explanation = "Extracts Green from Image";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
  /* no params */
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
  rgb_instance_t* inst = calloc(1, sizeof(*inst));
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

void f0r_update(f0r_instance_t instance, double time,
		const uint32_t* inframe, uint32_t* outframe)
{
  assert(instance);
  rgb_instance_t* inst = (rgb_instance_t*)instance;
  unsigned int w = inst->width;
  unsigned int h = inst->height;
  unsigned int x,y;
  
  uint32_t* dst = outframe;
  const uint32_t* src = inframe;
  for(y=0;y<h;++y)
      for(x=0;x<w;++x,++src) {
	  *dst++ = ( 0xff00ff00 & (*src) ) | ( (0x0000ff00 & (*src)) << 8 ) | ( (0x0000ff00 & (*src)) >> 8 ) ; 
      }
}

