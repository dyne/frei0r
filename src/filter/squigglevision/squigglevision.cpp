/* squigglevision.cpp
 * Copyright (C) 2026 Gabriel Lobo (https://olobo.xyz)
 * This file is a Frei0r plugin.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
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

#include <cmath>
#include <cstdint>
#include <vector>

#ifndef NO_FUTURE
#include <future>
#include <thread>
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_E
#define M_E 2.71828182845904523536
#endif

namespace
{

    float hash2(int x, int y)
    {
        unsigned int h = (unsigned int)(x * 374761393 + y * 668265263);
        h = (h ^ (h >> 13)) * 1274126177u;
        h ^= h >> 16;
        return (h & 0xffffffu) / (float)0x1000000;
    }

    float value_noise(float x, float y)
    {
        int xi = (int)std::floor(x);
        int yi = (int)std::floor(y);
        float fx = x - xi;
        float fy = y - yi;

        float a = hash2(xi, yi);
        float b = hash2(xi + 1, yi);
        float c = hash2(xi, yi + 1);
        float d = hash2(xi + 1, yi + 1);

        float ab = a + (b - a) * fx;
        float cd = c + (d - c) * fx;
        return ab + (cd - ab) * fy;
    }

    void sample_bilinear(const unsigned char *s, int w, int h,
                         float fx, float fy, unsigned char *o)
    {
        if (fx < 0)
            fx = 0;
        if (fy < 0)
            fy = 0;
        if (fx > w - 1)
            fx = w - 1;
        if (fy > h - 1)
            fy = h - 1;

        int x0 = (int)fx;
        int y0 = (int)fy;
        int x1 = x0 + 1 < w ? x0 + 1 : x0;
        int y1 = y0 + 1 < h ? y0 + 1 : y0;
        float dx = fx - x0;
        float dy = fy - y0;

        const unsigned char *corners[4] = {
            s + ((size_t)y0 * w + x0) * 4,
            s + ((size_t)y0 * w + x1) * 4,
            s + ((size_t)y1 * w + x0) * 4,
            s + ((size_t)y1 * w + x1) * 4};
        float weights[4] = {
            (1 - dx) * (1 - dy),
            dx * (1 - dy),
            (1 - dx) * dy,
            dx * dy};

        float inv255 = 1.0f / 255.0f;
        float pr = 0, pg = 0, pb = 0, pa = 0;

        for (int k = 0; k < 4; k++)
        {
            const unsigned char *p = corners[k];
            float a = p[3] * inv255;
            float wk = weights[k];
            pr += p[0] * a * wk;
            pg += p[1] * a * wk;
            pb += p[2] * a * wk;
            pa += p[3] * wk;
        }

        o[3] = (unsigned char)(pa + 0.5f);

        if (pa > 0.0f)
        {
            float inv = 255.0f / pa;
            o[0] = CLAMP0255((int32_t)(pr * inv + 0.5f));
            o[1] = CLAMP0255((int32_t)(pg * inv + 0.5f));
            o[2] = CLAMP0255((int32_t)(pb * inv + 0.5f));
        }
        else
        {
            o[0] = 0;
            o[1] = 0;
            o[2] = 0;
        }
    }

} // namespace

class squigglevision : public frei0r::filter
{
public:
    squigglevision(unsigned int width, unsigned int height)
    {
        (void)width;
        (void)height;

        register_param(m_strength, "Strength",
                       "Displacement amount, normalized 0..1 mapped to 0..100 percent (8 percent is a moderate default).");
        register_param(m_fps, "FPS",
                       "Squiggle updates per second, normalized 0..1 mapped to 0..30 fps (0 = frozen; 6 fps is a moderate default).");
        register_param(m_scale, "Scale",
                       "Noise cell size, normalized 0..1 mapped to 4..128 px (about 23 px is a moderate default).");

        m_strength = 0.08;
        m_fps = 0.2;
        m_scale = 0.15;
    }

    void update(double time, uint32_t *out, const uint32_t *in)
    {
        const int w = (int)width;
        const int h = (int)height;

        const double disp = m_strength * 5.0 * 0.005;
        const double fps = m_fps * 30.0;
        const double cell = 4.0 + m_scale * 124.0;

        m_inv_cell = (float)(1.0 / cell);
        m_dispx = (float)(disp * w);
        m_dispy = (float)(disp * h);

        const double frame = std::floor(time * fps);
        build_grid(w, h, (float)(frame * M_PI), (float)(frame * M_E));

        dispatch((const unsigned char *)in, (unsigned char *)out, w, h);
    }

private:
    void build_grid(int w, int h, float offx, float offy)
    {
        m_gw = (int)(w * m_inv_cell) + 2;
        m_gh = (int)(h * m_inv_cell) + 2;
        m_gx.resize((size_t)m_gw * m_gh);
        m_gy.resize((size_t)m_gw * m_gh);

        for (int j = 0; j < m_gh; j++)
        {
            for (int i = 0; i < m_gw; i++)
            {
                float angle = value_noise(i + offx, j + offy) * 4.0f * (float)M_PI;
                m_gx[(size_t)j * m_gw + i] = std::cos(angle) * m_dispx;
                m_gy[(size_t)j * m_gw + i] = std::sin(angle) * m_dispy;
            }
        }
    }

    void render_rows(int y0, int y1, const unsigned char *src, unsigned char *dst,
                     int w, int h) const
    {
        const float *gx = m_gx.data();
        const float *gy = m_gy.data();
        const int gw = m_gw;
        const float inv_cell = m_inv_cell;

        for (int y = y0; y < y1; y++)
        {
            const float v = y * inv_cell;
            const int j = (int)v;
            const float fv = v - j;

            for (int x = 0; x < w; x++)
            {
                const float u = x * inv_cell;
                const int i = (int)u;
                const float fu = u - i;
                const size_t g = (size_t)j * gw + i;

                float xa = gx[g] + (gx[g + 1] - gx[g]) * fu;
                float xb = gx[g + gw] + (gx[g + gw + 1] - gx[g + gw]) * fu;
                float ddx = xa + (xb - xa) * fv;

                float ya = gy[g] + (gy[g + 1] - gy[g]) * fu;
                float yb = gy[g + gw] + (gy[g + gw + 1] - gy[g + gw]) * fu;
                float ddy = ya + (yb - ya) * fv;

                unsigned char px[4];
                sample_bilinear(src, w, h, x + ddx, y + ddy, px);

                const size_t idx = ((size_t)y * w + x) * 4;
                const unsigned char *orig = src + idx;
                unsigned char *d = dst + idx;

                if (px[3] >= orig[3])
                {
                    d[0] = px[0];
                    d[1] = px[1];
                    d[2] = px[2];
                    d[3] = px[3];
                }
                else
                {
                    d[0] = orig[0];
                    d[1] = orig[1];
                    d[2] = orig[2];
                    d[3] = orig[3];
                }
            }
        }
    }

    void dispatch(const unsigned char *src, unsigned char *dst, int w, int h)
    {
#ifndef NO_FUTURE
        unsigned int nthreads = std::thread::hardware_concurrency();
        const int min_band = 64;

        if (nthreads > 1 && h >= 2 * min_band)
        {
            if (nthreads > (unsigned int)(h / min_band))
            {
                nthreads = (unsigned int)(h / min_band);
            }

            std::vector<std::future<void>> futures;
            futures.reserve(nthreads);

            const int band = h / nthreads;
            int y0 = 0;
            for (unsigned int t = 0; t < nthreads; t++)
            {
                int y1 = (t == nthreads - 1) ? h : y0 + band;
                futures.push_back(std::async(std::launch::async,
                                             &squigglevision::render_rows, this, y0, y1, src, dst, w, h));
                y0 = y1;
            }
            for (size_t k = 0; k < futures.size(); k++)
            {
                futures[k].wait();
            }
            return;
        }
#endif
        render_rows(0, h, src, dst, w, h);
    }

    double m_strength;
    double m_fps;
    double m_scale;

    std::vector<float> m_gx;
    std::vector<float> m_gy;
    int m_gw = 0;
    int m_gh = 0;
    float m_inv_cell = 0;
    float m_dispx = 0;
    float m_dispy = 0;
};

frei0r::construct<squigglevision> plugin(
    "squigglevision",
    "Hand-drawn wobble via time-quantized noise displacement",
    "Gabriel Lobo (https://olobo.xyz)",
    1, 0,
    F0R_COLOR_MODEL_RGBA8888);
