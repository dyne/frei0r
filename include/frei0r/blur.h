/* blur.h
 * Copyright (C) 2004--2005 Mathieu Guindon
 *                          Julien Keable
 *                          Jean-Sebastien Senecal (js@drone.ws)
 *
 * Modified by Richard Spindler (richard.spindler AT gmail.com) for blurring in
 * the mask0mate Frei0r plugin.
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
#include <string.h>

#include "frei0r.h"

#define SIZE_RGBA 4

static inline int MAX(int a, int b)
{
  return (a > b ? a : b);
}

static inline int MIN(int a, int b)
{
  return (a < b ? a : b);
}

typedef struct squareblur_instance
{
  unsigned int width;
  unsigned int height;
  double kernel; /* the kernel size, as a percentage of the biggest of width and height */
  uint32_t *mem; /* memory accumulation matrix of uint32_t (size = acc_width*acc_height*SIZE_RGBA) */
} squareblur_instance_t;

/* Updates the summed area table in a single pass per row. */
static void update_summed_area_table(squareblur_instance_t *inst, const uint32_t *src)
{
  const unsigned char *restrict iter_data = (const unsigned char *)src;
  uint32_t *restrict iter_mem = inst->mem;
  unsigned int x, y;

  unsigned int width = inst->width + 1;
  unsigned int height = inst->height + 1;
  size_t row_width = (size_t)SIZE_RGBA * width;

  /* Process first row (all zeros). */
  memset(iter_mem, 0, row_width * sizeof(uint32_t));
  iter_mem += row_width;

  if (height >= 2)
  {
    /* Process second row. */
    uint32_t a0 = 0, a1 = 0, a2 = 0, a3 = 0;
    iter_mem[0] = 0; iter_mem[1] = 0; iter_mem[2] = 0; iter_mem[3] = 0;
    uint32_t *restrict dst_row = iter_mem + SIZE_RGBA;
    for (x = 1; x < width; ++x)
    {
      a0 += iter_data[0];
      a1 += iter_data[1];
      a2 += iter_data[2];
      a3 += iter_data[3];
      dst_row[0] = a0;
      dst_row[1] = a1;
      dst_row[2] = a2;
      dst_row[3] = a3;
      dst_row += SIZE_RGBA;
      iter_data += SIZE_RGBA;
    }
    iter_mem += row_width;

    /* Process remaining rows in a single fused pass (no separate memcpy). */
    for (y = 2; y < height; ++y)
    {
      const uint32_t *restrict prev_row = iter_mem - row_width + SIZE_RGBA;
      uint32_t *restrict cur_row = iter_mem + SIZE_RGBA;
      a0 = 0; a1 = 0; a2 = 0; a3 = 0;
      iter_mem[0] = 0; iter_mem[1] = 0; iter_mem[2] = 0; iter_mem[3] = 0;
      for (x = 1; x < width; ++x)
      {
        a0 += iter_data[0];
        a1 += iter_data[1];
        a2 += iter_data[2];
        a3 += iter_data[3];
        cur_row[0] = prev_row[0] + a0;
        cur_row[1] = prev_row[1] + a1;
        cur_row[2] = prev_row[2] + a2;
        cur_row[3] = prev_row[3] + a3;
        cur_row += SIZE_RGBA;
        prev_row += SIZE_RGBA;
        iter_data += SIZE_RGBA;
      }
      iter_mem += row_width;
    }
  }
}

static void blur_get_param_info(f0r_param_info_t* info, int param_index)
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

static f0r_instance_t blur_construct(unsigned int width, unsigned int height)
{
  squareblur_instance_t* inst =
    (squareblur_instance_t*)malloc(sizeof(squareblur_instance_t));
  size_t acc_width = (size_t)width + 1;
  size_t acc_height = (size_t)height + 1;
  inst->width = width;
  inst->height = height;
  inst->kernel = 0.0;
  /* allocate memory for the summed-area-table */
  inst->mem = (uint32_t*)malloc(acc_width * acc_height * SIZE_RGBA * sizeof(uint32_t));
  return (f0r_instance_t)inst;
}

static void blur_destruct(f0r_instance_t instance)
{
  squareblur_instance_t* inst =
    (squareblur_instance_t*)instance;
  free(inst->mem);
  free(instance);
}

static void blur_set_param_value(f0r_instance_t instance,
                         f0r_param_t param, int param_index)
{
  assert(instance);
  squareblur_instance_t* inst = (squareblur_instance_t*)instance;

  switch(param_index)
  {
  case 0:
    /* kernel size */
    inst->kernel = *((double*)param);
    break;
  }
}

static void blur_get_param_value(f0r_instance_t instance,
                         f0r_param_t param, int param_index)
{
  assert(instance);
  squareblur_instance_t* inst = (squareblur_instance_t*)instance;

  switch(param_index)
  {
  case 0:
    *((double*)param) = inst->kernel;
    break;
  }
}

/* Exact unsigned division n / area for all 0 <= n <= 255 * area and 1 <= area <= 11,000,000 */
static inline uint64_t blur_inv_area(unsigned int area)
{
  return ((1ULL << 55) + (uint64_t)area - 1ULL) / (uint64_t)area;
}

