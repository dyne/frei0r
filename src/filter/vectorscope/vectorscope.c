/* vectorscope.c
 * Copyright (C) 2008 Albert Frisch (albert.frisch AT gmail.com)
 * Copyright (C) 2008 Richard Spindler (richard.spindler AT gmail.com)
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
#include <math.h>
#include <assert.h>
#include "frei0r.h"
#include <stdio.h>
#include <string.h>

#include <gavl/gavl.h>

#include "vectorscope_image.h"

#define OFFSET_R        0
#define OFFSET_G        8
#define OFFSET_B        16
#define OFFSET_A        24

#define SCOPE_WIDTH     256
#define SCOPE_HEIGHT    256

/* c99 seems to be extra clever, and removes the definition of M_PI,
 * this adds it again */
#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif

typedef struct
{
  double Y, Cb, Cr;
} YCbCr_t;

typedef struct
{
  double red, green, blue;
} rgb_t;

typedef struct vectorscope_instance
{
  int                  w, h;
  unsigned char *      scala;
  gavl_video_scaler_t *scope_scaler;
  gavl_video_frame_t * scope_frame_src;
  gavl_video_frame_t * scope_frame_dst;
  double               mix;
  double               overlay_sides;
} vectorscope_instance_t;

int f0r_init()
{
  return (1);
}

void f0r_deinit()
{ /* empty */
}

void f0r_get_plugin_info(f0r_plugin_info_t *info)
{
  info->name           = "Vectorscope";
  info->author         = "Albert Frisch";
  info->plugin_type    = F0R_PLUGIN_TYPE_FILTER;
  info->color_model    = F0R_COLOR_MODEL_RGBA8888;
  info->frei0r_version = FREI0R_MAJOR_VERSION;
  info->major_version  = 0;
  info->minor_version  = 2;
  info->num_params     = 2;
  info->explanation    = "Displays the vectorscope of the video-data";
}

