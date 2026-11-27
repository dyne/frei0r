/* Cartoon filter
 * main algorithm: (c) Copyright 2003 Dries Pruimboom <dries@irssystems.nl>
 * further optimizations and frei0r port by Denis Rojo <jaromil@dyne.org>
 * 
 * This source code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Public License as published 
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * This source code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * Please refer to the GNU Public License for more details.
 *
 * You should have received a copy of the GNU Public License along with
 * this source code; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * "$Id: cartoon.c 193 2004-06-01 11:00:25Z jaromil $"
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#if defined(__SSE2__)
#include <emmintrin.h>
#endif

#include <frei0r.hpp>

namespace {

static inline int gm_error(uint32_t c1, uint32_t c2) {
  int dr = (int)((c1 >> 16) & 0xFFu) - (int)((c2 >> 16) & 0xFFu);
  int dg = (int)((c1 >> 8) & 0xFFu) - (int)((c2 >> 8) & 0xFFu);
  int db = (int)(c1 & 0xFFu) - (int)(c2 & 0xFFu);
  return dr * dr + dg * dg + db * db;
}

#if defined(__SSE2__)
/* Computes squared RGB distance for 4 packed RGBA pixels in parallel. */
static inline __m128i gm_error_sse2(__m128i a, __m128i b, __m128i zero, __m128i rgb_mask) {
  a = _mm_and_si128(a, rgb_mask);
  b = _mm_and_si128(b, rgb_mask);
  __m128i d_lo = _mm_sub_epi16(_mm_unpacklo_epi8(a, zero), _mm_unpacklo_epi8(b, zero));
  __m128i d_hi = _mm_sub_epi16(_mm_unpackhi_epi8(a, zero), _mm_unpackhi_epi8(b, zero));
  /* Each pair of int32 lanes holds [db^2 + dg^2, dr^2 + 0] for one pixel */
  __m128i sq_lo = _mm_madd_epi16(d_lo, d_lo);
  __m128i sq_hi = _mm_madd_epi16(d_hi, d_hi);
  /* Gather even lanes [e0_bg, e1_bg, e2_bg, e3_bg] and odd lanes [e0_r, e1_r, e2_r, e3_r] */
  __m128i bg = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(sq_lo), _mm_castsi128_ps(sq_hi), _MM_SHUFFLE(2, 0, 2, 0)));
  __m128i r  = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(sq_lo), _mm_castsi128_ps(sq_hi), _MM_SHUFFLE(3, 1, 3, 1)));
  return _mm_add_epi32(bg, r);
}
#endif

class Cartoon: public frei0r::filter {
public:

  double triplevel;
  double diffspace;

  Cartoon(unsigned int width, unsigned int height) {
    register_param(triplevel, "triplevel", "level of trip: mapped to [0,1] asymptotical");
    register_param(diffspace, "diffspace", "difference space: a value from 0 to 256 (mapped to [0,1])");

    m_w = (int)width;
    m_h = (int)height;
    black = 0xFF000000u;

    triplevel = 1 - 1 / (1000 + 1);
    diffspace = 1 / 256.;
  }
  
  ~Cartoon() {
  }

  virtual void update(double time,
                      uint32_t* out,
                      const uint32_t* in) {
    (void)time;
    const int d = (int)(diffspace * 256);
    const int w = m_w;
    const int h = m_h;
    const int x_end = w - (1 + d);
    const int y_end = h - (1 + d);
    if (d >= x_end || d >= y_end) return;

    const double thresh_d = 1.0 / (1.0 - triplevel) - 1.0;
    int thresh_i;
    if (thresh_d < 0.0) {
      thresh_i = -1;
    } else if (thresh_d >= 200000.0) {
      thresh_i = 200000;
    } else {
      thresh_i = (int)floor(thresh_d);
    }

    const uint32_t blk = black;

#if defined(__SSE2__)
    const __m128i zero = _mm_setzero_si128();
    const __m128i rgb_mask = _mm_set1_epi32(0x00FFFFFF);
    const __m128i flat_mask = _mm_set1_epi32((int)0xFFE0E0E0u);
    const __m128i vblack = _mm_set1_epi32((int)blk);
    const __m128i vthresh = _mm_set1_epi32(thresh_i);
#endif

    for (int y = d; y < y_end; ++y) {
      const uint32_t* __restrict__ row_c = in + (size_t)y * w;
      const uint32_t* __restrict__ row_u = in + (size_t)(y - d) * w;
      const uint32_t* __restrict__ row_d = in + (size_t)(y + d) * w;
      uint32_t* __restrict__ out_row = out + (size_t)y * w;

      int x = d;
#if defined(__SSE2__)
      for (; x + 4 <= x_end; x += 4) {
        __m128i e0 = gm_error_sse2(_mm_loadu_si128((const __m128i*)(row_c + x - d)),
                                   _mm_loadu_si128((const __m128i*)(row_c + x + d)),
                                   zero, rgb_mask);
        __m128i e1 = gm_error_sse2(_mm_loadu_si128((const __m128i*)(row_u + x)),
                                   _mm_loadu_si128((const __m128i*)(row_d + x)),
                                   zero, rgb_mask);
        __m128i e2 = gm_error_sse2(_mm_loadu_si128((const __m128i*)(row_u + x - d)),
                                   _mm_loadu_si128((const __m128i*)(row_d + x + d)),
                                   zero, rgb_mask);
        __m128i e3 = gm_error_sse2(_mm_loadu_si128((const __m128i*)(row_u + x + d)),
                                   _mm_loadu_si128((const __m128i*)(row_d + x - d)),
                                   zero, rgb_mask);

        __m128i m01 = _mm_or_si128(_mm_cmpgt_epi32(e0, vthresh), _mm_cmpgt_epi32(e1, vthresh));
        __m128i m23 = _mm_or_si128(_mm_cmpgt_epi32(e2, vthresh), _mm_cmpgt_epi32(e3, vthresh));
        __m128i is_edge = _mm_or_si128(m01, m23);

        __m128i flat = _mm_and_si128(_mm_loadu_si128((const __m128i*)(row_c + x)), flat_mask);
        __m128i res = _mm_or_si128(_mm_and_si128(is_edge, vblack),
                                   _mm_andnot_si128(is_edge, flat));
        _mm_storeu_si128((__m128i*)(out_row + x), res);
      }
#endif
      for (; x < x_end; ++x) {
        if (gm_error(row_c[x - d], row_c[x + d]) > thresh_i ||
            gm_error(row_u[x],     row_d[x])     > thresh_i ||
            gm_error(row_u[x - d], row_d[x + d]) > thresh_i ||
            gm_error(row_u[x + d], row_d[x - d]) > thresh_i) {
          out_row[x] = blk;
        } else {
          out_row[x] = row_c[x] & 0xFFE0E0E0u;
        }
      }
    }
  }

private:
  int m_w;
  int m_h;
  uint32_t black;
};

} // namespace

static frei0r::construct<Cartoon> plugin("Cartoon",
				  "Cartoonify video, do a form of edge detect",
				  "Dries Pruimboom, Jaromil",
				  2,2);
