/* 
 * This file is a port of com.jhlabs.image.EmbossFilter.java
 * Copyright 2006 Jerry Huxtable
 *
 * emboss.c
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
#include <math.h>
#include <stdint.h>

#if defined(__SSE2__)
#include <emmintrin.h>
#endif

#include "frei0r.h"
#include "frei0r/math.h"

static const double PI = 3.14159;
static double pixelScale = 255.9;

typedef struct emboss_instance
{
  unsigned int width;
  unsigned int height;
  double azimuth;
  double elevation;
  double width45;
  unsigned char *bumpPixels;
} emboss_instance_t;

int f0r_init()
{
  return 1;
}

void f0r_deinit()
{ /* no initialization required */ }

void f0r_get_plugin_info(f0r_plugin_info_t* emposs_info)
{
  emposs_info->name = "emboss";
  emposs_info->author = "Janne Liljeblad";
  emposs_info->plugin_type = F0R_PLUGIN_TYPE_FILTER;
  emposs_info->color_model = F0R_COLOR_MODEL_RGBA8888;
  emposs_info->frei0r_version = FREI0R_MAJOR_VERSION;
  emposs_info->major_version = 0; 
  emposs_info->minor_version = 1; 
  emposs_info->num_params =  3; 
  emposs_info->explanation = "Creates embossed relief image of source image";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
  switch(param_index)
  {
  case 0:
    info->name = "azimuth";
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "Light direction";
    break;
  case 1:
    info->name = "elevation";
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "Background lightness";
    break;
  case 2:
    info->name = "width45";
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "Bump height";
    break;
  }
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
  emboss_instance_t* inst = (emboss_instance_t*)calloc(1, sizeof(*inst));
  inst->width = width; 
  inst->height = height;
  inst->azimuth = 135.0 / 360.0; //input range 0 - 1 will be interpreted as angle 0 - 360
  inst->elevation = 30.0 / 90.0;//input range 0 - 1 will be interpreted as lighness value 0 - 90
  inst->width45 = 10.0 / 40.0;//input range 0 - 1 will be interpreted as bump height value 1 - 40
  inst->bumpPixels = (unsigned char*)malloc((size_t)width * height);
  return (f0r_instance_t)inst;
}

void f0r_destruct(f0r_instance_t instance)
{
  emboss_instance_t* inst = (emboss_instance_t*)instance;
  free(inst->bumpPixels);
  free(instance);
}

void f0r_set_param_value(f0r_instance_t instance, 
                         f0r_param_t param, int param_index)
{
  assert(instance);
  emboss_instance_t* inst = (emboss_instance_t*)instance;

  switch(param_index)
  {
  case 0:
    inst->azimuth = *((double*)param);
    break;
  case 1:
    inst->elevation = *((double*)param);
    break;
  case 2:
    inst->width45 = *((double*)param);
    break;
  }
}

void f0r_get_param_value(f0r_instance_t instance,
                         f0r_param_t param, int param_index)
{
  assert(instance);
  emboss_instance_t* inst = (emboss_instance_t*)instance;
  
  switch(param_index)
  {
  case 0:
    *((double*)param) = inst->azimuth;
    break;
  case 1:
    *((double*)param) = inst->elevation;
    break;
  case 2:
    *((double*)param) = inst->width45;
    break;
  }
}

