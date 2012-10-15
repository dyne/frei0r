/*
 * This file is contains sigmoidal transfer function from file plug-ins/common/softglow.c in gimp.
 *
 * sigmoidaltransfer.c
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
#include <stdio.h>
#include <math.h>
#include "frei0r.h"
#include "frei0r_math.h"

#define SIGMOIDAL_BASE   2
#define SIGMOIDAL_RANGE 20

typedef struct sigmoidal_instance
{
  unsigned int width;
  unsigned int height;
  double brightness;
  double sharpness;
} sigmoidal_instance_t;

static inline int gimp_rgb_to_l_int (int red,
                   int green,
                   int blue)
{
  int min, max;

  if (red > green)
    {
      max = MAX (red,   blue);
      min = MIN (green, blue);
    }
  else
    {
      max = MAX (green, blue);
      min = MIN (red,   blue);
    }

  return ROUND ((max + min) / 2.0);
}

void sigmoidal_transfer(f0r_instance_t instance, double time,
                const uint32_t* inframe, uint32_t* outframe)
{
  assert(instance);
  sigmoidal_instance_t* inst = (sigmoidal_instance_t*)instance;
  unsigned int len = inst->width * inst->height;

  double brightness = inst->brightness;
  double sharpness = inst->sharpness;

  const unsigned char* src = (unsigned char*)inframe;
  unsigned char* dst = (unsigned char*)outframe;

  unsigned char luma, r, g, b;
  double val;
  while (len--)
  {
    r = *src++;
    g = *src++;
    b = *src++;

    //desaturate 
    luma = (unsigned char) gimp_rgb_to_l_int (r, g, b);

    //compute sigmoidal transfer
    val = luma / 255.0;
    val = 255.0 / (1 + exp (-(SIGMOIDAL_BASE + (sharpness * SIGMOIDAL_RANGE)) * (val - 0.5)));
    val = val * brightness;
    luma = (unsigned char) CLAMP (val, 0, 255);

    *dst++ = luma;
    *dst++ = luma;
    *dst++ = luma;
    *dst++ = *src++;
  }
}


int f0r_init()
{
  return 1;
}

void f0r_deinit()
{ /* no initialization required */ }

void f0r_get_plugin_info(f0r_plugin_info_t* sigmoidalInfo)
{
  sigmoidalInfo->name = "sigmoidaltransfer";
  sigmoidalInfo->author = "Janne Liljeblad";
  sigmoidalInfo->plugin_type = F0R_PLUGIN_TYPE_FILTER;
  sigmoidalInfo->color_model = F0R_COLOR_MODEL_RGBA8888;
  sigmoidalInfo->frei0r_version = FREI0R_MAJOR_VERSION;
  sigmoidalInfo->major_version = 0; 
  sigmoidalInfo->minor_version = 9; 
  sigmoidalInfo->num_params =  2; 
  sigmoidalInfo->explanation = "Desaturates image and creates a particular look that could be called Stamp, Newspaper or Photocopy";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
	switch ( param_index ) {
		case 0:
			info->name = "brightness";
			info->type = F0R_PARAM_DOUBLE;
			info->explanation = "Brightnesss of image";
			break;
    case 1:
      info->name = "sharpness";
      info->type = F0R_PARAM_DOUBLE;
      info->explanation = "Sharpness of transfer";
      break;
	}
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
  sigmoidal_instance_t* inst = (sigmoidal_instance_t*)calloc(1, sizeof(*inst));
  inst->width = width;
  inst->height = height;
  inst->brightness = 0.75;
  inst->sharpness = 0.85;
  return (f0r_instance_t)inst;
}

void f0r_destruct(f0r_instance_t instance)
{
  free(instance);
}

void f0r_set_param_value(f0r_instance_t instance, 
			 f0r_param_t param, int param_index)
{
	sigmoidal_instance_t* inst = (sigmoidal_instance_t*)instance;
	switch (param_index)
  {
    case 0:
      inst->brightness = *((double*)param);
      break;
    case 1:
      inst->sharpness = *((double*)param);
      break;
  }
}

void f0r_get_param_value(f0r_instance_t instance,
			 f0r_param_t param, int param_index)
{
	sigmoidal_instance_t* inst = (sigmoidal_instance_t*)instance;
	switch (param_index) 
  {
    case 0:
      *((double*)param) = inst->brightness;
      break;
    case 1:
      *((double*)param) = inst->sharpness;
      break;
  }
}

void f0r_update(f0r_instance_t instance, double time,
		const uint32_t* inframe, uint32_t* outframe)
{
  sigmoidal_transfer(instance, time, inframe, outframe);
}

