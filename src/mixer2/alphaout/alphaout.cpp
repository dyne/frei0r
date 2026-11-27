/* alphaout.cpp
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

#include "frei0r.hpp"
#include "frei0r/math.h"

#include <algorithm>

#if defined(__SSE2__)
#include <emmintrin.h>
#endif

namespace {

struct Recip24Table {
  uint32_t recip[256];
  Recip24Table() {
    recip[0] = 0;
    /* Exact 24-bit reciprocal for floor(num / d) for num <= 65025, d in [1, 255]. */
    for (uint32_t d = 1; d < 256; ++d) {
      recip[d] = ((1u << 24) + d - 1u) / d;
    }
  }
};

static const Recip24Table kRecip24;

#if defined(__SSE2__)
static inline __m128i sse2_int_mult_epu16(__m128i a, __m128i b) {
  const __m128i c80 = _mm_set1_epi16(0x80);
  const __m128i t = _mm_add_epi16(_mm_mullo_epi16(a, b), c80);
  return _mm_srli_epi16(_mm_add_epi16(t, _mm_srli_epi16(t, 8)), 8);
}

static inline __m128i sse2_scale_pixel_24(__m128i m32, uint32_t wr) {
  const __m128i vwr = _mm_set1_epi32(static_cast<int>(wr));
  const __m128i rb = _mm_srli_epi64(_mm_mul_epu32(m32, vwr), 24);
  const __m128i ga = _mm_srli_epi64(_mm_mul_epu32(_mm_srli_epi64(m32, 32), vwr), 24);
  return _mm_or_si128(rb, _mm_slli_epi64(ga, 32));
}
#endif

} // namespace

class alphaout : public frei0r::mixer2
{
public:
  alphaout(unsigned int width, unsigned int height)
  {
  }

  void update(double time,
              uint32_t* out,
              const uint32_t* in1,
              const uint32_t* in2)
  {
    uint32_t* __restrict__ out32 = out;
    const uint8_t* __restrict__ src1 = reinterpret_cast<const uint8_t*>(in1);
    const uint8_t* __restrict__ src2 = reinterpret_cast<const uint8_t*>(in2);
    const uint32_t* __restrict__ recip = kRecip24.recip;
    const unsigned int n = size;
    unsigned int i = 0;

#if defined(__SSE2__)
    const __m128i zero = _mm_setzero_si128();
    const __m128i alpha_mask = _mm_set1_epi32(static_cast<int>(0xFF000000u));

    for (; i + 4 <= n; i += 4)
    {
      const uint8_t* p1 = src1 + i * 4;
      const uint8_t* p2 = src2 + i * 4;

      /* Compute alpha_dst and w1*recip[alpha_dst] on GPRs in parallel with SSE2 */
      uint32_t t;
      const uint32_t w0 = 0xffu ^ p2[3],  w1 = 0xffu ^ p2[7];
      const uint32_t w2 = 0xffu ^ p2[11], w3 = 0xffu ^ p2[15];
      const uint32_t ad0 = INT_MULT(p1[3],  w0, t);
      const uint32_t ad1 = INT_MULT(p1[7],  w1, t);
      const uint32_t ad2 = INT_MULT(p1[11], w2, t);
      const uint32_t ad3 = INT_MULT(p1[15], w3, t);

      const uint32_t wr0 = w0 * recip[ad0];
      const uint32_t wr1 = w1 * recip[ad1];
      const uint32_t wr2 = w2 * recip[ad2];
      const uint32_t wr3 = w3 * recip[ad3];

      const __m128i v1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p1));
      const __m128i s1_lo = _mm_unpacklo_epi8(v1, zero);
      const __m128i s1_hi = _mm_unpackhi_epi8(v1, zero);

      const __m128i a1_lo = _mm_shufflelo_epi16(
          _mm_shufflehi_epi16(s1_lo, _MM_SHUFFLE(3, 3, 3, 3)),
          _MM_SHUFFLE(3, 3, 3, 3));
      const __m128i a1_hi = _mm_shufflelo_epi16(
          _mm_shufflehi_epi16(s1_hi, _MM_SHUFFLE(3, 3, 3, 3)),
          _MM_SHUFFLE(3, 3, 3, 3));

      const __m128i m_lo = sse2_int_mult_epu16(s1_lo, a1_lo);
      const __m128i m_hi = sse2_int_mult_epu16(s1_hi, a1_hi);

      const __m128i r0 = sse2_scale_pixel_24(_mm_unpacklo_epi16(m_lo, zero), wr0);
      const __m128i r1 = sse2_scale_pixel_24(_mm_unpackhi_epi16(m_lo, zero), wr1);
      const __m128i r2 = sse2_scale_pixel_24(_mm_unpacklo_epi16(m_hi, zero), wr2);
      const __m128i r3 = sse2_scale_pixel_24(_mm_unpackhi_epi16(m_hi, zero), wr3);

      const __m128i rgb = _mm_packus_epi16(_mm_packs_epi32(r0, r1), _mm_packs_epi32(r2, r3));
      const __m128i ad_vec = _mm_set_epi32(
          static_cast<int>(ad3 << 24), static_cast<int>(ad2 << 24),
          static_cast<int>(ad1 << 24), static_cast<int>(ad0 << 24));
      const __m128i res = _mm_or_si128(_mm_andnot_si128(alpha_mask, rgb), ad_vec);
      _mm_storeu_si128(reinterpret_cast<__m128i*>(out32 + i), res);
    }
#endif

    for (; i < n; ++i)
    {
      uint32_t tmp;
      const uint8_t alpha_src1 = src1[i * 4 + 3];
      const uint8_t w1 = 0xff ^ src2[i * 4 + 3];
      const uint8_t alpha_dst = static_cast<uint8_t>(INT_MULT(alpha_src1, w1, tmp));
      const uint64_t w1r = static_cast<uint64_t>(w1) * recip[alpha_dst];

      uint32_t c0 = static_cast<uint32_t>((INT_MULT(src1[i * 4 + 0], alpha_src1, tmp) * w1r) >> 24);
      if (c0 > 255u) c0 = 255u;
      uint32_t c1 = static_cast<uint32_t>((INT_MULT(src1[i * 4 + 1], alpha_src1, tmp) * w1r) >> 24);
      if (c1 > 255u) c1 = 255u;
      uint32_t c2 = static_cast<uint32_t>((INT_MULT(src1[i * 4 + 2], alpha_src1, tmp) * w1r) >> 24);
      if (c2 > 255u) c2 = 255u;

      out32[i] = c0 | (c1 << 8) | (c2 << 16) | (static_cast<uint32_t>(alpha_dst) << 24);
    }
  }
  
};


static frei0r::construct<alphaout> plugin("alphaout",
                                    "the alpha OUT operation",
                                    "Jean-Sebastien Senecal",
                                    0,2,
                                    F0R_COLOR_MODEL_RGBA8888);
