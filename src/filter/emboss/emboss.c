/* 
 * This file is a port of com.jhlabs.image.EmbossFilter.java
 * Copyright 2006 Jerry Huxtable
 *
 * emboss.c
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
#include "frei0r_math.h"

double PI = 3.14159; 
double pixelScale = 255.9;

typedef struct emboss_instance
{
  unsigned int width;
  unsigned int height;
	double azimuth;
  double elevation;
	double width45;
} emboss_instance_t;

int f0r_init()
{
  return 1;
}

void f0r_deinit()
{ /* no initialization required */ }

void f0r_get_plugin_info(f0r_plugin_info_t* emposs_info)
{
  emposs_info->name = "emboss";
  emposs_info->author = "Janne Liljeblad";
  emposs_info->plugin_type = F0R_PLUGIN_TYPE_FILTER;
  emposs_info->color_model = F0R_COLOR_MODEL_RGBA8888;
  emposs_info->frei0r_version = FREI0R_MAJOR_VERSION;
  emposs_info->major_version = 0; 
  emposs_info->minor_version = 1; 
  emposs_info->num_params =  3; 
  emposs_info->explanation = "Creates embossed relief image of source image";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
  switch(param_index)
  {
  case 0:
    info->name = "azimuth";
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "Light direction";
    break;
  case 1:
    info->name = "elevation";
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "Background lightness";
    break;
  case 2:
    info->name = "width45";
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "Bump height";
    break;
  }
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
	emboss_instance_t* inst = (emboss_instance_t*)calloc(1, sizeof(*inst));
	inst->width = width; 
  inst->height = height;
  inst->azimuth = 135.0 / 360.0; //input range 0 - 1 will be interpreted as angle 0 - 360
  inst->elevation = 30.0 / 90.0;//input range 0 - 1 will be interpreted as lighness value 0 - 90
  inst->width45 = 10.0 / 40.0;//input range 0 - 1 will be interpreted as bump height value 1 - 40
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
  emboss_instance_t* inst = (emboss_instance_t*)instance;

  switch(param_index)
  {
  case 0:
    inst->azimuth = *((double*)param);
    break;
  case 1:
    inst->elevation = *((double*)param);
    break;
  case 2:
    inst->width45 = *((double*)param);
    break;
  }
}

void f0r_get_param_value(f0r_instance_t instance,
                         f0r_param_t param, int param_index)
{
  assert(instance);
  emboss_instance_t* inst = (emboss_instance_t*)instance;
  
  switch(param_index)
  {
  case 0:
    *((double*)param) = inst->azimuth;
    break;
  case 1:
    *((double*)param) = inst->elevation;
    break;
  case 2:
    *((double*)param) = inst->width45;
    break;
  }
}

void f0r_update(f0r_instance_t instance, double time,
                const uint32_t* inframe, uint32_t* outframe)
{
  // Check and cast instance
  assert(instance);
  emboss_instance_t* inst = (emboss_instance_t*)instance;
 
  // Get render params values 0.0-1.0 in range used by filter
  double azimuthInput = inst->azimuth * 360.0;
  double elevationInput = inst->elevation * 90.0;
	double widthInput = inst->width45 * 40.0;

  // Force correct ranges on input
  azimuthInput = CLAMP(azimuthInput, 0.0, 360.0);
  elevationInput = CLAMP(elevationInput, 0.0, 90.0);
  widthInput = CLAMP(widthInput, 1.0, 40.0);

  // Convert to filter input values
	double azimuth = azimuthInput * PI / 180.0; 
  double elevation = elevationInput * PI / 180.0;
	double width45 = widthInput;

  // Create brightness image
  unsigned int len = inst->width * inst->height;
  unsigned char bumpPixels[len];
  unsigned char alphaVals[len];
  unsigned int index, r, g, b, a = 0;
  const unsigned char* src = (unsigned char*)inframe;
  while (len--)
  {
    r = *src++;
    g = *src++;
    b = *src++;
    a = *src++;

    bumpPixels[index++] = (r + g + b)/3;
    alphaVals[index - 1] = a;
  }

  // Create embossed image from brightness image
  unsigned char* dst = (unsigned char*)outframe;
  unsigned char shade, background = 0;
  int Nx, Ny, Nz, Lx, Ly, Lz, Nz2, NzLz, NdotL;

  Lx = (int)(cos(azimuth) * cos(elevation) * pixelScale);
  Ly = (int)(sin(azimuth) * cos(elevation) * pixelScale);
  Lz = (int)(sin(elevation) * pixelScale);

  Nz = (int)(6 * 255 / width45);
  Nz2 = Nz * Nz;
  NzLz = Nz * Lz;

  background = Lz;
  int bumpIndex = 0;
  int width = inst->width;
  int height = inst->height;
  int s1, s2, s3 = 0;
  int x, y = 0;
  for (y = 0; y < height; y++, bumpIndex += width) 
  {
    s1 = bumpIndex;
    s2 = s1 + width;
    s3 = s2 + width;
    for (x = 0; x < width; x++, s1++, s2++, s3++) 
    {
	    if (y != 0 && y < height-2 && x != 0 && x < width-2) 
      {
		    Nx = bumpPixels[s1-1] + bumpPixels[s2-1] + bumpPixels[s3-1] - bumpPixels[s1+1] - bumpPixels[s2+1] - bumpPixels[s3+1];
		    Ny = bumpPixels[s3-1] + bumpPixels[s3] + bumpPixels[s3+1] - bumpPixels[s1-1] - bumpPixels[s1] - bumpPixels[s1+1];
		    if (Nx == 0 && Ny == 0)
			    shade = background;
		    else if ((NdotL = Nx*Lx + Ny*Ly + NzLz) < 0)
			    shade = 0;
		    else
			    shade = (int)(NdotL / sqrt(Nx*Nx + Ny*Ny + Nz2));
	    } 
      else
      {
		    shade = background;
      }

      // Write value
      *dst++ = shade;
      *dst++ = shade;
      *dst++ = shade;
      *dst++ = alphaVals[s1]; //copy alpha
    }  
  }
}

