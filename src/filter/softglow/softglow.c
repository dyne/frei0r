/*
 * This file is a modified port of Softglow plug-in from Gimp.
 * It contains code from plug-ins/common/softglow.c, see that for copyrights.
 *
 * softglow.c
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
#include <math.h>
#include "frei0r.h"
#include "blur.h"
#include "frei0r_math.h"

#define SIGMOIDAL_BASE   2
#define SIGMOIDAL_RANGE 20
#define NBYTES 4
#define ALPHA 3

double PI=3.14159265358979;

typedef struct softglow_instance
{
  unsigned int width;
  unsigned int height;
	double blur;
  double brightness;
  double sharpness;
	double blendtype;
	f0r_instance_t* blur_instance;
  uint32_t* sigm_frame;
	uint32_t* blurred;
} softglow_instance_t;

void overlay(const uint32_t* source1, const uint32_t* source2, uint32_t* out, unsigned int len)
{
    unsigned char* src1 = (unsigned char*)source1;
    unsigned char* src2 = (unsigned char*)source2;
    unsigned char* dst = (unsigned char*)out;
            
    uint32_t b, tmp, tmpM;
  
    while (len--)
    {
      for (b = 0; b < ALPHA; b++)
      {
        dst[b] = INT_MULT(src1[b], src1[b] + INT_MULT(2 * src2[b], 255 - src1[b], tmpM), tmp);
      }

      dst[ALPHA] = MIN(src1[ALPHA], src2[ALPHA]);

      src1 += NBYTES;
      src2 += NBYTES;
      dst += NBYTES;
    }
}

void screen(const uint32_t* source1, const uint32_t* source2, uint32_t* out, unsigned int len)
{
    unsigned char* src1 = (unsigned char*)source1;
    unsigned char* src2 = (unsigned char*)source2;
    unsigned char* dst = (unsigned char*)out;
            
    uint32_t b, tmp;
  
    while (len--)
      {
        for (b = 0; b < ALPHA; b++)
          dst[b] = 255 - INT_MULT((255 - src1[b]), (255 - src2[b]), tmp);
  
        dst[ALPHA] = MIN(src1[ALPHA], src2[ALPHA]);
  
        src1 += NBYTES;
        src2 += NBYTES;
        dst += NBYTES;
      }
}

void add(const uint32_t* source1, const uint32_t* source2, uint32_t* out, unsigned int len)
{
    unsigned char* src1 = (unsigned char*)source1;
    unsigned char* src2 = (unsigned char*)source2;
    unsigned char* dst = (unsigned char*)out;
            
    uint32_t b, val;
  
    while (len--)
      {
        for (b = 0; b < ALPHA; b++)
        {
          val = src1[b] + src2[b];
          dst[b] = CLAMP(val, 0, 255);
        }

        dst[ALPHA] = MIN(src1[ALPHA], src2[ALPHA]);

        src1 += NBYTES;
        src2 += NBYTES;
        dst += NBYTES;
      }
  }

int
gimp_rgb_to_l_int (int red,
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

int f0r_init()
{
  return 1;
}

void f0r_deinit()
{ /* no initialization required */ }

void f0r_get_plugin_info(f0r_plugin_info_t* softglowInfo)
{
  softglowInfo->name = "softglow";
  softglowInfo->author = "Janne Liljeblad";
  softglowInfo->plugin_type = F0R_PLUGIN_TYPE_FILTER;
  softglowInfo->color_model = F0R_COLOR_MODEL_RGBA8888;
  softglowInfo->frei0r_version = FREI0R_MAJOR_VERSION;
  softglowInfo->major_version = 0; 
  softglowInfo->minor_version = 9; 
  softglowInfo->num_params =  4; 
  softglowInfo->explanation = "Does softglow effect on highlights";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
	switch ( param_index ) {
		case 0:
			info->name = "blur";
			info->type = F0R_PARAM_DOUBLE;
			info->explanation = "Blur of the glow";
			break;
    case 1:
      info->name = "brightness";
      info->type = F0R_PARAM_DOUBLE;
      info->explanation = "Brightness of highlight areas";
      break;
    case 2:
      info->name = "sharpness";
      info->type = F0R_PARAM_DOUBLE;
      info->explanation = "Sharpness of highlight areas";
      break;
    case 3: // 0 - 0.33 screen, 0.33 - 0.66 overla7, 0.66 - 1.0 add
      info->name = "blurblend";
      info->type = F0R_PARAM_DOUBLE;
      info->explanation = "Blend mode used to blend highlight blur with input image";
      break;
	}
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
  softglow_instance_t* inst = (softglow_instance_t*)calloc(1, sizeof(*inst));
  inst->width = width; 
  inst->height = height;
	inst->blur = 0.5;
  inst->brightness = 0.75;
  inst->sharpness = 0.85;
	inst->blendtype = 0.0;
	inst->blur_instance = (f0r_instance_t *)blur_construct(width, height);
  inst->sigm_frame = (uint32_t*)malloc(width * height * sizeof(uint32_t));
	inst->blurred = (uint32_t*)malloc(width * height * sizeof(uint32_t));
  return (f0r_instance_t)inst;
}

void f0r_destruct(f0r_instance_t instance)
{
	softglow_instance_t* inst = (softglow_instance_t*)instance;
	blur_destruct(inst->blur_instance);
  free(inst->sigm_frame);
  free(inst->blurred);
  free(instance);
}

void f0r_set_param_value(f0r_instance_t instance, 
			 f0r_param_t param, int param_index)
{
	softglow_instance_t* inst = (softglow_instance_t*)instance;
	switch (param_index)
  {
		case 0:
			inst->blur = *((double*)param);
			blur_set_param_value(inst->blur_instance, &inst->blur, 0 );
			break;
    case 1:
      inst->brightness = *((double*)param);
      break;
    case 2:
      inst->sharpness = *((double*)param);
      break;
    case 3:
      inst->blendtype = *((double*)param);
      break;
  }
}

void f0r_get_param_value(f0r_instance_t instance,
			 f0r_param_t param, int param_index)
{
	softglow_instance_t* inst = (softglow_instance_t*)instance;
	switch (param_index) 
  {
		case 0:
			*((double*)param) = inst->blur;
			break;
    case 1:
      *((double*)param) = inst->brightness;
      break;
    case 2:
      *((double*)param) = inst->sharpness;
      break;
    case 3:
      *((double*)param) = inst->blendtype;
      break;
  }
}

void f0r_update(f0r_instance_t instance, double time,
                const uint32_t* inframe, uint32_t* outframe)
{
  // Check and cast instance
  assert(instance);
  softglow_instance_t* inst = (softglow_instance_t*)instance;
  unsigned int len = inst->width * inst->height;

  double brightness = inst->brightness;
  double sharpness = inst->sharpness;

  const unsigned char* src = (unsigned char*)inframe;

  memcpy(inst->sigm_frame, inframe, len*sizeof(uint32_t));
  unsigned char* dst = (unsigned char*)inst->sigm_frame;

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

	blur_update(inst->blur_instance, 0.0, inst->sigm_frame, inst->blurred);

  if (inst->blendtype <= 0.33)
    screen(inst->blurred, inframe, outframe, inst->width * inst->height);
  else if(inst->blendtype <= 0.66)
    overlay(inst->blurred, inframe, outframe, inst->width * inst->height);
  else
    add(inst->blurred, inframe, outframe, inst->width * inst->height);
}




