/*
 * This file is a port of com.jhlabs.image.PosterizeFilter.java
 * Copyright 2006 Jerry Huxtable
 *
 * posterize.c
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

#if defined(__SSE2__)
#include <emmintrin.h>
#endif

#include "frei0r.h"
#include "frei0r/math.h"

typedef struct posterize_instance
{
  unsigned int width;
  unsigned int height;
  double levels;
} posterize_instance_t;

int f0r_init()
{
  return 1;
}

void f0r_deinit()
{ /* no initialization required */ }

void f0r_get_plugin_info(f0r_plugin_info_t* posterize_info)
{
  posterize_info->name = "posterize";
  posterize_info->author = "Janne Liljeblad";
  posterize_info->plugin_type = F0R_PLUGIN_TYPE_FILTER;
  posterize_info->color_model = F0R_COLOR_MODEL_RGBA8888;
  posterize_info->frei0r_version = FREI0R_MAJOR_VERSION;
  posterize_info->major_version = 0; 
  posterize_info->minor_version = 1; 
  posterize_info->num_params =  1; 
  posterize_info->explanation = "Posterizes image by reducing the number of colors used in image";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
  switch(param_index)
  {
  case 0:
    info->name = "levels"; 
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "Number of values per channel";
    break;
  }
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
	posterize_instance_t* inst = (posterize_instance_t*)calloc(1, sizeof(*inst));
	inst->width = width; 
  inst->height = height;
	inst->levels = 5.0 / 48.0;// input range 0 - 1 will be interpreted as levels range 2 - 50
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
  posterize_instance_t* inst = (posterize_instance_t*)instance;

  switch(param_index)
  {
  case 0:
    inst->levels = *((double*)param);
    break;
  }
}

void f0r_get_param_value(f0r_instance_t instance,
                         f0r_param_t param, int param_index)
{
  assert(instance);
  posterize_instance_t* inst = (posterize_instance_t*)instance;
  
  switch(param_index)
  {
  case 0:
    *((double*)param) = inst->levels;
    break;
  }
}

void f0r_update(f0r_instance_t instance, double time,
                const uint32_t* inframe, uint32_t* outframe)
{
  assert(instance);
  posterize_instance_t* inst = (posterize_instance_t*)instance;
  size_t len = (size_t)inst->width * inst->height;

  // convert input value 0.0-1.0 to int value 2-50
  double levelsInput = inst->levels * 48.0;
  levelsInput = CLAMP(levelsInput, 0.0, 48.0) + 2.0;
  int numLevels = (int)levelsInput;

  // Prevent division by zero
  if (numLevels < 2) numLevels = 2;

  size_t idx = 0;

#if defined(__SSE2__)
  {
    const __m128i rgb_mask = _mm_set1_epi32(0x00FFFFFF);
    const __m128i a_mask = _mm_set1_epi32((int)0xFF000000u);

    if (numLevels == 2)
    {
      const __m128i sign_flip = _mm_set1_epi8((char)0x80);
      const __m128i thr_m1 = _mm_set1_epi8((char)(127 ^ 0x80));
      for (; idx + 4 <= len; idx += 4)
      {
        __m128i px = _mm_loadu_si128((const __m128i*)(inframe + idx));
        __m128i mask = _mm_cmpgt_epi8(_mm_xor_si128(px, sign_flip), thr_m1);
        __m128i res = _mm_or_si128(_mm_and_si128(mask, rgb_mask), _mm_and_si128(px, a_mask));
        _mm_storeu_si128((__m128i*)(outframe + idx), res);
      }
    }
    else
    {
      int d = numLevels - 1;
      int shift = 0;
      unsigned int mult = 0;
      int found = 0;
      for (shift = 0; shift < 8; shift++)
      {
        unsigned int m = (1u << (16 + shift)) / (unsigned int)d + 1u;
        if (m > 65535u) continue;
        int ok = 1;
        int k;
        for (k = 0; k < numLevels; k++)
        {
          if ((((unsigned int)(k * 255) * m) >> (16 + shift)) != (unsigned int)((255 * k) / d))
          {
            ok = 0;
            break;
          }
        }
        if (ok)
        {
          mult = m;
          found = 1;
          break;
        }
      }

      if (found)
      {
        const __m128i zero = _mm_setzero_si128();
        const __m128i v_nl = _mm_set1_epi16((short)numLevels);
        const __m128i v_255 = _mm_set1_epi16(255);
        const __m128i v_m = _mm_set1_epi16((short)mult);

        for (; idx + 4 <= len; idx += 4)
        {
          __m128i px = _mm_loadu_si128((const __m128i*)(inframe + idx));
          __m128i lo = _mm_unpacklo_epi8(px, zero);
          __m128i hi = _mm_unpackhi_epi8(px, zero);

          lo = _mm_srli_epi16(_mm_mullo_epi16(lo, v_nl), 8);
          hi = _mm_srli_epi16(_mm_mullo_epi16(hi, v_nl), 8);

          lo = _mm_srli_epi16(_mm_mulhi_epu16(_mm_mullo_epi16(lo, v_255), v_m), shift);
          hi = _mm_srli_epi16(_mm_mulhi_epu16(_mm_mullo_epi16(hi, v_255), v_m), shift);

          __m128i packed = _mm_packus_epi16(lo, hi);
          __m128i res = _mm_or_si128(_mm_and_si128(packed, rgb_mask), _mm_and_si128(px, a_mask));
          _mm_storeu_si128((__m128i*)(outframe + idx), res);
        }
      }
    }
  }
#endif

  if (idx < len)
  {
    unsigned char levels[256];
    int i;
    for (i = 0; i < 256; i++)
    {
      levels[i] = 255 * (numLevels * i / 256) / (numLevels - 1);
    }

    unsigned char* dst = (unsigned char*)(outframe + idx);
    const unsigned char* src = (const unsigned char*)(inframe + idx);
    for (; idx < len; idx++)
    {
      *dst++ = levels[*src++];
      *dst++ = levels[*src++];
      *dst++ = levels[*src++];
      *dst++ = *src++; // copy alpha
    }
  }
}
