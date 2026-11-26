/*
 * colorenhance.c -- enhance the color contrast for vivid pictures
 *
 * Copyright (C) 2026 Cynthia (cynthia2048@proton.me)
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

#include <stdlib.h>
#if defined(__SSE2__)
#include <emmintrin.h>
#endif

#include <frei0r.h>
#include <frei0r/math.h>

#include "oklab.h"

typedef struct
{
    unsigned int width, height;
    uint8_t a_lut[256], 
            b_lut[256];
    float a_lut_f[256],
          b_lut_f[256];
    float a_base, a_slope, 
          b_base, b_slope;
} colorenhance_t;

static void make_sigmoid_lut(uint8_t *lut, float *lut_f, float base, float steep)
{
  float k = expf(steep * 5.0) / 255.0;
  float b = (base - 0.5) * 63.0;

  for (int i = 0; i < 256; ++i) {
    uint8_t v = CLAMP (255.0 / (1.0 + expf(-k * (i - b - 127.0))), 0, 255.0);
    lut[i] = v;
    lut_f[i] = (float)v / 255.0f - 0.5f;
  }
}

int f0r_init(void) 
{
    return 0;
}

void f0r_deinit(void) {}

void f0r_get_plugin_info(f0r_plugin_info_t* sigmoidalInfo)
{
  sigmoidalInfo->name = "colorenhance";
  sigmoidalInfo->author = "Cynthia";
  sigmoidalInfo->plugin_type = F0R_PLUGIN_TYPE_FILTER;
  sigmoidalInfo->color_model = F0R_COLOR_MODEL_RGBA8888;
  sigmoidalInfo->frei0r_version = FREI0R_MAJOR_VERSION;
  sigmoidalInfo->major_version = 1;
  sigmoidalInfo->minor_version = 0;
  sigmoidalInfo->num_params = 4;
  sigmoidalInfo->explanation = "Enhances the color contrast in OkLab with sigmoidal transfers";
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
    colorenhance_t *inst = malloc(sizeof(colorenhance_t));

    if (inst == NULL)
        return NULL;

    inst->width   = width;
    inst->height  = height;
    inst->a_base  = 0.55f;
    inst->a_slope = 0.4f;
    inst->b_base  = 0.5f;
    inst->b_slope = 0.4f;

    make_sigmoid_lut(inst->a_lut, inst->a_lut_f, inst->a_base, inst->a_slope);
    make_sigmoid_lut(inst->b_lut, inst->b_lut_f, inst->b_base, inst->b_slope);
    return inst;
}

void f0r_destruct(f0r_instance_t instance)
{
  free(instance);
}

void f0r_get_param_info(f0r_param_info_t* info, int index)
{
	switch (index) {
		case 0:
            info->name = "a_base";
            info->type = F0R_PARAM_DOUBLE;
            info->explanation = "Midpoint of sigmoidal curve for A channel";
		break;
        case 1:
            info->name = "b_base";
            info->type = F0R_PARAM_DOUBLE;
            info->explanation = "Midpoint of sigmoidal curve for B channel";
        break;
        case 2:
            info->name = "a_slope";
            info->type = F0R_PARAM_DOUBLE;
            info->explanation = "Steepness of the sigmoidal curve for A channel";
        break;
        case 3:
            info->name = "b_slope";
            info->type = F0R_PARAM_DOUBLE;
            info->explanation = "Steepness of the sigmoidal curve for B channel";
        break;
	}
}

void f0r_set_param_value(f0r_instance_t instance, f0r_param_t param, int index)
{
	colorenhance_t* inst = (colorenhance_t*)instance;
	switch (index)
    {
        case 0:
            inst->a_base = *((double*)param);
            make_sigmoid_lut(inst->a_lut, inst->a_lut_f, inst->a_base, inst->a_slope);
        break;
        case 1:
            inst->b_base = *((double*)param);
            make_sigmoid_lut(inst->b_lut, inst->b_lut_f, inst->b_base, inst->b_slope);
        break;
        case 2:
            inst->a_slope = *((double*)param);
            make_sigmoid_lut(inst->a_lut, inst->a_lut_f, inst->a_base, inst->a_slope);
        break;
        case 3:
            inst->b_slope = *((double*)param);
            make_sigmoid_lut(inst->b_lut, inst->b_lut_f, inst->b_base, inst->b_slope);
        break;
    }
}

void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index)
{
	colorenhance_t* inst = (colorenhance_t*)instance;
	switch (param_index) 
    {
        case 0:
            *((double*)param) = inst->a_base;
        break;
        case 1:
            *((double*)param) = inst->b_base;
        break;
        case 2:
            *((double*)param) = inst->a_slope;
        break;
        case 3:
            *((double*)param) = inst->b_slope;
        break;
    }
}

#if defined(__SSE2__)
/* Fast SSE2 cbrtf for x >= 0: bit-hack initial guess + 2 float Halley iterations (~1e-6 accuracy) */
static inline __m128 fast_cbrtf_sse2(__m128 x)
{
    const __m128 vzero = _mm_setzero_ps();
    const __m128 vtwo = _mm_set1_ps(2.0f);
    __m128 pos = _mm_cmpgt_ps(x, vzero);
    __m128i ix = _mm_castps_si128(x);
    __m128i q = _mm_add_epi32(_mm_srli_epi32(ix, 2), _mm_srli_epi32(ix, 4));
    q = _mm_add_epi32(q, _mm_srli_epi32(q, 4));
    q = _mm_add_epi32(q, _mm_srli_epi32(q, 8));
    __m128 y = _mm_castsi128_ps(_mm_add_epi32(q, _mm_set1_epi32(0x2a517d47)));
    __m128 two_x = _mm_mul_ps(vtwo, x);

    __m128 y3 = _mm_mul_ps(y, _mm_mul_ps(y, y));
    y = _mm_div_ps(_mm_mul_ps(y, _mm_add_ps(y3, two_x)), _mm_add_ps(_mm_mul_ps(vtwo, y3), x));

    y3 = _mm_mul_ps(y, _mm_mul_ps(y, y));
    y = _mm_div_ps(_mm_mul_ps(y, _mm_add_ps(y3, two_x)), _mm_add_ps(_mm_mul_ps(vtwo, y3), x));

    return _mm_and_ps(pos, y);
}

