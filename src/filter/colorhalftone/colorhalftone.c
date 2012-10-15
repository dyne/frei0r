/*
 * This file is a port of com.jhlabs.image.ColorHalftoneFilter.java
 * Copyright 2006 Jerry Huxtable
 *
 * colorhalftone.c
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

double PI=3.14159265358979;

typedef struct colorhalftone_instance
{
  unsigned int width;
  unsigned int height;
  double dot_radius;
  double cyan_angle;
  double magenta_angle;
  double yellow_angle;
} colorhalftone_instance_t;

static inline double degreeToRadian(double degree)
{
	double radian = degree * (PI/180);
	return radian;
}

static inline double modjhlabs(double a, double b) 
{
		int n = (int)(a/b);
		
		a -= n*b;
		if (a < 0)
			return a + b;
		return a;
}

static inline double smoothStep(double a, double b, double x) 
{
		if (x < a)
			return 0;
		if (x >= b)
			return 1;
		x = (x - a) / (b - a);
		return x*x * (3 - 2*x);
}
void color_halftone(f0r_instance_t instance, double time,
		const uint32_t* inframe, uint32_t* outframe)
{
  colorhalftone_instance_t* inst = (colorhalftone_instance_t*)instance;

  int width = inst->width;
  int height =  inst->height;

  double dotRadius = inst->dot_radius * 9.99;
  dotRadius = ceil(dotRadius);
  double cyanScreenAngle = degreeToRadian(inst->cyan_angle * 360.0);
  double magentaScreenAngle = degreeToRadian(inst->magenta_angle * 360.0);
  double yellowScreenAngle = degreeToRadian(inst->yellow_angle * 360.0);

  double gridSize = 2 * dotRadius * 1.414f;
  double angles[] = {cyanScreenAngle, magentaScreenAngle, yellowScreenAngle};
  double mx[] = {0, -1, 1,  0, 0};
  double my[] = {0, 0, 0, -1, 1};
  double halfGridSize = (double)gridSize / 2;

  const uint32_t* inPixels = (const uint32_t*)inframe;
  uint32_t* dst = outframe;
  int x, y, ix, channel, v, nr, shift, mask, i, nx,  argb, ny;

  double angle, sin_val, cos_val, tx, ty , ttx, tty, ntx, nty, l, dx , dy , dx2 , dy2 , R , f, f2;

  for (y = 0; y < height; y++)
  {

    for (channel = 0; channel < 3; channel++ )
    {
      shift = 16-8*channel;
      mask = 0x000000ff << shift;
      angle = angles[channel];
      sin_val = sin(angle);
      cos_val = cos(angle);

      for (x = 0; x < width; x++)
      {
        // Transform x,y into halftone screen coordinate space
        tx = x*cos_val + y*sin_val;
        ty = -x*sin_val + y*cos_val;
        
        // Find the nearest grid point
        tx = tx - modjhlabs(tx-halfGridSize, gridSize) + halfGridSize;
        ty = ty - modjhlabs(ty-halfGridSize, gridSize) + halfGridSize;

        f = 1;

        // TODO: Efficiency warning: Because the dots overlap, we need to check neighbouring grid squares.
        // We check all four neighbours, but in practice only one can ever overlap any given point.
        for (i = 0; i < 5; i++) 
        {
          // Find neigbouring grid point
          ttx = tx + mx[i]*gridSize;
          tty = ty + my[i]*gridSize;
          // Transform back into image space
          ntx = ttx*cos_val - tty*sin_val;
          nty = ttx*sin_val + tty*cos_val;
          // Clamp to the image
          nx = CLAMP( (int)ntx, 0, width - 1);
          ny = CLAMP( (int)nty, 0, height - 1);
          argb = inPixels[ny*width+nx];
          nr = (argb >> shift) & 0xff;
          l = nr/255.0f;
          l = 1-l*l;
          l *= halfGridSize * 1.414;
          dx = x - ntx;
          dy = y - nty;
          dx2 = dx * dx;
          dy2 = dy * dy;
          R = sqrt(dx2 + dy2);
          f2 = 1 - smoothStep(R, R+1, l);
          f = MIN(f, f2);
        }

        v = (int)(255 * f);
        v <<= shift;
        v ^= ~mask;
				v |= 0xff000000;
        *dst++ &= v;
      }
      if (channel != 2)
        dst = dst - width;// we're starting row again for next channel, unless this was last channel
    }

  }
}

int f0r_init()
{
  return 1;
}

void f0r_deinit()
{ /* no initialization required */ }

