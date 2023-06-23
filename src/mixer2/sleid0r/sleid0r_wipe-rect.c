/*
 * Copyright (C) 2013 Vadim Druzhin <cdslow@mail.ru>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <frei0r.h>

typedef struct instance_s
{
  int    w;
  int    h;
  double pos;
  int    bw;
  int    bw_scale;
  int *  bwk;
} instance_t;

int f0r_init(void)
{
  return (1);
}

void f0r_deinit(void)
{
}

void f0r_get_plugin_info(f0r_plugin_info_t *info)
{
  info->name           = "wipe-rect";
  info->author         = "Vadim Druzhin";
  info->plugin_type    = F0R_PLUGIN_TYPE_MIXER2;
  info->color_model    = F0R_COLOR_MODEL_RGBA8888;
  info->frei0r_version = FREI0R_MAJOR_VERSION;
  info->major_version  = 0;
  info->minor_version  = 1;
  info->num_params     = 1;
  info->explanation    = "Rectangular wipe";
}

void f0r_get_param_info(f0r_param_info_t *info, int index)
{
  if (0 == index)
  {
    info->name        = "position";
    info->type        = F0R_PARAM_DOUBLE;
    info->explanation = "Rectangle size";
  }
}

f0r_instance_t f0r_construct(unsigned width, unsigned height)
{
  instance_t *inst;
  unsigned    bw;
  unsigned    i;

  if (height < width)
  {
    bw = height / 16;
  }
  else
  {
    bw = width / 16;
  }

  inst = malloc(sizeof(*inst) + bw * sizeof(*inst->bwk));
  if (NULL == inst)
  {
    return (NULL);
  }

  inst->w        = width;
  inst->h        = height;
  inst->pos      = 0.0;
  inst->bw       = bw;
  inst->bw_scale = bw * bw;
  inst->bwk      = (int *)(inst + 1);

  for (i = 0; i < bw; ++i)
  {
    if (i < bw / 2)
    {
      inst->bwk[i] = i * i * 2;
    }
    else
    {
      inst->bwk[i] = inst->bw_scale - (bw - i) * (bw - i) * 2;
    }
  }

  return (inst);
}

void f0r_destruct(f0r_instance_t inst)
{
  free(inst);
}

void f0r_set_param_value(f0r_instance_t instance, f0r_param_t param, int param_index)
{
  instance_t *inst = instance;

  if (0 == param_index)
  {
    inst->pos = *(f0r_param_double *)param;
  }
}

void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index)
{
  instance_t *inst = instance;

  if (0 == param_index)
  {
    *(f0r_param_double *)param = inst->pos;
  }
}

void f0r_update2(
  f0r_instance_t instance,
  double time,
  const uint32_t *inframe1,
  const uint32_t *inframe2,
  const uint32_t *inframe3,
  uint32_t *outframe
  )
{
  instance_t *inst = instance;
  int         y, x;
  int         off_h, off_v;
  uint8_t *   c1, *c2, *co;
  int         b, k;
  int         x0, x1;
  int         y0, y1;

  (void)time;     /* Unused */
  (void)inframe3; /* Unused */

  off_v = (int)((inst->w / 2 + inst->bw) * inst->pos + 0.5) - inst->bw;

  off_h = (int)((inst->h / 2 + inst->bw) * inst->pos + 0.5) - inst->bw;

  if (inst->h / 2 > off_h + inst->bw)
  {
    memcpy(outframe, inframe1,
           inst->w * (inst->h / 2 - off_h - inst->bw) * sizeof(*outframe));

    memcpy(outframe + inst->w * (inst->h / 2 + off_h + inst->bw),
           inframe1 + inst->w * (inst->h / 2 + off_h + inst->bw),
           inst->w * (inst->h / 2 - off_h - inst->bw) * sizeof(*outframe));
  }

  if (inst->w / 2 > off_v + inst->bw)
  {
    for (y = inst->h / 2 - off_h - inst->bw; y < inst->h / 2 + off_h + inst->bw; ++y)
    {
      if (y < 0 || y >= inst->h)
      {
        continue;
      }

      memcpy(outframe + inst->w * y, inframe1 + inst->w * y,
             (inst->w / 2 - off_v - inst->bw) * sizeof(*outframe));

      memcpy(outframe + inst->w * y + inst->w / 2 + off_v + inst->bw,
             inframe1 + inst->w * y + inst->w / 2 + off_v + inst->bw,
             (inst->w / 2 - off_v - inst->bw) * sizeof(*outframe));
    }
  }

  if (off_v > 0)
  {
    for (y = inst->h / 2 - off_h; y < inst->h / 2 + off_h; ++y)
    {
      memcpy(
        outframe + inst->w * y + inst->w / 2 - off_v,
        inframe2 + inst->w * y + inst->w / 2 - off_v,
        off_v * 2 * sizeof(*outframe)
        );
    }
  }

  for (b = 0; b < inst->bw; ++b)
  {
    k = inst->bwk[b];

    y = inst->h / 2 - off_h - inst->bw + b;
    if (y < 0)
    {
      continue;
    }

    x0 = inst->w / 2 - off_v - inst->bw + b;
    if (x0 < 0)
    {
      x0 = 0;
    }

    x1 = inst->w / 2 + off_v + inst->bw - b;
    if (x1 > inst->w)
    {
      x1 = inst->w;
    }

    c1 = (uint8_t *)(inframe1 + inst->w * y + x0);
    c2 = (uint8_t *)(inframe2 + inst->w * y + x0);
    co = (uint8_t *)(outframe + inst->w * y + x0);

    for (x = 0; x < (x1 - x0) * 4; ++x)
    {
      *co = (*c1 * (inst->bw_scale - k) + *c2 * k + inst->bw_scale / 2) / inst->bw_scale;
      ++c1;
      ++c2;
      ++co;
    }
  }

  for (b = 0; b < inst->bw; ++b)
  {
    k = inst->bwk[b];

    y = inst->h / 2 + off_h + b;
    if (y >= inst->h)
    {
      continue;
    }

    x0 = inst->w / 2 - off_v - b;
    if (x0 < 0)
    {
      x0 = 0;
    }

    x1 = inst->w / 2 + off_v + b + 1;
    if (x1 > inst->w)
    {
      x1 = inst->w;
    }

    c1 = (uint8_t *)(inframe1 + inst->w * y + x0);
    c2 = (uint8_t *)(inframe2 + inst->w * y + x0);
    co = (uint8_t *)(outframe + inst->w * y + x0);

    for (x = 0; x < (x1 - x0) * 4; ++x)
    {
      *co = (*c1 * k + *c2 * (inst->bw_scale - k) + inst->bw_scale / 2) / inst->bw_scale;
      ++c1;
      ++c2;
      ++co;
    }
  }

  for (b = 0; b < inst->bw * 4; ++b)
  {
    k = inst->bwk[b / 4];

    x0 = inst->w / 2 - off_v - inst->bw;
    if (x0 + b / 4 < 0)
    {
      continue;
    }

    y0 = inst->h / 2 - off_h - inst->bw + b / 4;
    if (y0 < 0)
    {
      y0 = 0;
    }

    y1 = inst->h / 2 + off_h + inst->bw - b / 4;
    if (y1 > inst->h)
    {
      y1 = inst->h;
    }

    c1 = (uint8_t *)(inframe1 + inst->w * y0 + x0) + b;
    c2 = (uint8_t *)(inframe2 + inst->w * y0 + x0) + b;
    co = (uint8_t *)(outframe + inst->w * y0 + x0) + b;

    for (y = 0; y < y1 - y0; ++y)
    {
      *co = (*c1 * (inst->bw_scale - k) + *c2 * k + inst->bw_scale / 2) / inst->bw_scale;
      c1 += inst->w * 4;
      c2 += inst->w * 4;
      co += inst->w * 4;
    }
  }

  for (b = 0; b < inst->bw * 4; ++b)
  {
    k = inst->bwk[b / 4];

    x0 = inst->w / 2 + off_v;
    if (x0 + b / 4 >= inst->w)
    {
      continue;
    }

    y0 = inst->h / 2 - off_h - b / 4;
    if (y0 < 0)
    {
      y0 = 0;
    }

    y1 = inst->h / 2 + off_h + b / 4 + 1;
    if (y1 > inst->h)
    {
      y1 = inst->h;
    }

    c1 = (uint8_t *)(inframe1 + inst->w * y0 + x0) + b;
    c2 = (uint8_t *)(inframe2 + inst->w * y0 + x0) + b;
    co = (uint8_t *)(outframe + inst->w * y0 + x0) + b;

    for (y = 0; y < y1 - y0; ++y)
    {
      *co = (*c1 * k + *c2 * (inst->bw_scale - k) + inst->bw_scale / 2) / inst->bw_scale;
      c1 += inst->w * 4;
      c2 += inst->w * 4;
      co += inst->w * 4;
    }
  }
}
