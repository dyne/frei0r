/* brightness.c
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
#include "frei0r_math.h"

typedef struct brightness_instance
{
  unsigned int  width;
  unsigned int  height;
  int           brightness; /* the brightness [-256, 256] */
  unsigned char lut[256];   /* look-up table */
} brightness_instance_t;

/* Updates the look-up-table. */
void update_lut(brightness_instance_t *inst)
{
  int            i;
  unsigned char *lut        = inst->lut;
  int            brightness = inst->brightness;

  if (brightness < 0)
  {
    for (i = 0; i < 256; ++i)
    {
      lut[i] = CLAMP0255((i * (256 + brightness)) >> 8);
    }
  }
  else
  {
    for (i = 0; i < 256; ++i)
    {
      lut[i] = CLAMP0255(i + (((256 - i) * brightness) >> 8));
    }
  }
}

int f0r_init()
{
  return (1);
}

void f0r_deinit()
{ /* no initialization required */
}

void f0r_get_plugin_info(f0r_plugin_info_t *brightness_info)
{
  brightness_info->name           = "Brightness";
  brightness_info->author         = "Jean-Sebastien Senecal";
  brightness_info->plugin_type    = F0R_PLUGIN_TYPE_FILTER;
  brightness_info->color_model    = F0R_COLOR_MODEL_RGBA8888;
  brightness_info->frei0r_version = FREI0R_MAJOR_VERSION;
  brightness_info->major_version  = 0;
  brightness_info->minor_version  = 2;
  brightness_info->num_params     = 1;
  brightness_info->explanation    = "Adjusts the brightness of a source image";
}

void f0r_get_param_info(f0r_param_info_t *info, int param_index)
{
  switch (param_index)
  {
  case 0:
    info->name        = "Brightness";
    info->type        = F0R_PARAM_DOUBLE;
    info->explanation = "The brightness value";
    break;
  }
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
  brightness_instance_t *inst = (brightness_instance_t *)calloc(1, sizeof(*inst));

  inst->width = width; inst->height = height;
  /* init look-up-table */
  update_lut(inst);
  return ((f0r_instance_t)inst);
}

void f0r_destruct(f0r_instance_t instance)
{
  free(instance);
}

void f0r_set_param_value(f0r_instance_t instance,
                         f0r_param_t param, int param_index)
{
  assert(instance);
  brightness_instance_t *inst = (brightness_instance_t *)instance;

  switch (param_index)
  {
  int val;

  case 0:
    /* constrast */
    val = (int)((*((double *)param) - 0.5) * 512.0); /* remap to [-256, 256] */
    if (val != inst->brightness)
    {
      inst->brightness = val;
      update_lut(inst);
    }
    break;
  }
}

void f0r_get_param_value(f0r_instance_t instance,
                         f0r_param_t param, int param_index)
{
  assert(instance);
  brightness_instance_t *inst = (brightness_instance_t *)instance;

  switch (param_index)
  {
  case 0:
    *((double *)param) = (double)((inst->brightness + 256.0) / 512.0);
    break;
  }
}

void f0r_update(f0r_instance_t instance, double time,
                const uint32_t *inframe, uint32_t *outframe)
{
  assert(instance);
  brightness_instance_t *inst = (brightness_instance_t *)instance;
  unsigned int           len  = inst->width * inst->height;

  unsigned char *      lut = inst->lut;
  unsigned char *      dst = (unsigned char *)outframe;
  const unsigned char *src = (unsigned char *)inframe;
  while (len--)
  {
    *dst++ = lut[*src++];
    *dst++ = lut[*src++];
    *dst++ = lut[*src++];
    *dst++ = *src++;// copy alpha
  }
}
