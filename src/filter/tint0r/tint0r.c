/* tint0r.c
 * Copyright (C) 2009 Maksim Golovkin (m4ks1k@gmail.com),
 *               2025 Cynthia (cynthia2048@proton.me)
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

#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

/* Check for SSE4.1 support */
#if defined(__SSE4_1__)
#include <smmintrin.h>
#define USE_SSE4_1 1
#else
#define USE_SSE4_1 0
#endif

/* Check for other SIMD instruction sets */
#if defined(__AVX__) && defined(__AVX2__)
#include <immintrin.h>
#define USE_AVX2 1
#else
#define USE_AVX2 0
#endif

#if defined(__ARM_NEON__) || defined(__ARM_NEON)
#include <arm_neon.h>
#define USE_NEON 1
#else
#define USE_NEON 0
#endif

#include <frei0r.h>
#include <frei0r/math.h>

typedef struct tint0r_instance
{
  unsigned int width;
  unsigned int height;
  f0r_param_color_t blackColor;
  f0r_param_color_t whiteColor;
  double amount; /* the amount value [0, 1] */
} tint0r_instance_t;

int f0r_init()
{
  return 1;
}

void f0r_deinit()
{ /* no initialization required */ }

void f0r_get_plugin_info(f0r_plugin_info_t* info)
{
  info->name = "Tint0r";
  info->author = "Maksim Golovkin & Cynthia";
  info->plugin_type = F0R_PLUGIN_TYPE_FILTER;
  info->color_model = F0R_COLOR_MODEL_BGRA8888;
  info->frei0r_version = FREI0R_MAJOR_VERSION;
  info->major_version = 0;
  info->minor_version = 1;
  info->num_params = 3;
  info->explanation = "Tint a source image with specified colors";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
  switch(param_index)
  {
    case 0:
      info->name = "Map black to";
      info->type = F0R_PARAM_COLOR;
      info->explanation = "The color to map source color with null luminance";
      break;
    case 1:
      info->name = "Map white to";
      info->type = F0R_PARAM_COLOR;
      info->explanation = "The color to map source color with full luminance";
      break;
    case 2:
      info->name = "Tint amount";
      info->type = F0R_PARAM_DOUBLE;
      info->explanation = "Amount of color";
      break;
  }
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
  tint0r_instance_t* inst = (tint0r_instance_t*)calloc(1, sizeof(*inst));
  inst->width = width; inst->height = height;
  inst->amount = 0.25;
  inst->whiteColor.r = 0.5;
  inst->whiteColor.g = 1.0;
  inst->whiteColor.b = 0.5;
  inst->blackColor.r = 0.0;
  inst->blackColor.g = 0.0;
  inst->blackColor.b = 0.0;
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
  tint0r_instance_t* inst = (tint0r_instance_t*)instance;

  switch(param_index)
  {
    case 0:
      /* black color */
      inst->blackColor =  *((f0r_param_color_t *)param);
      break;
    case 1:
      /* white color */
      inst->whiteColor =  *((f0r_param_color_t *)param);
      break;
    case 2:
      /* amount */
      inst->amount = *((double *)param);
      break;
  }
}

void f0r_get_param_value(f0r_instance_t instance,
                         f0r_param_t param, int param_index)
{
  assert(instance);
  tint0r_instance_t* inst = (tint0r_instance_t*)instance;

  switch(param_index)
  {
    case 0:
      *((f0r_param_color_t*)param) = inst->blackColor;
      break;
    case 1:
      *((f0r_param_color_t*)param) = inst->whiteColor;
      break;
    case 2:
      *((double *)param) = inst->amount;
      break;
  }
}

static inline unsigned char map_color(double amount, double comp_amount, float color, float luma, float minColor, float maxColor)
{
  double val = (comp_amount * color) + amount * (luma * (maxColor - minColor) + minColor);
  return (unsigned char)(255*CLAMP(val, 0, 1));
}

