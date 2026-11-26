/* hueshift0r.c
 * Copyright (C) 2005 Jean-Sebastien Senecal (Drone)
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
#if defined(_MSC_VER)
#define _USE_MATH_DEFINES
#endif /* _MSC_VER */
#include <math.h>
#include <assert.h>
#include <string.h>
#if defined(__SSE2__)
#include <emmintrin.h>
#endif

#include "frei0r.h"
#include "matrix.h"

typedef struct hueshift0r_instance
{
  unsigned int width;
  unsigned int height;
  int hueshift; /* the shift [0, 360] */
  float mat[4][4];
} hueshift0r_instance_t;

/* Updates the shift matrix. */
static void update_mat(hueshift0r_instance_t *inst)
{
  identmat((float*)inst->mat);
  huerotatemat(inst->mat, (float)inst->hueshift);
}

int f0r_init()
{
  return 1;
}

void f0r_deinit()
{ /* no initialization required */ }

void f0r_get_plugin_info(f0r_plugin_info_t* info)
{
  info->name = "Hueshift0r";
  info->author = "Jean-Sebastien Senecal";
  info->plugin_type = F0R_PLUGIN_TYPE_FILTER;
  info->color_model = F0R_COLOR_MODEL_RGBA8888;
  info->frei0r_version = FREI0R_MAJOR_VERSION;
  info->major_version = 0; 
  info->minor_version = 3; 
  info->num_params =  1; 
  info->explanation = "Shifts the hue of a source image";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
  switch(param_index)
  {
  case 0:
    info->name = "Hue";
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "The shift value";
    break;
  }
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
  hueshift0r_instance_t* inst = (hueshift0r_instance_t*)calloc(1, sizeof(*inst));
  inst->width = width; inst->height = height;
  /* init transformation matrix */
  inst->hueshift = 0;
  update_mat(inst);
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
  hueshift0r_instance_t* inst = (hueshift0r_instance_t*)instance;

  switch(param_index)
  {
    int val;
  case 0:
    /* contrast */
    val = (int) (*((double*)param) * 360.0); /* remap to [0, 360] */
    if (val != inst->hueshift)
    {
      inst->hueshift = val;
      update_mat(inst);
    }
    break;
  }
}

void f0r_get_param_value(f0r_instance_t instance,
                         f0r_param_t param, int param_index)
{
  assert(instance);
  hueshift0r_instance_t* inst = (hueshift0r_instance_t*)instance;
  
  switch(param_index)
  {
  case 0:
    *((double*)param) = (double) (inst->hueshift / 360.0);
    break;
  }
}

void f0r_update(f0r_instance_t instance, double time,
                const uint32_t* inframe, uint32_t* outframe)
{
  assert(instance);
  hueshift0r_instance_t* inst = (hueshift0r_instance_t*)instance;
  unsigned int len = inst->width * inst->height;
  unsigned int i = 0;

  const float m00 = inst->mat[0][0], m10 = inst->mat[1][0], m20 = inst->mat[2][0], m30 = inst->mat[3][0];
  const float m01 = inst->mat[0][1], m11 = inst->mat[1][1], m21 = inst->mat[2][1], m31 = inst->mat[3][1];
  const float m02 = inst->mat[0][2], m12 = inst->mat[1][2], m22 = inst->mat[2][2], m32 = inst->mat[3][2];

#if defined(__SSE2__)
  const __m128i maskff = _mm_set1_epi32(0xFF);
  const __m128i alpha_mask = _mm_set1_epi32((int)0xFF000000u);
  const __m128 vm00 = _mm_set1_ps(m00), vm10 = _mm_set1_ps(m10), vm20 = _mm_set1_ps(m20), vm30 = _mm_set1_ps(m30);
  const __m128 vm01 = _mm_set1_ps(m01), vm11 = _mm_set1_ps(m11), vm21 = _mm_set1_ps(m21), vm31 = _mm_set1_ps(m31);
  const __m128 vm02 = _mm_set1_ps(m02), vm12 = _mm_set1_ps(m12), vm22 = _mm_set1_ps(m22), vm32 = _mm_set1_ps(m32);
  const __m128i vzero = _mm_setzero_si128();
  const __m128i v255 = _mm_set1_epi32(255);

  for (; i + 3 < len; i += 4)
  {
    __m128i px = _mm_loadu_si128((const __m128i*)(inframe + i));
    __m128 vr = _mm_cvtepi32_ps(_mm_and_si128(px, maskff));
    __m128 vg = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(px, 8), maskff));
    __m128 vb = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(px, 16), maskff));

    __m128 fr = _mm_add_ps(_mm_add_ps(_mm_add_ps(_mm_mul_ps(vr, vm00), _mm_mul_ps(vg, vm10)), _mm_mul_ps(vb, vm20)), vm30);
    __m128 fg = _mm_add_ps(_mm_add_ps(_mm_add_ps(_mm_mul_ps(vr, vm01), _mm_mul_ps(vg, vm11)), _mm_mul_ps(vb, vm21)), vm31);
    __m128 fb = _mm_add_ps(_mm_add_ps(_mm_add_ps(_mm_mul_ps(vr, vm02), _mm_mul_ps(vg, vm12)), _mm_mul_ps(vb, vm22)), vm32);

    /* Convert with truncation to match C int cast, then clamp [0, 255] via pack */
    __m128i ir = _mm_cvttps_epi32(fr);
    __m128i ig = _mm_cvttps_epi32(fg);
    __m128i ib = _mm_cvttps_epi32(fb);

    /* Clamp [0, 255] in 32-bit lanes using SSE2 packs -> packus -> unpack */
    __m128i r8 = _mm_packus_epi16(_mm_packs_epi32(ir, vzero), vzero);
    __m128i g8 = _mm_packus_epi16(_mm_packs_epi32(ig, vzero), vzero);
    __m128i b8 = _mm_packus_epi16(_mm_packs_epi32(ib, vzero), vzero);

    __m128i r32 = _mm_unpacklo_epi16(_mm_unpacklo_epi8(r8, vzero), vzero);
    __m128i g32 = _mm_slli_epi32(_mm_unpacklo_epi16(_mm_unpacklo_epi8(g8, vzero), vzero), 8);
    __m128i b32 = _mm_slli_epi32(_mm_unpacklo_epi16(_mm_unpacklo_epi8(b8, vzero), vzero), 16);
    __m128i a32 = _mm_and_si128(px, alpha_mask);
    (void)v255;

    __m128i out = _mm_or_si128(_mm_or_si128(r32, g32), _mm_or_si128(b32, a32));
    _mm_storeu_si128((__m128i*)(outframe + i), out);
  }
#endif

  for (; i < len; i++)
  {
    uint32_t px = inframe[i];
    int ir = (int)(px & 0xFFu);
    int ig = (int)((px >> 8) & 0xFFu);
    int ib = (int)((px >> 16) & 0xFFu);
    int r = (int)(ir * m00 + ig * m10 + ib * m20 + m30);
    int g = (int)(ir * m01 + ig * m11 + ib * m21 + m31);
    int b = (int)(ir * m02 + ig * m12 + ib * m22 + m32);
    outframe[i] = (uint32_t)CLAMP0255(r) | ((uint32_t)CLAMP0255(g) << 8)
                | ((uint32_t)CLAMP0255(b) << 16) | (px & 0xFF000000u);
  }
}
