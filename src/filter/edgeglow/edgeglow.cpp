/* edgeglow.cpp
 * Copyright (C) 2008 Salsaman (salsaman@gmail.com)
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
#include <stdint.h>

#if defined(__SSE2__)
#include <emmintrin.h>

static inline __m128i abs_epi16_sse2(__m128i x)
{
  __m128i sign = _mm_srai_epi16(x, 15);
  return _mm_sub_epi16(_mm_xor_si128(x, sign), sign);
}
#endif

struct HueChannelLUT {
  double w[3];
};

class edgeglow : public frei0r::filter
{
public:

  double lthresh;
  double lupscale;
  double lredscale;

  unsigned char *sobel_l_row;
  HueChannelLUT hue_lut[361];

  static double calc_w(float t)
  {
    if (t < 1. / 6.) return 6. * (double)t;
    if (t < 0.5) return 1.0;
    if (t < 2. / 3.) return 6. * (2. / 3. - (double)t);
    return 0.0;
  }

  edgeglow(unsigned int width, unsigned int height)
  {
    lthresh = 0.0;
    lupscale = 0.0;
    lredscale = 0.0;
    register_param(lthresh, "lthresh", "threshold for edge lightening");
    register_param(lupscale, "lupscale", "multiplier for upscaling edge brightness");
    register_param(lredscale, "lredscale", "multiplier for downscaling non-edge brightness");

    sobel_l_row = (unsigned char*)malloc(width > 0 ? width : 1);
    for (unsigned int h = 0; h <= 360; ++h)
    {
      float hk = (float)h / 360.;
      float tr = hk + 1. / 3.;
      float tg = hk;
      float tb = hk - 1. / 3.;
      if (tr > 1.) tr -= 1.;
      if (tb < 0.) tb += 1.;
      hue_lut[h].w[0] = calc_w(tr);
      hue_lut[h].w[1] = calc_w(tg);
      hue_lut[h].w[2] = calc_w(tb);
    }
  }

  ~edgeglow()
  {
    free(sobel_l_row);
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

    // Copy border rows
    memcpy(out, in, (size_t)width * sizeof(uint32_t));
    memcpy(out + (size_t)(height - 1) * width,
           in + (size_t)(height - 1) * width,
           (size_t)width * sizeof(uint32_t));

    const float lt = (float)(lthresh * 255.);
    const double upscale = lupscale;
    const double redscale = lredscale;
    const double one_minus_red = 1. - redscale;
    const bool has_red = (redscale > 0.);

#if defined(__SSE2__)
    const __m128i zero = _mm_setzero_si128();
#endif

    for (unsigned int y = 1; y < height - 1; ++y)
    {
      const uint32_t *r0 = in + (size_t)(y - 1) * width;
      const uint32_t *r1 = in + (size_t)y * width;
      const uint32_t *r2 = in + (size_t)(y + 1) * width;
      uint32_t *dst = out + (size_t)y * width;

      dst[0] = r1[0];
      dst[width - 1] = r1[width - 1];

      // Step 1: Compute Sobel lightness l = (max(R,G,B) + min(R,G,B)) >> 1 for the row
      unsigned int x = 1;
#if defined(__SSE2__)
      for (; x + 4 <= width - 1; x += 4)
      {
        __m128i v1 = _mm_loadu_si128((const __m128i*)(r0 + x - 1));
        __m128i v2 = _mm_loadu_si128((const __m128i*)(r0 + x));
        __m128i v3 = _mm_loadu_si128((const __m128i*)(r0 + x + 1));
        __m128i v4 = _mm_loadu_si128((const __m128i*)(r1 + x - 1));
        __m128i v6 = _mm_loadu_si128((const __m128i*)(r1 + x + 1));
        __m128i v7 = _mm_loadu_si128((const __m128i*)(r2 + x - 1));
        __m128i v8 = _mm_loadu_si128((const __m128i*)(r2 + x));
        __m128i v9 = _mm_loadu_si128((const __m128i*)(r2 + x + 1));

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

        __m128i sob = _mm_packus_epi16(magl, magh);
        alignas(16) unsigned char sb[16];
        _mm_store_si128((__m128i*)sb, sob);

        for (int k = 0; k < 4; ++k)
        {
          unsigned char R = sb[k * 4 + 0];
          unsigned char G = sb[k * 4 + 1];
          unsigned char B = sb[k * 4 + 2];
          unsigned char mx = R > G ? R : G;
          if (B > mx) mx = B;
          unsigned char mn = R < G ? R : G;
          if (B < mn) mn = B;
          sobel_l_row[x + k] = (unsigned char)((mx + mn) >> 1);
        }
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

        unsigned char g0 = CLAMP0255(
          abs(p1[0] + p2[0]*2 + p3[0] - p7[0] - p8[0]*2 - p9[0]) +
          abs(p3[0] + p6[0]*2 + p9[0] - p1[0] - p4[0]*2 - p7[0]));
        unsigned char g1 = CLAMP0255(
          abs(p1[1] + p2[1]*2 + p3[1] - p7[1] - p8[1]*2 - p9[1]) +
          abs(p3[1] + p6[1]*2 + p9[1] - p1[1] - p4[1]*2 - p7[1]));
        unsigned char g2 = CLAMP0255(
          abs(p1[2] + p2[2]*2 + p3[2] - p7[2] - p8[2]*2 - p9[2]) +
          abs(p3[2] + p6[2]*2 + p9[2] - p1[2] - p4[2]*2 - p7[2]));

        unsigned char mx = g0 > g1 ? g0 : g1;
        if (g2 > mx) mx = g2;
        unsigned char mn = g0 < g1 ? g0 : g1;
        if (g2 < mn) mn = g2;
        sobel_l_row[x] = (unsigned char)((mx + mn) >> 1);
      }

      // Step 2: Apply HSL lightness adjustment
      for (x = 1; x < width - 1; ++x)
      {
        unsigned char l = sobel_l_row[x];
        const unsigned char *p5 = (const unsigned char *)&r1[x];
        unsigned char *g = (unsigned char *)&dst[x];

        if (!has_red && !(l > lt))
        {
          dst[x] = r1[x];
          continue;
        }

        unsigned char R = p5[0];
        unsigned char G = p5[1];
        unsigned char B = p5[2];
        unsigned char mx = R > G ? R : G;
        if (B > mx) mx = B;
        unsigned char mn = R < G ? R : G;
        if (B < mn) mn = B;

        double half_sum = ((float)mx + (float)mn) * 0.5;
        if (l > lt)
        {
          l = CLAMP0255((int32_t)((float)l * upscale + half_sum));
        }
        else
        {
          l = (unsigned char)(half_sum * one_minus_red);
        }

        if (has_red || l > lt)
        {
          g[3] = p5[3];
          if (l == 0)
          {
            g[0] = 0;
            g[1] = 0;
            g[2] = 0;
            continue;
          }
          if (mx == mn)
          {
            g[0] = l;
            g[1] = l;
            g[2] = l;
            continue;
          }

          int diff_i = (int)mx - (int)mn;
          int num = (mx == R) ? ((int)G - (int)B) : ((mx == G) ? ((int)B - (int)R) : ((int)R - (int)G));
          double off = (mx == R) ? 0.0 : ((mx == G) ? 120.0 : 240.0);
          unsigned int h = (unsigned int)(60.0 * (double)num / (double)diff_i + off) + ((mx == R && G < B) ? 360u : 0u);
          float s = (float)((double)diff_i / (2.0 - ((double)mx + (double)mn)));
          float fl = (float)l;
          float q = fl + s - (fl * s);
          float p = 2.f * fl - q;

          if (h <= 360)
          {
            const double *w = hue_lut[h].w;
            double qp = (double)(q - p);
            g[0] = CLAMP0255((double)p + qp * w[0]);
            g[1] = CLAMP0255((double)p + qp * w[1]);
            g[2] = CLAMP0255((double)p + qp * w[2]);
          }
          else
          {
            float hk = (float)h / 360.;
            float tr = hk + 1. / 3.;
            float tg = hk;
            float tb = hk - 1. / 3.;
            if (tr > 1.) tr -= 1.;
            if (tb < 0.) tb += 1.;

            if (tr < 1./6.) g[0] = CLAMP0255(p + ((q - p) * 6. * tr));
            else if (tr < 0.5) g[0] = CLAMP0255((int32_t)q);
            else if (tr < 2./3.) g[0] = CLAMP0255(p + ((q - p) * 6. * (2./3. - tr)));
            else g[0] = CLAMP0255((int32_t)p);

            if (tg < 1./6.) g[1] = CLAMP0255(p + ((q - p) * 6. * tg));
            else if (tg < 0.5) g[1] = CLAMP0255((int32_t)q);
            else if (tg < 2./3.) g[1] = CLAMP0255(p + ((q - p) * 6. * (2./3. - tg)));
            else g[1] = CLAMP0255((int32_t)p);

            if (tb < 1./6.) g[2] = CLAMP0255(p + ((q - p) * 6. * tb));
            else if (tb < 0.5) g[2] = CLAMP0255((int32_t)q);
            else if (tb < 2./3.) g[2] = CLAMP0255(p + ((q - p) * 6. * (2./3. - tb)));
            else g[2] = CLAMP0255((int32_t)p);
          }
        }
        else
        {
          dst[x] = r1[x];
        }
      }
    }
  }
};


static frei0r::construct<edgeglow> plugin("Edgeglow",
                                "Edgeglow filter",
                                "Salsaman",
                                0,3,
                                F0R_COLOR_MODEL_RGBA8888);
