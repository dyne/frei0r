/* blend.cpp
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

#include <cstring>

#if defined(__SSE2__)
#include <emmintrin.h>
#endif

#define NBYTES 4
#define ALPHA 3

class blend : public frei0r::mixer2
{
public:
  blend(unsigned int width, unsigned int height)
  {
    this->width = width;
    this->height = height;
    this->size = width * height;
  	blend_factor = 0.5;
  	register_param(blend_factor,"blend","blend factor");
  }

  /**
   *
   * Perform a blend operation between sources in1 and in2, using
   * the generalised algorithm: D = A * (255 - &beta;) + B * &beta;
   *
   * The result is left in out
   **/
  void update(double time,
              uint32_t* out,
              const uint32_t* in1,
              const uint32_t* in2)
  {
    // Validate inputs
    if (!out || !in1 || !in2) {
      return;
    }

    const uint32_t w = size;
    // Validate size
    if (w == 0) {
      return;
    }

    const uint8_t* __restrict__ src1 = reinterpret_cast<const uint8_t*>(in1);
    const uint8_t* __restrict__ src2 = reinterpret_cast<const uint8_t*>(in2);
    uint8_t* __restrict__ dst = reinterpret_cast<uint8_t*>(out);
    const uint8_t bf = static_cast<uint8_t>(255 * blend_factor);
    const uint8_t one_minus_bf = static_cast<uint8_t>(255 - bf);
    const size_t total_bytes = static_cast<size_t>(w) * NBYTES;

    if (bf == 0) {
      std::memcpy(dst, src1, total_bytes);
      return;
    }
    if (bf == 255) {
      std::memcpy(dst, src2, total_bytes);
      return;
    }

    size_t i = 0;

#if defined(__SSE2__)
    const __m128i zero = _mm_setzero_si128();
    const __m128i v_bf = _mm_set1_epi16(bf);
    const __m128i v_ombf = _mm_set1_epi16(one_minus_bf);
    const __m128i v_one = _mm_set1_epi16(1);

    for (; i + 16 <= total_bytes; i += 16)
      {
        const __m128i va = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src1 + i));
        const __m128i vb = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src2 + i));

        const __m128i a_lo = _mm_unpacklo_epi8(va, zero);
        const __m128i a_hi = _mm_unpackhi_epi8(va, zero);
        const __m128i b_lo = _mm_unpacklo_epi8(vb, zero);
        const __m128i b_hi = _mm_unpackhi_epi8(vb, zero);

        const __m128i sum_lo = _mm_add_epi16(_mm_mullo_epi16(a_lo, v_ombf),
                                             _mm_mullo_epi16(b_lo, v_bf));
        const __m128i sum_hi = _mm_add_epi16(_mm_mullo_epi16(a_hi, v_ombf),
                                             _mm_mullo_epi16(b_hi, v_bf));

        /* Exact floor(sum / 255) for sum in [0, 65025]: (sum + 1 + (sum >> 8)) >> 8 */
        const __m128i res_lo = _mm_srli_epi16(
            _mm_add_epi16(_mm_add_epi16(sum_lo, v_one), _mm_srli_epi16(sum_lo, 8)), 8);
        const __m128i res_hi = _mm_srli_epi16(
            _mm_add_epi16(_mm_add_epi16(sum_hi, v_one), _mm_srli_epi16(sum_hi, 8)), 8);

        _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + i),
                         _mm_packus_epi16(res_lo, res_hi));
      }
#endif

    for (; i < total_bytes; i++)
      {
        const uint32_t sum = static_cast<uint32_t>(src1[i]) * one_minus_bf
                           + static_cast<uint32_t>(src2[i]) * bf;
        dst[i] = static_cast<uint8_t>((sum + 1u + (sum >> 8)) >> 8);
      }
  }
  
private:
  double blend_factor;
    
};


static frei0r::construct<blend> plugin("blend",
                                "Perform a blend operation between two sources",
                                "Jean-Sebastien Senecal",
                                0,2,
                                F0R_COLOR_MODEL_RGBA8888);
