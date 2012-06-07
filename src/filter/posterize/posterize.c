/*
 * This file is a port of com.jhlabs.image.PosterizeFilter.java
 * Copyright 2006 Jerry Huxtable
 *
 * posterize.c
 * Copyright 2012 Janne Liljeblad 
 *
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

typedef struct posterize_instance
{
  unsigned int width;
  unsigned int height;
  double levels;
} posterize_instance_t;

int f0r_init()
{
  return 1;
}

void f0r_deinit()
{ /* no initialization required */ }

void f0r_get_plugin_info(f0r_plugin_info_t* posterize_info)
{
  posterize_info->name = "posterize";
  posterize_info->author = "Janne Liljeblad";
  posterize_info->plugin_type = F0R_PLUGIN_TYPE_FILTER;
  posterize_info->color_model = F0R_COLOR_MODEL_RGBA8888;
  posterize_info->frei0r_version = FREI0R_MAJOR_VERSION;
  posterize_info->major_version = 0; 
  posterize_info->minor_version = 1; 
  posterize_info->num_params =  1; 
  posterize_info->explanation = "Posterizes image by reducing the number of colors used in image";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
  switch(param_index)
  {
  case 0:
    info->name = "levels"; 
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "Number of values per channel";
    break;
  }
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
	posterize_instance_t* inst = (posterize_instance_t*)calloc(1, sizeof(*inst));
	inst->width = width; 
  inst->height = height;
	inst->levels = 5.0 / 48.0;// input range 0 - 1 will be interpreted as levels range 2 - 50
	return (f0r_instance_t)inst;
}

void f0r_destruct(f0r_instance_t instance)
{
  free(instance);
}

void f0r_set_param_value(f0r_instance_t instance, 
                         f0r_param_t param, int param_index)
{
  assert(instance);
  posterize_instance_t* inst = (posterize_instance_t*)instance;

  switch(param_index)
  {
  case 0:
    inst->levels = *((double*)param);
    break;
  }
}

void f0r_get_param_value(f0r_instance_t instance,
                         f0r_param_t param, int param_index)
{
  assert(instance);
  posterize_instance_t* inst = (posterize_instance_t*)instance;
  
  switch(param_index)
  {
  case 0:
    *((double*)param) = inst->levels;
    break;
  }
}

void f0r_update(f0r_instance_t instance, double time,
                const uint32_t* inframe, uint32_t* outframe)
{
  assert(instance);
  posterize_instance_t* inst = (posterize_instance_t*)instance;
  unsigned int len = inst->width * inst->height;

  // convert input value 0.0-1.0 to int value 2-50
  double levelsInput = inst->levels * 48.0;
  levelsInput = CLAMP(levelsInput, 0.0, 48.0) + 2.0;
  int numLevels = (int)levelsInput;

  // create levels table
  unsigned char levels[256];
  int i;
  for (i = 0; i < 256; i++)
  {
		  levels[i] = 255 * (numLevels*i / 256) / (numLevels-1);
  }

  unsigned char* dst = (unsigned char*)outframe;
  const unsigned char* src = (unsigned char*)inframe;
  unsigned char r,g,b = 0;
  while (len--)
  {
    r = *src++;
    g = *src++;
    b = *src++;

    r = levels[r];
    g = levels[g];
    b = levels[b];

    *dst++ = r;
    *dst++ = g;
    *dst++ = b;
    *dst++ = *src++;//copy alpha
  }
}

