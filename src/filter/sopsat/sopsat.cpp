/* 
 * Copyright (C) 2010 Simon Andreas Eugster (simon.eu@gmail.com)
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

#include <math.h>
#include <stdlib.h>
#if defined(__SSE2__)
#include <emmintrin.h>
#endif
#include "frei0r.hpp"
#include "frei0r/math.h"

class SOPSat : public frei0r::filter
{

public:

    double rSlope;
    double gSlope;
    double bSlope;
    double aSlope;
    double rOffset;
    double gOffset;
    double bOffset;
    double aOffset;
    double rPower;
    double gPower;
    double bPower;
    double aPower;
    double saturation;

    SOPSat(unsigned int, unsigned int)
    {

        register_param(rSlope, "rSlope", "Slope of the red color component");
        register_param(gSlope, "gSlope", "Slope of the green color component");
        register_param(bSlope, "bSlope", "Slope of the blue color component");
        register_param(aSlope, "aSlope", "Slope of the alpha component");
        register_param(rOffset, "rOffset", "Offset of the red color component");
        register_param(gOffset, "gOffset", "Offset of the green color component");
        register_param(bOffset, "bOffset", "Offset of the blue color component");
        register_param(aOffset, "aOffset", "Offset of the alpha component");
        register_param(rPower, "rPower", "Power (Gamma) of the red color component");
        register_param(gPower, "gPower", "Power (Gamma) of the green color component");
        register_param(bPower, "bPower", "Power (Gamma) of the blue color component");
        register_param(aPower, "aPower", "Power (Gamma) of the alpha component");
        register_param(saturation, "saturation", "Overall saturation");
        rSlope = 1 / 20.;
        gSlope = 1 / 20.;
        bSlope = 1 / 20.;
        aSlope = 1 / 20.;
        rOffset = (0 - (-4)) / (double)(4 - (-4));
        gOffset = (0 - (-4)) / (double)(4 - (-4));
        bOffset = (0 - (-4)) / (double)(4 - (-4));
        aOffset = (0 - (-4)) / (double)(4 - (-4));
        rPower = 1 / 20.;
        gPower = 1 / 20.;
        bPower = 1 / 20.;
        aPower = 1 / 20.;
        saturation = 1 / 10.;

        m_lutInitialized = false;
        updateLUT();
    }

    ~SOPSat()
    {
    }

    virtual void update(double time,
                        uint32_t* out,
                        const uint32_t* in)
    {
        if (!m_lutInitialized ||
            rSlope != m_prevRSlope || gSlope != m_prevGSlope ||
            bSlope != m_prevBSlope || aSlope != m_prevASlope ||
            rOffset != m_prevROffset || gOffset != m_prevGOffset ||
            bOffset != m_prevBOffset || aOffset != m_prevAOffset ||
            rPower != m_prevRPower || gPower != m_prevGPower ||
            bPower != m_prevBPower || aPower != m_prevAPower ||
            saturation != m_prevSaturation) {
            updateLUT();
        }

        const unsigned char *pixel = (const unsigned char *) in;
        const unsigned char *lutR = m_lutR;
        const unsigned char *lutG = m_lutG;
        const unsigned char *lutB = m_lutB;
        const unsigned char *lutA = m_lutA;

        if (fabs(m_sat - 1.0) < 0.001) {
            unsigned int i = 0;
            for (; i + 3 < size; i += 4) {
                uint32_t p0 = in[i];
                uint32_t p1 = in[i + 1];
                uint32_t p2 = in[i + 2];
                uint32_t p3 = in[i + 3];
                out[i]     = (uint32_t)lutR[p0 & 0xFFu] | ((uint32_t)lutG[(p0 >> 8) & 0xFFu] << 8)
                           | ((uint32_t)lutB[(p0 >> 16) & 0xFFu] << 16) | ((uint32_t)lutA[p0 >> 24] << 24);
                out[i + 1] = (uint32_t)lutR[p1 & 0xFFu] | ((uint32_t)lutG[(p1 >> 8) & 0xFFu] << 8)
                           | ((uint32_t)lutB[(p1 >> 16) & 0xFFu] << 16) | ((uint32_t)lutA[p1 >> 24] << 24);
                out[i + 2] = (uint32_t)lutR[p2 & 0xFFu] | ((uint32_t)lutG[(p2 >> 8) & 0xFFu] << 8)
                           | ((uint32_t)lutB[(p2 >> 16) & 0xFFu] << 16) | ((uint32_t)lutA[p2 >> 24] << 24);
                out[i + 3] = (uint32_t)lutR[p3 & 0xFFu] | ((uint32_t)lutG[(p3 >> 8) & 0xFFu] << 8)
                           | ((uint32_t)lutB[(p3 >> 16) & 0xFFu] << 16) | ((uint32_t)lutA[p3 >> 24] << 24);
            }
            for (; i < size; i++) {
                uint32_t p = in[i];
                out[i] = (uint32_t)lutR[p & 0xFFu] | ((uint32_t)lutG[(p >> 8) & 0xFFu] << 8)
                       | ((uint32_t)lutB[(p >> 16) & 0xFFu] << 16) | ((uint32_t)lutA[p >> 24] << 24);
            }
        } else {
            const double *lumaR = m_lumaR;
            const double *lumaG = m_lumaG;
            const double *lumaB = m_lumaB;
            const double *lutRd = m_lutRd;
            const double *lutGd = m_lutGd;
            const double *lutBd = m_lutBd;
            const double sat = m_sat;
            unsigned int i = 0;

#if defined(__SSE2__)
            const __m128d vsat = _mm_set1_pd(sat);
            const __m128d vzero = _mm_setzero_pd();
            const __m128d v255 = _mm_set1_pd(255.0);

            for (; i + 1 < size; i += 2) {
                unsigned char r0 = pixel[0], g0 = pixel[1], b0 = pixel[2], a0 = pixel[3];
                unsigned char r1 = pixel[4], g1 = pixel[5], b1 = pixel[6], a1 = pixel[7];
                pixel += 8;

                __m128d lr = _mm_set_pd(lumaR[r1], lumaR[r0]);
                __m128d lg = _mm_set_pd(lumaG[g1], lumaG[g0]);
                __m128d lb = _mm_set_pd(lumaB[b1], lumaB[b0]);
                __m128d luma = _mm_add_pd(_mm_add_pd(lr, lg), lb);

                __m128d vr = _mm_set_pd(lutRd[r1], lutRd[r0]);
                __m128d vg = _mm_set_pd(lutGd[g1], lutGd[g0]);
                __m128d vb = _mm_set_pd(lutBd[b1], lutBd[b0]);

                __m128d or_ = _mm_add_pd(luma, _mm_mul_pd(vsat, _mm_sub_pd(vr, luma)));
                __m128d og_ = _mm_add_pd(luma, _mm_mul_pd(vsat, _mm_sub_pd(vg, luma)));
                __m128d ob_ = _mm_add_pd(luma, _mm_mul_pd(vsat, _mm_sub_pd(vb, luma)));

                or_ = _mm_min_pd(_mm_max_pd(or_, vzero), v255);
                og_ = _mm_min_pd(_mm_max_pd(og_, vzero), v255);
                ob_ = _mm_min_pd(_mm_max_pd(ob_, vzero), v255);

                __m128i ir = _mm_cvttpd_epi32(or_);
                __m128i ig = _mm_cvttpd_epi32(og_);
                __m128i ib = _mm_cvttpd_epi32(ob_);

                uint32_t r0o = (uint32_t)_mm_cvtsi128_si32(ir);
                uint32_t r1o = (uint32_t)_mm_cvtsi128_si32(_mm_srli_si128(ir, 4));
                uint32_t g0o = (uint32_t)_mm_cvtsi128_si32(ig);
                uint32_t g1o = (uint32_t)_mm_cvtsi128_si32(_mm_srli_si128(ig, 4));
                uint32_t b0o = (uint32_t)_mm_cvtsi128_si32(ib);
                uint32_t b1o = (uint32_t)_mm_cvtsi128_si32(_mm_srli_si128(ib, 4));

                out[i]     = r0o | (g0o << 8) | (b0o << 16) | ((uint32_t)lutA[a0] << 24);
                out[i + 1] = r1o | (g1o << 8) | (b1o << 16) | ((uint32_t)lutA[a1] << 24);
            }
#endif
            for (; i < size; i++) {
                unsigned char r = pixel[0], g = pixel[1], b = pixel[2], a = pixel[3];
                pixel += 4;
                double luma = lumaR[r] + lumaG[g] + lumaB[b];
                uint32_t ro = (uint32_t)CLAMP0255(luma + sat * (lutRd[r] - luma));
                uint32_t go = (uint32_t)CLAMP0255(luma + sat * (lutGd[g] - luma));
                uint32_t bo = (uint32_t)CLAMP0255(luma + sat * (lutBd[b] - luma));
                out[i] = ro | (go << 8) | (bo << 16) | ((uint32_t)lutA[a] << 24);
            }
        }
    }

private:
    unsigned char m_lutR[256];
    unsigned char m_lutG[256];
    unsigned char m_lutB[256];
    unsigned char m_lutA[256];

    double m_lutRd[256];
    double m_lutGd[256];
    double m_lutBd[256];
    double m_lumaR[256];
    double m_lumaG[256];
    double m_lumaB[256];

    double m_sat;
    bool m_lutInitialized;
    double m_prevRSlope, m_prevGSlope, m_prevBSlope, m_prevASlope;
    double m_prevROffset, m_prevGOffset, m_prevBOffset, m_prevAOffset;
    double m_prevRPower, m_prevGPower, m_prevBPower, m_prevAPower;
    double m_prevSaturation;

    void updateLUT() {
        double rS = rSlope * 20;
        double gS = gSlope * 20;
        double bS = bSlope * 20;
        double aS = aSlope * 20;

        double rO = -4 + rOffset * (4 - (-4));
        double gO = -4 + gOffset * (4 - (-4));
        double bO = -4 + bOffset * (4 - (-4));
        double aO = -4 + aOffset * (4 - (-4));

        double rP = rPower * 20;
        double gP = gPower * 20;
        double bP = bPower * 20;
        double aP = aPower * 20;

        m_sat = saturation * 10;

        for (int i = 0; i < 256; i++) {
            unsigned char r = CLAMP0255((int) (pow(above0((float)i/255 * rS + rO), rP)*255));
            unsigned char g = CLAMP0255((int) (pow(above0((float)i/255 * gS + gO), gP)*255));
            unsigned char b = CLAMP0255((int) (pow(above0((float)i/255 * bS + bO), bP)*255));
            unsigned char a = CLAMP0255((int) (pow(above0((float)i/255 * aS + aO), aP)*255));
            m_lutR[i] = r;
            m_lutG[i] = g;
            m_lutB[i] = b;
            m_lutA[i] = a;
            m_lutRd[i] = (double)r;
            m_lutGd[i] = (double)g;
            m_lutBd[i] = (double)b;
            m_lumaR[i] = 0.2126 * (double)r;
            m_lumaG[i] = 0.7152 * (double)g;
            m_lumaB[i] = 0.0722 * (double)b;
        }

        m_prevRSlope = rSlope; m_prevGSlope = gSlope; m_prevBSlope = bSlope; m_prevASlope = aSlope;
        m_prevROffset = rOffset; m_prevGOffset = gOffset; m_prevBOffset = bOffset; m_prevAOffset = aOffset;
        m_prevRPower = rPower; m_prevGPower = gPower; m_prevBPower = bPower; m_prevAPower = aPower;
        m_prevSaturation = saturation;
        m_lutInitialized = true;
    }

    double above0(double f) {
        if (f < 0) {
            return 0;
        } else {
            return f;
        }
    }

};



static frei0r::construct<SOPSat> plugin("SOP/Sat",
                "Slope/Offset/Power and Saturation color corrections according to the ASC CDL (Color Decision List)",
                "Simon A. Eugster (Granjow)",
                0,3,
                F0R_COLOR_MODEL_RGBA8888);
