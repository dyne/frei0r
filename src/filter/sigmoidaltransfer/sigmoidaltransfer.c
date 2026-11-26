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
#if defined(__SSE2__)
#include <emmintrin.h>
#endif
#include "frei0r.h"
#include "frei0r/math.h"

typedef struct sigmoidal_instance
{
  unsigned int width;
  unsigned int height;
  double base;
  double sharpness;

  uint32_t lut32[256];
  double lumaR[256];
  double lumaG[256];
  double lumaB[256];
} sigmoidal_instance_t;

static void gen_sigmoid_lut (uint32_t *const lut32, const float base, const float sharpness)
{
  float k = expf(sharpness * 5.0) / 255.0;
  float b = (base - 0.5) * 63.0;

  for (int i = 0; i < 256; ++i) {
    uint32_t v = (uint8_t)CLAMP (255.0 / (1.0 + expf(-k * (i - b - 127.0))), 0, 255.0);
    lut32[i] = v | (v << 8) | (v << 16);
  }
}

static void sigmoidal_transfer(f0r_instance_t instance, double time,
                const uint32_t* inframe, uint32_t* outframe)
{
  assert(instance);
  sigmoidal_instance_t* inst = (sigmoidal_instance_t*)instance;
  unsigned int len = inst->width * inst->height;
  unsigned int i = 0;

  const uint32_t* lut32 = inst->lut32;
  const double* lumaR = inst->lumaR;
  const double* lumaG = inst->lumaG;
  const double* lumaB = inst->lumaB;

#if defined(__SSE2__)
  for (; i + 1 < len; i += 2)
  {
    uint32_t p0 = inframe[i];
    uint32_t p1 = inframe[i + 1];
    __m128d vr = _mm_set_pd(lumaR[p1 & 0xFFu], lumaR[p0 & 0xFFu]);
    __m128d vg = _mm_set_pd(lumaG[(p1 >> 8) & 0xFFu], lumaG[(p0 >> 8) & 0xFFu]);
    __m128d vb = _mm_set_pd(lumaB[(p1 >> 16) & 0xFFu], lumaB[(p0 >> 16) & 0xFFu]);
    __m128d vl = _mm_add_pd(_mm_add_pd(vr, vg), vb);
    __m128i il = _mm_cvttpd_epi32(vl);
    uint32_t l0 = (uint32_t)_mm_cvtsi128_si32(il);
    uint32_t l1 = (uint32_t)_mm_cvtsi128_si32(_mm_srli_si128(il, 4));
    outframe[i]     = lut32[l0 & 0xFFu] | (p0 & 0xFF000000u);
    outframe[i + 1] = lut32[l1 & 0xFFu] | (p1 & 0xFF000000u);
  }
#endif

  for (; i < len; i++)
  {
    uint32_t p = inframe[i];
    unsigned char luma = (unsigned char)(lumaR[p & 0xFFu] + lumaG[(p >> 8) & 0xFFu] + lumaB[(p >> 16) & 0xFFu]);
    outframe[i] = lut32[luma] | (p & 0xFF000000u);
  }
}

int f0r_init()
{
  return 1;
}

void f0r_deinit()
{ }

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
			info->explanation = "Brightness of image. Midpoint of sigmoidal curve";
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

  for (int i = 0; i < 256; i++) {
    inst->lumaR[i] = 0.299 * i;
    inst->lumaG[i] = 0.587 * i;
    inst->lumaB[i] = 0.114 * i;
  }
  gen_sigmoid_lut (inst->lut32, inst->base, inst->sharpness);
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

  gen_sigmoid_lut (inst->lut32, inst->base, inst->sharpness);
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