#if USE_SSE4_1
static void tint_sse41(const uint32_t* inframe, uint32_t* outframe, size_t len,
                       double amount, f0r_param_color_t blackColor, f0r_param_color_t whiteColor)
{
  const __m128 weights = _mm_set_ps(0.0, 0.114, 0.587, 0.299),
  sse_amount = _mm_set1_ps(amount),
  /* Pass the alpha channel */
  comp_amount = _mm_set_ps(1.0,
                           1.0 - amount,
                           1.0 - amount,
                           1.0 - amount);

  /* Zero the alpha component to exclude it from calculations. */
  const __m128 cmin = _mm_set_ps(0.0, blackColor.b, blackColor.g, blackColor.r),
  cdelta = _mm_sub_ps(_mm_set_ps(0.0, whiteColor.b, whiteColor.g, whiteColor.r), cmin),
  tmp0 = _mm_mul_ps(cdelta, sse_amount),
  tmp1 = _mm_mul_ps(_mm_mul_ps(sse_amount, _mm_set1_ps(255.0)), cmin);

  __m128 p, p0, p1, p2, p3, luma;

  // Process pixels in groups of 4
  for (size_t i = 0; i < len; i++)
  {
    /* Load four pixels at once. */
    p = _mm_loadu_si128((__m128i*)(inframe + i * 4));

    /* Extract four pixels into separate XMM registers and convert them to float. */
    p0 = _mm_cvtepi32_ps(_mm_cvtepu8_epi32(p));
    p1 = _mm_cvtepi32_ps(_mm_cvtepu8_epi32(_mm_srli_si128(p, 4)));
    p2 = _mm_cvtepi32_ps(_mm_cvtepu8_epi32(_mm_srli_si128(p, 8)));
    p3 = _mm_cvtepi32_ps(_mm_cvtepu8_epi32(_mm_srli_si128(p, 12)));

    #define tint(v) \
      luma = _mm_dp_ps((v), weights, 0x7F); \
      v = _mm_add_ps(_mm_mul_ps(comp_amount, (v)), \
                     _mm_add_ps(_mm_mul_ps(luma, tmp0), tmp1)); \
      v = _mm_cvtps_epi32(v)

    tint(p0); tint(p1); tint(p2); tint(p3);

    /* Gather the processed pixels */
    p = _mm_packus_epi16(_mm_packus_epi32(p0, p1),
                         _mm_packus_epi32(p2, p3));

    _mm_storeu_si128((__m128i*)(outframe + i * 4), p);
  }
}
#elif USE_AVX2
static void tint_avx2(const uint32_t* inframe, uint32_t* outframe, size_t len,
                      double amount, f0r_param_color_t blackColor, f0r_param_color_t whiteColor)
{
  // AVX2 implementation would go here
  // For now, fall back to scalar implementation
  // This is a placeholder for a future AVX2 implementation
}
#elif USE_NEON
static void tint_neon(const uint32_t* inframe, uint32_t* outframe, size_t len,
                      double amount, f0r_param_color_t blackColor, f0r_param_color_t whiteColor)
{
  // NEON implementation would go here
  // For now, fall back to scalar implementation
  // This is a placeholder for a future NEON implementation
}
#endif

static void tint_scalar(const uint32_t* inframe, uint32_t* outframe, size_t len,
                        double amount, f0r_param_color_t blackColor, f0r_param_color_t whiteColor)
{
  double comp_amount = 1.0 - amount;

  const unsigned char* src = (const unsigned char*)inframe;
  unsigned char* dst = (unsigned char*)outframe;

  float b, g, r;
  float luma;

  while (len--)
  {
    b = src[0] / 255.0f;
    g = src[1] / 255.0f;
    r = src[2] / 255.0f;

    luma = (b * 0.114f + g * 0.587f + r * 0.299f);

    dst[0] = map_color(amount, comp_amount, b, luma, blackColor.b, whiteColor.b);
    dst[1] = map_color(amount, comp_amount, g, luma, blackColor.g, whiteColor.g);
    dst[2] = map_color(amount, comp_amount, r, luma, blackColor.r, whiteColor.r);
    dst[3] = src[3]; // Copy alpha

    src += 4;
    dst += 4;
  }
}

void f0r_update(f0r_instance_t instance, double time,
                const uint32_t* inframe, uint32_t* outframe)
{
  assert(instance);
  tint0r_instance_t* inst = (tint0r_instance_t*)instance;
  size_t len = inst->width * inst->height;

#if USE_SSE4_1
  // Process in chunks of 4 pixels for SSE
  size_t sse_len = len / 4;
  size_t remainder = len % 4;

  if (sse_len > 0) {
    tint_sse41(inframe, outframe, sse_len, inst->amount, inst->blackColor, inst->whiteColor);
  }

  // Handle remaining pixels with scalar implementation
  if (remainder > 0) {
    const uint32_t* remaining_in = inframe + (sse_len * 4);
    uint32_t* remaining_out = outframe + (sse_len * 4);
    tint_scalar(remaining_in, remaining_out, remainder, inst->amount, inst->blackColor, inst->whiteColor);
  }
#else
  // Use scalar implementation for all pixels
  tint_scalar(inframe, outframe, len, inst->amount, inst->blackColor, inst->whiteColor);
#endif
}

