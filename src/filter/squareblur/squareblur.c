/* squareblur.c
 * Copyright (C) 2004--2005 Mathieu Guindon
 *                          Julien Keable
 *                          Jean-Sebastien Senecal (js@drone.ws)
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
#include <assert.h>
#include <string.h>

#include "frei0r.h"

#define SIZE_RGBA 4

typedef struct squareblur_fast_instance
{
  unsigned int width;
  unsigned int height;
  double kernel;
  uint32_t *col_sum; /* width * 4 uint32_t vertical sliding sums (also aliased as uint16_t when y_span <= 257) */
} squareblur_fast_instance_t;

/* Exact division sum / d for 0 <= sum <= 255 * d and 1 <= d <= 11000000:
 * M = ceil(2^55 / d). */
static inline uint64_t fast_div_magic(unsigned int d)
{
  return ((1ULL << 55) + d - 1) / d;
}

static inline uint32_t pack_avg_magic(uint32_t s0, uint32_t s1,
                                      uint32_t s2, uint32_t s3,
                                      uint64_t magic)
{
  uint32_t r0 = (uint32_t)(((uint64_t)s0 * magic) >> 55);
  uint32_t r1 = (uint32_t)(((uint64_t)s1 * magic) >> 55);
  uint32_t r2 = (uint32_t)(((uint64_t)s2 * magic) >> 55);
  uint32_t r3 = (uint32_t)(((uint64_t)s3 * magic) >> 55);
  return r0 | (r1 << 8) | (r2 << 16) | (r3 << 24);
}

static inline uint32_t pack_avg_div(uint32_t s0, uint32_t s1,
                                    uint32_t s2, uint32_t s3,
                                    unsigned int area)
{
  if (area <= 11000000u)
  {
    return pack_avg_magic(s0, s1, s2, s3, fast_div_magic(area));
  }
  return (s0 / area) | ((s1 / area) << 8) | ((s2 / area) << 16) | ((s3 / area) << 24);
}

static void blur_u16_colsum(unsigned int width, unsigned int height, unsigned int k,
                            const unsigned char* __restrict__ src_bytes,
                            uint32_t* __restrict__ outframe,
                            uint16_t* __restrict__ col_sum)
{
  const size_t row_bytes = (size_t)width * SIZE_RGBA;
  memset(col_sum, 0, row_bytes * sizeof(uint16_t));
  const unsigned int init_y1 = (k + 1 < height) ? (k + 1) : height;
  for (unsigned int ry = 0; ry < init_y1; ++ry)
  {
    const unsigned char* __restrict__ row = src_bytes + (size_t)ry * row_bytes;
    for (size_t i = 0; i < row_bytes; ++i)
      col_sum[i] = (uint16_t)(col_sum[i] + row[i]);
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
        const unsigned char* __restrict__ old_row = src_bytes + (size_t)(y - k - 1) * row_bytes;
        if (y + k < height)
        {
          const unsigned char* __restrict__ new_row = src_bytes + (size_t)(y + k) * row_bytes;
          for (size_t i = 0; i < row_bytes; ++i)
            col_sum[i] = (uint16_t)(col_sum[i] + new_row[i] - old_row[i]);
        }
        else
        {
          for (size_t i = 0; i < row_bytes; ++i)
            col_sum[i] = (uint16_t)(col_sum[i] - old_row[i]);
        }
      }
      else if (y + k < height)
      {
        const unsigned char* __restrict__ new_row = src_bytes + (size_t)(y + k) * row_bytes;
        for (size_t i = 0; i < row_bytes; ++i)
          col_sum[i] = (uint16_t)(col_sum[i] + new_row[i]);
      }
    }

    const unsigned int y0 = (y > k) ? (y - k) : 0;
    const unsigned int y1 = (y + k + 1 < height) ? (y + k + 1) : height;
    const unsigned int y_span = y1 - y0;

    uint32_t s0 = 0, s1 = 0, s2 = 0, s3 = 0;
    for (unsigned int cx = 0; cx < init_x1; ++cx)
    {
      const uint16_t* cs = col_sum + (size_t)cx * 4;
      s0 += cs[0]; s1 += cs[1]; s2 += cs[2]; s3 += cs[3];
    }

    uint32_t* __restrict__ dst = outframe + (size_t)y * width;
    *dst++ = pack_avg_div(s0, s1, s2, s3, init_x1 * y_span);

    for (unsigned int x = 1; x < x_mid_start; ++x)
    {
      if (x + k < width)
      {
        const uint16_t* add_c = col_sum + (size_t)(x + k) * 4;
        s0 += add_c[0]; s1 += add_c[1]; s2 += add_c[2]; s3 += add_c[3];
      }
      const unsigned int x1 = (x + k + 1 < width) ? (x + k + 1) : width;
      *dst++ = pack_avg_div(s0, s1, s2, s3, x1 * y_span);
    }

    if (x_mid_start < x_mid_end)
    {
      const unsigned int area = (2 * k + 1) * y_span;
      const uint64_t magic = fast_div_magic(area);
      const uint16_t* __restrict__ sub_c = col_sum + (size_t)(x_mid_start - k - 1) * 4;
      const uint16_t* __restrict__ add_c = col_sum + (size_t)(x_mid_start + k) * 4;
      for (unsigned int x = x_mid_start; x < x_mid_end; ++x)
      {
        s0 += (uint32_t)add_c[0] - (uint32_t)sub_c[0];
        s1 += (uint32_t)add_c[1] - (uint32_t)sub_c[1];
        s2 += (uint32_t)add_c[2] - (uint32_t)sub_c[2];
        s3 += (uint32_t)add_c[3] - (uint32_t)sub_c[3];
        sub_c += 4;
        add_c += 4;
        *dst++ = pack_avg_magic(s0, s1, s2, s3, magic);
      }
    }

    for (unsigned int x = x_mid_end; x < width; ++x)
    {
      if (x > k)
      {
        const uint16_t* sub_c = col_sum + (size_t)(x - k - 1) * 4;
        s0 -= sub_c[0]; s1 -= sub_c[1]; s2 -= sub_c[2]; s3 -= sub_c[3];
      }
      if (x + k < width)
      {
        const uint16_t* add_c = col_sum + (size_t)(x + k) * 4;
        s0 += add_c[0]; s1 += add_c[1]; s2 += add_c[2]; s3 += add_c[3];
      }
      const unsigned int x0 = (x > k) ? (x - k) : 0;
      const unsigned int x1 = (x + k + 1 < width) ? (x + k + 1) : width;
      *dst++ = pack_avg_div(s0, s1, s2, s3, (x1 - x0) * y_span);
    }
  }
}

