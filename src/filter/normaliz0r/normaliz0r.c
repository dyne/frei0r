/*
 * normaliz0r.c
 * Copyright (C) 2017 Chungzuwalla
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

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#if defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#include <emmintrin.h>
#define NORMALIZ0R_USE_SSE2 1
#endif

#include "frei0r.h"
#include "frei0r/math.h"

#define MAX_HISTORY_LEN     128

typedef struct
{
  int num_pixels;     // Number of pixels in a frame.
  int frame_num;      // Increments on each frame, starting from 0.

  // Per-extrema, per-channel fri0r params
  struct
  {
    uint8_t history[MAX_HISTORY_LEN]; // Param history, for temporal smoothing.
    uint16_t history_sum;       // Sum of history entries.
    float param;                // Target output value [0,255]
  } min[3], max[3];             // Min and max for each channel in {R,G,B}.

  // Per-instance frei0r params, each a float in the range [0,1].
  int history_len;      // Global max history length, which controls amount of
                        // temporal smoothing.  [1,MAX_HISTORY_LEN].
  float independence;   // Ratio of independent vs linked normalization [0,1].
  float strength;       // Mixing strength for the normalization [0,1].
} normaliz0r_instance_t;

int
f0r_init ()
{
  return 1;
}

void
f0r_deinit ()
{
}

void
f0r_get_plugin_info (f0r_plugin_info_t* info)
{
  info->name = "Normaliz0r";
  info->author = "Chungzuwalla (chungzuwalla\100rling.com)";
  info->plugin_type = F0R_PLUGIN_TYPE_FILTER;
  info->color_model = F0R_COLOR_MODEL_RGBA8888;
  info->frei0r_version = FREI0R_MAJOR_VERSION;
  info->major_version = 0;
  info->minor_version = 1;
  info->num_params = 5;
  info->explanation = "Normalize (aka histogram stretch, contrast stretch)";
}

static char *param_infos[] =
{
  "BlackPt",      "Output color to which darkest input color is mapped (default black)",
  "WhitePt",      "Output color to which brightest input color is mapped (default white)",
  "Smoothing",    "Amount of temporal smoothing of the input range, to reduce flicker (default 0.0)",
  "Independence", "Proportion of independent to linked channel normalization (default 1.0)",
  "Strength",     "Strength of filter, from no effect to full normalization (default 1.0)",
};

void
f0r_get_param_info (f0r_param_info_t* info, int param_index)
{
  info->name = param_infos[param_index * 2];
  info->type = (param_index <= 1) ? F0R_PARAM_COLOR : F0R_PARAM_DOUBLE;
  info->explanation = param_infos[param_index * 2 + 1];
}

f0r_instance_t
f0r_construct (unsigned int width, unsigned int height)
{
  normaliz0r_instance_t* inst = (normaliz0r_instance_t*)calloc(1, sizeof(*inst));
  int c;
  inst->num_pixels = width * height;
  inst->frame_num = 0;
  for (c = 0; c < 3; c++)
  {
    inst->min[c].history_sum = 0;
    inst->max[c].history_sum = 0;
    // Set default per-channel frei0r param values.
    inst->min[c].param = 0.0;   // black
    inst->max[c].param = 255.0; // white
  }
  // Set default per-instance frei0r param values.
  inst->history_len = 1;        // [1,MAX_HISTORY_LEN]; default is no smoothing
  inst->independence = 1.0;     // [0,1]; default is fully independent
  inst->strength = 1.0;         // [0,1]; default is full strength
  return (f0r_instance_t)inst;
}

void
f0r_destruct (f0r_instance_t instance)
{
  free (instance);
}

void
f0r_set_param_value (f0r_instance_t instance, f0r_param_t param,
                     int param_index)
{
  assert(instance);
  normaliz0r_instance_t* inst = (normaliz0r_instance_t*)instance;
  float val;

  switch (param_index)
    {
    case 0:
      inst->min[0].param = ((f0r_param_color_t*)param)->r * 255.0;
      inst->min[1].param = ((f0r_param_color_t*)param)->g * 255.0;
      inst->min[2].param = ((f0r_param_color_t*)param)->b * 255.0;
      break;
    case 1:
      inst->max[0].param = ((f0r_param_color_t*)param)->r * 255.0;
      inst->max[1].param = ((f0r_param_color_t*)param)->g * 255.0;
      inst->max[2].param = ((f0r_param_color_t*)param)->b * 255.0;
      break;
    case 2:
      val = (float)CLAMP(*((double* )param), 0.0, 1.0);
      // Map [0,1] <-> [1,MAX_HISTORY_LEN]
      inst->history_len = (int)(val * (MAX_HISTORY_LEN - 1)) + 1;
      break;
    case 3:
      val = (float)CLAMP(*((double* )param), 0.0, 1.0);
      inst->independence = val;
      break;
    case 4:
      val = (float)CLAMP(*((double* )param), 0.0, 1.0);
      inst->strength = val;
      break;
    }
}

void
f0r_get_param_value (f0r_instance_t instance, f0r_param_t param,
                     int param_index)
{
  assert(instance);
  normaliz0r_instance_t* inst = (normaliz0r_instance_t*)instance;
  switch (param_index)
    {
    case 0:
      ((f0r_param_color_t*)param)->r = inst->min[0].param / 255.0;
      ((f0r_param_color_t*)param)->g = inst->min[1].param / 255.0;
      ((f0r_param_color_t*)param)->b = inst->min[2].param / 255.0;
      break;
    case 1:
      ((f0r_param_color_t*)param)->r = inst->max[0].param / 255.0;
      ((f0r_param_color_t*)param)->g = inst->max[1].param / 255.0;
      ((f0r_param_color_t*)param)->b = inst->max[2].param / 255.0;
      break;
    case 2:
      // Map [0,1] <-> [1,MAX_HISTORY_LEN]
      *((double*)param) = (double)(inst->history_len - 1) / (MAX_HISTORY_LEN - 1);
      break;
    case 3:
      *((double*)param) = inst->independence;
      break;
    case 4:
      *((double*)param) = inst->strength;
      break;
    }
}

void
f0r_update (f0r_instance_t instance, double time, const uint32_t* inframe,
            uint32_t* outframe)
{
  (void)time;
  assert(instance);
  normaliz0r_instance_t* inst = (normaliz0r_instance_t*)instance;
  int c;
  const unsigned int num_pixels = (unsigned int)inst->num_pixels;

  // Per-extrema, per-channel local variables.
  struct
  {
    uint8_t in;                 // Original input byte value for this frame.
    float smoothed;             // Smoothed input value [0,255].
    float out;                  // Output value [0,255].
  } min[3], max[3];             // Min and max for each channel in {R,G,B}.

  // First, scan the input frame to find, for each channel, the minimum
  // (min.in) and maximum (max.in) values present in the channel.
  {
    const uint8_t *in = (const uint8_t*)inframe;
    uint8_t min0 = in[0], max0 = in[0];
    uint8_t min1 = in[1], max1 = in[1];
    uint8_t min2 = in[2], max2 = in[2];
    unsigned int i = 0;

#ifdef NORMALIZ0R_USE_SSE2
    if (num_pixels >= 8)
    {
      __m128i vmin0 = _mm_loadu_si128((const __m128i*)inframe);
      __m128i vmax0 = vmin0;
      __m128i vmin1 = _mm_loadu_si128((const __m128i*)(inframe + 4));
      __m128i vmax1 = vmin1;
      for (i = 8; i + 7 < num_pixels; i += 8)
      {
        __m128i v0 = _mm_loadu_si128((const __m128i*)(inframe + i));
        __m128i v1 = _mm_loadu_si128((const __m128i*)(inframe + i + 4));
        vmin0 = _mm_min_epu8(vmin0, v0);
        vmax0 = _mm_max_epu8(vmax0, v0);
        vmin1 = _mm_min_epu8(vmin1, v1);
        vmax1 = _mm_max_epu8(vmax1, v1);
      }
      __m128i vmin = _mm_min_epu8(vmin0, vmin1);
      __m128i vmax = _mm_max_epu8(vmax0, vmax1);
      /* Reduce 4 pixels (16 bytes) -> 2 pixels -> 1 pixel */
      vmin = _mm_min_epu8(vmin, _mm_srli_si128(vmin, 8));
      vmin = _mm_min_epu8(vmin, _mm_srli_si128(vmin, 4));
      vmax = _mm_max_epu8(vmax, _mm_srli_si128(vmax, 8));
      vmax = _mm_max_epu8(vmax, _mm_srli_si128(vmax, 4));
      uint32_t mn = (uint32_t)_mm_cvtsi128_si32(vmin);
      uint32_t mx = (uint32_t)_mm_cvtsi128_si32(vmax);
      min0 = (uint8_t)(mn & 0xffu);
      min1 = (uint8_t)((mn >> 8) & 0xffu);
      min2 = (uint8_t)((mn >> 16) & 0xffu);
      max0 = (uint8_t)(mx & 0xffu);
      max1 = (uint8_t)((mx >> 8) & 0xffu);
      max2 = (uint8_t)((mx >> 16) & 0xffu);
    }
