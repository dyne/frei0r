/* RGB.c
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

#include <stdlib.h>
#include <assert.h>

#include "frei0r.h"

typedef struct RGB_instance
{
  unsigned int width;
  unsigned int height;
} RGB_instance_t;

int f0r_init()
{
  return (1);
}

void f0r_deinit()
{ /* no initialization required */
}

void f0r_get_plugin_info(f0r_plugin_info_t *RGBInfo)
{
  RGBInfo->name           = "RGB";
  RGBInfo->author         = "Richard Spindler";
  RGBInfo->plugin_type    = F0R_PLUGIN_TYPE_MIXER3;
  RGBInfo->color_model    = F0R_COLOR_MODEL_RGBA8888;
  RGBInfo->frei0r_version = FREI0R_MAJOR_VERSION;
  RGBInfo->major_version  = 0;
  RGBInfo->minor_version  = 9;
  RGBInfo->num_params     = 0;
  RGBInfo->explanation    = "Averages each Input and uses each as R, G or B channel of the Output";
}

void f0r_get_param_info(f0r_param_info_t *info, int param_index)
{
  /* no params */
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
  RGB_instance_t *inst = (RGB_instance_t *)calloc(1, sizeof(*inst));

  inst->width = width; inst->height = height;
  return ((f0r_instance_t)inst);
}

void f0r_destruct(f0r_instance_t instance)
{
  free(instance);
}

void f0r_set_param_value(f0r_instance_t instance,
                         f0r_param_t param, int param_index)
{ /* no params */
}

void f0r_get_param_value(f0r_instance_t instance,
                         f0r_param_t param, int param_index)
{ /* no params */
}

void f0r_update2(f0r_instance_t instance,
                 double time,
                 const uint32_t *inframe1,
                 const uint32_t *inframe2,
                 const uint32_t *inframe3,
                 uint32_t *outframe)
{
  assert(instance);
  RGB_instance_t *inst = (RGB_instance_t *)instance;
  unsigned int    w    = inst->width;
  unsigned int    h    = inst->height;
  unsigned int    x, y;

  uint32_t *      dst  = outframe;
  const uint32_t *src1 = inframe1;
  const uint32_t *src2 = inframe2;
  const uint32_t *src3 = inframe3;
  for (y = 0; y < h; ++y)
  {
    for (x = 0; x < w; ++x)
    {
      int            tmpbw1;
      int            tmpbw2;
      int            tmpbw3;
      unsigned char *tmpc1 = (unsigned char *)src1;
      unsigned char *tmpc2 = (unsigned char *)src2;
      unsigned char *tmpc3 = (unsigned char *)src3;
      tmpbw1 = (tmpc1[0] + tmpc1[1] + tmpc1[2]) / 3;
      tmpbw2 = (tmpc2[0] + tmpc2[1] + tmpc2[2]) / 3;
      tmpbw3 = (tmpc3[0] + tmpc3[1] + tmpc3[2]) / 3;

      *dst++ = (0xff000000) | (tmpbw3 << 16) | (tmpbw2 << 8) | (tmpbw1);

      src1++;
      src2++;
      src3++;
    }
  }
}
