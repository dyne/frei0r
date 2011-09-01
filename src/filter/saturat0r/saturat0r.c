/* saturat0r.c
 * Copyright (C) 2004 Jean-Sebastien Senecal (js@drone.ws)
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

typedef struct saturat0r_instance
{
  unsigned int width;
  unsigned int height;
  double saturation; /* the saturation value [0, 1] */
} saturat0r_instance_t;

int f0r_init()
{
  return 1;
}

void f0r_deinit()
{ /* no initialization required */ }

void f0r_get_plugin_info(f0r_plugin_info_t* saturat0r_info)
{
  saturat0r_info->name = "Saturat0r";
  saturat0r_info->author = "Jean-Sebastien Senecal";
  saturat0r_info->plugin_type = F0R_PLUGIN_TYPE_FILTER;
  saturat0r_info->color_model = F0R_COLOR_MODEL_RGBA8888;
  saturat0r_info->frei0r_version = FREI0R_MAJOR_VERSION;
  saturat0r_info->major_version = 0; 
  saturat0r_info->minor_version = 3; 
  saturat0r_info->num_params =  1; 
  saturat0r_info->explanation = "Adjusts the saturation of a source image";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
  switch(param_index)
  {
  case 0:
    info->name = "Saturation";
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "The saturation value";
    break;
  }
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
  saturat0r_instance_t* inst = (saturat0r_instance_t*)calloc(1, sizeof(*inst));
  inst->width = width; inst->height = height;
  inst->saturation=1.0/MAX_SATURATION;
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
  saturat0r_instance_t* inst = (saturat0r_instance_t*)instance;

  switch(param_index)
  {
  case 0:
    /* saturations */
    inst->saturation =  *((double*)param);
    break;
  }
}

void f0r_get_param_value(f0r_instance_t instance,
                         f0r_param_t param, int param_index)
{
  assert(instance);
  saturat0r_instance_t* inst = (saturat0r_instance_t*)instance;
  
  switch(param_index)
  {
  case 0:
    *((double*)param) = inst->saturation;
    break;
  }
}

void f0r_update(f0r_instance_t instance, double time,
                const uint32_t* inframe, uint32_t* outframe)
{
  assert(instance);
  saturat0r_instance_t* inst = (saturat0r_instance_t*)instance;
  unsigned int len = inst->width * inst->height;
  double saturation = inst->saturation * MAX_SATURATION;
  
  unsigned char* dst = (unsigned char*)outframe;
  const unsigned char* src = (unsigned char*)inframe;

  double one_minus_saturation = 1.0-saturation;
  int bwgt = (int)(7471.0  * one_minus_saturation);
  int gwgt = (int)(38470.0 * one_minus_saturation);
  int rwgt = (int)(19595.0 * one_minus_saturation);

  int b, g, r, bw;

  if (0 <= saturation && saturation <=1) // optimisation: no clamping needed
  {
    while (len--)
    {
      b = *src++;
      g = *src++;
      r = *src++;
      
      bw = (b*bwgt + g*gwgt + r*rwgt) >> 16;
      
      *dst++ = (unsigned char) (bw + b*saturation);
      *dst++ = (unsigned char) (bw + g*saturation);
      *dst++ = (unsigned char) (bw + r*saturation);

      *dst++ = *src++;  // copy alpha
    }
  }
  else
  {
    while (len--)
    {
      b = *src++;
      g = *src++;
      r = *src++;

      bw = (b*bwgt + g*gwgt + r*rwgt) >> 16;
      
      *dst++ = CLAMP0255((int) (bw + b*saturation));
      *dst++ = CLAMP0255((int) (bw + g*saturation));
      *dst++ = CLAMP0255((int) (bw + r*saturation));

      *dst++ = *src++;  // copy alpha
    }
  }
}