static void blur_u32_colsum(unsigned int width, unsigned int height, unsigned int k,
                            const unsigned char* __restrict__ src_bytes,
                            uint32_t* __restrict__ outframe,
                            uint32_t* __restrict__ col_sum)
{
  const size_t row_bytes = (size_t)width * SIZE_RGBA;
  memset(col_sum, 0, row_bytes * sizeof(uint32_t));
  const unsigned int init_y1 = (k + 1 < height) ? (k + 1) : height;
  for (unsigned int ry = 0; ry < init_y1; ++ry)
  {
    const unsigned char* __restrict__ row = src_bytes + (size_t)ry * row_bytes;
    for (size_t i = 0; i < row_bytes; ++i)
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
        const unsigned char* __restrict__ old_row = src_bytes + (size_t)(y - k - 1) * row_bytes;
        if (y + k < height)
        {
          const unsigned char* __restrict__ new_row = src_bytes + (size_t)(y + k) * row_bytes;
          for (size_t i = 0; i < row_bytes; ++i)
            col_sum[i] += (uint32_t)new_row[i] - (uint32_t)old_row[i];
        }
        else
        {
          for (size_t i = 0; i < row_bytes; ++i)
            col_sum[i] -= old_row[i];
        }
      }
      else if (y + k < height)
      {
        const unsigned char* __restrict__ new_row = src_bytes + (size_t)(y + k) * row_bytes;
        for (size_t i = 0; i < row_bytes; ++i)
          col_sum[i] += new_row[i];
      }
    }

    const unsigned int y0 = (y > k) ? (y - k) : 0;
    const unsigned int y1 = (y + k + 1 < height) ? (y + k + 1) : height;
    const unsigned int y_span = y1 - y0;

    uint32_t s0 = 0, s1 = 0, s2 = 0, s3 = 0;
    for (unsigned int cx = 0; cx < init_x1; ++cx)
    {
      const uint32_t* cs = col_sum + (size_t)cx * 4;
      s0 += cs[0]; s1 += cs[1]; s2 += cs[2]; s3 += cs[3];
    }

    uint32_t* __restrict__ dst = outframe + (size_t)y * width;
    *dst++ = pack_avg_div(s0, s1, s2, s3, init_x1 * y_span);

    for (unsigned int x = 1; x < x_mid_start; ++x)
    {
      if (x + k < width)
      {
        const uint32_t* add_c = col_sum + (size_t)(x + k) * 4;
        s0 += add_c[0]; s1 += add_c[1]; s2 += add_c[2]; s3 += add_c[3];
      }
      const unsigned int x1 = (x + k + 1 < width) ? (x + k + 1) : width;
      *dst++ = pack_avg_div(s0, s1, s2, s3, x1 * y_span);
    }

    if (x_mid_start < x_mid_end)
    {
      const unsigned int area = (2 * k + 1) * y_span;
      const uint32_t* __restrict__ sub_c = col_sum + (size_t)(x_mid_start - k - 1) * 4;
      const uint32_t* __restrict__ add_c = col_sum + (size_t)(x_mid_start + k) * 4;
      if (area <= 11000000u)
      {
        const uint64_t magic = fast_div_magic(area);
        for (unsigned int x = x_mid_start; x < x_mid_end; ++x)
        {
          s0 += add_c[0] - sub_c[0];
          s1 += add_c[1] - sub_c[1];
          s2 += add_c[2] - sub_c[2];
          s3 += add_c[3] - sub_c[3];
          sub_c += 4;
          add_c += 4;
          *dst++ = pack_avg_magic(s0, s1, s2, s3, magic);
        }
      }
      else
      {
        for (unsigned int x = x_mid_start; x < x_mid_end; ++x)
        {
          s0 += add_c[0] - sub_c[0];
          s1 += add_c[1] - sub_c[1];
          s2 += add_c[2] - sub_c[2];
          s3 += add_c[3] - sub_c[3];
          sub_c += 4;
          add_c += 4;
          *dst++ = (s0 / area) | ((s1 / area) << 8) | ((s2 / area) << 16) | ((s3 / area) << 24);
        }
      }
    }

    for (unsigned int x = x_mid_end; x < width; ++x)
    {
      if (x > k)
      {
        const uint32_t* sub_c = col_sum + (size_t)(x - k - 1) * 4;
        s0 -= sub_c[0]; s1 -= sub_c[1]; s2 -= sub_c[2]; s3 -= sub_c[3];
      }
      if (x + k < width)
      {
        const uint32_t* add_c = col_sum + (size_t)(x + k) * 4;
        s0 += add_c[0]; s1 += add_c[1]; s2 += add_c[2]; s3 += add_c[3];
      }
      const unsigned int x0 = (x > k) ? (x - k) : 0;
      const unsigned int x1 = (x + k + 1 < width) ? (x + k + 1) : width;
      *dst++ = pack_avg_div(s0, s1, s2, s3, (x1 - x0) * y_span);
    }
  }
}

