/* luminance.c
 * Copyright (C) 2004 Jean-Sebastien Senecal (js@drone.ws)
 * Copyright (C) 2007 Richard Spindler (richard.spindler@gmail.com)
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
#if defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#include <emmintrin.h>
#define LUMINANCE_USE_SSE2 1
#endif

#include "frei0r.h"
#include "frei0r/math.h"

#define MAX_SATURATION 8.0

typedef struct luminance_instance
{
  unsigned int width;
  unsigned int height;
} luminance_instance_t;

int f0r_init()
{
  return 1;
}

void f0r_deinit()
{ /* no initialization required */ }

void f0r_get_plugin_info(f0r_plugin_info_t* luminance_info)
{
  luminance_info->name = "Luminance";
  luminance_info->author = "Richard Spindler";
  luminance_info->plugin_type = F0R_PLUGIN_TYPE_FILTER;
  luminance_info->color_model = F0R_COLOR_MODEL_RGBA8888;
  luminance_info->frei0r_version = FREI0R_MAJOR_VERSION;
  luminance_info->major_version = 0; 
  luminance_info->minor_version = 2; 
  luminance_info->num_params = 0; 
  luminance_info->explanation = "Creates a luminance map of the image";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
  (void)info;
  (void)param_index;
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
  luminance_instance_t* inst = (luminance_instance_t*)calloc(1, sizeof(*inst));
  inst->width = width; inst->height = height;
  return (f0r_instance_t)inst;
}

void f0r_destruct(f0r_instance_t instance)
{
  free(instance);
}

void f0r_set_param_value(f0r_instance_t instance, 
                         f0r_param_t param, int param_index)
{
  (void)instance;
  (void)param;
  (void)param_index;
}

void f0r_get_param_value(f0r_instance_t instance,
                         f0r_param_t param, int param_index)
{
  (void)instance;
  (void)param;
  (void)param_index;
}

void f0r_update(f0r_instance_t instance, double time,
                const uint32_t* inframe, uint32_t* outframe)
{
  (void)time;
  assert(instance);
  luminance_instance_t* inst = (luminance_instance_t*)instance;
  unsigned int len = inst->width * inst->height;
  unsigned int i = 0;

#ifdef LUMINANCE_USE_SSE2
  const __m128i v_ff = _mm_set1_epi32(0xff);
  const __m128i v_alpha_mask = _mm_set1_epi32((int)0xff000000u);
  const __m128i v_zero = _mm_setzero_si128();
  const __m128i v_30 = _mm_set1_epi16(30);
  const __m128i v_59 = _mm_set1_epi16(59);
  const __m128i v_11 = _mm_set1_epi16(11);
  /* Exact division by 100 for all s in [0, 25500]: (s * 41944) >> 22 */
  const __m128i v_div100 = _mm_set1_epi16((short)41944u);

  for (; i + 7 < len; i += 8)
  {
    __m128i px0 = _mm_loadu_si128((const __m128i*)(inframe + i));
    __m128i px1 = _mm_loadu_si128((const __m128i*)(inframe + i + 4));

    __m128i r = _mm_packs_epi32(_mm_and_si128(px0, v_ff), _mm_and_si128(px1, v_ff));
    __m128i g = _mm_packs_epi32(_mm_and_si128(_mm_srli_epi32(px0, 8), v_ff),
                                _mm_and_si128(_mm_srli_epi32(px1, 8), v_ff));
    __m128i b = _mm_packs_epi32(_mm_and_si128(_mm_srli_epi32(px0, 16), v_ff),
                                _mm_and_si128(_mm_srli_epi32(px1, 16), v_ff));

    __m128i sum = _mm_add_epi16(_mm_add_epi16(_mm_mullo_epi16(r, v_30),
                                              _mm_mullo_epi16(g, v_59)),
                                _mm_mullo_epi16(b, v_11));
    __m128i lum = _mm_srli_epi16(_mm_mulhi_epu16(sum, v_div100), 6);

    __m128i l0 = _mm_unpacklo_epi16(lum, v_zero);
    __m128i l1 = _mm_unpackhi_epi16(lum, v_zero);

    __m128i rgb0 = _mm_or_si128(_mm_or_si128(l0, _mm_slli_epi32(l0, 8)), _mm_slli_epi32(l0, 16));
    __m128i rgb1 = _mm_or_si128(_mm_or_si128(l1, _mm_slli_epi32(l1, 8)), _mm_slli_epi32(l1, 16));

    _mm_storeu_si128((__m128i*)(outframe + i),     _mm_or_si128(rgb0, _mm_and_si128(px0, v_alpha_mask)));
    _mm_storeu_si128((__m128i*)(outframe + i + 4), _mm_or_si128(rgb1, _mm_and_si128(px1, v_alpha_mask)));
  }
#endif

  for (; i < len; i++)
  {
    uint32_t p = inframe[i];
    uint32_t r = p & 0xffu;
    uint32_t g = (p >> 8) & 0xffu;
    uint32_t b = (p >> 16) & 0xffu;
    uint32_t l = ((30u * r + 59u * g + 11u * b) * 41944u) >> 22;
    outframe[i] = l | (l << 8) | (l << 16) | (p & 0xff000000u);
  }
}
