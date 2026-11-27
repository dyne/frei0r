/*
 * Copyright (C) 2011 Simon Andreas Eugster (simon.eu@gmail.com)
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

#if defined(_MSC_VER)
#define _USE_MATH_DEFINES
#endif /* _MSC_VER */
#include <cmath>
#include <cstdint>
#if defined(__SSE2__)
#include <emmintrin.h>
#endif
#include "frei0r.hpp"
#include "frei0r/math.h"


/**
  Lens vignetting effect.

  This effect simulates «natural vignetting» whose light falloff can be described
  with a cos⁴ curve. Additionally the x:y aspect ratio of the vignette can be
  changed (note that normal cameras with a round aperture always have an aspect ratio
  of 1:1, but for cinematic effects the aspect ratio is often adjusted to match
  the frame's aspect ratio). The ClearCenter value allows one to shift the
  vignetting away from the center, preserving it from changes.

  */
class Vignette : public frei0r::filter
{

public:

    double m_aspect; ///< Neutral value: 0.5
    double m_cc; ///< Neutral value: 0
    double m_soft; ///< Suggested value: 0.6

    Vignette(unsigned int width, unsigned int height) :
        m_width(width),
        m_height(height)
    {
        register_param(m_aspect, "aspect", "Aspect ratio");
        register_param(m_cc, "clearCenter", "Size of the unaffected center");
        register_param(m_soft, "soft", "Softness");

        // Suggested default values
        m_aspect = .5;
        m_cc = 0;
        m_soft = .6;

        m_initialized = width*height > 0;
        if (m_initialized) {
            m_vignette = new float[width*height];
            updateVignette();
        }
    }

    ~Vignette()
    {
        if (m_initialized) {
            delete[] m_vignette;
        }
    }

    virtual void update(double time,
                        uint32_t* out,
                        const uint32_t* in)
    {
        (void)time;

        // Rebuild the vignette matrix if a parameter has changed
        if (m_prev_aspect != m_aspect
                || m_prev_cc != m_cc
                || m_prev_soft != m_soft) {
            updateVignette();
        }

        const unsigned int n = size;
        const uint32_t* __restrict__ src = in;
        uint32_t* __restrict__ dst = out;
        const float* __restrict__ vig = m_vignette;

        unsigned int i = 0;
#if defined(__SSE2__)
        const __m128i zero = _mm_setzero_si128();
        const __m128i alpha_mask = _mm_set1_epi32((int)0xFF000000u);
        const __m128i rgb_mask = _mm_set1_epi32(0x00FFFFFF);

        for (; i + 4 <= n; i += 4) {
            __m128i px = _mm_loadu_si128((const __m128i*)(src + i));
            __m128 v = _mm_loadu_ps(vig + i);

            __m128i lo16 = _mm_unpacklo_epi8(px, zero);
            __m128i hi16 = _mm_unpackhi_epi8(px, zero);

            __m128 p0 = _mm_cvtepi32_ps(_mm_unpacklo_epi16(lo16, zero));
            __m128 p1 = _mm_cvtepi32_ps(_mm_unpackhi_epi16(lo16, zero));
            __m128 p2 = _mm_cvtepi32_ps(_mm_unpacklo_epi16(hi16, zero));
            __m128 p3 = _mm_cvtepi32_ps(_mm_unpackhi_epi16(hi16, zero));

            __m128 v0 = _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0));
            __m128 v1 = _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1));
            __m128 v2 = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2));
            __m128 v3 = _mm_shuffle_ps(v, v, _MM_SHUFFLE(3, 3, 3, 3));

            __m128i r0 = _mm_cvttps_epi32(_mm_mul_ps(p0, v0));
            __m128i r1 = _mm_cvttps_epi32(_mm_mul_ps(p1, v1));
            __m128i r2 = _mm_cvttps_epi32(_mm_mul_ps(p2, v2));
            __m128i r3 = _mm_cvttps_epi32(_mm_mul_ps(p3, v3));

            __m128i out_lo16 = _mm_packs_epi32(r0, r1);
            __m128i out_hi16 = _mm_packs_epi32(r2, r3);
            __m128i out_rgb = _mm_packus_epi16(out_lo16, out_hi16);

            __m128i res = _mm_or_si128(_mm_and_si128(out_rgb, rgb_mask),
                                       _mm_and_si128(px, alpha_mask));
            _mm_storeu_si128((__m128i*)(dst + i), res);
        }
#endif
        const unsigned char* pixel = (const unsigned char*)(src + i);
        unsigned char* dest = (unsigned char*)(dst + i);
        for (; i < n; i++) {
            float v = vig[i];
            *dest++ = (unsigned char)(char)(v * *pixel++);
            *dest++ = (unsigned char)(char)(v * *pixel++);
            *dest++ = (unsigned char)(char)(v * *pixel++);
            *dest++ = *pixel++;
        }
    }

private:
    double m_prev_aspect;
    double m_prev_cc;
    double m_prev_soft;

    float *m_vignette;
    bool m_initialized;

    unsigned int m_width;
    unsigned int m_height;

    void updateVignette()
    {
        m_prev_aspect = m_aspect;
        m_prev_cc = m_cc;
        m_prev_soft = m_soft;

        float soft = 5*std::pow(float(1)-m_soft,2)+.01;
        float scaleX = 1;
        float scaleY = 1;

        // Distance from 0.5 (\in [0,0.5]) scaled to [0,1]
        float scale = std::fabs(m_aspect-.5)*2;
        // Map scale to [0,5] in a way that values near 0 can be adjusted more precisely
        scale = 1 + 4*std::pow(scale, 3);
        // Scale either x or y, depending on the aspect value being above or below 0.5
        if (m_aspect > 0.5) {
            scaleX = scale;
        } else {
            scaleY = scale;
        }

        int cx = m_width/2;
        int cy = m_height/2;
        float rmax = std::sqrt(std::pow(float(cx), 2) + std::pow(float(cy), 2));
        float r;

        for (unsigned int y = 0; y < m_height; y++) {
            for (unsigned int x = 0; x < m_width; x++) {

                // Euclidean distance to the center, normalized to [0,1]
                r = std::sqrt(std::pow(scaleX*(int(x)-cx), 2) + std::pow(scaleY*(int(y)-cy), 2))/rmax;

                // Subtract the clear center
                r -= m_cc;

                if (r <= 0) {
                    // Clear center: Do not modify the brightness here
                    m_vignette[y*m_width+x] = 1;
                } else {
                    r *= soft;
                    if (r > M_PI_2) {
                        m_vignette[y*m_width+x] = 0;
                    } else {
                        m_vignette[y*m_width+x] = std::pow(std::cos(r), 4);
                    }
                }

            }
        }
    }

};



static frei0r::construct<Vignette> plugin("Vignette",
                "Lens vignetting effect, applies natural vignetting",
                "Simon A. Eugster (Granjow)",
                0,2,
                F0R_COLOR_MODEL_RGBA8888);