void f0r_get_param_info(f0r_param_info_t *info, int param_index)
{
  switch (param_index)
  {
  case 0:
    info->name        = "mix";
    info->type        = F0R_PARAM_DOUBLE;
    info->explanation = "The amount of source image mixed into background of display";
    break;

  case 1:
    info->name        = "overlay sides";
    info->type        = F0R_PARAM_BOOL;
    info->explanation = "If false, the sides of image are shown without overlay";
    break;
  }
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
  vectorscope_instance_t *inst = (vectorscope_instance_t *)calloc(1, sizeof(*inst));

  inst->w = width;
  inst->h = height;
  if (width == 0 || height == 0)
  {
    free(inst);
    return (NULL);
  }

  inst->mix           = 0.0;
  inst->overlay_sides = 1.0;

  inst->scala = (unsigned char *)malloc(width * height * 4);

  gavl_video_scaler_t *video_scaler;
  gavl_video_frame_t * frame_src;
  gavl_video_frame_t * frame_dst;

  video_scaler          = gavl_video_scaler_create();
  frame_src             = gavl_video_frame_create(0);
  frame_dst             = gavl_video_frame_create(0);
  frame_dst->strides[0] = width * 4;
  frame_src->strides[0] = vectorscope_image.width * 4;

  gavl_video_options_t *options = gavl_video_scaler_get_options(video_scaler);
  gavl_video_format_t   format_src;
  gavl_video_format_t   format_dst;

  memset(&format_src, 0, sizeof(format_src));
  memset(&format_dst, 0, sizeof(format_dst));

  format_dst.frame_width    = inst->w;
  format_dst.frame_height   = inst->h;
  format_dst.image_width    = inst->w;
  format_dst.image_height   = inst->h;
  format_dst.pixel_width    = 1;
  format_dst.pixel_height   = 1;
  format_dst.pixelformat    = GAVL_RGBA_32;
  format_dst.interlace_mode = GAVL_INTERLACE_NONE;

  format_src.frame_width    = vectorscope_image.width;
  format_src.frame_height   = vectorscope_image.height;
  format_src.image_width    = vectorscope_image.width;
  format_src.image_height   = vectorscope_image.height;
  format_src.pixel_width    = 1;
  format_src.pixel_height   = 1;
  format_src.pixelformat    = GAVL_RGBA_32;
  format_src.interlace_mode = GAVL_INTERLACE_NONE;

  gavl_rectangle_f_t src_rect;
  gavl_rectangle_i_t dst_rect;

  src_rect.x = 0;
  src_rect.y = 0;
  src_rect.w = vectorscope_image.width;
  src_rect.h = vectorscope_image.height;

  float dst_x, dst_y, dst_w, dst_h;
  if ((float)inst->w / inst->h > (float)vectorscope_image.width / vectorscope_image.height)
  {
    dst_y = 0;
    dst_h = inst->h;
    dst_w = ((float)vectorscope_image.width / vectorscope_image.height) * inst->h;
    dst_x = (inst->w - dst_w) / 2.0;
  }
  else
  {
    dst_x = 0;
    dst_w = inst->w;
    dst_h = ((float)vectorscope_image.height / vectorscope_image.width) * inst->w;
    dst_y = (inst->h - dst_h) / 2.0;
  }
  dst_rect.x = (int)(dst_x);
  dst_rect.y = (int)(dst_y);
  dst_rect.w = (int)(dst_w);
  dst_rect.h = (int)(dst_h);

  gavl_video_options_set_rectangles(options, &src_rect, &dst_rect);
  gavl_video_scaler_init(video_scaler, &format_src, &format_dst);

  frame_src->planes[0] = (uint8_t *)vectorscope_image.pixel_data;
  frame_dst->planes[0] = (uint8_t *)inst->scala;

  /* Pad the source image to make the stride a multiple of 16. */
  gavl_video_frame_t *padded = gavl_video_frame_create(&format_src);
  gavl_video_frame_copy(&format_src, padded, frame_src);

  float transparent[4] = { 0.0, 0.0, 0.0, 0.0 };
  gavl_video_frame_fill(frame_dst, &format_dst, transparent);

  gavl_video_scaler_scale(video_scaler, padded, frame_dst);

  gavl_video_scaler_destroy(video_scaler);
  gavl_video_frame_null(frame_src);
  gavl_video_frame_destroy(frame_src);
  gavl_video_frame_null(frame_dst);
  gavl_video_frame_destroy(frame_dst);
  gavl_video_frame_destroy(padded);

  inst->scope_scaler                = gavl_video_scaler_create();
  inst->scope_frame_src             = gavl_video_frame_create(0);
  inst->scope_frame_dst             = gavl_video_frame_create(0);
  inst->scope_frame_src->strides[0] = SCOPE_WIDTH * 4;
  inst->scope_frame_dst->strides[0] = width * 4;
  options = gavl_video_scaler_get_options(inst->scope_scaler);

  format_src.frame_width  = SCOPE_WIDTH;
  format_src.frame_height = SCOPE_HEIGHT;
  format_src.image_width  = SCOPE_WIDTH;
  format_src.image_height = SCOPE_HEIGHT;
  format_src.pixel_width  = 1;
  format_src.pixel_height = 1;
  format_src.pixelformat  = GAVL_RGBA_32;
  format_dst.frame_width  = width;
  format_dst.frame_height = height;
  format_dst.image_width  = width;
  format_dst.image_height = height;
  format_dst.pixel_width  = 1;
  format_dst.pixel_height = 1;
  format_dst.pixelformat  = GAVL_RGBA_32;

  gavl_rectangle_f_set_all(&src_rect, &format_src);
  if (width > height)
  {
    dst_rect.x = (width - height) / 2;
    dst_rect.y = 0;
    dst_rect.w = height;
    dst_rect.h = height;
  }
  else
  {
    dst_rect.x = 0;
    dst_rect.y = (height - width) / 2;
    dst_rect.w = width;
    dst_rect.h = width;
  }
  gavl_video_options_set_rectangles(options, &src_rect, &dst_rect);
  gavl_video_scaler_init(inst->scope_scaler, &format_src, &format_dst);

  return ((f0r_instance_t)inst);
}

void f0r_destruct(f0r_instance_t instance)
{
  vectorscope_instance_t *inst = (vectorscope_instance_t *)instance;

  if (inst->w == 0 || inst->h == 0)
  {
    free(instance);
    return;
  }
  free(inst->scala);
  gavl_video_scaler_destroy(inst->scope_scaler);
  gavl_video_frame_null(inst->scope_frame_src);
  gavl_video_frame_destroy(inst->scope_frame_src);
  gavl_video_frame_null(inst->scope_frame_dst);
  gavl_video_frame_destroy(inst->scope_frame_dst);
  free(instance);
}

void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index)
{
  assert(instance);
  vectorscope_instance_t *inst = (vectorscope_instance_t *)instance;

  switch (param_index)
  {
  case 0:
    *((double *)param) = inst->mix;
    break;

  case 1:
    *((double *)param) = inst->overlay_sides;
    break;
  }
}

void f0r_set_param_value(f0r_instance_t instance, f0r_param_t param, int param_index)
{
  assert(instance);
  vectorscope_instance_t *inst = (vectorscope_instance_t *)instance;

  switch (param_index)
  {
  case 0:
    inst->mix = *((double *)param);
    break;

  case 1:
    inst->overlay_sides = *((double *)param);
    break;
  }
}