static inline __m128 exact_cbrtf_sse2(__m128 x)
{
    float a[4];
    _mm_storeu_ps(a, x);
    a[0] = cbrtf(a[0]);
    a[1] = cbrtf(a[1]);
    a[2] = cbrtf(a[2]);
    a[3] = cbrtf(a[3]);
    return _mm_loadu_ps(a);
}

static inline __m128 near_int_mask(__m128 v, __m128i iv, __m128 veps_lo, __m128 veps_hi)
{
    __m128 frac = _mm_sub_ps(v, _mm_cvtepi32_ps(iv));
    return _mm_or_ps(_mm_cmplt_ps(frac, veps_lo), _mm_cmpgt_ps(frac, veps_hi));
}
#endif

void f0r_update(f0r_instance_t instance, double time, const uint32_t* inframe, uint32_t* outframe)
{
    colorenhance_t* inst = (colorenhance_t*)instance;
    const long total = (long)inst->width * inst->height;
    long i = 0;

#if defined(__SSE2__)
    const __m128 vzero = _mm_setzero_ps();
    const __m128 v255 = _mm_set1_ps(255.0f);
    const __m128 vhalf = _mm_set1_ps(0.5f);
    const __m128 veps_lo = _mm_set1_ps(0.002f);
    const __m128 veps_hi = _mm_set1_ps(0.998f);

    const __m128 m_lx = _mm_set1_ps(0.412221f), m_ly = _mm_set1_ps(0.536332f), m_lz = _mm_set1_ps(0.051445f);
    const __m128 m_mx = _mm_set1_ps(0.211903f), m_my = _mm_set1_ps(0.680699f), m_mz = _mm_set1_ps(0.107396f);
    const __m128 m_sx = _mm_set1_ps(0.088302f), m_sy = _mm_set1_ps(0.281718f), m_sz = _mm_set1_ps(0.629978f);

    const __m128 o_ll = _mm_set1_ps(0.210454f), o_lm = _mm_set1_ps(0.793617f), o_ls = _mm_set1_ps(0.004072f);
    const __m128 o_al = _mm_set1_ps(1.977998f), o_am = _mm_set1_ps(2.428592f), o_as = _mm_set1_ps(0.450593f);
    const __m128 o_bl = _mm_set1_ps(0.025904f), o_bm = _mm_set1_ps(0.782771f), o_bs = _mm_set1_ps(0.808675f);

    const __m128 i_la = _mm_set1_ps(0.396337f), i_lb = _mm_set1_ps(0.215803f);
    const __m128 i_ma = _mm_set1_ps(0.105561f), i_mb = _mm_set1_ps(0.063854f);
    const __m128 i_sa = _mm_set1_ps(0.089484f), i_sb = _mm_set1_ps(1.291485f);

    const __m128 r_xl = _mm_set1_ps(4.076741f), r_xm = _mm_set1_ps(3.307711f), r_xs = _mm_set1_ps(0.230969f);
    const __m128 r_yl = _mm_set1_ps(-1.268438f), r_ym = _mm_set1_ps(2.609757f), r_ys = _mm_set1_ps(0.341319f);
    const __m128 r_zl = _mm_set1_ps(-0.004196f), r_zm = _mm_set1_ps(0.703418f), r_zs = _mm_set1_ps(1.707614f);

    const float *a_lut_f = inst->a_lut_f;
    const float *b_lut_f = inst->b_lut_f;

    for (; i + 3 < total; i += 4)
    {
        uint32_t p0 = inframe[i];
        uint32_t p1 = inframe[i + 1];
        uint32_t p2 = inframe[i + 2];
        uint32_t p3 = inframe[i + 3];

        __m128 ux = _mm_set_ps(gamma_expand_table[p3 & 0xFFu], gamma_expand_table[p2 & 0xFFu],
                               gamma_expand_table[p1 & 0xFFu], gamma_expand_table[p0 & 0xFFu]);
        __m128 uy = _mm_set_ps(gamma_expand_table[(p3 >> 8) & 0xFFu], gamma_expand_table[(p2 >> 8) & 0xFFu],
                               gamma_expand_table[(p1 >> 8) & 0xFFu], gamma_expand_table[(p0 >> 8) & 0xFFu]);
        __m128 uz = _mm_set_ps(gamma_expand_table[(p3 >> 16) & 0xFFu], gamma_expand_table[(p2 >> 16) & 0xFFu],
                               gamma_expand_table[(p1 >> 16) & 0xFFu], gamma_expand_table[(p0 >> 16) & 0xFFu]);

        __m128 l_in = _mm_add_ps(_mm_add_ps(_mm_mul_ps(m_lx, ux), _mm_mul_ps(m_ly, uy)), _mm_mul_ps(m_lz, uz));
        __m128 m_in = _mm_add_ps(_mm_add_ps(_mm_mul_ps(m_mx, ux), _mm_mul_ps(m_my, uy)), _mm_mul_ps(m_mz, uz));
        __m128 s_in = _mm_add_ps(_mm_add_ps(_mm_mul_ps(m_sx, ux), _mm_mul_ps(m_sy, uy)), _mm_mul_ps(m_sz, uz));

        __m128 l = fast_cbrtf_sse2(l_in);
        __m128 m = fast_cbrtf_sse2(m_in);
        __m128 s = fast_cbrtf_sse2(s_in);

        __m128 vl = _mm_mul_ps(_mm_sub_ps(_mm_add_ps(_mm_mul_ps(l, o_ll), _mm_mul_ps(m, o_lm)), _mm_mul_ps(s, o_ls)), v255);
        __m128 va = _mm_mul_ps(_mm_add_ps(_mm_add_ps(_mm_sub_ps(_mm_mul_ps(l, o_al), _mm_mul_ps(m, o_am)), _mm_mul_ps(s, o_as)), vhalf), v255);
        __m128 vb = _mm_mul_ps(_mm_add_ps(_mm_sub_ps(_mm_add_ps(_mm_mul_ps(l, o_bl), _mm_mul_ps(m, o_bm)), _mm_mul_ps(s, o_bs)), vhalf), v255);

        __m128 vl_c = _mm_min_ps(_mm_max_ps(vl, vzero), v255);
        __m128 va_c = _mm_min_ps(_mm_max_ps(va, vzero), v255);
        __m128 vb_c = _mm_min_ps(_mm_max_ps(vb, vzero), v255);

        __m128i il = _mm_cvttps_epi32(vl_c);
        __m128i ia = _mm_cvttps_epi32(va_c);
        __m128i ib = _mm_cvttps_epi32(vb_c);

        /* Boundary guard: if any lane is within 0.002 of an integer boundary, use exact libm cbrtf */
        __m128 near = _mm_or_ps(_mm_or_ps(near_int_mask(vl_c, il, veps_lo, veps_hi),
                                          near_int_mask(va_c, ia, veps_lo, veps_hi)),
                                near_int_mask(vb_c, ib, veps_lo, veps_hi));
        if (__builtin_expect(_mm_movemask_ps(near) != 0, 0)) {
            l = exact_cbrtf_sse2(l_in);
            m = exact_cbrtf_sse2(m_in);
            s = exact_cbrtf_sse2(s_in);
            vl = _mm_mul_ps(_mm_sub_ps(_mm_add_ps(_mm_mul_ps(l, o_ll), _mm_mul_ps(m, o_lm)), _mm_mul_ps(s, o_ls)), v255);
            va = _mm_mul_ps(_mm_add_ps(_mm_add_ps(_mm_sub_ps(_mm_mul_ps(l, o_al), _mm_mul_ps(m, o_am)), _mm_mul_ps(s, o_as)), vhalf), v255);
            vb = _mm_mul_ps(_mm_add_ps(_mm_sub_ps(_mm_add_ps(_mm_mul_ps(l, o_bl), _mm_mul_ps(m, o_bm)), _mm_mul_ps(s, o_bs)), vhalf), v255);
            il = _mm_cvttps_epi32(_mm_min_ps(_mm_max_ps(vl, vzero), v255));
            ia = _mm_cvttps_epi32(_mm_min_ps(_mm_max_ps(va, vzero), v255));
            ib = _mm_cvttps_epi32(_mm_min_ps(_mm_max_ps(vb, vzero), v255));
        }

        int ia_arr[4], ib_arr[4];
        _mm_storeu_si128((__m128i*)ia_arr, ia);
        _mm_storeu_si128((__m128i*)ib_arr, ib);

        __m128 ul = _mm_div_ps(_mm_cvtepi32_ps(il), v255);
        __m128 ua = _mm_set_ps(a_lut_f[ia_arr[3]], a_lut_f[ia_arr[2]], a_lut_f[ia_arr[1]], a_lut_f[ia_arr[0]]);
        __m128 ub = _mm_set_ps(b_lut_f[ib_arr[3]], b_lut_f[ib_arr[2]], b_lut_f[ib_arr[1]], b_lut_f[ib_arr[0]]);

        __m128 ll = _mm_add_ps(_mm_add_ps(ul, _mm_mul_ps(i_la, ua)), _mm_mul_ps(i_lb, ub));
        __m128 mm = _mm_sub_ps(_mm_sub_ps(ul, _mm_mul_ps(i_ma, ua)), _mm_mul_ps(i_mb, ub));
        __m128 ss = _mm_sub_ps(_mm_sub_ps(ul, _mm_mul_ps(i_sa, ua)), _mm_mul_ps(i_sb, ub));

        /* Double-precision cube rounded to float matches glibc powf(x, 3) */
        {
            __m128d lo, hi;
            lo = _mm_cvtps_pd(ll); lo = _mm_mul_pd(lo, _mm_mul_pd(lo, lo));
            hi = _mm_cvtps_pd(_mm_movehl_ps(ll, ll)); hi = _mm_mul_pd(hi, _mm_mul_pd(hi, hi));
            ll = _mm_movelh_ps(_mm_cvtpd_ps(lo), _mm_cvtpd_ps(hi));

            lo = _mm_cvtps_pd(mm); lo = _mm_mul_pd(lo, _mm_mul_pd(lo, lo));
            hi = _mm_cvtps_pd(_mm_movehl_ps(mm, mm)); hi = _mm_mul_pd(hi, _mm_mul_pd(hi, hi));
            mm = _mm_movelh_ps(_mm_cvtpd_ps(lo), _mm_cvtpd_ps(hi));

            lo = _mm_cvtps_pd(ss); lo = _mm_mul_pd(lo, _mm_mul_pd(lo, lo));
            hi = _mm_cvtps_pd(_mm_movehl_ps(ss, ss)); hi = _mm_mul_pd(hi, _mm_mul_pd(hi, hi));
            ss = _mm_movelh_ps(_mm_cvtpd_ps(lo), _mm_cvtpd_ps(hi));
        }

        __m128 ox = _mm_mul_ps(_mm_add_ps(_mm_sub_ps(_mm_mul_ps(r_xl, ll), _mm_mul_ps(r_xm, mm)), _mm_mul_ps(r_xs, ss)), v255);
        __m128 oy = _mm_mul_ps(_mm_sub_ps(_mm_add_ps(_mm_mul_ps(r_yl, ll), _mm_mul_ps(r_ym, mm)), _mm_mul_ps(r_ys, ss)), v255);
        __m128 oz = _mm_mul_ps(_mm_add_ps(_mm_sub_ps(_mm_mul_ps(r_zl, ll), _mm_mul_ps(r_zm, mm)), _mm_mul_ps(r_zs, ss)), v255);

        __m128i ir = _mm_cvttps_epi32(_mm_min_ps(_mm_max_ps(ox, vzero), v255));
        __m128i ig = _mm_cvttps_epi32(_mm_min_ps(_mm_max_ps(oy, vzero), v255));
        __m128i ibo = _mm_cvttps_epi32(_mm_min_ps(_mm_max_ps(oz, vzero), v255));

        int r_arr[4], g_arr[4], b_arr[4];
        _mm_storeu_si128((__m128i*)r_arr, ir);
        _mm_storeu_si128((__m128i*)g_arr, ig);
        _mm_storeu_si128((__m128i*)b_arr, ibo);

        outframe[i]     = (uint32_t)gamma_compress_table[r_arr[0]] | ((uint32_t)gamma_compress_table[g_arr[0]] << 8)
                        | ((uint32_t)gamma_compress_table[b_arr[0]] << 16) | (p0 & 0xFF000000u);
        outframe[i + 1] = (uint32_t)gamma_compress_table[r_arr[1]] | ((uint32_t)gamma_compress_table[g_arr[1]] << 8)
                        | ((uint32_t)gamma_compress_table[b_arr[1]] << 16) | (p1 & 0xFF000000u);
        outframe[i + 2] = (uint32_t)gamma_compress_table[r_arr[2]] | ((uint32_t)gamma_compress_table[g_arr[2]] << 8)
                        | ((uint32_t)gamma_compress_table[b_arr[2]] << 16) | (p2 & 0xFF000000u);
        outframe[i + 3] = (uint32_t)gamma_compress_table[r_arr[3]] | ((uint32_t)gamma_compress_table[g_arr[3]] << 8)
                        | ((uint32_t)gamma_compress_table[b_arr[3]] << 16) | (p3 & 0xFF000000u);
    }
#endif

    for (; i < total; i++)
    {
        uint32_t p = inframe[i];
        ciexyz_t u;
        oklab_t v;

        u.x = gamma_expand_table[p & 0xFFu];
        u.y = gamma_expand_table[(p >> 8) & 0xFFu];
        u.z = gamma_expand_table[(p >> 16) & 0xFFu];

        v = ciexyz_to_oklab(u);

        v.l = CLAMP0255(v.l);
        v.a = inst->a_lut[CLAMP0255(v.a)];
        v.b = inst->b_lut[CLAMP0255(v.b)];

        u = oklab_to_ciexyz(v);

        uint8_t r = CLAMP0255(u.x);
        uint8_t g = CLAMP0255(u.y);
        uint8_t b = CLAMP0255(u.z);

        outframe[i] = (uint32_t)gamma_compress_table[r] | ((uint32_t)gamma_compress_table[g] << 8)
                    | ((uint32_t)gamma_compress_table[b] << 16) | (p & 0xFF000000u);
    }
}
