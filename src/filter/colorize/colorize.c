/*
 * This file is a port of Colorize plug-in from Gimp.
 * It contains code from files listed below:
 * gimpoperationcolorize.c
 * gimpcolortypes.h
 * gimpcolorspace.c
 * gimprgb.h
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Copyright (C) 2007 Michael Natterer
 *
 * This file, colorize.c
 * Copyright 2012 Janne Liljeblad 
 *
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
#if defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#include <emmintrin.h>
#define COLORIZE_USE_SSE2 1
#endif

#include "frei0r.h"
#include "frei0r/math.h"

#define GIMP_RGB_LUMINANCE_RED    (0.2126)
#define GIMP_RGB_LUMINANCE_GREEN  (0.7152)
#define GIMP_RGB_LUMINANCE_BLUE   (0.0722)

typedef struct colorize_instance
{
  unsigned int width;
  unsigned int height;
  double hue;
  double saturation;
  double lightness;
} colorize_instance_t;

static double lum_r_lut[256];
static double lum_g_lut[256];
static double lum_b_lut[256];
static int lum_lut_initialized = 0;

static void init_lum_luts(void)
{
  if (!lum_lut_initialized)
  {
    for (int i = 0; i < 256; i++)
    {
      double v = (double)i / 255.0;
      lum_r_lut[i] = v * GIMP_RGB_LUMINANCE_RED;
      lum_g_lut[i] = v * GIMP_RGB_LUMINANCE_GREEN;
      lum_b_lut[i] = v * GIMP_RGB_LUMINANCE_BLUE;
    }
    lum_lut_initialized = 1;
  }
}

/* mode: 0 = m1, 1 = m2, 2 = m1 + (m2 - m1) * weight */
static inline void
classify_hue_channel(double hue, int *mode, double *weight)
{
  if (hue > 6.0)
    hue -= 6.0;
  else if (hue < 0.0)
    hue += 6.0;

  if (hue < 1.0)
  {
    *mode = 2;
    *weight = hue;
  }
  else if (hue < 3.0)
  {
    *mode = 1;
    *weight = 0.0;
  }
  else if (hue < 4.0)
  {
    *mode = 2;
    *weight = 4.0 - hue;
  }
  else
  {
    *mode = 0;
    *weight = 0.0;
  }
}

int f0r_init()
{
  init_lum_luts();
  return 1;
}

void f0r_deinit()
{ /* no initialization required */ }

void f0r_get_plugin_info(f0r_plugin_info_t* colorize_info)
{
  colorize_info->name = "colorize";
  colorize_info->author = "Janne Liljeblad";
  colorize_info->plugin_type = F0R_PLUGIN_TYPE_FILTER;
  colorize_info->color_model = F0R_COLOR_MODEL_RGBA8888;
  colorize_info->frei0r_version = FREI0R_MAJOR_VERSION;
  colorize_info->major_version = 0; 
  colorize_info->minor_version = 1; 
  colorize_info->num_params =  3; 
  colorize_info->explanation = "Colorizes image to selected hue, saturation and lightness";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
  switch(param_index)
  {
  case 0:
    info->name = "hue"; 
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "Color shade of the colorized image";
    break;
  case 1:
    info->name = "saturation"; 
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "Amount of color in the colorized image";
    break;
  case 2:
    info->name = "lightness"; 
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "Lightness of the colorized image";
    break;
  }
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
  init_lum_luts();
  colorize_instance_t* inst = (colorize_instance_t*)calloc(1, sizeof(*inst));
  inst->width = width;
  inst->height = height;
  inst->hue = 0.5;
  inst->saturation = 0.5;
  inst->lightness = 0.5;
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
  colorize_instance_t* inst = (colorize_instance_t*)instance;

  switch(param_index)
  {
  case 0:
    inst->hue = *((double*)param);
    break;
  case 1:
    inst->saturation = *((double*)param);
    break;
  case 2:
    inst->lightness = *((double*)param);
    break;
  }
}

void f0r_get_param_value(f0r_instance_t instance,
                         f0r_param_t param, int param_index)
{
  assert(instance);
  colorize_instance_t* inst = (colorize_instance_t*)instance;
  
  switch(param_index)
  {
  case 0:
    *((double*)param) = inst->hue;
    break;
  case 1:
    *((double*)param) = inst->saturation;
    break;
  case 2:
    *((double*)param) = inst->lightness;
    break;
  }
}

