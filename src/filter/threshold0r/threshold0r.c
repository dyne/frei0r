/* threshold0r.c
 * Copyright (C) 2005 Jean-Sebastien Senecal (js@drone.ws)
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
#include <assert.h>

#if defined(__SSE2__)
#include <emmintrin.h>
#endif

#include "frei0r.h"

typedef struct threshold0r_instance
{
  unsigned int width;
  unsigned int height;
  unsigned char threshold; /* the threshold [0, 255] */
  unsigned char lut[256]; /* look-up table */
} threshold0r_instance_t;

/* Updates the look-up-table. */
static void update_lut(threshold0r_instance_t *inst)
{
  int i;
  unsigned char *lut = inst->lut;
  unsigned char thresh = inst->threshold;
  if (thresh == 0xff)
    memset(lut, 0x00, 256*sizeof(unsigned char));
  else if (thresh == 0x00)
    memset(lut, 0xff, 256*sizeof(unsigned char));
  else
  {
    for (i=0; i<thresh; ++i)
      lut[i] = 0x00;
    for (i=thresh; i<256; ++i)
      lut[i] = 0xff;
  }
}

int f0r_init()
{
  return 1;
}

void f0r_deinit()
{ /* no initialization required */ }

void f0r_get_plugin_info(f0r_plugin_info_t* threshold0r_info)
{
  threshold0r_info->name = "Threshold0r";
  threshold0r_info->author = "Jean-Sebastien Senecal";
  threshold0r_info->plugin_type = F0R_PLUGIN_TYPE_FILTER;
  threshold0r_info->color_model = F0R_COLOR_MODEL_RGBA8888;
  threshold0r_info->frei0r_version = FREI0R_MAJOR_VERSION;
  threshold0r_info->major_version = 0; 
  threshold0r_info->minor_version = 2; 
  threshold0r_info->num_params =  1; 
  threshold0r_info->explanation = "Thresholds a source image";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
  switch(param_index)
  {
  case 0:
    info->name = "Threshold";
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "The threshold";
    break;
  }
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
  threshold0r_instance_t* inst = (threshold0r_instance_t*)calloc(1, sizeof(*inst));
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
  threshold0r_instance_t* inst = (threshold0r_instance_t*)instance;

  switch(param_index)
  {
    unsigned char val;
  case 0:
    /* threshold */
    val = (unsigned char) (255.0 * *((double*)param));
    if (val != inst->threshold)
    {
      inst->threshold = val;
      update_lut(inst);
    }
    break;
  }
}

void f0r_get_param_value(f0r_instance_t instance,
                         f0r_param_t param, int param_index)
{
  assert(instance);
  threshold0r_instance_t* inst = (threshold0r_instance_t*)instance;
  
  switch(param_index)
  {
  case 0:
    *((double*)param) = (double)(inst->threshold) / 255.0;
    break;
  }
}

void f0r_update(f0r_instance_t instance, double time,
                const uint32_t* inframe, uint32_t* outframe)
{
  assert(instance);
  threshold0r_instance_t* inst = (threshold0r_instance_t*)instance;
  size_t len = (size_t)inst->width * inst->height;
  size_t i = 0;

#if defined(__SSE2__)
  {
    const __m128i rgb_mask = _mm_set1_epi32(0x00FFFFFF);
    const __m128i a_mask = _mm_set1_epi32((int)0xFF000000u);
    unsigned char thresh = inst->threshold;

    if (thresh == 0xff)
    {
      for (; i + 4 <= len; i += 4)
      {
        __m128i px = _mm_loadu_si128((const __m128i*)(inframe + i));
        _mm_storeu_si128((__m128i*)(outframe + i), _mm_and_si128(px, a_mask));
      }
    }
    else if (thresh == 0x00)
    {
      for (; i + 4 <= len; i += 4)
      {
        __m128i px = _mm_loadu_si128((const __m128i*)(inframe + i));
        _mm_storeu_si128((__m128i*)(outframe + i), _mm_or_si128(px, rgb_mask));
      }
    }
    else
    {
      const __m128i sign_flip = _mm_set1_epi8((char)0x80);
      const __m128i thr_m1 = _mm_set1_epi8((char)((thresh - 1) ^ 0x80));
      for (; i + 4 <= len; i += 4)
      {
        __m128i px = _mm_loadu_si128((const __m128i*)(inframe + i));
        __m128i mask = _mm_cmpgt_epi8(_mm_xor_si128(px, sign_flip), thr_m1);
        __m128i res = _mm_or_si128(_mm_and_si128(mask, rgb_mask), _mm_and_si128(px, a_mask));
        _mm_storeu_si128((__m128i*)(outframe + i), res);
      }
    }
  }
#endif

  unsigned char* lut = inst->lut;
  unsigned char* dst = (unsigned char*)(outframe + i);
  const unsigned char* src = (const unsigned char*)(inframe + i);
  for (; i < len; i++)
  {
    *dst++ = lut[*src++];
    *dst++ = lut[*src++];
    *dst++ = lut[*src++];
    *dst++ = *src++; // copy alpha
  }
}
