/*
 * This file used to contain sigmoidal transfer function from file plug-ins/common/softglow.c in gimp.
 * However, it is now modified to match ImageMagick; i.e. whereas only the steepness of the sigmoidal
 * curves was tunable earlier, now the midpoint of the curve is adjustable as well -- both to a degree.
 *
 * sigmoidaltransfer.c
 * Copyright 2012 Janne Liljeblad, 2025 Cynthia
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
#include "frei0r/math.h"

typedef struct sigmoidal_instance
{
  unsigned int width;
  unsigned int height;
  double base;
  double sharpness;

  /* Precomputed values of the (scaled and shifted) sigmoid function
     is stored in this lookup table. */
  uint8_t lut[256];
} sigmoidal_instance_t;

void gen_sigmoid_lut (uint8_t *const lut, const float base, const float sharpness)
{
  float k = expf(sharpness * 5.0) / 255.0;
  float b = (base - 0.5) * 63.0;

  for (int i = 0; i < 256; ++i)
    lut[i] = CLAMP (255.0 / (1.0 + expf(-k * (i - b - 127.0))), 0, 255.0);
}

void sigmoidal_transfer(f0r_instance_t instance, double time,
                const uint32_t* inframe, uint32_t* outframe)
{
  assert(instance);
  sigmoidal_instance_t* inst = (sigmoidal_instance_t*)instance;
  unsigned int len = inst->width * inst->height;

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
    luma = (unsigned char)(0.299 * r + 0.587 * g + 0.114 * b);
    //compute sigmoidal transfer
    luma = inst->lut[luma];

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
  sigmoidalInfo->author = "Janne Liljeblad & Cynthia";
  sigmoidalInfo->plugin_type = F0R_PLUGIN_TYPE_FILTER;
  sigmoidalInfo->color_model = F0R_COLOR_MODEL_RGBA8888;
  sigmoidalInfo->frei0r_version = FREI0R_MAJOR_VERSION;
  sigmoidalInfo->major_version = 1;
  sigmoidalInfo->minor_version = 0;
  sigmoidalInfo->num_params =  2; 
  sigmoidalInfo->explanation = "Desaturates image and creates a particular look that could be called Stamp, Newspaper or Photocopy";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
	switch ( param_index ) {
		case 0:
			info->name = "base";
			info->type = F0R_PARAM_DOUBLE;
			info->explanation = "Midpoint of sigmoidal curve";
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
  inst->base = 0.5;
  inst->sharpness = 3.0 / 5.0;

  gen_sigmoid_lut (inst->lut, inst->base, inst->sharpness);
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
      inst->base = *((double*)param);
      break;
    case 1:
      inst->sharpness = *((double*)param);
      break;
  }

  gen_sigmoid_lut (inst->lut, inst->base, inst->sharpness);
}

void f0r_get_param_value(f0r_instance_t instance,
			 f0r_param_t param, int param_index)
{
	sigmoidal_instance_t* inst = (sigmoidal_instance_t*)instance;
	switch (param_index) 
  {
    case 0:
      *((double*)param) = inst->base;
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

