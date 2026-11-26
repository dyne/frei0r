/* colordistance.c
 * Copyright (C) 2007 Richard Spindler (richard.spindler@gmail.com)
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

#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#if defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#include <emmintrin.h>
#define COLORDIST_USE_SSE2 1
#endif

#include "frei0r.h"
#include "frei0r/math.h"

typedef struct colordistance_instance
{
	unsigned int width;
	unsigned int height;
	f0r_param_color_t color;
} colordistance_instance_t;

int f0r_init()
{
	return 1;
}

void f0r_deinit()
{ /* no initialization required */ }

void f0r_get_plugin_info(f0r_plugin_info_t* colordistance_info)
{
	colordistance_info->name = "Color Distance";
	colordistance_info->author = "Richard Spindler";
	colordistance_info->plugin_type = F0R_PLUGIN_TYPE_FILTER;
	colordistance_info->color_model = F0R_COLOR_MODEL_RGBA8888;
	colordistance_info->frei0r_version = FREI0R_MAJOR_VERSION;
	colordistance_info->major_version = 0; 
	colordistance_info->minor_version = 2; 
	colordistance_info->num_params = 1; 
	colordistance_info->explanation = "Calculates the distance between the selected color and the current pixel and uses that value as new pixel value";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
	switch(param_index)
	{
		case 0:
			info->name = "Color";
			info->type = F0R_PARAM_COLOR;
			info->explanation = "The Source Color";
			break;
	}

}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
	colordistance_instance_t* inst = (colordistance_instance_t*)calloc(1, sizeof(*inst));
	inst->width = width; inst->height = height;
	inst->color.r = 0.5;
	inst->color.g = 0.5;
	inst->color.b = 0.5;
	return (f0r_instance_t)inst;
}

void f0r_destruct(f0r_instance_t instance)
{
	free(instance);
}

void f0r_set_param_value(f0r_instance_t instance, 
		f0r_param_t param, int param_index)
{
	assert(instance);
	colordistance_instance_t* inst = (colordistance_instance_t*)instance;

	switch(param_index) {
		case 0:
			inst->color = *((f0r_param_color_t*)param);
			break;
	}

}

void f0r_get_param_value(f0r_instance_t instance,
		f0r_param_t param, int param_index)
{
	assert(instance);
	colordistance_instance_t* inst = (colordistance_instance_t*)instance;

	switch(param_index) {
		case 0:
			*((f0r_param_color_t*)param) = inst->color;
			break;
	}

}

void f0r_update(f0r_instance_t instance, double time,
		const uint32_t* inframe, uint32_t* outframe)
{
	(void)time;
	assert(instance);
	colordistance_instance_t* inst = (colordistance_instance_t*)instance;
	unsigned int len = inst->width * inst->height;

	float r1 = inst->color.r * 255.0f;
	float g1 = inst->color.g * 255.0f;
	float b1 = inst->color.b * 255.0f;
	/* Scale factor to normalize distance to 0-255 range:
	   0.705724361914764 ~= 255.0 / sqrt(3 * 255^2) = 255.0 / (255.0 * sqrt(3)) */
	const float SCALE_FACTOR = 0.705724361914764f;
	unsigned int i = 0;

#ifdef COLORDIST_USE_SSE2
	const __m128 v_r1 = _mm_set1_ps(r1);
	const __m128 v_g1 = _mm_set1_ps(g1);
	const __m128 v_b1 = _mm_set1_ps(b1);
	const __m128 v_scale = _mm_set1_ps(SCALE_FACTOR);
	const __m128i v_ff = _mm_set1_epi32(0xff);
	const __m128i v_alpha_mask = _mm_set1_epi32((int)0xff000000u);
	const __m128i v_zero = _mm_setzero_si128();

	for (; i + 3 < len; i += 4) {
		__m128i px = _mm_loadu_si128((const __m128i*)(inframe + i));
		__m128 vr = _mm_cvtepi32_ps(_mm_and_si128(px, v_ff));
		__m128 vg = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(px, 8), v_ff));
		__m128 vb = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(px, 16), v_ff));

		__m128 dr = _mm_sub_ps(v_r1, vr);
		__m128 dg = _mm_sub_ps(v_g1, vg);
		__m128 db = _mm_sub_ps(v_b1, vb);

		__m128 sum = _mm_add_ps(_mm_add_ps(_mm_mul_ps(dr, dr), _mm_mul_ps(dg, dg)), _mm_mul_ps(db, db));
		__m128 dist = _mm_mul_ps(_mm_sqrt_ps(sum), v_scale);
		__m128i il = _mm_cvtps_epi32(dist);

		/* Clamp 4x int32 to [0, 255] via saturating packs */
		__m128i il16 = _mm_packs_epi32(il, il);
		__m128i il8 = _mm_packus_epi16(il16, il16);
		__m128i cl = _mm_unpacklo_epi16(_mm_unpacklo_epi8(il8, v_zero), v_zero);

		__m128i rgb = _mm_or_si128(_mm_or_si128(cl, _mm_slli_epi32(cl, 8)), _mm_slli_epi32(cl, 16));
		__m128i out = _mm_or_si128(rgb, _mm_and_si128(px, v_alpha_mask));
		_mm_storeu_si128((__m128i*)(outframe + i), out);
	}
#endif

	for (; i < len; i++) {
		uint32_t p = inframe[i];
		float dr = r1 - (float)(p & 0xffu);
		float dg = g1 - (float)((p >> 8) & 0xffu);
		float db = b1 - (float)((p >> 16) & 0xffu);
		int l = (int)rint(sqrtf(dr * dr + dg * dg + db * db) * SCALE_FACTOR);
		l = CLAMP(l, 0, 255);
		outframe[i] = (uint32_t)l | ((uint32_t)l << 8) | ((uint32_t)l << 16) | (p & 0xff000000u);
	}
}
