/*
 * This file is a modified port of Softglow plug-in from Gimp.
 * It contains code from plug-ins/common/softglow.c, see that for copyrights.
 *
 * softglow.c
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
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "frei0r.h"
#include "frei0r/math.h"

#define SIGMOIDAL_BASE   2
#define SIGMOIDAL_RANGE 20

typedef struct softglow_instance
{
  unsigned int width;
  unsigned int height;
  double blur;
  double blur_kernel;
  double brightness;
  double sharpness;
  double blendtype;
  double lut_brightness;
  double lut_sharpness;
  int lut_valid;
  uint8_t sigm_lut[256];
  uint8_t* la_buf;   /* width * height * 2 bytes: interleaved [luma, alpha] */
  uint32_t* col_sum; /* width * 2 uint32_t vertical column sums */
} softglow_instance_t;

static void update_sigm_lut(softglow_instance_t* inst)
{
  const double brightness = inst->brightness;
  const double sharpness = inst->sharpness;
  const double factor = -(SIGMOIDAL_BASE + (sharpness * SIGMOIDAL_RANGE));
  for (int i = 0; i < 256; ++i)
  {
    double val = i / 255.0;
    val = 255.0 / (1.0 + exp(factor * (val - 0.5)));
    val = val * brightness;
    inst->sigm_lut[i] = (uint8_t)CLAMP(val, 0, 255);
  }
  inst->lut_brightness = brightness;
  inst->lut_sharpness = sharpness;
  inst->lut_valid = 1;
}

static inline uint8_t rgb_to_luma_int(uint32_t r, uint32_t g, uint32_t b)
{
  uint32_t mn = (r < g) ? r : g;
  uint32_t mx = (r > g) ? r : g;
  mn = (b < mn) ? b : mn;
  mx = (b > mx) ? b : mx;
  return (uint8_t)((mx + mn + 1u) >> 1);
}

static inline uint32_t blend_screen_px(uint32_t bl, uint32_t ba, uint32_t s2)
{
  uint32_t inv_bl = 255u - bl;
  uint32_t r2 = s2 & 0xFFu;
  uint32_t g2 = (s2 >> 8) & 0xFFu;
  uint32_t b2 = (s2 >> 16) & 0xFFu;
  uint32_t a2 = s2 >> 24;
  uint32_t t0, t1, t2;
  uint32_t r = 255u - INT_MULT(inv_bl, 255u - r2, t0);
  uint32_t g = 255u - INT_MULT(inv_bl, 255u - g2, t1);
  uint32_t b = 255u - INT_MULT(inv_bl, 255u - b2, t2);
  uint32_t a = (ba < a2) ? ba : a2;
  return r | (g << 8) | (b << 16) | (a << 24);
}

static inline uint32_t blend_overlay_px(uint32_t bl, uint32_t ba, uint32_t s2)
{
  uint32_t inv_bl = 255u - bl;
  uint32_t r2 = s2 & 0xFFu;
  uint32_t g2 = (s2 >> 8) & 0xFFu;
  uint32_t b2 = (s2 >> 16) & 0xFFu;
  uint32_t a2 = s2 >> 24;
  uint32_t tm0, tm1, tm2, t0, t1, t2;
  uint32_t r = INT_MULT(bl, bl + INT_MULT(2u * r2, inv_bl, tm0), t0);
  uint32_t g = INT_MULT(bl, bl + INT_MULT(2u * g2, inv_bl, tm1), t1);
  uint32_t b = INT_MULT(bl, bl + INT_MULT(2u * b2, inv_bl, tm2), t2);
  uint32_t a = (ba < a2) ? ba : a2;
  return r | (g << 8) | (b << 16) | (a << 24);
}

static inline uint32_t blend_add_px(uint32_t bl, uint32_t ba, uint32_t s2)
{
  uint32_t r2 = s2 & 0xFFu;
  uint32_t g2 = (s2 >> 8) & 0xFFu;
  uint32_t b2 = (s2 >> 16) & 0xFFu;
  uint32_t a2 = s2 >> 24;
  uint32_t r = bl + r2; if (r > 255u) r = 255u;
  uint32_t g = bl + g2; if (g > 255u) g = 255u;
  uint32_t b = bl + b2; if (b > 255u) b = 255u;
  uint32_t a = (ba < a2) ? ba : a2;
  return r | (g << 8) | (b << 16) | (a << 24);
}

