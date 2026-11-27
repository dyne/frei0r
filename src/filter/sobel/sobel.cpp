/* sobel.cpp
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
#include <stdlib.h>
#include <string.h>

#if defined(__SSE2__)
#include <emmintrin.h>

static inline __m128i abs_epi16_sse2(__m128i x)
{
  __m128i sign = _mm_srai_epi16(x, 15);
  return _mm_sub_epi16(_mm_xor_si128(x, sign), sign);
}
#endif

class sobel : public frei0r::filter
{
public:
  sobel(unsigned int width, unsigned int height)
  {
    this->width = width;
    this->height = height;
  }
  
  virtual void update(double time,
                      uint32_t* out,
                      const uint32_t* in)
  {
    if (width == 0 || height == 0) return;

    if (width < 3 || height < 3) {
      memcpy(out, in, (size_t)width * height * sizeof(uint32_t));
      return;
    }

    // Copy top and bottom border rows
    memcpy(out, in, (size_t)width * sizeof(uint32_t));
    memcpy(out + (size_t)(height - 1) * width,
           in + (size_t)(height - 1) * width,
           (size_t)width * sizeof(uint32_t));

#if defined(__SSE2__)
    const __m128i zero = _mm_setzero_si128();
    const __m128i rgb_mask = _mm_set1_epi32(0x00FFFFFF);
    const __m128i a_mask = _mm_set1_epi32((int)0xFF000000u);
#endif

    for (unsigned int y = 1; y < height - 1; ++y)
    {
      const uint32_t *r0 = in + (size_t)(y - 1) * width;
      const uint32_t *r1 = in + (size_t)y * width;
      const uint32_t *r2 = in + (size_t)(y + 1) * width;
      uint32_t *dst = out + (size_t)y * width;

      dst[0] = r1[0];
      dst[width - 1] = r1[width - 1];

      unsigned int x = 1;
#if defined(__SSE2__)
      for (; x + 4 <= width - 1; x += 4)
      {
        __m128i v1 = _mm_loadu_si128((const __m128i*)(r0 + x - 1));
        __m128i v2 = _mm_loadu_si128((const __m128i*)(r0 + x));
        __m128i v3 = _mm_loadu_si128((const __m128i*)(r0 + x + 1));
        __m128i v4 = _mm_loadu_si128((const __m128i*)(r1 + x - 1));
        __m128i v5 = _mm_loadu_si128((const __m128i*)(r1 + x));
        __m128i v6 = _mm_loadu_si128((const __m128i*)(r1 + x + 1));
        __m128i v7 = _mm_loadu_si128((const __m128i*)(r2 + x - 1));
        __m128i v8 = _mm_loadu_si128((const __m128i*)(r2 + x));
        __m128i v9 = _mm_loadu_si128((const __m128i*)(r2 + x + 1));

        // Low 2 pixels (8 bytes -> 8 int16)
        __m128i p1l = _mm_unpacklo_epi8(v1, zero);
        __m128i p2l = _mm_unpacklo_epi8(v2, zero);
        __m128i p3l = _mm_unpacklo_epi8(v3, zero);
        __m128i p4l = _mm_unpacklo_epi8(v4, zero);
        __m128i p6l = _mm_unpacklo_epi8(v6, zero);
        __m128i p7l = _mm_unpacklo_epi8(v7, zero);
        __m128i p8l = _mm_unpacklo_epi8(v8, zero);
        __m128i p9l = _mm_unpacklo_epi8(v9, zero);

        __m128i gyl = _mm_sub_epi16(
          _mm_add_epi16(_mm_add_epi16(p1l, p3l), _mm_slli_epi16(p2l, 1)),
          _mm_add_epi16(_mm_add_epi16(p7l, p9l), _mm_slli_epi16(p8l, 1)));
        __m128i gxl = _mm_sub_epi16(
          _mm_add_epi16(_mm_add_epi16(p3l, p9l), _mm_slli_epi16(p6l, 1)),
          _mm_add_epi16(_mm_add_epi16(p1l, p7l), _mm_slli_epi16(p4l, 1)));
        __m128i magl = _mm_add_epi16(abs_epi16_sse2(gyl), abs_epi16_sse2(gxl));

        // High 2 pixels (8 bytes -> 8 int16)
        __m128i p1h = _mm_unpackhi_epi8(v1, zero);
        __m128i p2h = _mm_unpackhi_epi8(v2, zero);
        __m128i p3h = _mm_unpackhi_epi8(v3, zero);
        __m128i p4h = _mm_unpackhi_epi8(v4, zero);
        __m128i p6h = _mm_unpackhi_epi8(v6, zero);
        __m128i p7h = _mm_unpackhi_epi8(v7, zero);
        __m128i p8h = _mm_unpackhi_epi8(v8, zero);
        __m128i p9h = _mm_unpackhi_epi8(v9, zero);

        __m128i gyh = _mm_sub_epi16(
          _mm_add_epi16(_mm_add_epi16(p1h, p3h), _mm_slli_epi16(p2h, 1)),
          _mm_add_epi16(_mm_add_epi16(p7h, p9h), _mm_slli_epi16(p8h, 1)));
        __m128i gxh = _mm_sub_epi16(
          _mm_add_epi16(_mm_add_epi16(p3h, p9h), _mm_slli_epi16(p6h, 1)),
          _mm_add_epi16(_mm_add_epi16(p1h, p7h), _mm_slli_epi16(p4h, 1)));
        __m128i magh = _mm_add_epi16(abs_epi16_sse2(gyh), abs_epi16_sse2(gxh));

        __m128i res = _mm_packus_epi16(magl, magh);
        res = _mm_or_si128(_mm_and_si128(res, rgb_mask), _mm_and_si128(v5, a_mask));
        _mm_storeu_si128((__m128i*)(dst + x), res);
      }
#endif
      for (; x < width - 1; ++x)
      {
        const unsigned char *p1 = (const unsigned char *)&r0[x - 1];
        const unsigned char *p2 = (const unsigned char *)&r0[x];
        const unsigned char *p3 = (const unsigned char *)&r0[x + 1];
        const unsigned char *p4 = (const unsigned char *)&r1[x - 1];
        const unsigned char *p6 = (const unsigned char *)&r1[x + 1];
        const unsigned char *p7 = (const unsigned char *)&r2[x - 1];
        const unsigned char *p8 = (const unsigned char *)&r2[x];
        const unsigned char *p9 = (const unsigned char *)&r2[x + 1];

        unsigned char *g = (unsigned char *)&dst[x];

        for (int i = 0; i < 3; ++i)
          g[i] = CLAMP0255(
            abs(p1[i] + p2[i]*2 + p3[i] - p7[i] - p8[i]*2 - p9[i]) +
            abs(p3[i] + p6[i]*2 + p9[i] - p1[i] - p4[i]*2 - p7[i]) );

        g[3] = ((const unsigned char*)&r1[x])[3];
      }
    }
  }
};


static frei0r::construct<sobel> plugin("Sobel",
                                "Sobel filter",
                                "Jean-Sebastien Senecal (Drone)",
                                0,2,
                                F0R_COLOR_MODEL_RGBA8888);