#ifdef COLORIZE_USE_SSE2
static void
colorize_sse2_perm(const uint32_t* inframe, uint32_t* outframe, unsigned int len,
                   double s, int has_lum_adj, double lum_scale, double lum_offset,
                   double w, int sh_m1, int sh_m2, int sh_lerp)
{
  const __m128d v_half = _mm_set1_pd(0.5);
  const __m128d v_two = _mm_set1_pd(2.0);
  const __m128d v_255 = _mm_set1_pd(255.0);
  const __m128d v_s = _mm_set1_pd(s);
  const __m128d v_one_plus_s = _mm_set1_pd(1.0 + s);
  const __m128d v_w = _mm_set1_pd(w);
  const __m128i v_ff = _mm_set1_epi32(0xff);
  const __m128i v_sh_m1 = _mm_cvtsi32_si128(sh_m1);
  const __m128i v_sh_m2 = _mm_cvtsi32_si128(sh_m2);
  const __m128i v_sh_lerp = _mm_cvtsi32_si128(sh_lerp);

  if (!has_lum_adj)
  {
    for (unsigned int i = 0; i < len; i += 2)
    {
      uint32_t p0 = inframe[i];
      uint32_t p1 = inframe[i + 1];

      double lum0 = (lum_r_lut[p0 & 0xff] + lum_g_lut[(p0 >> 8) & 0xff]) + lum_b_lut[(p0 >> 16) & 0xff];
      double lum1 = (lum_r_lut[p1 & 0xff] + lum_g_lut[(p1 >> 8) & 0xff]) + lum_b_lut[(p1 >> 16) & 0xff];
      __m128d lum = _mm_set_pd(lum1, lum0);

      __m128d m2_lo = _mm_mul_pd(lum, v_one_plus_s);
      __m128d m2_hi = _mm_sub_pd(_mm_add_pd(lum, v_s), _mm_mul_pd(lum, v_s));
      __m128d le_half = _mm_cmple_pd(lum, v_half);
      __m128d m2 = _mm_or_pd(_mm_and_pd(le_half, m2_lo), _mm_andnot_pd(le_half, m2_hi));
      __m128d m1 = _mm_sub_pd(_mm_mul_pd(v_two, lum), m2);
      __m128d vl = _mm_add_pd(m1, _mm_mul_pd(_mm_sub_pd(m2, m1), v_w));

      __m128i im1 = _mm_sll_epi32(_mm_and_si128(_mm_cvttpd_epi32(_mm_mul_pd(m1, v_255)), v_ff), v_sh_m1);
      __m128i im2 = _mm_sll_epi32(_mm_and_si128(_mm_cvttpd_epi32(_mm_mul_pd(m2, v_255)), v_ff), v_sh_m2);
      __m128i ilp = _mm_sll_epi32(_mm_and_si128(_mm_cvttpd_epi32(_mm_mul_pd(vl, v_255)), v_ff), v_sh_lerp);
      __m128i rgb = _mm_or_si128(_mm_or_si128(im1, im2), ilp);

      uint64_t pair = (uint64_t)_mm_cvtsi128_si64(rgb);
      outframe[i]     = (uint32_t)pair | (p0 & 0xff000000u);
      outframe[i + 1] = (uint32_t)(pair >> 32) | (p1 & 0xff000000u);
    }
  }
  else
  {
    const __m128d v_lum_scale = _mm_set1_pd(lum_scale);
    const __m128d v_lum_offset = _mm_set1_pd(lum_offset);
    for (unsigned int i = 0; i < len; i += 2)
    {
      uint32_t p0 = inframe[i];
      uint32_t p1 = inframe[i + 1];

      double lum0 = (lum_r_lut[p0 & 0xff] + lum_g_lut[(p0 >> 8) & 0xff]) + lum_b_lut[(p0 >> 16) & 0xff];
      double lum1 = (lum_r_lut[p1 & 0xff] + lum_g_lut[(p1 >> 8) & 0xff]) + lum_b_lut[(p1 >> 16) & 0xff];
      __m128d lum = _mm_add_pd(_mm_mul_pd(_mm_set_pd(lum1, lum0), v_lum_scale), v_lum_offset);

      __m128d m2_lo = _mm_mul_pd(lum, v_one_plus_s);
      __m128d m2_hi = _mm_sub_pd(_mm_add_pd(lum, v_s), _mm_mul_pd(lum, v_s));
      __m128d le_half = _mm_cmple_pd(lum, v_half);
      __m128d m2 = _mm_or_pd(_mm_and_pd(le_half, m2_lo), _mm_andnot_pd(le_half, m2_hi));
      __m128d m1 = _mm_sub_pd(_mm_mul_pd(v_two, lum), m2);
      __m128d vl = _mm_add_pd(m1, _mm_mul_pd(_mm_sub_pd(m2, m1), v_w));

      __m128i im1 = _mm_sll_epi32(_mm_and_si128(_mm_cvttpd_epi32(_mm_mul_pd(m1, v_255)), v_ff), v_sh_m1);
      __m128i im2 = _mm_sll_epi32(_mm_and_si128(_mm_cvttpd_epi32(_mm_mul_pd(m2, v_255)), v_ff), v_sh_m2);
      __m128i ilp = _mm_sll_epi32(_mm_and_si128(_mm_cvttpd_epi32(_mm_mul_pd(vl, v_255)), v_ff), v_sh_lerp);
      __m128i rgb = _mm_or_si128(_mm_or_si128(im1, im2), ilp);

      uint64_t pair = (uint64_t)_mm_cvtsi128_si64(rgb);
      outframe[i]     = (uint32_t)pair | (p0 & 0xff000000u);
      outframe[i + 1] = (uint32_t)(pair >> 32) | (p1 & 0xff000000u);
    }
  }
}
#endif

