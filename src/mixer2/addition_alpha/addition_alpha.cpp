/* addition_alpha.cpp
 * Copyright (C) 2006 Jean-Sebastien Senecal (js@drone.ws)
 *               2011 Simon A. Eugster (simon.eu@gmail.com)
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

namespace {

class addition_alpha : public frei0r::mixer2
{
public:
  addition_alpha(unsigned int width, unsigned int height)
  {
  }

  /**
   *
   * Perform an RGB[A] addition_alpha operation of the pixel sources in1
   * and in2.
   *
   * This is a modification of the original addition filter, adding the color of the
   * second channel only if its alpha channel is not 0.
   *
   **/
  void update(double time,
              uint32_t* out,
              const uint32_t* in1,
              const uint32_t* in2)
  {
    const uint8_t* __restrict__ A = reinterpret_cast<const uint8_t*>(in1);
    const uint8_t* __restrict__ B = reinterpret_cast<const uint8_t*>(in2);
    uint8_t* __restrict__ D = reinterpret_cast<uint8_t*>(out);
    const size_t total_bytes = static_cast<size_t>(size) * NBYTES;
    size_t i = 0;

#if defined(__SSE2__)
    const __m128i zero = _mm_setzero_si128();
    const __m128i alpha_255 = _mm_set1_epi32(static_cast<int>(0xFF000000u));
    for (; i + 16 <= total_bytes; i += 16)
      {
        const __m128i va = _mm_loadu_si128(reinterpret_cast<const __m128i*>(A + i));
        const __m128i vb = _mm_loadu_si128(reinterpret_cast<const __m128i*>(B + i));

        const __m128i b_lo = _mm_unpacklo_epi8(vb, zero);
        const __m128i b_hi = _mm_unpackhi_epi8(vb, zero);

        const __m128i ba_lo = _mm_shufflelo_epi16(
            _mm_shufflehi_epi16(b_lo, _MM_SHUFFLE(3, 3, 3, 3)),
            _MM_SHUFFLE(3, 3, 3, 3));
        const __m128i ba_hi = _mm_shufflelo_epi16(
            _mm_shufflehi_epi16(b_hi, _MM_SHUFFLE(3, 3, 3, 3)),
            _MM_SHUFFLE(3, 3, 3, 3));

        const __m128i scaled_lo = _mm_srli_epi16(_mm_mullo_epi16(b_lo, ba_lo), 8);
        const __m128i scaled_hi = _mm_srli_epi16(_mm_mullo_epi16(b_hi, ba_hi), 8);
        const __m128i scaled = _mm_packus_epi16(scaled_lo, scaled_hi);

        const __m128i res = _mm_or_si128(_mm_adds_epu8(va, scaled), alpha_255);
        _mm_storeu_si128(reinterpret_cast<__m128i*>(D + i), res);
      }
#endif

    for (; i < total_bytes; i += NBYTES)
      {
        const uint32_t ba = B[i + ALPHA];
        for (uint32_t b = 0; b < ALPHA; b++)
          {
            const uint32_t sum = static_cast<uint32_t>(A[i + b]) + ((B[i + b] * ba) >> 8);
            D[i + b] = static_cast<uint8_t>(sum > 255u ? 255u : sum);
          }
        D[i + ALPHA] = 255;
      }
  }
};

} // namespace

static frei0r::construct<addition_alpha> plugin("addition_alpha",
                                  "Perform an RGB[A] addition_alpha operation of the pixel sources.",
                                  "Jean-Sebastien Senecal",
                                  0,2,
                                  F0R_COLOR_MODEL_RGBA8888);
