/*
 * This file is a modified port of RGB Noise plug-in from Gimp.
 * It contains code from plug-ins/common/noise-rgb.c, see that for copyrights.
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
#include "frei0r_math.h"

static int MY_MAX_RAND = 32767;// I assume RAND_MAX to be at least this big.
static double gaussian_lookup[32767];
static int TABLE_INITED = 0;
static int next_gaussian_index = 0;
static int last_in_range = 32766;

typedef struct rgbnoise_instance
{
  unsigned int width;
  unsigned int height;
  double noise;
} rgbnoise_instance_t;



void f0r_deinit()
{}

void f0r_get_plugin_info(f0r_plugin_info_t* rgbnoiseInfo)
{
  rgbnoiseInfo->name = "rgbnoise";
  rgbnoiseInfo->author = "Janne Liljeblad";
  rgbnoiseInfo->plugin_type = F0R_PLUGIN_TYPE_FILTER;
  rgbnoiseInfo->color_model = F0R_COLOR_MODEL_RGBA8888;
  rgbnoiseInfo->frei0r_version = FREI0R_MAJOR_VERSION;
  rgbnoiseInfo->major_version = 0; 
  rgbnoiseInfo->minor_version = 9; 
  rgbnoiseInfo->num_params =  1; 
  rgbnoiseInfo->explanation = "Adds RGB noise to image.";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
	switch ( param_index ) {
		case 0:
			info->name = "noise";
			info->type = F0R_PARAM_DOUBLE;
			info->explanation = "Amount of noise added";
			break;
	}
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
  rgbnoise_instance_t* inst = (rgbnoise_instance_t*)calloc(1, sizeof(*inst));
  inst->width = width; 
  inst->height = height;
  inst->noise = 0.2;
  return (f0r_instance_t)inst;
}

void f0r_destruct(f0r_instance_t instance)
{
  free(instance);
}

void f0r_set_param_value(f0r_instance_t instance, 
			 f0r_param_t param, int param_index)
{
	rgbnoise_instance_t* inst = (rgbnoise_instance_t*)instance;
	switch (param_index)
  {
		case 0:
			inst->noise = *((double*)param);
			break;
  }
}

void f0r_get_param_value(f0r_instance_t instance,
			 f0r_param_t param, int param_index)
{ 
	rgbnoise_instance_t* inst = (rgbnoise_instance_t*)instance;
	switch (param_index) 
  {
		case 0:
			*((double*)param) = inst->noise;
			break;
  }
}

//-------------------------------------------------------- filter methods
static inline double nextDouble()
{
  double val = ((double) rand()) / ((double) RAND_MAX);
	return val;
}

static inline double gauss()
{
  double u, v, x;
  do
  {
		  v = nextDouble();

		  do u = nextDouble();
		  while (u == 0);

		  x = 1.71552776992141359295 * (v - 0.5) / u;
  }
  while ( x * x > -4.0 * log(u) );

  return x;
}

static void create_new_lookup_range()
{
    int first, last, tmp;
    first = rand() % (MY_MAX_RAND - 1);
    last = rand() % (MY_MAX_RAND - 1);
    if (first > last)
    {
      tmp = last;
      last = first;
      first = tmp;
    }
    next_gaussian_index = first;
    last_in_range = last;
}

static inline double next_gauss()
{
  next_gaussian_index++;
  if (next_gaussian_index >= last_in_range)
  {
    create_new_lookup_range(); 
  }
  return gaussian_lookup[next_gaussian_index];
}

static inline int addNoise(int sample, double noise)
{
  int byteNoise = 0;
  int noiseSample = 0;

  byteNoise = (int) (noise * next_gauss());
  noiseSample = sample + byteNoise;
  noiseSample = CLAMP(noiseSample, 0, 255);
  return noiseSample;
}	

int f0r_init()
{
  if (TABLE_INITED == 0)
  {
    int i;
    for( i = 0; i < MY_MAX_RAND; i++)
    {
      gaussian_lookup[i] = gauss() * 127.0;
    }
    TABLE_INITED = 1;
  }
  return 1;
}

void rgb_noise(f0r_instance_t instance, double time,
		const uint32_t* inframe, uint32_t* outframe)
{
  rgbnoise_instance_t* inst = (rgbnoise_instance_t*)instance;
  unsigned int len = inst->width * inst->height;

  unsigned char* dst = (unsigned char*)outframe;
  const unsigned char* src = (unsigned char*)inframe;

  int sample;
  double noise = inst->noise;
  while (len--)
  {
    sample = *src++;
    *dst++ = addNoise(sample, noise);
    sample = *src++;
    *dst++ = addNoise(sample, noise);
    sample = *src++;
    *dst++ = addNoise(sample, noise);
    *src++;
    *dst++;
  }
}

//---------------------------------------------------- update
void f0r_update(f0r_instance_t instance, double time,
		const uint32_t* inframe, uint32_t* outframe)
{
  assert(instance);
  rgb_noise(instance, time, inframe, outframe);
}