/* RGB to YCbCr range 0-255 */
YCbCr_t rgb_to_YCbCr(rgb_t rgb)
{
  YCbCr_t dest;

  dest.Y  = (float)((0.299 * (float)rgb.red + 0.587 * (float)rgb.green + 0.114 * (float)rgb.blue));
  dest.Cb = 128 + (float)((-0.16874 * (float)rgb.red - 0.33126 * (float)rgb.green + 0.5 * (float)rgb.blue));
  dest.Cr = 128 + (float)((0.5 * (float)rgb.red - 0.41869 * (float)rgb.green - 0.08131 * (float)rgb.blue));
  return (dest);
}

void f0r_update(f0r_instance_t instance, double time, const uint32_t *inframe, uint32_t *outframe)
{
  assert(instance);
  vectorscope_instance_t *inst = (vectorscope_instance_t *)instance;

  int    width     = inst->w;
  int    height    = inst->h;
  double mix       = inst->mix;
  int    len       = inst->w * inst->h;
  int    scope_len = SCOPE_WIDTH * SCOPE_HEIGHT;

  uint32_t *      dst = outframe;
  uint32_t *      dst_end;
  const uint32_t *src = inframe;
  const uint32_t *src_end;
  uint32_t *      scope = (uint32_t *)malloc(scope_len * 4);
  uint32_t *      scope_end;

  YCbCr_t  YCbCr;
  rgb_t    rgb;
  uint8_t *pixel;
  int      x, y;
  dst_end   = dst + len;
  src_end   = src + len;
  scope_end = scope + scope_len;

  if (inst->overlay_sides > 0.5)
  {
    while (dst < dst_end)
    {
      *(dst++) = 0xFF000000;
    }
  }
  else
  {
    while (dst < dst_end)
    {
      *(dst++) = *(src++);
    }
    src -= len;
  }

  dst = outframe;
  while (scope < scope_end)
  {
    *(scope++) = 0xFF000000;
  }
  scope -= scope_len;

  while (src < src_end)
  {
    rgb.red   = (((*src) & 0x000000FF) >> OFFSET_R);
    rgb.green = (((*src) & 0x0000FF00) >> OFFSET_G);
    rgb.blue  = (((*src) & 0x00FF0000) >> OFFSET_B);
    src++;
    YCbCr = rgb_to_YCbCr(rgb);
    x     = YCbCr.Cb;
    y     = 255 - YCbCr.Cr;
    if (x >= 0 && x < SCOPE_WIDTH && y >= 0 && y < SCOPE_HEIGHT)
    {
      pixel = (uint8_t *)&scope[x + SCOPE_WIDTH * y];
      if (pixel[0] < 255)
      {
        pixel[0]++;
        pixel[1]++;
        pixel[2]++;
      }
    }
  }

  inst->scope_frame_src->planes[0] = (uint8_t *)scope;
  inst->scope_frame_dst->planes[0] = (uint8_t *)dst;

  gavl_video_scaler_scale(inst->scope_scaler, inst->scope_frame_src, inst->scope_frame_dst);

  unsigned char *scala8, *dst8, *dst8_end, *src8;

  scala8   = inst->scala;
  src8     = (unsigned char *)inframe;
  dst8     = (unsigned char *)outframe;
  dst8_end = dst8 + (len * 4);
  if (mix > 0.001)          // to not lose performance for non-mixing users
  {
    while (dst8 < dst8_end)
    {
      dst8[0] = (((scala8[0] - dst8[0]) * 255 * scala8[3]) >> 16) + dst8[0];
      dst8[1] = (((scala8[1] - dst8[1]) * 255 * scala8[3]) >> 16) + dst8[1];
      dst8[2] = (((scala8[2] - dst8[2]) * 255 * scala8[3]) >> 16) + dst8[2];
      if (dst8[0] == 0)
      {
        dst8[0] = src8[0] * mix;
        dst8[1] = src8[1] * mix;
        dst8[2] = src8[2] * mix;
      }
      scala8 += 4;
      dst8   += 4;
      src8   += 4;
    }
  }
  else
  {
    while (dst8 < dst8_end)
    {
      dst8[0] = (((scala8[0] - dst8[0]) * 255 * scala8[3]) >> 16) + dst8[0];
      dst8[1] = (((scala8[1] - dst8[1]) * 255 * scala8[3]) >> 16) + dst8[1];
      dst8[2] = (((scala8[2] - dst8[2]) * 255 * scala8[3]) >> 16) + dst8[2];
      scala8 += 4;
      dst8   += 4;
    }
  }
  free(scope);
}
