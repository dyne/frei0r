/* tint0r.c
 * Copyright (C) 2009 Maksim Golovkin (m4ks1k@gmail.com)
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

typedef struct tint0r_instance
{
  unsigned int width;
  unsigned int height;
  f0r_param_color_t blackColor;
  f0r_param_color_t whiteColor;
  double amount; /* the amount value [0, 1] */
} tint0r_instance_t;

int f0r_init()
{
  return 1;
}

void f0r_deinit()
{ /* no initialization required */ }

void f0r_get_plugin_info(f0r_plugin_info_t* tint0r_instance_t)
{
  tint0r_instance_t->name = "Tint0r";
  tint0r_instance_t->author = "Maksim Golovkin";
  tint0r_instance_t->plugin_type = F0R_PLUGIN_TYPE_FILTER;
  tint0r_instance_t->color_model = F0R_COLOR_MODEL_RGBA8888;
  tint0r_instance_t->frei0r_version = FREI0R_MAJOR_VERSION;
  tint0r_instance_t->major_version = 0; 
  tint0r_instance_t->minor_version = 1; 
  tint0r_instance_t->num_params = 3; 
  tint0r_instance_t->explanation = "Tint a source image with specified color";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
  switch(param_index)
  {
  case 0:
    info->name = "Map black to";
    info->type = F0R_PARAM_COLOR;
    info->explanation = "The color to map source color with null luminance";
    break;
  case 1:
	  info->name = "Map white to";
	  info->type = F0R_PARAM_COLOR;
	  info->explanation = "The color to map source color with full luminance";
	  break;
  case 2:
	  info->name = "Tint amount";
	  info->type = F0R_PARAM_DOUBLE;
	  info->explanation = "Amount of color";
	  break;
  }
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
  tint0r_instance_t* inst = (tint0r_instance_t*)calloc(1, sizeof(*inst));
  inst->width = width; inst->height = height;
  inst->amount = .25;
  inst->whiteColor.r = .5;
  inst->whiteColor.g = 1.0;
  inst->whiteColor.b = .5;
  inst->blackColor.r = 0.0;
  inst->blackColor.g = 0.0;
  inst->blackColor.b = 0.0;
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
  tint0r_instance_t* inst = (tint0r_instance_t*)instance;

  switch(param_index)
  {
	case 0:
	  /* black color */
	  inst->blackColor =  *((f0r_param_color_t *)param);
	  break;
	case 1:
	  /* white color */
	  inst->whiteColor =  *((f0r_param_color_t *)param);
	  break;
	case 2:
	  /* amount */
	  inst->amount = *((double *)param);
	  break;
  }
}

void f0r_get_param_value(f0r_instance_t instance,
                         f0r_param_t param, int param_index)
{
  assert(instance);
  tint0r_instance_t* inst = (tint0r_instance_t*)instance;
  
  switch(param_index)
  {
  case 0:
	*((f0r_param_color_t*)param) = inst->blackColor;
	break;
  case 1:
	*((f0r_param_color_t*)param) = inst->whiteColor;
	break;
  case 2:
	*((double *)param) = inst->amount;
	break;
  }
}

unsigned char map_color(double amount, double comp_amount, float color, float luma, float minColor, float maxColor) {
  double val = (comp_amount * color) + amount * (luma * (maxColor - minColor) + minColor);
  return (unsigned char)(255*CLAMP(val, 0, 1));
}

void f0r_update(f0r_instance_t instance, double time,
                const uint32_t* inframe, uint32_t* outframe)
{
  assert(instance);
  tint0r_instance_t* inst = (tint0r_instance_t*)instance;
  unsigned int len = inst->width * inst->height;
  double amount = inst->amount;
  double comp_amount = 1.0 - inst->amount;
  
  unsigned char* dst = (unsigned char*)outframe;
  const unsigned char* src = (unsigned char*)inframe;
  float b, g, r;
  float luma;

  while (len--)
  {
	r = *src++ / 255.;
	g = *src++ / 255.;
	b = *src++ / 255.;
	
	luma = (b * .114 + g * .587 + r * .299);
	
	*dst++ = map_color(amount, comp_amount, r, luma, inst->blackColor.r, inst->whiteColor.r);
	*dst++ = map_color(amount, comp_amount, g, luma, inst->blackColor.g, inst->whiteColor.g);
	*dst++ = map_color(amount, comp_amount, b, luma, inst->blackColor.b, inst->whiteColor.b);

	*dst++ = *src++;  // copy alpha
  }
}

