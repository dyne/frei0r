/* 
 * spillsupress.c
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
#include <string.h>

#include "frei0r.h"

void green_limited_by_blue(unsigned int len, const uint32_t* inframe, uint32_t* outframe)
{

  unsigned char* dst = (unsigned char*)outframe;
  const unsigned char* src = (unsigned char*)inframe;
  unsigned char b = 0;
  unsigned char g = 0;

  while (len--)
  {
    *dst++ = *src++;
    g = *src++;
    *dst++;
    b = *src++;
    *dst++;
    *dst++ = *src++;

    if( g > b )
    {
      *(dst - 3) = b;
      *(dst - 2) = b;
    }
    else
    {
      *(dst - 3) = g;
      *(dst - 2) = b;
    }
  
  }
}

void blue_limited_by_green(unsigned int len, const uint32_t* inframe, uint32_t* outframe)
{

  unsigned char* dst = (unsigned char*)outframe;
  const unsigned char* src = (unsigned char*)inframe;
  unsigned char b = 0;
  unsigned char g = 0;

  while (len--)
  {
    *dst++ = *src++;
    g = *src++;
    *dst++;
    b = *src++;
    *dst++;
    *dst++ = *src++;

    if( b > g )
    {
      *(dst - 3) = g;
      *(dst - 2) = g;
    }
    else
    {
      *(dst - 3) = g;
      *(dst - 2) = b;
    }
  
  }
}

typedef struct spillsupress_instance
{
  unsigned int width;
  unsigned int height;
  double supress_type; /* type of spill supression applied to image 
                        <= 0.5, green supress
                        > 0.5, blue supress */
} spillsupress_instance_t;

int f0r_init()
{
  return 1;
}

void f0r_deinit()
{ /* no initialization required */ }

void f0r_get_plugin_info(f0r_plugin_info_t* spillsupress_info)
{
  spillsupress_info->name = "spillsupress";
  spillsupress_info->author = "Janne Liljeblad";
  spillsupress_info->plugin_type = F0R_PLUGIN_TYPE_FILTER;
  spillsupress_info->color_model = F0R_COLOR_MODEL_RGBA8888;
  spillsupress_info->frei0r_version = FREI0R_MAJOR_VERSION;
  spillsupress_info->major_version = 0; 
  spillsupress_info->minor_version = 1; 
  spillsupress_info->num_params =  1; 
  spillsupress_info->explanation = "Remove green or blue spill light from subjects shot in front of green or blue screen";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
  switch(param_index)
  {
  case 0:
    info->name = "supresstype";
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "Defines if green or blue screen spill supress is applied";
    break;
  }
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
	spillsupress_instance_t* inst = (spillsupress_instance_t*)calloc(1, sizeof(*inst));
	inst->width = width; 
  inst->height = height;
	inst->supress_type = 0.0; // default supress type is green supress
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
  spillsupress_instance_t* inst = (spillsupress_instance_t*)instance;

  switch(param_index)
  {
  case 0:
    inst->supress_type = *((double*)param);
    break;
  }
}

void f0r_get_param_value(f0r_instance_t instance,
                         f0r_param_t param, int param_index)
{
  assert(instance);
  spillsupress_instance_t* inst = (spillsupress_instance_t*)instance;
  
  switch(param_index)
  {
  case 0:
    *((double*)param) = inst->supress_type;
    break;
  }
}

void f0r_update(f0r_instance_t instance, double time,
                const uint32_t* inframe, uint32_t* outframe)
{
  assert(instance);
  spillsupress_instance_t* inst = (spillsupress_instance_t*)instance;
  unsigned int len = inst->width * inst->height;

  if (inst->supress_type > 0.5)
  {
    blue_limited_by_green(len, inframe, outframe);
  }
  else
  {
    green_limited_by_blue(len, inframe, outframe);
  }
}