int f0r_init()
{
  return 1;
}

void f0r_deinit()
{ /* no initialization required */ }

void f0r_get_plugin_info(f0r_plugin_info_t* squareblur_info)
{
  squareblur_info->name = "Squareblur";
  squareblur_info->author = "Drone";
  squareblur_info->plugin_type = F0R_PLUGIN_TYPE_FILTER;
  squareblur_info->color_model = F0R_COLOR_MODEL_RGBA8888;
  squareblur_info->frei0r_version = FREI0R_MAJOR_VERSION;
  squareblur_info->major_version = 0; 
  squareblur_info->minor_version = 1; 
  squareblur_info->num_params =  1; 
  squareblur_info->explanation = "Variable-size square blur";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
  switch(param_index)
  {
  case 0:
    info->name = "Kernel size";
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "The size of the kernel, as a proportion to its coverage of the image";
    break;
  }
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
  squareblur_fast_instance_t* inst =
    (squareblur_fast_instance_t*)malloc(sizeof(squareblur_fast_instance_t));
  inst->width = width;
  inst->height = height;
  inst->kernel = 0.0;
  inst->col_sum = (uint32_t*)malloc((size_t)width * SIZE_RGBA * sizeof(uint32_t));
  return (f0r_instance_t)inst;
}

void f0r_destruct(f0r_instance_t instance)
{
  squareblur_fast_instance_t* inst = (squareblur_fast_instance_t*)instance;
  free(inst->col_sum);
  free(instance);
}

void f0r_set_param_value(f0r_instance_t instance, 
                         f0r_param_t param, int param_index)
{
  assert(instance);
  squareblur_fast_instance_t* inst = (squareblur_fast_instance_t*)instance;
  switch(param_index)
  {
  case 0:
    inst->kernel = *((double*)param);
    break;
  }
}

void f0r_get_param_value(f0r_instance_t instance,
                         f0r_param_t param, int param_index)
{
  assert(instance);
  squareblur_fast_instance_t* inst = (squareblur_fast_instance_t*)instance;
  switch(param_index)
  {
  case 0:
    *((double*)param) = inst->kernel;
    break;
  }
}

void f0r_update(f0r_instance_t instance, double time,
                const uint32_t* inframe, uint32_t* outframe)
{
  (void)time;
  assert(instance);
  squareblur_fast_instance_t* inst = (squareblur_fast_instance_t*)instance;

  const unsigned int width = inst->width;
  const unsigned int height = inst->height;
  const unsigned int max_dim = (width > height) ? width : height;
  const unsigned int k = (unsigned int)(inst->kernel * max_dim / 2.0);

  if (k == 0 || width == 0 || height == 0)
  {
    memcpy(outframe, inframe, (size_t)width * height * sizeof(uint32_t));
    return;
  }

  const unsigned int max_y_span = (2 * k + 1 < height) ? (2 * k + 1) : height;
  if (max_y_span <= 257u)
  {
    blur_u16_colsum(width, height, k, (const unsigned char*)inframe, outframe,
                    (uint16_t*)inst->col_sum);
  }
  else
  {
    blur_u32_colsum(width, height, k, (const unsigned char*)inframe, outframe,
                    inst->col_sum);
  }
}