void f0r_update(f0r_instance_t instance, double time,
                const uint32_t* inframe, uint32_t* outframe)
{
  // Check and cast instance
  assert(instance);
  emboss_instance_t* inst = (emboss_instance_t*)instance;
 
  // Get render params values 0.0-1.0 in range used by filter
  double azimuthInput = inst->azimuth * 360.0;
  double elevationInput = inst->elevation * 90.0;
  double widthInput = inst->width45 * 40.0;

  // Force correct ranges on input
  azimuthInput = CLAMP(azimuthInput, 0.0, 360.0);
  elevationInput = CLAMP(elevationInput, 0.0, 90.0);
  widthInput = CLAMP(widthInput, 1.0, 40.0);

  // Convert to filter input values
  double azimuth = azimuthInput * PI / 180.0; 
  double elevation = elevationInput * PI / 180.0;
  double width45 = widthInput;

  // Create brightness image (exact (r + g + b) / 3 using (sum * 0x5556) >> 16)
  size_t len = (size_t)inst->width * inst->height;
  unsigned char *bumpPixels = inst->bumpPixels;
  size_t idx = 0;

#if defined(__SSE2__)
  {
    const __m128i mask_u8 = _mm_set1_epi32(0x000000FF);
    const __m128i mul_div3 = _mm_set1_epi16(0x5556);
    for (; idx + 8 <= len; idx += 8)
    {
      __m128i p0 = _mm_loadu_si128((const __m128i*)(inframe + idx));
      __m128i p1 = _mm_loadu_si128((const __m128i*)(inframe + idx + 4));

      __m128i r0 = _mm_and_si128(p0, mask_u8);
      __m128i g0 = _mm_and_si128(_mm_srli_epi32(p0, 8), mask_u8);
      __m128i b0 = _mm_and_si128(_mm_srli_epi32(p0, 16), mask_u8);
      __m128i s0 = _mm_add_epi32(_mm_add_epi32(r0, g0), b0);

      __m128i r1 = _mm_and_si128(p1, mask_u8);
      __m128i g1 = _mm_and_si128(_mm_srli_epi32(p1, 8), mask_u8);
      __m128i b1 = _mm_and_si128(_mm_srli_epi32(p1, 16), mask_u8);
      __m128i s1 = _mm_add_epi32(_mm_add_epi32(r1, g1), b1);

      __m128i s16 = _mm_packs_epi32(s0, s1);
      __m128i q16 = _mm_mulhi_epu16(s16, mul_div3);
      __m128i q8 = _mm_packus_epi16(q16, q16);
      _mm_storel_epi64((__m128i*)(bumpPixels + idx), q8);
    }
  }
#endif
  for (; idx < len; idx++)
  {
    uint32_t px = inframe[idx];
    unsigned int sum = (px & 0xFFu) + ((px >> 8) & 0xFFu) + ((px >> 16) & 0xFFu);
    bumpPixels[idx] = (unsigned char)((sum * 0x5556u) >> 16);
  }

  // Create embossed image from brightness image
  int Lx = (int)(cos(azimuth) * cos(elevation) * pixelScale);
  int Ly = (int)(sin(azimuth) * cos(elevation) * pixelScale);
  int Lz = (int)(sin(elevation) * pixelScale);

  int Nz = (int)(6 * 255 / width45);
  int Nz2 = Nz * Nz;
  int NzLz = Nz * Lz;

  unsigned char background = (unsigned char)Lz;
  uint32_t bg_rgb = (uint32_t)background * 0x00010101u;

  int width = (int)inst->width;
  int height = (int)inst->height;

  int y;
  for (y = 0; y < height; y++)
  {
    const uint32_t *in_row = inframe + (size_t)y * width;
    uint32_t *out_row = outframe + (size_t)y * width;

    if (y == 0 || y >= height - 2 || width <= 2)
    {
      int x;
      for (x = 0; x < width; x++)
      {
        out_row[x] = bg_rgb | (in_row[x] & 0xFF000000u);
      }
      continue;
    }

    const unsigned char *r0 = bumpPixels + (size_t)y * width;
    const unsigned char *r1 = r0 + width;
    const unsigned char *r2 = r1 + width;

    out_row[0] = bg_rgb | (in_row[0] & 0xFF000000u);

    int x = 1;
#if defined(__SSE2__)
    {
      const __m128d vNz2 = _mm_set1_pd((double)Nz2);
      const __m128d vZero = _mm_setzero_pd();
      for (; x + 1 < width - 2; x += 2)
      {
        int Nx0 = (int)r0[x - 1] + (int)r1[x - 1] + (int)r2[x - 1]
                - (int)r0[x + 1] - (int)r1[x + 1] - (int)r2[x + 1];
        int Ny0 = (int)r2[x - 1] + (int)r2[x] + (int)r2[x + 1]
                - (int)r0[x - 1] - (int)r0[x] - (int)r0[x + 1];

        int Nx1 = (int)r0[x] + (int)r1[x] + (int)r2[x]
                - (int)r0[x + 2] - (int)r1[x + 2] - (int)r2[x + 2];
        int Ny1 = (int)r2[x] + (int)r2[x + 1] + (int)r2[x + 2]
                - (int)r0[x] - (int)r0[x + 1] - (int)r0[x + 2];

        int nd0 = Nx0 * Lx + Ny0 * Ly + NzLz;
        int nd1 = Nx1 * Lx + Ny1 * Ly + NzLz;

        __m128d vnd = _mm_set_pd((double)nd1, (double)nd0);
        __m128d vnorm2 = _mm_add_pd(
          _mm_set_pd((double)(Nx1 * Nx1 + Ny1 * Ny1), (double)(Nx0 * Nx0 + Ny0 * Ny0)),
          vNz2);
        __m128d vres = _mm_div_pd(_mm_max_pd(vnd, vZero), _mm_sqrt_pd(vnorm2));
        __m128i vi = _mm_cvttpd_epi32(vres);

        unsigned char s0 = (unsigned char)_mm_cvtsi128_si32(vi);
        unsigned char s1 = (unsigned char)_mm_cvtsi128_si32(_mm_srli_si128(vi, 4));

        out_row[x]     = ((uint32_t)s0 * 0x00010101u) | (in_row[x] & 0xFF000000u);
        out_row[x + 1] = ((uint32_t)s1 * 0x00010101u) | (in_row[x + 1] & 0xFF000000u);
      }
    }
#endif
    for (; x < width - 2; x++)
    {
      int Nx = (int)r0[x - 1] + (int)r1[x - 1] + (int)r2[x - 1]
             - (int)r0[x + 1] - (int)r1[x + 1] - (int)r2[x + 1];
      int Ny = (int)r2[x - 1] + (int)r2[x] + (int)r2[x + 1]
             - (int)r0[x - 1] - (int)r0[x] - (int)r0[x + 1];
      unsigned char shade;
      int NdotL;
      if (Nx == 0 && Ny == 0)
        shade = background;
      else if ((NdotL = Nx * Lx + Ny * Ly + NzLz) < 0)
        shade = 0;
      else
        shade = (unsigned char)(int)(NdotL / sqrt((double)(Nx * Nx + Ny * Ny + Nz2)));

      out_row[x] = ((uint32_t)shade * 0x00010101u) | (in_row[x] & 0xFF000000u);
    }

    for (; x < width; x++)
    {
      out_row[x] = bg_rgb | (in_row[x] & 0xFF000000u);
    }
  }
}
