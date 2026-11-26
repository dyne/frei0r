/* gamma.c
 * Copyright (C) 2004 Jean-Sebastien Senecal
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
#include "frei0r/math.h"

#define MAX_GAMMA 4.0

typedef struct gamma_instance
{
  unsigned int width;
  unsigned int height;
  double gamma; /* the gamma value [0, 1] */
  unsigned char lut[256]; /* look-up table */
  uint32_t lut_r[256];
  uint32_t lut_g[256];
  uint32_t lut_b[256];
} gamma_instance_t;

/* Updates the look-up-table. */
static void update_lut(gamma_instance_t *inst)
{
  int i;
  unsigned char *lut = inst->lut;
  double inv_gamma = 1.0 / (inst->gamma * MAX_GAMMA); /* set gamma in the range [0,MAX_GAMMA] and take its inverse */

  lut[0] = 0;
  for (i=1; i<256; ++i)
    lut[i] = CLAMP0255( ROUND(255.0 * pow( (double)i / 255.0, inv_gamma ) ) );
  for (i=0; i<256; ++i)
  {
    uint32_t v = lut[i];
    inst->lut_r[i] = v;
    inst->lut_g[i] = v << 8;
    inst->lut_b[i] = v << 16;
  }
}

int f0r_init()
{
  return 1;
}

void f0r_deinit()
{ /* no initialization required */ }

void f0r_get_plugin_info(f0r_plugin_info_t* gamma_info)
{
  gamma_info->name = "Gamma";
  gamma_info->author = "Jean-Sebastien Senecal";
  gamma_info->plugin_type = F0R_PLUGIN_TYPE_FILTER;
  gamma_info->color_model = F0R_COLOR_MODEL_RGBA8888;
  gamma_info->frei0r_version = FREI0R_MAJOR_VERSION;
  gamma_info->major_version = 0; 
  gamma_info->minor_version = 2; 
  gamma_info->num_params =  1; 
  gamma_info->explanation = "Adjusts the gamma value of a source image";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
  switch(param_index)
  {
  case 0:
    info->name = "Gamma";
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "The gamma value";
    break;
  }
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
  gamma_instance_t* inst = (gamma_instance_t*)calloc(1, sizeof(*inst));
  inst->width = width; inst->height = height;
  /* init look-up-table */
  inst->gamma = 1.0;
  update_lut(inst);
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
  gamma_instance_t* inst = (gamma_instance_t*)instance;

  switch(param_index)
  {
    double val;
  case 0:
    /* gamma */
    val = *((double*)param);
    if (val != inst->gamma)
    {
      inst->gamma = val;
      update_lut(inst);
    }
    break;
  }
}

void f0r_get_param_value(f0r_instance_t instance,
                         f0r_param_t param, int param_index)
{
  assert(instance);
  gamma_instance_t* inst = (gamma_instance_t*)instance;
  
  switch(param_index)
  {
  case 0:
    *((double*)param) = inst->gamma;
    break;
  }
}

void f0r_update(f0r_instance_t instance, double time,
                const uint32_t* inframe, uint32_t* outframe)
{
  assert(instance);
  gamma_instance_t* inst = (gamma_instance_t*)instance;
  unsigned int len = inst->width * inst->height;
  
  const uint32_t* lr = inst->lut_r;
  const uint32_t* lg = inst->lut_g;
  const uint32_t* lb = inst->lut_b;
  const unsigned char* src = (const unsigned char*)inframe;
  for (unsigned int i = 0; i < len; ++i)
  {
    outframe[i] = lr[src[0]] | lg[src[1]] | lb[src[2]] | ((uint32_t)src[3] << 24);
    src += 4;
  }
}
