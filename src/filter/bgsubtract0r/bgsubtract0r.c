/* bgsubtract0r.c */

/*
 * Frei0r plugin to perform background subtraction on a still video.
 *
 * Copyright (C) 2013 Samuel Mimram
 *
 * This plugin operates on still videos, i.e. the camera is not moving, and its
 * aim is to remove the background image, keeping only the moving parts. It is
 * thus a way to have a green screen without a green screen. The first image is
 * taken as reference for the background, so the video should start without the
 * moving stuff, who should come in afterwards.
 *
 * A few settings can be set such as:
 * - the threshold in order to decide whether a pixel is the same as in the
 *   reference image
 * - whether to remove "noise" (i.e. isolated pixels)
 * - optional blurring of edges
 *
 * Some recommendations:
 * - obviously the background should be of a (really) different color than the
 *   moving actor
 * - light in order to avoid shadows on the background
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
#include <string.h>

#include "frei0r.h"

typedef struct bgsubtract0r_instance
{
  unsigned int width;
  unsigned int height;
  uint8_t threshold;
  char denoise; /* Remove noise from mask. */
  uint32_t* reference; /* The reference image. */
  uint8_t* mask; /* Where the mask is computed. */
  int blur; /* Width of alpha-channel blurring. */
} bgsubtract0r_instance_t;

int f0r_init()
{
  return 1;
}

void f0r_deinit()
{
}

void f0r_get_plugin_info(f0r_plugin_info_t* bgsubtract0r_info)
{
  bgsubtract0r_info->name = "BgSubtract0r";
  bgsubtract0r_info->author = "Samuel Mimram";
  bgsubtract0r_info->plugin_type = F0R_PLUGIN_TYPE_FILTER;
  bgsubtract0r_info->color_model = F0R_COLOR_MODEL_RGBA8888;
  bgsubtract0r_info->frei0r_version = FREI0R_MAJOR_VERSION;
  bgsubtract0r_info->major_version = 0;
  bgsubtract0r_info->minor_version = 2;
  bgsubtract0r_info->num_params =  3;
  bgsubtract0r_info->explanation = "Bluescreen the background of a static video.";
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
  bgsubtract0r_instance_t* inst = (bgsubtract0r_instance_t*)calloc(1, sizeof(*inst));
  inst->width = width;
  inst->height = height;
  inst->denoise = 1;
  inst->blur = 0;
  inst->threshold = 26;
  inst->reference = NULL;
  inst->mask = malloc(width*height);
  return (f0r_instance_t)inst;
}

void f0r_destruct(f0r_instance_t instance)
{
  bgsubtract0r_instance_t* inst = (bgsubtract0r_instance_t*)instance;
  free(inst->reference);
  free(inst->mask);
  free(inst);
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
  switch(param_index)
  {
  case 0:
    info->name = "threshold";
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "Threshold for difference";
    break;

  case 1:
    info->name = "denoise";
    info->type = F0R_PARAM_BOOL;
    info->explanation = "Remove noise";
    break;

  case 2:
    info->name = "blur";
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "Blur alpha channel by given radius (to remove sharp edges)";
    break;
  }
}

void f0r_set_param_value(f0r_instance_t instance, f0r_param_t param, int param_index)
{
  assert(instance);
  bgsubtract0r_instance_t* inst = (bgsubtract0r_instance_t*)instance;
  f0r_param_color_t *c;

  switch(param_index)
  {
  case 0:
    inst->threshold = *((double*)param) * 255.;
    break;

  case 1:
    inst->denoise = *((double*)param) >= 0.5;
    break;

  case 2:
    inst->blur = (int)(*((double*)param)+0.5);
    break;
  }
}

void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index)
{
  assert(instance);
  bgsubtract0r_instance_t* inst = (bgsubtract0r_instance_t*)instance;
  f0r_param_color_t *c;

  switch(param_index)
  {
  case 0:
    *((double*)param) = (double)inst->threshold / 255.;
    break;

  case 1:
    *((double*)param) = inst->denoise ? 1. : 0.;
    break;

  case 2:
    *((double*)param) = inst->blur;
    break;
  }
}

#define MAX(x,y) ((x) > (y) ? (x) : (y));

inline static uint8_t dst(uint32_t x, uint32_t y)
{
  uint8_t d;
  uint8_t* px = (uint8_t*)&x;
  uint8_t* py = (uint8_t*)&y;

  d = abs(px[0]-py[0]);
  d = MAX(d, abs(px[1]-py[1]));
  d = MAX(d, abs(px[2]-py[2]));

  return d;
}

void f0r_update(f0r_instance_t instance, double time, const uint32_t* inframe, uint32_t* outframe)
{
  assert(instance);
  bgsubtract0r_instance_t* inst = (bgsubtract0r_instance_t*)instance;
  unsigned int width = inst->width;
  unsigned int height = inst->height;
  unsigned int len = width * height;
  uint8_t *mask = inst->mask;
  int blur = inst->blur;
  int i;
  int j;
  int n;
  uint8_t* pi;
  uint8_t* po;

  if (!inst->reference)
  {
    int blen = sizeof(uint32_t)*len;
    inst->reference = malloc(blen);
    memmove(inst->reference, inframe, blen);
    memset(mask, 0, blen);
  }
  else
  {
    for (i=0; i<len; i++)
      mask[i] = (dst(inst->reference[i], inframe[i]) > inst->threshold) ? 0xff : 0;
  }

  /* Clean up the mask. */
  if (inst->denoise)
    for (j=1; j<height-1; j++)
      for (i=1; i<width-1; i++)
      {
        n = (mask[width*j+i-1]+mask[width*j+i+1]+mask[width*(j-1)+i]+mask[width*(j+1)+i]
             + mask[width*(j-1)+i-1]+mask[width*(j-1)+i+1]+mask[width*(j+1)+i-1]+mask[width*(j+1)+i+1])/0xff;
        if (mask[width*j+i])
        {
          if (n<=2) mask[width*j+i] = 0;
        }
        else
        {
          if (n>=6) mask[width*j+i] = 0xff;
        }
      }

  for (i=0; i<len; i++)
    {
      pi = (uint8_t*)&inframe[i];
      po = (uint8_t*)&outframe[i];
      po[0] = pi[0];
      po[1] = pi[1];
      po[2] = pi[2];
      po[3] = mask[i];
    }

  if (blur)
  {
    int ii, jj;
    int di, dj;
    unsigned int a;
    // Number of pixels in the surface
    unsigned int s = (2*blur+1)*(2*blur+1);

    for (j=0; j<height; j++)
      for (i=0; i<width; i++)
      {
        a = 0;
        for (dj=-blur; dj<=blur; dj++)
          for (di=-blur; di<=blur; di++)
          {
            ii = i+di;
            jj = j+dj;
            if (ii < 0 || ii >= width || jj < 0 || jj >= height)
              a += 0xff;
            else
              a += mask[width*jj+ii];
          }
        a /= s;
        po = (uint8_t*)&outframe[width*j+i];
        po[3] = a;
      }
  }
}