static inline uint64_t fast_div_magic(unsigned int d)
{
  return ((1ULL << 55) + d - 1) / d;
}

static inline uint32_t blend_px_mode(int mode, uint32_t bl, uint32_t ba, uint32_t s2)
{
  if (mode == 0) return blend_screen_px(bl, ba, s2);
  if (mode == 1) return blend_overlay_px(bl, ba, s2);
  return blend_add_px(bl, ba, s2);
}

static void blur_and_blend_la(unsigned int width, unsigned int height, unsigned int k,
                              int mode,
                              const uint8_t* __restrict__ la_buf,
                              const uint32_t* __restrict__ inframe,
                              uint32_t* __restrict__ outframe,
                              uint32_t* __restrict__ col_sum)
{
  const size_t row_elems = (size_t)width * 2;
  memset(col_sum, 0, row_elems * sizeof(uint32_t));
  const unsigned int init_y1 = (k + 1 < height) ? (k + 1) : height;
  for (unsigned int ry = 0; ry < init_y1; ++ry)
  {
    const uint8_t* __restrict__ row = la_buf + (size_t)ry * row_elems;
    for (size_t i = 0; i < row_elems; ++i)
      col_sum[i] += row[i];
  }

  const unsigned int init_x1 = (k + 1 < width) ? (k + 1) : width;
  unsigned int x_mid_start = (k + 1 < width) ? (k + 1) : width;
  unsigned int x_mid_end = (width > k) ? (width - k) : 0;
  if (x_mid_end < x_mid_start) x_mid_end = x_mid_start;

  for (unsigned int y = 0; y < height; ++y)
  {
    if (y > 0)
    {
      if (y > k)
      {
        const uint8_t* __restrict__ old_row = la_buf + (size_t)(y - k - 1) * row_elems;
        if (y + k < height)
        {
          const uint8_t* __restrict__ new_row = la_buf + (size_t)(y + k) * row_elems;
          for (size_t i = 0; i < row_elems; ++i)
            col_sum[i] += (uint32_t)new_row[i] - (uint32_t)old_row[i];
        }
        else
        {
          for (size_t i = 0; i < row_elems; ++i)
            col_sum[i] -= old_row[i];
        }
      }
      else if (y + k < height)
      {
        const uint8_t* __restrict__ new_row = la_buf + (size_t)(y + k) * row_elems;
        for (size_t i = 0; i < row_elems; ++i)
          col_sum[i] += new_row[i];
      }
    }

    const unsigned int y0 = (y > k) ? (y - k) : 0;
    const unsigned int y1 = (y + k + 1 < height) ? (y + k + 1) : height;
    const unsigned int y_span = y1 - y0;

    uint32_t sl = 0, sa = 0;
    for (unsigned int cx = 0; cx < init_x1; ++cx)
    {
      sl += col_sum[(size_t)cx * 2];
      sa += col_sum[(size_t)cx * 2 + 1];
    }

    const uint32_t* __restrict__ src_row = inframe + (size_t)y * width;
    uint32_t* __restrict__ dst_row = outframe + (size_t)y * width;

    {
      unsigned int area = init_x1 * y_span;
      dst_row[0] = blend_px_mode(mode, sl / area, sa / area, src_row[0]);
    }

    for (unsigned int x = 1; x < x_mid_start; ++x)
    {
      if (x + k < width)
      {
        sl += col_sum[(size_t)(x + k) * 2];
        sa += col_sum[(size_t)(x + k) * 2 + 1];
      }
      unsigned int x1 = (x + k + 1 < width) ? (x + k + 1) : width;
      unsigned int area = x1 * y_span;
      dst_row[x] = blend_px_mode(mode, sl / area, sa / area, src_row[x]);
    }

    if (x_mid_start < x_mid_end)
    {
      const unsigned int area = (2 * k + 1) * y_span;
      const uint32_t* __restrict__ sub_c = col_sum + (size_t)(x_mid_start - k - 1) * 2;
      const uint32_t* __restrict__ add_c = col_sum + (size_t)(x_mid_start + k) * 2;
      if (area <= 11000000u)
      {
        const uint64_t magic = fast_div_magic(area);
        if (mode == 0)
        {
          for (unsigned int x = x_mid_start; x < x_mid_end; ++x)
          {
            sl += add_c[0] - sub_c[0];
            sa += add_c[1] - sub_c[1];
            sub_c += 2; add_c += 2;
            uint32_t bl = (uint32_t)(((uint64_t)sl * magic) >> 55);
            uint32_t ba = (uint32_t)(((uint64_t)sa * magic) >> 55);
            dst_row[x] = blend_screen_px(bl, ba, src_row[x]);
          }
        }
        else if (mode == 1)
        {
          for (unsigned int x = x_mid_start; x < x_mid_end; ++x)
          {
            sl += add_c[0] - sub_c[0];
            sa += add_c[1] - sub_c[1];
            sub_c += 2; add_c += 2;
            uint32_t bl = (uint32_t)(((uint64_t)sl * magic) >> 55);
            uint32_t ba = (uint32_t)(((uint64_t)sa * magic) >> 55);
            dst_row[x] = blend_overlay_px(bl, ba, src_row[x]);
          }
        }
        else
        {
          for (unsigned int x = x_mid_start; x < x_mid_end; ++x)
          {
            sl += add_c[0] - sub_c[0];
            sa += add_c[1] - sub_c[1];
            sub_c += 2; add_c += 2;
            uint32_t bl = (uint32_t)(((uint64_t)sl * magic) >> 55);
            uint32_t ba = (uint32_t)(((uint64_t)sa * magic) >> 55);
            dst_row[x] = blend_add_px(bl, ba, src_row[x]);
          }
        }
      }
      else
      {
        for (unsigned int x = x_mid_start; x < x_mid_end; ++x)
        {
          sl += add_c[0] - sub_c[0];
          sa += add_c[1] - sub_c[1];
          sub_c += 2; add_c += 2;
          dst_row[x] = blend_px_mode(mode, sl / area, sa / area, src_row[x]);
        }
      }
    }

    for (unsigned int x = x_mid_end; x < width; ++x)
    {
      if (x > k)
      {
        sl -= col_sum[(size_t)(x - k - 1) * 2];
        sa -= col_sum[(size_t)(x - k - 1) * 2 + 1];
      }
      if (x + k < width)
      {
        sl += col_sum[(size_t)(x + k) * 2];
        sa += col_sum[(size_t)(x + k) * 2 + 1];
      }
      unsigned int x0 = (x > k) ? (x - k) : 0;
      unsigned int x1 = (x + k + 1 < width) ? (x + k + 1) : width;
      unsigned int area = (x1 - x0) * y_span;
      dst_row[x] = blend_px_mode(mode, sl / area, sa / area, src_row[x]);
    }
  }
}