static inline void blur_box_pixel(unsigned char *restrict dst,
                                  const uint32_t *restrict row0,
                                  const uint32_t *restrict row1,
                                  unsigned int x0, unsigned int x1,
                                  unsigned int area, uint64_t inv, int use_inv)
{
  const uint32_t *p11 = row1 + (size_t)x1 * SIZE_RGBA;
  const uint32_t *p10 = row1 + (size_t)x0 * SIZE_RGBA;
  const uint32_t *p01 = row0 + (size_t)x1 * SIZE_RGBA;
  const uint32_t *p00 = row0 + (size_t)x0 * SIZE_RGBA;
  uint32_t s0 = p11[0] - p10[0] - p01[0] + p00[0];
  uint32_t s1 = p11[1] - p10[1] - p01[1] + p00[1];
  uint32_t s2 = p11[2] - p10[2] - p01[2] + p00[2];
  uint32_t s3 = p11[3] - p10[3] - p01[3] + p00[3];
  if (use_inv)
  {
    dst[0] = (unsigned char)(((uint64_t)s0 * inv) >> 55);
    dst[1] = (unsigned char)(((uint64_t)s1 * inv) >> 55);
    dst[2] = (unsigned char)(((uint64_t)s2 * inv) >> 55);
    dst[3] = (unsigned char)(((uint64_t)s3 * inv) >> 55);
  }
  else
  {
    dst[0] = (unsigned char)(s0 / area);
    dst[1] = (unsigned char)(s1 / area);
    dst[2] = (unsigned char)(s2 / area);
    dst[3] = (unsigned char)(s3 / area);
  }
}

static void blur_update(f0r_instance_t instance, double time,
                const uint32_t* inframe, uint32_t* outframe)
{
  assert(instance);
  (void)time;
  squareblur_instance_t* inst = (squareblur_instance_t*)instance;

  unsigned int width = inst->width;
  unsigned int height = inst->height;
  size_t acc_row_stride = ((size_t)width + 1) * SIZE_RGBA;
  unsigned int max = MAX(width, height);
  unsigned int kernel_size = (unsigned int)(inst->kernel * max / 2.0);

  if (kernel_size == 0)
  {
    /* No blur, just copy image. */
    memcpy(outframe, inframe, (size_t)width * height * sizeof(uint32_t));
    return;
  }

  assert(inst->mem);
  unsigned char *restrict dst = (unsigned char *)outframe;
  const uint32_t *restrict mem = inst->mem;

  /* Compute the summed area table. */
  update_summed_area_table(inst, inframe);

  unsigned int x_start = kernel_size < width ? kernel_size : width;
  unsigned int x_end = (width > kernel_size + 1) ? (width - kernel_size - 1) : 0;
  if (x_end < x_start) x_end = x_start;
  unsigned int interior_dx = 2 * kernel_size + 1;

  for (unsigned int y = 0; y < height; ++y)
  {
    unsigned int y0 = (y > kernel_size) ? (y - kernel_size) : 0;
    unsigned int y1 = (y + kernel_size + 1 < height) ? (y + kernel_size + 1) : height;
    unsigned int dy = y1 - y0;
    const uint32_t *restrict row0 = mem + (size_t)y0 * acc_row_stride;
    const uint32_t *restrict row1 = mem + (size_t)y1 * acc_row_stride;

    /* Left border */
    for (unsigned int x = 0; x < x_start; ++x)
    {
      unsigned int x0 = 0;
      unsigned int x1 = (x + kernel_size + 1 < width) ? (x + kernel_size + 1) : width;
      unsigned int area = (x1 - x0) * dy;
      int use_inv = (area <= 11000000u);
      uint64_t inv = use_inv ? blur_inv_area(area) : 0;
      blur_box_pixel(dst, row0, row1, x0, x1, area, inv, use_inv);
      dst += SIZE_RGBA;
    }

    /* Interior: constant area across entire span */
    if (x_start < x_end)
    {
      unsigned int area = interior_dx * dy;
      if (area <= 11000000u)
      {
        uint64_t inv = blur_inv_area(area);
        const uint32_t *restrict p11 = row1 + (size_t)(x_start + kernel_size + 1) * SIZE_RGBA;
        const uint32_t *restrict p10 = row1 + (size_t)(x_start - kernel_size) * SIZE_RGBA;
        const uint32_t *restrict p01 = row0 + (size_t)(x_start + kernel_size + 1) * SIZE_RGBA;
        const uint32_t *restrict p00 = row0 + (size_t)(x_start - kernel_size) * SIZE_RGBA;
        size_t count = (size_t)(x_end - x_start) * SIZE_RGBA;
        for (size_t i = 0; i < count; ++i)
        {
          uint32_t s = p11[i] - p10[i] - p01[i] + p00[i];
          dst[i] = (unsigned char)(((uint64_t)s * inv) >> 55);
        }
        dst += count;
      }
      else
      {
        for (unsigned int x = x_start; x < x_end; ++x)
        {
          blur_box_pixel(dst, row0, row1, x - kernel_size, x + kernel_size + 1, area, 0, 0);
          dst += SIZE_RGBA;
        }
      }
    }

    /* Right border */
    for (unsigned int x = x_end; x < width; ++x)
    {
      unsigned int x0 = (x > kernel_size) ? (x - kernel_size) : 0;
      unsigned int x1 = width;
      unsigned int area = (x1 - x0) * dy;
      int use_inv = (area <= 11000000u);
      uint64_t inv = use_inv ? blur_inv_area(area) : 0;
      blur_box_pixel(dst, row0, row1, x0, x1, area, inv, use_inv);
      dst += SIZE_RGBA;
    }
  }
}
