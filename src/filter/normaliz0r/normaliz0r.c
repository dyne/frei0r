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

/*
 * Normalize video (aka histogram stretching, contrast stretching).
 * See: https://en.wikipedia.org/wiki/Normalization_(image_processing)
 *
 * For each channel of each frame, the filter computes the input range and maps
 * it linearly to the user-specified output range.  The output range defaults
 * to the full dynamic range from pure black to pure white.
 *
 * The filter can apply temporal smoothing to the input range to reduce rapid
 * changes in brightness (flickering) caused by small dark or bright objects
 * entering or leaving the scene.  This effect is similar to the auto-exposure
 * on a video camera.
 *
 * The filter can normalize the R,G,B channels independently, which may cause
 * color shifting, or link them together as a single channel, which prevents
 * color shifting.  More precisely, linked normalization preserves hues (as
 * defined in HSV/HSL color spaces) while independent normalization does not.
 * Independent normalization can be used to remove color casts, such as the
 * blue cast from underwater video, restoring more natural colors.  The filter
 * can also combine independent and linked normalization in any ratio.
 *
 * Finally the overall strength of the filter can be adjusted, from no effect
 * to full normalization.
 *
 * The 5 user parameters are:
 *   BlackPt,   Colors which define the output range.  The minimum input value
 *   WhitePt    is mapped to the BlackPt.  The maximum input value is mapped to
 *              the WhitePt.  The defaults are black and white respectively.
 *              Specifying white for BlackPt and black for WhitePt will give
 *              color-inverted, normalized video.  Shades of grey can be used
 *              to reduce the dynamic range (contrast). Specifying saturated
 *              colors here can create some interesting effects.
 *
 *   Smoothing  The amount of temporal smoothing.  The parameter is converted
 *              into a number of frames N in the range [1,MAX_HISTORY_LEN], and
 *              the minimum and maximum input value of each channel are then
 *              smoothed using a rolling average over the last N frames.
 *              Defaults to 0.0 (no temporal smoothing).
 *
 *   Independence
 *              Controls the ratio of independent (color shifting) channel
 *              normalization to linked (color preserving) normalization.  0.0
 *              is fully linked, 1.0 is fully independent.  Defaults to fully
 *              independent.
 *
 *   Strength   Overall strength of the filter.  1.0 is full strength.  0.0 is
 *              a rather expensive no-op.  Values in between can give a gentle
 *              boost to low-contrast video without creating an artificial
 *              over-processed look.  The default is full strength.
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "frei0r.h"
#include "frei0r_math.h"

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
  assert(instance);
  normaliz0r_instance_t* inst = (normaliz0r_instance_t*)instance;
  int c;

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

#define INIT(c)     (min[c].in = max[c].in = in[c])
#define EXTEND(c)   (min[c].in = MIN(min[c].in, in[c])), \
                    (max[c].in = MAX(max[c].in, in[c]))
    INIT(0);
    INIT(1);
    INIT(2);
    in += 4;
    // Process (num_pixels - 1), as we used the first pixel to initialize
    unsigned int num_pixels = inst->num_pixels - 1;
    while (num_pixels--)
    {
      EXTEND(0);
      EXTEND(1);
      EXTEND(2);
      in += 4;
    }
  }

  // Next, for each channel, push min.in and max.in into their respective
  // histories, to determine the min.smoothed and max.smoothed for this frame.
  {
    int history_idx = inst->frame_num % inst->history_len;
    // Assume the history is not yet full; num_history_vals is the number of
    // frames received so far including the current frame.
    int num_history_vals = inst->frame_num + 1;
    if (inst->frame_num >= inst->history_len)
    {
      //The history is full; drop oldest value and cap num_history_vals.
      for (c = 0; c < 3; c++)
      {
        inst->min[c].history_sum -= inst->min[c].history[history_idx];
        inst->max[c].history_sum -= inst->max[c].history[history_idx];
      }
      num_history_vals = inst->history_len;
    }
    // For each extremum, update history_sum and calculate smoothed value as
    // the rolling average of the history entries.
    for (c = 0; c < 3; c++)
    {
      inst->min[c].history_sum += (inst->min[c].history[history_idx] = min[c].in);
      min[c].smoothed = (float)inst->min[c].history_sum / (float)num_history_vals;
      inst->max[c].history_sum += (inst->max[c].history[history_idx] = max[c].in);
      max[c].smoothed = (float)inst->max[c].history_sum / (float)num_history_vals;
    }
  }

  // Determine the input range for linked normalization.  This is simply the
  // minimum of the per-channel minimums, and the maximum of the per-channel
  // maximums.
  float rgb_min_smoothed = min[0].smoothed;
  float rgb_max_smoothed = max[0].smoothed;
  rgb_min_smoothed = MIN(rgb_min_smoothed, min[1].smoothed);
  rgb_max_smoothed = MAX(rgb_max_smoothed, max[1].smoothed);
  rgb_min_smoothed = MIN(rgb_min_smoothed, min[2].smoothed);
  rgb_max_smoothed = MAX(rgb_max_smoothed, max[2].smoothed);

  // Now, process each channel to determine the input and output range and
  // build the lookup tables.
  uint8_t lut[3][256];
  for (c = 0; c < 3; c++)
  {
    // Adjust the input range for this channel [min.smoothed,max.smoothed] by
    // mixing in the correct proportion of the linked normalization input range
    // [rgb_min_smoothed,rgb_max_smoothed].
    min[c].smoothed = (min[c].smoothed * inst->independence)
        + (rgb_min_smoothed * (1.0 - inst->independence));
    max[c].smoothed = (max[c].smoothed * inst->independence)
        + (rgb_max_smoothed * (1.0 - inst->independence));

    // Calculate the output range [min.out,max.out] as a ratio of the full-
    // strength output range [min.param,max.param] and the original input
    // range [min.in,max.in], based on the user-specified filter strength.
    min[c].out = (inst->min[c].param * inst->strength)
        + ((float)min[c].in * (1.0 - inst->strength));
    max[c].out = (inst->max[c].param * inst->strength)
        + ((float)max[c].in * (1.0 - inst->strength));

    // Now, build a lookup table which linearly maps the adjusted input range
    // [min.smoothed,max.smoothed] to the output range [min.out,max.out].
    // Perform the linear interpolation for each x:
    //     lut[x] = (int)(float(x - min.smoothed) * scale + max.out + 0.5)
    // where scale = (max.out - min.out) / (max.smoothed - min.smoothed)
    if (min[c].smoothed == max[c].smoothed)
    {
      // There is no dynamic range to expand.  No mapping for this channel.
      int in_val;
      for (in_val = min[c].in; in_val <= max[c].in; in_val++)
        lut[c][in_val] = min[c].out;
    }
    else
    {
      // We must set lookup values for all values in the original input range
      // [min.in,max.in].  Since the original input range may be larger than
      // [min.smoothed,max.smoothed], some output values may fall outside the
      // [0,255] dynamic range.  We need to CLAMP() them.
      float scale = (max[c].out - min[c].out) / (max[c].smoothed - min[c].smoothed);
      int in_val;
      for (in_val = min[c].in; in_val <= max[c].in; in_val++)
      {
        int out_val = ROUND((in_val - min[c].smoothed) * scale + min[c].out);
        lut[c][in_val] = CLAMP(out_val, 0, 255);
      }
    }
  }

  // Finally, process the pixels of the input frame using the lookup tables.
  // Copy alpha as-is.
  {
    const uint8_t *in = (const uint8_t*)inframe;
    uint8_t *out = (uint8_t*)outframe;

#define MAP(c)      (out[c] = lut[c][in[c]])

    unsigned int num_pixels = inst->num_pixels;
    while (num_pixels--)
    {
      MAP(0);
      MAP(1);
      MAP(2);
      out[3] = in[3];  // copy alpha
      in += 4;
      out += 4;
    }
  }

  inst->frame_num++;
}
