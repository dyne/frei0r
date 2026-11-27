/*
 * Copyright (C) 2018 Dan Dennedy <dan@dennedy.org>
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
#if defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#include <emmintrin.h>
#define PREMULTIPLY_USE_SSE2 1
#endif

namespace {

static uint32_t g_unpremult_inv[256];
static bool g_unpremult_init = false;

static void init_unpremult_table()
{
    if (!g_unpremult_init) {
        g_unpremult_inv[0] = 65536u;
        g_unpremult_inv[255] = 65536u;
        for (uint32_t a = 1; a < 255; ++a) {
            g_unpremult_inv[a] = (1u << 24) / a + 1u;
        }
        g_unpremult_init = true;
    }
}

class Premultiply : public frei0r::filter
{

public:

    Premultiply(unsigned int width, unsigned int height)
        : m_unpremultiply(0)
    {
        init_unpremult_table();
        register_param(m_unpremultiply, "unpremultiply", "Whether to unpremultiply instead");
    }

    ~Premultiply()
    {
    }

    virtual void update(double,
                        uint32_t* out,
                        const uint32_t* in)
    {
        const unsigned int len = width * height;
        unsigned int i = 0;

        if (!m_unpremultiply) {
            // premultiply
#ifdef PREMULTIPLY_USE_SSE2
            const __m128i v_zero = _mm_setzero_si128();
            const __m128i v_rgb_mask = _mm_set1_epi32(0x00ffffff);
            const __m128i v_alpha_mask = _mm_set1_epi32((int)0xff000000u);
            for (; i + 3 < len; i += 4) {
                __m128i px = _mm_loadu_si128((const __m128i*)(in + i));
                __m128i lo = _mm_unpacklo_epi8(px, v_zero);
                __m128i hi = _mm_unpackhi_epi8(px, v_zero);

                __m128i a_lo = _mm_shufflehi_epi16(_mm_shufflelo_epi16(lo, 0xff), 0xff);
                __m128i a_hi = _mm_shufflehi_epi16(_mm_shufflelo_epi16(hi, 0xff), 0xff);

                __m128i p_lo = _mm_srli_epi16(_mm_mullo_epi16(lo, a_lo), 8);
                __m128i p_hi = _mm_srli_epi16(_mm_mullo_epi16(hi, a_hi), 8);

                __m128i res = _mm_packus_epi16(p_lo, p_hi);
                res = _mm_or_si128(_mm_and_si128(res, v_rgb_mask),
                                   _mm_and_si128(px, v_alpha_mask));
                _mm_storeu_si128((__m128i*)(out + i), res);
            }
#endif
            for (; i < len; ++i) {
                uint32_t p = in[i];
                uint32_t a = p >> 24;
                uint32_t r = ((p & 0xffu) * a) >> 8;
                uint32_t g = (((p >> 8) & 0xffu) * a) >> 8;
                uint32_t b = (((p >> 16) & 0xffu) * a) >> 8;
                out[i] = r | (g << 8) | (b << 16) | (p & 0xff000000u);
            }
        } else {
            // unpremultiply: exact branchless fixed-point reciprocal
            for (; i < len; ++i) {
                uint32_t p = in[i];
                uint32_t a = p >> 24;
                uint32_t inv = g_unpremult_inv[a];
                uint32_t r = ((p & 0xffu) * inv) >> 16;
                uint32_t g = (((p >> 8) & 0xffu) * inv) >> 16;
                uint32_t b = (((p >> 16) & 0xffu) * inv) >> 16;
                if (r > 255u) r = 255u;
                if (g > 255u) g = 255u;
                if (b > 255u) b = 255u;
                out[i] = r | (g << 8) | (b << 16) | (p & 0xff000000u);
            }
        }
    }

private:
    bool m_unpremultiply;

};

static frei0r::construct<Premultiply> plugin("Premultiply or Unpremultiply",
                "Multiply (or divide) each color component by the pixel's alpha value",
                "Dan Dennedy",
                0, 2,
                F0R_COLOR_MODEL_RGBA8888);

}