void f0r_update(f0r_instance_t instance, double time,
                const uint32_t* inframe, uint32_t* outframe)
{
  (void)time;
  assert(instance);
  colorize_instance_t* inst = (colorize_instance_t*)instance;
  unsigned int len = inst->width * inst->height;

  const double h = inst->hue;
  const double s = inst->saturation;
  const double lightness = inst->lightness - 0.5;

  const double lum_scale = (lightness > 0.0) ? (1.0 - lightness)
                         : (lightness < 0.0) ? (lightness + 1.0)
                         : 1.0;
  const double lum_offset = (lightness > 0.0) ? (1.0 - (1.0 - lightness)) : 0.0;
  const int has_lum_adj = (lightness != 0.0);

  if (s == 0.0)
  {
    for (unsigned int i = 0; i < len; i++)
    {
      uint32_t p = inframe[i];
      double lum = (lum_r_lut[p & 0xff] + lum_g_lut[(p >> 8) & 0xff]) + lum_b_lut[(p >> 16) & 0xff];
      if (has_lum_adj)
        lum = lum * lum_scale + lum_offset;
      unsigned char v = (unsigned char)(lum * 255.0);
      outframe[i] = (uint32_t)v | ((uint32_t)v << 8) | ((uint32_t)v << 16) | (p & 0xff000000u);
    }
    return;
  }

  int mode[3];
  double w[3];
  classify_hue_channel(h * 6.0 + 2.0, &mode[0], &w[0]);
  classify_hue_channel(h * 6.0,       &mode[1], &w[1]);
  classify_hue_channel(h * 6.0 - 2.0, &mode[2], &w[2]);

  unsigned int i = 0;

#ifdef COLORIZE_USE_SSE2
  int sh_m1 = -1, sh_m2 = -1, sh_lerp = -1;
  double w_lerp = 0.0;
  for (int c = 0; c < 3; c++)
  {
    if (mode[c] == 0 && sh_m1 < 0) sh_m1 = c * 8;
    else if (mode[c] == 1 && sh_m2 < 0) sh_m2 = c * 8;
    else if (mode[c] == 2 && sh_lerp < 0) { sh_lerp = c * 8; w_lerp = w[c]; }
  }
  if (sh_m1 >= 0 && sh_m2 >= 0 && sh_lerp >= 0 && len >= 2)
  {
    unsigned int even_len = len & ~1u;
    colorize_sse2_perm(inframe, outframe, even_len, s, has_lum_adj, lum_scale, lum_offset,
                       w_lerp, sh_m1, sh_m2, sh_lerp);
    i = even_len;
  }
#endif

  const double one_plus_s = 1.0 + s;
  for (; i < len; i++)
  {
    uint32_t p = inframe[i];
    double lum = (lum_r_lut[p & 0xff] + lum_g_lut[(p >> 8) & 0xff]) + lum_b_lut[(p >> 16) & 0xff];
    if (has_lum_adj)
      lum = lum * lum_scale + lum_offset;

    double m2 = (lum <= 0.5) ? (lum * one_plus_s) : (lum + s - lum * s);
    double m1 = 2.0 * lum - m2;
    double diff = m2 - m1;

    double vr = (mode[0] == 1) ? m2 : (mode[0] == 0 ? m1 : (m1 + diff * w[0]));
    double vg = (mode[1] == 1) ? m2 : (mode[1] == 0 ? m1 : (m1 + diff * w[1]));
    double vb = (mode[2] == 1) ? m2 : (mode[2] == 0 ? m1 : (m1 + diff * w[2]));

    unsigned char or_ = (unsigned char)(vr * 255.0);
    unsigned char og_ = (unsigned char)(vg * 255.0);
    unsigned char ob_ = (unsigned char)(vb * 255.0);
    outframe[i] = (uint32_t)or_ | ((uint32_t)og_ << 8) | ((uint32_t)ob_ << 16) | (p & 0xff000000u);
  }
}
