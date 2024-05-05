/* composition.c
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
#include "frei0r/math.h"

typedef struct composition_instance
{
  unsigned int width;
  unsigned int height;
} composition_instance_t;

int f0r_init()
{
  return 1;
}

void f0r_deinit()
{ /* no initialization required */ }

void f0r_get_plugin_info(f0r_plugin_info_t* compositionInfo)
{
  compositionInfo->name = "Composition";
  compositionInfo->author = "Richard Spindler";
  compositionInfo->plugin_type = F0R_PLUGIN_TYPE_MIXER2;
  compositionInfo->color_model = F0R_COLOR_MODEL_RGBA8888;
  compositionInfo->frei0r_version = FREI0R_MAJOR_VERSION;
  compositionInfo->major_version = 0; 
  compositionInfo->minor_version = 9; 
  compositionInfo->num_params =  0; 
  compositionInfo->explanation = "Composites Image 2 onto Image 1 according to its Alpha Channel";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
  /* no params */
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
  composition_instance_t* inst = (composition_instance_t*)calloc(1, sizeof(*inst));
  inst->width = width; inst->height = height;
  return (f0r_instance_t)inst;
}

void f0r_destruct(f0r_instance_t instance)
{
  free(instance);
}

void f0r_set_param_value(f0r_instance_t instance, 
			 f0r_param_t param, int param_index)
{ /* no params */ }

void f0r_get_param_value(f0r_instance_t instance,
			 f0r_param_t param, int param_index)
{ /* no params */ }


void f0r_update2(f0r_instance_t instance,
		 double time,
		 const uint32_t* inframe1,
		 const uint32_t* inframe2,
		 const uint32_t* inframe3,
		 uint32_t* outframe)
{
  assert(instance);
  composition_instance_t* inst = (composition_instance_t*)instance;
  unsigned int w = inst->width;
  unsigned int h = inst->height;

  unsigned char *ps1, *ps2, *pd, *pd_end;
  ps1 = (unsigned char *)inframe2;
  ps2 = (unsigned char *)inframe1;
  pd = (unsigned char *)outframe;
  pd_end = pd + ( w * h * 4 );
  while ( pd < pd_end ) {
	  pd[0] = ( ( ( ps1[0] - ps2[0] ) * 255 * ps1[3] ) >> 16 ) + ps2[0];
	  pd[1] = ( ( ( ps1[1] - ps2[1] ) * 255 * ps1[3] ) >> 16 ) + ps2[1];
	  pd[2] = ( ( ( ps1[2] - ps2[2] ) * 255 * ps1[3] ) >> 16 ) + ps2[2];
	  pd[3] = CLAMP0255( ps1[3] + ps2[3] );
	  ps1 += 4;
	  ps2 += 4;
	  pd += 4;
  }
}


