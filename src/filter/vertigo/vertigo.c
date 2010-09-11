/* 
 * VertigoTV - Alpha blending with zoomed and rotated images.
 * Copyright (C) 2001 FUKUCHI Kentarou
 * parametrization by jaromil
 * ported to frei0r by joepadmiraal
 *
 * This source code  is free software; you can  redistribute it and/or
 * modify it under the terms of the GNU Public License as published by
 * the Free Software  Foundation; either version 3 of  the License, or
 * (at your option) any later version.
 *
 * This source code is distributed in the hope that it will be useful,
 * but  WITHOUT ANY  WARRANTY; without  even the  implied  warranty of
 * MERCHANTABILITY or FITNESS FOR  A PARTICULAR PURPOSE.  Please refer
 * to the GNU Public License for more details.
 *
 * You should  have received  a copy of  the GNU Public  License along
 * with this source code; if  not, write to: Free Software Foundation,
 * Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */


#include "frei0r.h"
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <string.h>


typedef struct vertigo_instance
{
  unsigned int width;
  unsigned int height;
  int x,y,xc,yc;
  double phase_increment;
  double zoomrate;
  double tfactor;
  uint32_t *current_buffer, *alt_buffer;
  
  uint32_t *buffer;
  int dx, dy;
  int sx, sy;
  
  int pixels;
  double phase;
} vertigo_instance_t;



int f0r_init()
{
  return 1;
}

void f0r_deinit()
{ 
  /* no deinitialization required */ 
}

void f0r_get_plugin_info(f0r_plugin_info_t* vertigoInfo)
{
  vertigoInfo->name = "Vertigo";
  vertigoInfo->author = "Fukuchi Kentarou";
  vertigoInfo->plugin_type = F0R_PLUGIN_TYPE_FILTER;
  vertigoInfo->color_model = F0R_COLOR_MODEL_RGBA8888;
  vertigoInfo->frei0r_version = FREI0R_MAJOR_VERSION;
  vertigoInfo->major_version = 1;
  vertigoInfo->minor_version = 0;
  vertigoInfo->num_params =  3;
  vertigoInfo->explanation = "alpha blending with zoomed and rotated images";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
  switch(param_index)
  {
  case 0:
    info->name = "PhaseIncrement";
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "Phase increment";
    break;
  case 1:
    info->name = "Zoomrate";
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "Zoomrate";
    break;
  }
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
  vertigo_instance_t* inst = 
    (vertigo_instance_t*)calloc(1, sizeof(*inst));
  inst->width = width; inst->height = height;

  inst->pixels = width*height;

  inst->buffer = (uint32_t*)calloc(inst->pixels*2, sizeof(uint32_t));
  if(inst->buffer == NULL) {
    free(inst);
    return 0;
  }

  inst->current_buffer = inst->buffer;
  inst->alt_buffer = inst->buffer + inst->pixels;

  inst->phase = 0.0;
  inst->phase_increment = 0.02;
  inst->zoomrate = 1.01;

  inst->x = width>>1;
  inst->y = height>>1;
  inst->xc = inst->x*inst->x;
  inst->yc = inst->y*inst->y;
  inst->tfactor = (inst->xc+inst->yc) * inst->zoomrate;

  return (f0r_instance_t)inst;
}

void f0r_destruct(f0r_instance_t instance)
{
  vertigo_instance_t* inst = (vertigo_instance_t*)instance;
  if(inst->buffer!=NULL) 
  {
    free(inst->buffer); 
    inst->buffer = NULL; 
  }
  free(instance);
}

void f0r_set_param_value(f0r_instance_t instance, 
			 f0r_param_t param, int param_index)
{ 
  assert(instance);
  vertigo_instance_t* inst = (vertigo_instance_t*)instance;

  switch(param_index)
  {
  case 0:
    /* phase_increment */
    inst->phase_increment = *((double*)param);
    break;
  case 1:
    /* zoomrate */
    inst->zoomrate = *((double*)param);
    inst->tfactor = (inst->xc+inst->yc) * inst->zoomrate;
    break;
  }
}

void f0r_get_param_value(f0r_instance_t instance,
			 f0r_param_t param, int param_index)
{ 
  assert(instance);
  vertigo_instance_t* inst = (vertigo_instance_t*)instance;
  
  switch(param_index)
  {
  case 0:
    /* phase_increment */
    *((double*)param) = (double) (inst->phase_increment);
    break;
  case 1:
    /* zoomrate */
    *((double*)param) = (double) (inst->zoomrate);
    break;
  }
}

void f0r_update(f0r_instance_t instance, double time,
		const uint32_t* inframe, uint32_t* outframe)
{
  vertigo_instance_t* inst = (vertigo_instance_t*)instance;
  unsigned int w = inst->width;
  unsigned int h = inst->height;
  int x = inst->x;
  int y = inst->y;
  int xc = inst->xc;
  int yc = inst->yc;
  double tfactor = inst->tfactor;

  uint32_t* dst = outframe;
  const uint32_t* src = inframe;
  uint32_t *p;
  uint32_t v;
  int ox, oy;
  int i;

  double vx, vy;
  double dizz;
  
  dizz = sin(inst->phase) * 10 + sin(inst->phase*1.9+5) * 5;

  if(w > h) 
  {
    if(dizz >= 0) 
    {
      if(dizz > x) dizz = x;
      vx = (x*(x-dizz) + yc) / tfactor;
    } else 
    {
      if(dizz < -x) dizz = -x;
      vx = (x*(x+dizz) + yc) / tfactor;
    }
    vy = (dizz*y) / tfactor;

  } else 
  {
    if(dizz >= 0) 
    {
      if(dizz > y) dizz = y;
      vx = (xc + y*(y-dizz)) / tfactor;
    } else 
    {
      if(dizz < -y) dizz = -y;
      vx = (xc + y*(y+dizz)) / tfactor;
    }
    vy = (dizz*x) / tfactor;
  }

  inst->dx = vx * 65536;
  inst->dy = vy * 65536;
  inst->sx = (-vx * x + vy * y + x + cos(inst->phase*5) * 2) * 65536;
  inst->sy = (-vx * y - vy * x + y + sin(inst->phase*6) * 2) * 65536;
  
  inst->phase += inst->phase_increment;
  if(inst->phase > 5700000) inst->phase = 0;


  p = inst->alt_buffer;

  for(y=h; y>0; y--) 
  {
    ox = inst->sx;
    oy = inst->sy;
    for(x=w; x>0; x--) 
    {
      i = (oy>>16)*w + (ox>>16);
      if(i<0) i = 0;
      if(i>=inst->pixels) i = inst->pixels;
      v = inst->current_buffer[i] & 0xfcfcff;
      v = (v * 3) + ((*src++) & 0xfcfcff);
      *dst++ = (v>>2);
      *p++ = (v>>2);
      ox += inst->dx;
      oy += inst->dy;
    }
    inst->sx -= inst->dy;
    inst->sy += inst->dx;
  }

  p = inst->current_buffer;
  inst->current_buffer = inst->alt_buffer;
  inst->alt_buffer = p;

}