void f0r_get_plugin_info(f0r_plugin_info_t* colorhalftoneInfo)
{
  colorhalftoneInfo->name = "colorhalftone";
  colorhalftoneInfo->author = "Janne Liljeblad";
  colorhalftoneInfo->plugin_type = F0R_PLUGIN_TYPE_FILTER;
  colorhalftoneInfo->color_model = F0R_COLOR_MODEL_RGBA8888;
  colorhalftoneInfo->frei0r_version = FREI0R_MAJOR_VERSION;
  colorhalftoneInfo->major_version = 0; 
  colorhalftoneInfo->minor_version = 9; 
  colorhalftoneInfo->num_params =  4; 
  colorhalftoneInfo->explanation = "Filters image to resemble a halftone print in which tones are represented as variable sized dots";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
	switch ( param_index ) {
		case 0:
			info->name = "dot radius";
			info->type = F0R_PARAM_DOUBLE;
			info->explanation = "Halftone pattern dot size";
			break;
    case 1:
      info->name = "cyan angle";
      info->type = F0R_PARAM_DOUBLE;
      info->explanation = "Cyan dots angle";
      break;
    case 2:
      info->name = "magenta angle";
      info->type = F0R_PARAM_DOUBLE;
      info->explanation = "Magenta dots angle";
      break;
    case 3:
      info->name = "yellow angle";
      info->type = F0R_PARAM_DOUBLE;
      info->explanation = "Yellow dots angle";
      break;
	}
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
  colorhalftone_instance_t* inst = (colorhalftone_instance_t*)calloc(1, sizeof(*inst));
  inst->width = width; 
  inst->height = height;
  inst->dot_radius = 0.4;// interpreted as range 1 - 10
  inst->cyan_angle = 108.0/360.0; // in degrees
  inst->magenta_angle = 162.0/360.0; // in degrees
  inst->yellow_angle = 90.0/360.0; // in degrees
  return (f0r_instance_t)inst;
}

void f0r_destruct(f0r_instance_t instance)
{
  free(instance);
}

void f0r_set_param_value(f0r_instance_t instance, 
			 f0r_param_t param, int param_index)
{
	colorhalftone_instance_t* inst = (colorhalftone_instance_t*)instance;

	switch (param_index)
  {
		case 0:
			inst->dot_radius = *((double*)param);
			break;
    case 1:
      inst->cyan_angle = *((double*)param);
      break;
    case 2:
      inst->magenta_angle = *((double*)param);
      break;
    case 3:
      inst->yellow_angle = *((double*)param);
      break;
  }
}

void f0r_get_param_value(f0r_instance_t instance,
			 f0r_param_t param, int param_index)
{
	colorhalftone_instance_t* inst = (colorhalftone_instance_t*)instance;
	switch (param_index) 
  {
		case 0:
			*((double*)param) = inst->dot_radius;
			break;
    case 1:
      *((double*)param) = inst->cyan_angle;
      break;
    case 2:
      *((double*)param) = inst->magenta_angle;
      break;
    case 3:
      *((double*)param) = inst->yellow_angle;
      break;
  }
}

void f0r_update(f0r_instance_t instance, double time,
		const uint32_t* inframe, uint32_t* outframe)
{
  assert(instance);
  color_halftone(instance, time, inframe, outframe);
}

