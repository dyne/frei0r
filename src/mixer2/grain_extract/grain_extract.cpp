/* grain_extract.cpp
 * Copyright (C) 2006 Jean-Sebastien Senecal (js@drone.ws)
 * This file is a Frei0r plugin.
 * The code is a modified version of code from the Gimp.
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

#include "frei0r.hpp"
#include "frei0r/math.h"

#if defined(__SSE2__)
#include <emmintrin.h>
#endif

#define NBYTES 4
#define ALPHA 3

class grain_extract : public frei0r::mixer2
{
public:
  grain_extract(unsigned int width, unsigned int height)
  {
  }

  /**
   *
   * Perform an RGB[A] grain-extract operation between the pixel sources
   * in1 and in2.
   *
   **/
  void update(double time,
              uint32_t* out,
              const uint32_t* in1,
              const uint32_t* in2)
  {
    const uint8_t* __restrict__ src1 = reinterpret_cast<const uint8_t*>(in1);
    const uint8_t* __restrict__ src2 = reinterpret_cast<const uint8_t*>(in2);
    uint8_t* __restrict__ dst = reinterpret_cast<uint8_t*>(out);
    const size_t total_bytes = static_cast<size_t>(size) * NBYTES;
    size_t i = 0;

#if defined(__SSE2__)
    const __m128i sign = _mm_set1_epi8(static_cast<char>(0x80));
    const __m128i alpha_mask = _mm_set1_epi32(static_cast<int>(0xFF000000u));
    for (; i + 16 <= total_bytes; i += 16)
      {
        const __m128i va = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src1 + i));
        const __m128i vb = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src2 + i));
        /* clamp(a - b + 128, 0, 255) == sat_i8((a ^ 0x80) - (b ^ 0x80)) ^ 0x80 */
        const __m128i rgb = _mm_xor_si128(
            _mm_subs_epi8(_mm_xor_si128(va, sign), _mm_xor_si128(vb, sign)),
            sign);
        const __m128i a = _mm_min_epu8(va, vb);
        const __m128i res = _mm_or_si128(_mm_andnot_si128(alpha_mask, rgb),
                                         _mm_and_si128(alpha_mask, a));
        _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + i), res);
      }
#endif

    for (; i < total_bytes; i += NBYTES)
      {
        for (uint32_t b = 0; b < ALPHA; b++)
          {
            const int diff = static_cast<int>(src1[i + b]) - src2[i + b] + 128;
            dst[i + b] = static_cast<uint8_t>(CLAMP0255(diff));
          }
        dst[i + ALPHA] = MIN(src1[i + ALPHA], src2[i + ALPHA]);
      }
  }
};


static frei0r::construct<grain_extract> plugin("grain_extract",
                                        "Perform an RGB[A] grain-extract operation between the pixel sources.",
                                        "Jean-Sebastien Senecal",
                                        0,2,
                                        F0R_COLOR_MODEL_RGBA8888);
