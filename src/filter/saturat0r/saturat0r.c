/* saturat0r.c
 * Copyright (C) 2004 Jean-Sebastien Senecal (js@drone.ws)
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
#include "frei0r/math.h"

#define MAX_SATURATION 8.0

typedef struct saturat0r_instance
{
  unsigned int width;
  unsigned int height;
  double saturation; /* the saturation value [0, 1] */
} saturat0r_instance_t;

int f0r_init()
{
  return 1;
}

void f0r_deinit()
{ /* no initialization required */ }

void f0r_get_plugin_info(f0r_plugin_info_t* saturat0r_info)
{
  saturat0r_info->name = "Saturat0r";
  saturat0r_info->author = "Jean-Sebastien Senecal";
  saturat0r_info->plugin_type = F0R_PLUGIN_TYPE_FILTER;
  saturat0r_info->color_model = F0R_COLOR_MODEL_RGBA8888;
  saturat0r_info->frei0r_version = FREI0R_MAJOR_VERSION;
  saturat0r_info->major_version = 0; 
  saturat0r_info->minor_version = 3; 
  saturat0r_info->num_params =  1; 
  saturat0r_info->explanation = "Adjusts the saturation of a source image";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
  switch(param_index)
  {
  case 0:
    info->name = "Saturation";
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "The saturation value";
    break;
  }
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
  saturat0r_instance_t* inst = (saturat0r_instance_t*)calloc(1, sizeof(*inst));
  inst->width = width; inst->height = height;
  inst->saturation=1.0/MAX_SATURATION;
  return (f0r_instance_t)inst;
}

void f0r_destruct(f0r_instance_t instance)
{
  free(instance);
}

void f0r_set_param_value(f0r_instance_t instance, 
                         f0r_param_t param, int param_index)
{
  assert(instance);
  saturat0r_instance_t* inst = (saturat0r_instance_t*)instance;

  switch(param_index)
  {
  case 0:
    /* saturations */
    inst->saturation =  *((double*)param);
    break;
  }
}

void f0r_get_param_value(f0r_instance_t instance,
                         f0r_param_t param, int param_index)
{
  assert(instance);
  saturat0r_instance_t* inst = (saturat0r_instance_t*)instance;
  
  switch(param_index)
  {
  case 0:
    *((double*)param) = inst->saturation;
    break;
  }
}

void f0r_update(f0r_instance_t instance, double time,
                const uint32_t* inframe, uint32_t* outframe)
{
  assert(instance);
  (void)time;
  saturat0r_instance_t* inst = (saturat0r_instance_t*)instance;
  unsigned int len = inst->width * inst->height;
  double saturation = inst->saturation * MAX_SATURATION;

  const uint32_t *restrict in = inframe;
  uint32_t *restrict out = outframe;

  double one_minus_saturation = 1.0 - saturation;
  int bwgt = (int)(7471.0  * one_minus_saturation);
  int gwgt = (int)(38470.0 * one_minus_saturation);
  int rwgt = (int)(19595.0 * one_minus_saturation);

  /* Verify in 256 iterations whether 20-bit fixed-point matches (int)(c * saturation)
     bit-for-bit for all 8-bit inputs. If so, run a pure vectorizable integer loop. */
  if (saturation >= 0.0 && saturation <= 8.0)
  {
    double scaled = saturation * 1048576.0;
    uint32_t sat_fp = (uint32_t)scaled;
    if ((double)sat_fp < scaled) sat_fp++;
    int exact_fp = 1;
    for (uint32_t c = 0; c < 256; ++c)
    {
      if ((int)((c * sat_fp) >> 20) != (int)(c * saturation))
      {
        exact_fp = 0;
        break;
      }
    }

    if (exact_fp)
    {
      if (saturation <= 1.0)
      {
        for (unsigned int i = 0; i < len; ++i)
        {
          uint32_t px = in[i];
          int b = (int)(px & 0xFFu);
          int g = (int)((px >> 8) & 0xFFu);
          int r = (int)((px >> 16) & 0xFFu);
          int bw = (b * bwgt + g * gwgt + r * rwgt) >> 16;

          uint32_t nb = (uint32_t)(unsigned char)(bw + (int)(((uint32_t)b * sat_fp) >> 20));
          uint32_t ng = (uint32_t)(unsigned char)(bw + (int)(((uint32_t)g * sat_fp) >> 20));
          uint32_t nr = (uint32_t)(unsigned char)(bw + (int)(((uint32_t)r * sat_fp) >> 20));
          out[i] = nb | (ng << 8) | (nr << 16) | (px & 0xFF000000u);
        }
      }
      else
      {
        for (unsigned int i = 0; i < len; ++i)
        {
          uint32_t px = in[i];
          int b = (int)(px & 0xFFu);
          int g = (int)((px >> 8) & 0xFFu);
          int r = (int)((px >> 16) & 0xFFu);
          int bw = (b * bwgt + g * gwgt + r * rwgt) >> 16;

          uint32_t nb = CLAMP0255(bw + (int)(((uint32_t)b * sat_fp) >> 20));
          uint32_t ng = CLAMP0255(bw + (int)(((uint32_t)g * sat_fp) >> 20));
          uint32_t nr = CLAMP0255(bw + (int)(((uint32_t)r * sat_fp) >> 20));
          out[i] = nb | (ng << 8) | (nr << 16) | (px & 0xFF000000u);
        }
      }
      return;
    }
  }

  unsigned char *restrict dst = (unsigned char*)outframe;
  const unsigned char *restrict src = (const unsigned char*)inframe;
  if (0 <= saturation && saturation <= 1)
  {
    for (unsigned int i = 0; i < len; ++i)
    {
      int b = src[0], g = src[1], r = src[2];
      int bw = (b * bwgt + g * gwgt + r * rwgt) >> 16;
      dst[0] = (unsigned char)(bw + b * saturation);
      dst[1] = (unsigned char)(bw + g * saturation);
      dst[2] = (unsigned char)(bw + r * saturation);
      dst[3] = src[3];
      src += 4; dst += 4;
    }
  }
  else
  {
    for (unsigned int i = 0; i < len; ++i)
    {
      int b = src[0], g = src[1], r = src[2];
      int bw = (b * bwgt + g * gwgt + r * rwgt) >> 16;
      dst[0] = CLAMP0255((int)(bw + b * saturation));
      dst[1] = CLAMP0255((int)(bw + g * saturation));
      dst[2] = CLAMP0255((int)(bw + r * saturation));
      dst[3] = src[3];
      src += 4; dst += 4;
    }
  }
}
