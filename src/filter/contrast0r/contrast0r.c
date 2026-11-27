/* contrast0r.c
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
#include <string.h>
#ifdef __SSE2__
#include <emmintrin.h>
#endif
#include <assert.h>
#include <stdio.h>

#include "frei0r.h"
#include "frei0r/math.h"

typedef struct contrast0r_instance
{
  unsigned int width;
  unsigned int height;
  int contrast; /* the contrast [-256, 256] */
  unsigned char lut[256]; /* look-up table */
} contrast0r_instance_t;

/* Updates the look-up-table. */
static void update_lut(contrast0r_instance_t *inst)
{
  int i;
  unsigned char *lut = inst->lut;
  int contrast = inst->contrast;
  for (i=0; i<128; ++i)
    lut[i] = CLAMP0255(i - (((128 - i)*contrast)>>8));
  for (i=128; i<256; ++i)
    lut[i] = CLAMP0255(i + (((i - 128)*contrast)>>8));
}

int f0r_init()
{
  return 1;
}

void f0r_deinit()
{ /* no initialization required */ }

void f0r_get_plugin_info(f0r_plugin_info_t* contrast0r_info)
{
  contrast0r_info->name = "Contrast0r";
  contrast0r_info->author = "Jean-Sebastien Senecal";
  contrast0r_info->plugin_type = F0R_PLUGIN_TYPE_FILTER;
  contrast0r_info->color_model = F0R_COLOR_MODEL_RGBA8888;
  contrast0r_info->frei0r_version = FREI0R_MAJOR_VERSION;
  contrast0r_info->major_version = 0; 
  contrast0r_info->minor_version = 2; 
  contrast0r_info->num_params =  1; 
  contrast0r_info->explanation = "Adjusts the contrast of a source image";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
  switch(param_index)
  {
  case 0:
    info->name = "Contrast";
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "The contrast value";
    break;
  }
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
  contrast0r_instance_t* inst = (contrast0r_instance_t*)calloc(1, sizeof(*inst));
  inst->width = width; inst->height = height;
  /* init look-up-table */
  update_lut(inst);
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
  contrast0r_instance_t* inst = (contrast0r_instance_t*)instance;
  switch(param_index)
  {
    int val;
  case 0:
    /* contrast */
    val = (int) ((*((double*)param) - 0.5) * 512.0); /* remap to [-256, 256] */
    if (val != inst->contrast)
    {
      inst->contrast = val;
      update_lut(inst);
    }
    break;
  }
}

void f0r_get_param_value(f0r_instance_t instance,
                         f0r_param_t param, int param_index)
{
  assert(instance);
  contrast0r_instance_t* inst = (contrast0r_instance_t*)instance;
  
  switch(param_index)
  {
  case 0:
    *((double*)param) = (double) ( (inst->contrast + 256.0) / 512.0 );
    break;
  }
}

void f0r_update(f0r_instance_t instance, double time,
                const uint32_t* inframe, uint32_t* outframe)
{
  assert(instance);
  contrast0r_instance_t* inst = (contrast0r_instance_t*)instance;
  unsigned int len = inst->width * inst->height;
  int c = inst->contrast;

  if (c == 0)
  {
    if (outframe != inframe)
      memcpy(outframe, inframe, (size_t)len * sizeof(uint32_t));
    return;
  }

#ifdef __SSE2__
  if (c >= -256 && c <= 255)
  {
    __m128i vZero = _mm_setzero_si128();
    __m128i v128 = _mm_set1_epi16(128);
    __m128i vC = _mm_set1_epi16((short)c);
    __m128i vAlphaMask = _mm_set1_epi32((int)0xFF000000u);
    __m128i vRgbMask = _mm_set1_epi32(0x00FFFFFF);
    unsigned int i = 0;
    for (; i + 3 < len; i += 4)
    {
      __m128i px = _mm_loadu_si128((const __m128i*)(inframe + i));
      __m128i lo = _mm_unpacklo_epi8(px, vZero);
      __m128i hi = _mm_unpackhi_epi8(px, vZero);

      __m128i lo_diff = _mm_sub_epi16(lo, v128);
      __m128i lo_sign = _mm_srai_epi16(lo_diff, 15);
      __m128i lo_abs  = _mm_sub_epi16(_mm_xor_si128(lo_diff, lo_sign), lo_sign);
      __m128i lo_step = _mm_srai_epi16(_mm_mullo_epi16(lo_abs, vC), 8);
      lo = _mm_add_epi16(lo, _mm_sub_epi16(_mm_xor_si128(lo_step, lo_sign), lo_sign));

      __m128i hi_diff = _mm_sub_epi16(hi, v128);
      __m128i hi_sign = _mm_srai_epi16(hi_diff, 15);
      __m128i hi_abs  = _mm_sub_epi16(_mm_xor_si128(hi_diff, hi_sign), hi_sign);
      __m128i hi_step = _mm_srai_epi16(_mm_mullo_epi16(hi_abs, vC), 8);
      hi = _mm_add_epi16(hi, _mm_sub_epi16(_mm_xor_si128(hi_step, hi_sign), hi_sign));

      __m128i res = _mm_packus_epi16(lo, hi);
      res = _mm_or_si128(_mm_and_si128(res, vRgbMask), _mm_and_si128(px, vAlphaMask));
      _mm_storeu_si128((__m128i*)(outframe + i), res);
    }
    const unsigned char* lut = inst->lut;
    unsigned char* dst = (unsigned char*)(outframe + i);
    const unsigned char* src = (const unsigned char*)(inframe + i);
    for (; i < len; ++i)
    {
      *dst++ = lut[*src++];
      *dst++ = lut[*src++];
      *dst++ = lut[*src++];
      *dst++ = *src++;
    }
    return;
  }
#endif

  unsigned char* lut = inst->lut;
  unsigned char* dst = (unsigned char*)outframe;
  const unsigned char* src = (const unsigned char*)inframe;
  while (len--)
  {
    *dst++ = lut[*src++];
    *dst++ = lut[*src++];
    *dst++ = lut[*src++];
    *dst++ = *src++; // copy alpha
  }
}