int f0r_init()
{
  return 1;
}

void f0r_deinit()
{ /* no initialization required */ }

void f0r_get_plugin_info(f0r_plugin_info_t* softglowInfo)
{
  softglowInfo->name = "softglow";
  softglowInfo->author = "Janne Liljeblad";
  softglowInfo->plugin_type = F0R_PLUGIN_TYPE_FILTER;
  softglowInfo->color_model = F0R_COLOR_MODEL_RGBA8888;
  softglowInfo->frei0r_version = FREI0R_MAJOR_VERSION;
  softglowInfo->major_version = 0; 
  softglowInfo->minor_version = 9; 
  softglowInfo->num_params =  4; 
  softglowInfo->explanation = "Does softglow effect on highlights";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
  switch ( param_index ) {
    case 0:
      info->name = "blur";
      info->type = F0R_PARAM_DOUBLE;
      info->explanation = "Blur of the glow";
      break;
    case 1:
      info->name = "brightness";
      info->type = F0R_PARAM_DOUBLE;
      info->explanation = "Brightness of highlight areas";
      break;
    case 2:
      info->name = "sharpness";
      info->type = F0R_PARAM_DOUBLE;
      info->explanation = "Sharpness of highlight areas";
      break;
    case 3: // 0 - 0.33 screen, 0.33 - 0.66 overlay, 0.66 - 1.0 add
      info->name = "blurblend";
      info->type = F0R_PARAM_DOUBLE;
      info->explanation = "Blend mode used to blend highlight blur with input image";
      break;
  }
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
  softglow_instance_t* inst = (softglow_instance_t*)calloc(1, sizeof(*inst));
  inst->width = width; 
  inst->height = height;
  inst->blur = 0.5;
  inst->blur_kernel = 0.0; /* Match original blur_construct() initial kernel */
  inst->brightness = 0.75;
  inst->sharpness = 0.85;
  inst->blendtype = 0.0;
  inst->lut_valid = 0;
  inst->la_buf = (uint8_t*)malloc((size_t)width * height * 2);
  inst->col_sum = (uint32_t*)malloc((size_t)width * 2 * sizeof(uint32_t));
  return (f0r_instance_t)inst;
}