#endif

    for (; i < num_pixels; i++)
    {
      const uint8_t *p = in + (size_t)i * 4;
      if (p[0] < min0) min0 = p[0];
      if (p[0] > max0) max0 = p[0];
      if (p[1] < min1) min1 = p[1];
      if (p[1] > max1) max1 = p[1];
      if (p[2] < min2) min2 = p[2];
      if (p[2] > max2) max2 = p[2];
    }
    min[0].in = min0; max[0].in = max0;
    min[1].in = min1; max[1].in = max1;
    min[2].in = min2; max[2].in = max2;
  }

  // Next, for each channel, push min.in and max.in into their respective
  // histories, to determine the min.smoothed and max.smoothed for this frame.
  {
    int history_idx = inst->frame_num % inst->history_len;
    int num_history_vals = inst->frame_num + 1;
    if (inst->frame_num >= inst->history_len)
    {
      for (c = 0; c < 3; c++)
      {
        inst->min[c].history_sum -= inst->min[c].history[history_idx];
        inst->max[c].history_sum -= inst->max[c].history[history_idx];
      }
      num_history_vals = inst->history_len;
    }
    for (c = 0; c < 3; c++)
    {
      inst->min[c].history_sum += (inst->min[c].history[history_idx] = min[c].in);
      min[c].smoothed = (float)inst->min[c].history_sum / (float)num_history_vals;
      inst->max[c].history_sum += (inst->max[c].history[history_idx] = max[c].in);
      max[c].smoothed = (float)inst->max[c].history_sum / (float)num_history_vals;
    }
  }

  float rgb_min_smoothed = min[0].smoothed;
  float rgb_max_smoothed = max[0].smoothed;
  rgb_min_smoothed = MIN(rgb_min_smoothed, min[1].smoothed);
  rgb_max_smoothed = MAX(rgb_max_smoothed, max[1].smoothed);
  rgb_min_smoothed = MIN(rgb_min_smoothed, min[2].smoothed);
  rgb_max_smoothed = MAX(rgb_max_smoothed, max[2].smoothed);

  // Now, process each channel to determine the input and output range and
  // build pre-shifted lookup tables.
  uint32_t lut[3][256];
  int is_identity = 1;
  for (c = 0; c < 3; c++)
  {
    const unsigned int shift = (unsigned int)c * 8u;
    min[c].smoothed = (min[c].smoothed * inst->independence)
        + (rgb_min_smoothed * (1.0 - inst->independence));
    max[c].smoothed = (max[c].smoothed * inst->independence)
        + (rgb_max_smoothed * (1.0 - inst->independence));

    min[c].out = (inst->min[c].param * inst->strength)
        + ((float)min[c].in * (1.0 - inst->strength));
    max[c].out = (inst->max[c].param * inst->strength)
        + ((float)max[c].in * (1.0 - inst->strength));

    if (min[c].smoothed == max[c].smoothed)
    {
      uint8_t ov = (uint8_t)min[c].out;
      for (int in_val = min[c].in; in_val <= max[c].in; in_val++)
      {
        if (ov != in_val) is_identity = 0;
        lut[c][in_val] = (uint32_t)ov << shift;
      }
    }
    else
    {
      float scale = (max[c].out - min[c].out) / (max[c].smoothed - min[c].smoothed);
      for (int in_val = min[c].in; in_val <= max[c].in; in_val++)
      {
        int out_val = ROUND((in_val - min[c].smoothed) * scale + min[c].out);
        uint8_t ov = (uint8_t)CLAMP(out_val, 0, 255);
        if (ov != in_val) is_identity = 0;
        lut[c][in_val] = (uint32_t)ov << shift;
      }
    }
  }

  if (is_identity)
  {
    if (outframe != inframe)
      memcpy(outframe, inframe, (size_t)num_pixels * sizeof(uint32_t));
  }
  else
  {
    const uint32_t *l0 = lut[0];
    const uint32_t *l1 = lut[1];
    const uint32_t *l2 = lut[2];
    unsigned int i = 0;
    for (; i + 3 < num_pixels; i += 4)
    {
      uint32_t p0 = inframe[i];
      uint32_t p1 = inframe[i + 1];
      uint32_t p2 = inframe[i + 2];
      uint32_t p3 = inframe[i + 3];

      outframe[i]     = l0[p0 & 0xffu] | l1[(p0 >> 8) & 0xffu] | l2[(p0 >> 16) & 0xffu] | (p0 & 0xff000000u);
      outframe[i + 1] = l0[p1 & 0xffu] | l1[(p1 >> 8) & 0xffu] | l2[(p1 >> 16) & 0xffu] | (p1 & 0xff000000u);
      outframe[i + 2] = l0[p2 & 0xffu] | l1[(p2 >> 8) & 0xffu] | l2[(p2 >> 16) & 0xffu] | (p2 & 0xff000000u);
      outframe[i + 3] = l0[p3 & 0xffu] | l1[(p3 >> 8) & 0xffu] | l2[(p3 >> 16) & 0xffu] | (p3 & 0xff000000u);
    }
    for (; i < num_pixels; i++)
    {
      uint32_t p = inframe[i];
      outframe[i] = l0[p & 0xffu] | l1[(p >> 8) & 0xffu] | l2[(p >> 16) & 0xffu] | (p & 0xff000000u);
    }
  }

  inst->frame_num++;
}
