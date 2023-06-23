/* mask0mate.c
 * Copyright (C) 2007 Richard Spindler (richard.spindler@gmail.com)
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

#include <math.h>
#include "frei0r.h"
#include <stdlib.h>
#include "blur.h"

typedef struct mask0mate_instance
{
  double          left, top, right, bottom;
  double          blur;
  int             invert;
  int             w, h;
  uint32_t *      mask;
  uint32_t *      mask_blurred;
  f0r_instance_t *blur_instance;
} mask0mate_instance_t;

void update_mask(mask0mate_instance_t *i)
{
  int l, r, t, b;

  l = (int)(i->left * i->w);
  r = (int)(i->w - (i->right * i->w));
  t = (int)(i->top * i->h);
  b = (int)(i->h - (i->bottom * i->h));

  if (l < 0)
  {
    l = 0;
  }
  if (r < 0)
  {
    r = 0;
  }
  if (t < 0)
  {
    t = 0;
  }
  if (b < 0)
  {
    b = 0;
  }

  if (l > i->w)
  {
    l = i->w;
  }
  if (r > i->w)
  {
    r = i->w;
  }
  if (t > i->h)
  {
    t = i->h;
  }
  if (b > i->h)
  {
    b = i->h;
  }

  if (l > r)
  {
    int c = l; l = r; r = c;
  }
  if (t > b)
  {
    int c = t; t = b; b = c;
  }

  int      len = i->w * i->h;
  int      j;
  uint32_t v;
  if (i->invert)
  {
    v = 0x00ffffff;
  }
  else
  {
    v = 0xffffffff;
  }
  for (j = 0; j < len; j++)
  {
    i->mask[j] = v;
  }
  if (!i->invert)
  {
    v = 0x00ffffff;
  }
  else
  {
    v = 0xffffffff;
  }
  int y, x;
  for (y = t; y < b; y++)
  {
    for (x = l; x < r; x++)
    {
      i->mask[y * i->w + x] = v;
    }
  }

  blur_set_param_value(i->blur_instance, &i->blur, 0);
  blur_update(i->blur_instance, 0.0, i->mask, i->mask_blurred);
}

int f0r_init()
{
  return (1);
}

void f0r_deinit()
{ /* empty */
}

void f0r_get_plugin_info(f0r_plugin_info_t *info)
{
  info->name           = "Mask0Mate";
  info->author         = "Richard Spindler";
  info->plugin_type    = F0R_PLUGIN_TYPE_FILTER;
  info->color_model    = F0R_COLOR_MODEL_RGBA8888;
  info->frei0r_version = FREI0R_MAJOR_VERSION;
  info->major_version  = 0;
  info->minor_version  = 1;
  info->num_params     = 6;
  info->explanation    = "Creates an square alpha-channel mask";
}

void f0r_get_param_info(f0r_param_info_t *info, int param_index)
{
  switch (param_index)
  {
  case 0:
    info->name        = "Left";
    info->type        = F0R_PARAM_DOUBLE;
    info->explanation = "";
    break;

  case 1:
    info->name        = "Right";
    info->type        = F0R_PARAM_DOUBLE;
    info->explanation = "";
    break;

  case 2:
    info->name        = "Top";
    info->type        = F0R_PARAM_DOUBLE;
    info->explanation = "";
    break;

  case 3:
    info->name        = "Bottom";
    info->type        = F0R_PARAM_DOUBLE;
    info->explanation = "";
    break;

  case 4:
    info->name        = "Invert";
    info->type        = F0R_PARAM_BOOL;
    info->explanation = "Invert the mask, creates a hole in the frame.";
    break;

  case 5:
    info->name        = "Blur";
    info->type        = F0R_PARAM_DOUBLE;
    info->explanation = "Blur the outline of the mask";
    break;
  }
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
  mask0mate_instance_t *inst = (mask0mate_instance_t *)calloc(1, sizeof(*inst));

  inst->w             = width;
  inst->h             = height;
  inst->left          = 0.2;
  inst->right         = 0.2;
  inst->top           = 0.2;
  inst->bottom        = 0.2;
  inst->mask          = (uint32_t *)malloc(width * height * sizeof(uint32_t));
  inst->mask_blurred  = (uint32_t *)malloc(width * height * sizeof(uint32_t));
  inst->blur_instance = (f0r_instance_t *)blur_construct(width, height);
  update_mask(inst);
  return ((f0r_instance_t)inst);
}

void f0r_destruct(f0r_instance_t instance)
{
  mask0mate_instance_t *inst = (mask0mate_instance_t *)instance;

  blur_destruct(inst->blur_instance);
  free(inst->mask);
  free(inst->mask_blurred);
  free(instance);
}

void f0r_set_param_value(f0r_instance_t instance,
                         f0r_param_t param, int param_index)
{
  mask0mate_instance_t *inst = (mask0mate_instance_t *)instance;

  switch (param_index)
  {
  case 0:
    inst->left = *((double *)param);
    break;

  case 1:
    inst->right = *((double *)param);
    break;

  case 2:
    inst->top = *((double *)param);
    break;

  case 3:
    inst->bottom = *((double *)param);
    break;

  case 4:
    if (*((double *)param) < 0.5)
    {
      inst->invert = 0;
    }
    else
    {
      inst->invert = 1;
    }
    break;

  case 5:
    inst->blur = *((double *)param);
    break;
  }
  update_mask(inst);
}

void f0r_get_param_value(f0r_instance_t instance,
                         f0r_param_t param, int param_index)
{
  mask0mate_instance_t *inst = (mask0mate_instance_t *)instance;

  switch (param_index)
  {
  case 0:
    *((double *)param) = inst->left;
    break;

  case 1:
    *((double *)param) = inst->right;
    break;

  case 2:
    *((double *)param) = inst->top;
    break;

  case 3:
    *((double *)param) = inst->bottom;
    break;

  case 4:
    *((double *)param) = (double)inst->invert;
    break;

  case 5:
    *((double *)param) = inst->blur;
    break;
  }
}

void f0r_update(f0r_instance_t instance, double time,
                const uint32_t *inframe, uint32_t *outframe)
{
  mask0mate_instance_t *inst = (mask0mate_instance_t *)instance;

  uint32_t *      dst   = outframe;
  const uint32_t *src   = inframe;
  const uint32_t *alpha = inst->mask_blurred;

  int len = inst->w * inst->h;

  int i;

  for (i = 0; i < len; i++)
  {
    *dst = *src & (*alpha | 0x00ffffff);
    dst++;
    src++;
    alpha++;
  }
}
