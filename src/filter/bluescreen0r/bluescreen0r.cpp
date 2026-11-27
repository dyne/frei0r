/* 
 * Bluescreen0r
 * 2009 Hedde Bosman
 *
 * This source code  is free software; you can  redistribute it and/or
 * modify it under the terms of the GNU Public License as published by
 * the Free Software  Foundation; either version 3 of  the License, or
 * (at your option) any later version.
 *
 * This source code is distributed in the hope that it will be useful,
 * but  WITHOUT ANY  WARRANTY; without  even the  implied  warranty of
 * MERCHANTABILITY or FITNESS FOR  A PARTICULAR PURPOSE.  Please refer
 * to the GNU Public License for more details.
 *
 * You should  have received  a copy of  the GNU Public  License along
 * with this source code; if  not, write to: Free Software Foundation,
 * Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
#include "frei0r.hpp"

#include <algorithm>
#include <vector>
#include <utility>
#include <cassert>

#include <iostream>

#include <math.h>
#if defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#include <emmintrin.h>
#define BLUESCREEN0R_USE_SSE2 1
#endif
#if defined(__GNUC__) || defined(__clang__)
#define BLUESCREEN0R_LIKELY(x) __builtin_expect(!!(x), 1)
#else
#define BLUESCREEN0R_LIKELY(x) (x)
#endif


class bluescreen0r : public frei0r::filter
{
private:
	double dist;
	f0r_param_color color;
	bool invert;
	uint32_t r256,g256,b256;
	
	// returns the distance to 'color', but does not normalize
	inline uint32_t distance(uint32_t pixel, int r_ref, int g_ref, int b_ref) const {
		int tb = (int)((pixel & 0x00FF0000u) >> 16) - b_ref;
		int tg = (int)((pixel & 0x0000FF00u) >> 8)  - g_ref;
		int tr = (int)(pixel & 0x000000FFu)         - r_ref;
		return (uint32_t)(tb * tb + tg * tg + tr * tr);
	}
public:
	bluescreen0r(unsigned int width, unsigned int height)
	{
		(void)width;
		(void)height;
		dist = 0.288;
		
		color.r = 0;
		color.g = 0.94;
		color.b = 0;

		invert = false;

		register_param(color,  "Color",    "The color to make transparent (B G R)");
		register_param(dist,   "Distance", "Distance to Color (127 is good)");
		register_param(invert, "Invert",   "Whether to produce the inverse of the effect on the alpha channel");
	}

	virtual void update(double time,
	                    uint32_t* out,
		            const uint32_t* in) {
		(void)time;
		const uint32_t distInt = (uint32_t)(dist * dist * 195075);
		const uint32_t distInt2 = distInt / 2;
		
		r256 = (uint32_t)(255 * color.r);
		g256 = (uint32_t)(255 * color.g);
		b256 = (uint32_t)(255 * color.b);

		const int r_ref = (int)r256;
		const int g_ref = (int)g256;
		const int b_ref = (int)b256;
		const uint32_t alpha_xor = invert ? 0xFF000000u : 0u;
		const unsigned int len = (unsigned int)size;
		unsigned int i = 0;

#ifdef BLUESCREEN0R_USE_SSE2
		if (distInt <= 0x7FFFFFFFu && r_ref >= 0 && r_ref <= 255 &&
		    g_ref >= 0 && g_ref <= 255 && b_ref >= 0 && b_ref <= 255) {
			const __m128i v_zero = _mm_setzero_si128();
			const __m128i v_ref_rg = _mm_set1_epi32((g_ref << 16) | (r_ref & 0xFFFF));
			const __m128i v_ref_b  = _mm_set1_epi32(b_ref & 0xFFFF);
			const __m128i v_mask_b = _mm_set1_epi32(0x0000FFFF);
			const __m128i v_distInt  = _mm_set1_epi32((int)distInt);
			const __m128i v_distInt2 = _mm_set1_epi32((int)distInt2);
			const __m128i v_rgb_mask = _mm_set1_epi32(0x00FFFFFF);
			const __m128i v_alpha_mask = _mm_set1_epi32((int)0xFF000000u);
			const __m128i v_alpha_xor  = _mm_set1_epi32((int)alpha_xor);

			for (; i + 3 < len; i += 4) {
				__m128i px = _mm_loadu_si128((const __m128i*)(in + i));
				/* Unpack 4 pixels to 16-bit channels:
				   lo = [R0, G0, B0, A0, R1, G1, B1, A1]
				   hi = [R2, G2, B2, A2, R3, G3, B3, A3] */
				__m128i lo = _mm_unpacklo_epi8(px, v_zero);
				__m128i hi = _mm_unpackhi_epi8(px, v_zero);

				/* Collect [R0,G0, R1,G1, R2,G2, R3,G3] as 4x32-bit pairs of 16-bit lanes */
				__m128i rg = _mm_castps_si128(_mm_shuffle_ps(
					_mm_castsi128_ps(lo), _mm_castsi128_ps(hi), _MM_SHUFFLE(2, 0, 2, 0)));
				/* Collect [B0,A0, B1,A1, B2,A2, B3,A3] and zero A to get [B0,0, B1,0, B2,0, B3,0] */
				__m128i ba = _mm_castps_si128(_mm_shuffle_ps(
					_mm_castsi128_ps(lo), _mm_castsi128_ps(hi), _MM_SHUFFLE(3, 1, 3, 1)));
				__m128i b0 = _mm_and_si128(ba, v_mask_b);

				__m128i drg = _mm_sub_epi16(rg, v_ref_rg);
				__m128i db  = _mm_sub_epi16(b0, v_ref_b);

				__m128i d = _mm_add_epi32(_mm_madd_epi16(drg, drg), _mm_madd_epi16(db, db));

				__m128i lt_dist = _mm_cmplt_epi32(d, v_distInt);
				__m128i gt_dist2 = _mm_cmpgt_epi32(d, v_distInt2);
				__m128i in_band = _mm_and_si128(lt_dist, gt_dist2);

				int band_mask = _mm_movemask_ps(_mm_castsi128_ps(in_band));
				if (BLUESCREEN0R_LIKELY(band_mask == 0)) {
					__m128i alpha = _mm_andnot_si128(lt_dist, _mm_and_si128(px, v_alpha_mask));
					alpha = _mm_xor_si128(alpha, v_alpha_xor);
					__m128i res = _mm_or_si128(_mm_and_si128(px, v_rgb_mask), alpha);
					_mm_storeu_si128((__m128i*)(out + i), res);
				} else {
					uint32_t d_arr[4];
					_mm_storeu_si128((__m128i*)d_arr, d);
					for (unsigned int k = 0; k < 4; ++k) {
						uint32_t p = in[i + k];
						uint32_t dk = d_arr[k];
						unsigned char a = (unsigned char)(p >> 24);
						if (dk < distInt) {
							a = 0;
							if (dk > distInt2) {
								a = (unsigned char)(256u * (dk - distInt2) / distInt2);
							}
						}
						out[i + k] = (p & 0x00FFFFFFu) | (((uint32_t)a << 24) ^ alpha_xor);
					}
				}
			}
		}
#endif

		for (; i < len; ++i) {
			uint32_t p = in[i];
			uint32_t d = distance(p, r_ref, g_ref, b_ref);
			unsigned char a = (unsigned char)(p >> 24);
			if (d < distInt) {
				a = 0;
				if (d > distInt2) {
					a = (unsigned char)(256u * (d - distInt2) / distInt2);
				}
			}
			out[i] = (p & 0x00FFFFFFu) | (((uint32_t)a << 24) ^ alpha_xor);
		}
	}
};


static frei0r::construct<bluescreen0r> plugin("bluescreen0r",
									   "Color to alpha (blit SRCALPHA)",
									   "Hedde Bosman",
									   0, 5,
									   F0R_COLOR_MODEL_RGBA8888);
