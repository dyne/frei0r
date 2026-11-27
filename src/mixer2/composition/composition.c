/* composition.c
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

#if defined(__SSE2__)
#include <emmintrin.h>
#endif

#include "frei0r.h"
#include "frei0r/math.h"

typedef struct composition_instance
{
  unsigned int width;
  unsigned int height;
} composition_instance_t;

int f0r_init()
{
  return 1;
}

void f0r_deinit()
{ /* no initialization required */ }

void f0r_get_plugin_info(f0r_plugin_info_t* compositionInfo)
{
  compositionInfo->name = "Composition";
  compositionInfo->author = "Richard Spindler";
  compositionInfo->plugin_type = F0R_PLUGIN_TYPE_MIXER2;
  compositionInfo->color_model = F0R_COLOR_MODEL_RGBA8888;
  compositionInfo->frei0r_version = FREI0R_MAJOR_VERSION;
  compositionInfo->major_version = 0; 
  compositionInfo->minor_version = 9; 
  compositionInfo->num_params =  0; 
  compositionInfo->explanation = "Composites Image 2 onto Image 1 according to its Alpha Channel";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
  /* no params */
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
  composition_instance_t* inst = (composition_instance_t*)calloc(1, sizeof(*inst));
  inst->width = width; inst->height = height;
  return (f0r_instance_t)inst;
}

void f0r_destruct(f0r_instance_t instance)
{
  free(instance);
}

void f0r_set_param_value(f0r_instance_t instance, 
			 f0r_param_t param, int param_index)
{ /* no params */ }

void f0r_get_param_value(f0r_instance_t instance,
			 f0r_param_t param, int param_index)
{ /* no params */ }


void f0r_update2(f0r_instance_t instance,
		 double time,
		 const uint32_t* inframe1,
		 const uint32_t* inframe2,
		 const uint32_t* inframe3,
		 uint32_t* outframe)
{
  assert(instance);
  composition_instance_t* inst = (composition_instance_t*)instance;
  const size_t total_bytes = (size_t)inst->width * inst->height * 4;

  const unsigned char* __restrict__ ps1 = (const unsigned char*)inframe2;
  const unsigned char* __restrict__ ps2 = (const unsigned char*)inframe1;
  unsigned char* __restrict__ pd = (unsigned char*)outframe;
  size_t i = 0;

#if defined(__SSE2__)
  const __m128i zero = _mm_setzero_si128();
  const __m128i alpha_mask = _mm_set1_epi32((int)0xFF000000u);

  for (; i + 16 <= total_bytes; i += 16) {
    const __m128i v1 = _mm_loadu_si128((const __m128i*)(ps1 + i));
    const __m128i v2 = _mm_loadu_si128((const __m128i*)(ps2 + i));

    const __m128i v1_lo = _mm_unpacklo_epi8(v1, zero);
    const __m128i v1_hi = _mm_unpackhi_epi8(v1, zero);
    const __m128i v2_lo = _mm_unpacklo_epi8(v2, zero);
    const __m128i v2_hi = _mm_unpackhi_epi8(v2, zero);

    const __m128i diff_lo = _mm_sub_epi16(v1_lo, v2_lo);
    const __m128i diff_hi = _mm_sub_epi16(v1_hi, v2_hi);

    /* Broadcast ps1[3] across each pixel's 4 16-bit lanes */
    const __m128i a_lo = _mm_shufflelo_epi16(
        _mm_shufflehi_epi16(v1_lo, _MM_SHUFFLE(3, 3, 3, 3)),
        _MM_SHUFFLE(3, 3, 3, 3));
    const __m128i a_hi = _mm_shufflelo_epi16(
        _mm_shufflehi_epi16(v1_hi, _MM_SHUFFLE(3, 3, 3, 3)),
        _MM_SHUFFLE(3, 3, 3, 3));

    /* w = 255 * a = (a << 8) - a (unsigned 16-bit in [0, 65025]) */
    const __m128i w_lo = _mm_sub_epi16(_mm_slli_epi16(a_lo, 8), a_lo);
    const __m128i w_hi = _mm_sub_epi16(_mm_slli_epi16(a_hi, 8), a_hi);

    /* Exact (diff * (uint16_t)w) >> 16 using signed mulhi + bit-15 correction */
    const __m128i d_lo = _mm_add_epi16(
        _mm_mulhi_epi16(diff_lo, w_lo),
        _mm_and_si128(diff_lo, _mm_srai_epi16(w_lo, 15)));
    const __m128i d_hi = _mm_add_epi16(
        _mm_mulhi_epi16(diff_hi, w_hi),
        _mm_and_si128(diff_hi, _mm_srai_epi16(w_hi, 15)));

    const __m128i rgb = _mm_packus_epi16(_mm_add_epi16(d_lo, v2_lo),
                                         _mm_add_epi16(d_hi, v2_hi));
    const __m128i alpha = _mm_adds_epu8(v1, v2);
    const __m128i res = _mm_or_si128(_mm_andnot_si128(alpha_mask, rgb),
                                     _mm_and_si128(alpha_mask, alpha));
    _mm_storeu_si128((__m128i*)(pd + i), res);
  }
#endif

  for (; i < total_bytes; i += 4) {
    const int w = 255 * ps1[i + 3];
    pd[i + 0] = (unsigned char)((((ps1[i + 0] - ps2[i + 0]) * w) >> 16) + ps2[i + 0]);
    pd[i + 1] = (unsigned char)((((ps1[i + 1] - ps2[i + 1]) * w) >> 16) + ps2[i + 1]);
    pd[i + 2] = (unsigned char)((((ps1[i + 2] - ps2[i + 2]) * w) >> 16) + ps2[i + 2]);
    pd[i + 3] = CLAMP0255(ps1[i + 3] + ps2[i + 3]);
  }
}
