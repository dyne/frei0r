/* luminance.c
 * Copyright (C) 2004 Jean-Sebastien Senecal (js@drone.ws)
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
#include "frei0r_math.h"

#define MAX_SATURATION 8.0

typedef struct luminance_instance
{
  unsigned int width;
  unsigned int height;
} luminance_instance_t;

int f0r_init()
{
  return 1;
}

void f0r_deinit()
{ /* no initialization required */ }

void f0r_get_plugin_info(f0r_plugin_info_t* luminance_info)
{
  luminance_info->name = "Luminance";
  luminance_info->author = "Richard Spindler";
  luminance_info->plugin_type = F0R_PLUGIN_TYPE_FILTER;
  luminance_info->color_model = F0R_COLOR_MODEL_RGBA8888;
  luminance_info->frei0r_version = FREI0R_MAJOR_VERSION;
  luminance_info->major_version = 0; 
  luminance_info->minor_version = 2; 
  luminance_info->num_params = 0; 
  luminance_info->explanation = "Creates a luminance map of the image";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
  luminance_instance_t* inst = (luminance_instance_t*)calloc(1, sizeof(*inst));
  inst->width = width; inst->height = height;
  return (f0r_instance_t)inst;
}

void f0r_destruct(f0r_instance_t instance)
{
  free(instance);
}

void f0r_set_param_value(f0r_instance_t instance, 
                         f0r_param_t param, int param_index)
{
}

void f0r_get_param_value(f0r_instance_t instance,
                         f0r_param_t param, int param_index)
{
}

void f0r_update(f0r_instance_t instance, double time,
                const uint32_t* inframe, uint32_t* outframe)
{
  assert(instance);
  luminance_instance_t* inst = (luminance_instance_t*)instance;
  unsigned int len = inst->width * inst->height;


  unsigned char* dst = (unsigned char*)outframe;
  const unsigned char* src = (unsigned char*)inframe;

  int b, g, r, l;
  while (len--)
  {
	  r = *src++;
	  g = *src++;
	  b = *src++;
	  l = ( 30 * r + 59 * g + 11 * b ) / 100;


	  *dst++ = (unsigned char) (l);
	  *dst++ = (unsigned char) (l);
	  *dst++ = (unsigned char) (l);

	  *dst++ = *src++;  // copy alpha
  }
}