void f0r_destruct(f0r_instance_t instance)
{
  softglow_instance_t* inst = (softglow_instance_t*)instance;
  free(inst->la_buf);
  free(inst->col_sum);
  free(instance);
}

void f0r_set_param_value(f0r_instance_t instance, 
                         f0r_param_t param, int param_index)
{
  softglow_instance_t* inst = (softglow_instance_t*)instance;
  switch (param_index)
  {
    case 0:
      inst->blur = *((double*)param);
      inst->blur_kernel = inst->blur;
      break;
    case 1:
      inst->brightness = *((double*)param);
      break;
    case 2:
      inst->sharpness = *((double*)param);
      break;
    case 3:
      inst->blendtype = *((double*)param);
      break;
  }
}

void f0r_get_param_value(f0r_instance_t instance,
                         f0r_param_t param, int param_index)
{
  softglow_instance_t* inst = (softglow_instance_t*)instance;
  switch (param_index) 
  {
    case 0:
      *((double*)param) = inst->blur;
      break;
    case 1:
      *((double*)param) = inst->brightness;
      break;
    case 2:
      *((double*)param) = inst->sharpness;
      break;
    case 3:
      *((double*)param) = inst->blendtype;
      break;
  }
}

void f0r_update(f0r_instance_t instance, double time,
                const uint32_t* inframe, uint32_t* outframe)
{
  (void)time;
  assert(instance);
  softglow_instance_t* inst = (softglow_instance_t*)instance;
  const unsigned int width = inst->width;
  const unsigned int height = inst->height;
  const size_t len = (size_t)width * height;

  if (!inst->lut_valid || inst->lut_brightness != inst->brightness || inst->lut_sharpness != inst->sharpness)
  {
    update_sigm_lut(inst);
  }
  const uint8_t* __restrict__ lut = inst->sigm_lut;

  int mode;
  if (inst->blendtype <= 0.33) mode = 0;
  else if (inst->blendtype <= 0.66) mode = 1;
  else mode = 2;

  const unsigned int max_dim = (width > height) ? width : height;
  const unsigned int k = (unsigned int)(inst->blur_kernel * max_dim / 2.0);

  if (k == 0)
  {
    /* Single-pass fused luma + sigmoid LUT + blend (no intermediate buffer) */
    const uint32_t* __restrict__ src = inframe;
    uint32_t* __restrict__ dst = outframe;
    if (mode == 0)
    {
      for (size_t i = 0; i < len; ++i)
      {
        uint32_t px = src[i];
        uint8_t luma = lut[rgb_to_luma_int(px & 0xFFu, (px >> 8) & 0xFFu, (px >> 16) & 0xFFu)];
        dst[i] = blend_screen_px(luma, px >> 24, px);
      }
    }
    else if (mode == 1)
    {
      for (size_t i = 0; i < len; ++i)
      {
        uint32_t px = src[i];
        uint8_t luma = lut[rgb_to_luma_int(px & 0xFFu, (px >> 8) & 0xFFu, (px >> 16) & 0xFFu)];
        dst[i] = blend_overlay_px(luma, px >> 24, px);
      }
    }
    else
    {
      for (size_t i = 0; i < len; ++i)
      {
        uint32_t px = src[i];
        uint8_t luma = lut[rgb_to_luma_int(px & 0xFFu, (px >> 8) & 0xFFu, (px >> 16) & 0xFFu)];
        dst[i] = blend_add_px(luma, px >> 24, px);
      }
    }
    return;
  }

  /* Pass 1: extract packed [sigm_luma, alpha] */
  const uint32_t* __restrict__ src = inframe;
  uint8_t* __restrict__ la = inst->la_buf;
  for (size_t i = 0; i < len; ++i)
  {
    uint32_t px = src[i];
    la[i * 2] = lut[rgb_to_luma_int(px & 0xFFu, (px >> 8) & 0xFFu, (px >> 16) & 0xFFu)];
    la[i * 2 + 1] = (uint8_t)(px >> 24);
  }

  /* Pass 2: 2-channel sliding-window box blur fused with blend */
  blur_and_blend_la(width, height, k, mode, la, inframe, outframe, inst->col_sum);
}
