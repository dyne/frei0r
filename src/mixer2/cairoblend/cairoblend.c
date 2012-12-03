/*
 * cairoblend.c
 * Copyright 2012 Janne Liljeblad 
 *
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
#include <cairo.h>
#include <stdio.h>
#include <string.h>

#include "frei0r.h"
#include "frei0r_cairo.h"

typedef struct cairo_blend_instance
{
  unsigned int width;
  unsigned int height;
  double opacity;
  char *blend_mode;
} cairo_blend_instance_t;

int f0r_init()
{
  return 1;
}

void f0r_deinit()
{

}

void f0r_get_plugin_info(f0r_plugin_info_t* info)
{
  info->name = "cairoblend";
  info->author = "Janne Liljeblad";
  info->plugin_type = F0R_PLUGIN_TYPE_MIXER2;
  info->color_model = F0R_COLOR_MODEL_RGBA8888;
  info->frei0r_version = FREI0R_MAJOR_VERSION;
  info->major_version = 0;
  info->minor_version = 9; 
  info->num_params =  2; 
  info->explanation = "Composites second input on the first input with user-defined blend mode and opacity.";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
	switch(param_index) {
		case 0:
			info->name = "opacity";
			info->type = F0R_PARAM_DOUBLE;
			info->explanation = "Opacity of composited image";
			break;
		case 1:
			info->name = "blend mode";
      info->type = F0R_PARAM_STRING;
			info->explanation = "Blend mode used to compose image. Accepted values: 'normal', 'add', 'saturate', 'multiply', 'screen', 'overlay', 'darken', 'lighten', 'colordodge', 'colorburn', 'hardlight', 'softlight', 'difference', 'exclusion', 'hslhue', 'hslsaturation', 'hslcolor', 'hslluminosity'";
      break;
	}  
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
  cairo_blend_instance_t* inst = (cairo_blend_instance_t*)calloc(1, sizeof(*inst));
  inst->width = width; 
  inst->height = height;

  inst->opacity = 1.0;

	const char* blend_val = NORMAL;
  inst->blend_mode  = (char*) malloc (strlen(blend_val) + 1 );
	strcpy (inst->blend_mode, blend_val);

  return (f0r_instance_t)inst;
}

void f0r_destruct(f0r_instance_t instance)
{
  cairo_blend_instance_t* inst = (cairo_blend_instance_t*)instance;
  free(instance);
}

void f0r_set_param_value(f0r_instance_t instance, f0r_param_t param, int param_index)
{
	assert(instance);
  cairo_blend_instance_t* inst = (cairo_blend_instance_t*) instance;
  char* sval;
	switch(param_index) {
		case 0:
			inst->opacity = *((double*)param);
      break;
		case 1:
			sval = (*(char**)param);
			inst->blend_mode = (char*)realloc (inst->blend_mode, strlen(sval) + 1);
			strcpy (inst->blend_mode, sval);
      break;
  }
}

void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index)
{
	assert(instance);
  cairo_blend_instance_t* inst = (cairo_blend_instance_t*)instance;

	switch(param_index) {
		case 0:
    *((double*)param) = inst->opacity;
			break;
		case 1:
      *((f0r_param_string *)param) = inst->blend_mode;
			break;
  }
}

void draw_composite(cairo_blend_instance_t* inst, unsigned char* out, unsigned char* dst, unsigned char* src, double time)
{
  int w = inst->width;
  int h = inst->height;
  int stride = cairo_format_stride_for_width (CAIRO_FORMAT_ARGB32, w);

  cairo_surface_t* out_image = cairo_image_surface_create_for_data (out,
                                                                    CAIRO_FORMAT_ARGB32,
                                                                    w,
                                                                    h,
                                                                    stride);
  cairo_t* cr = cairo_create (out_image);

  cairo_surface_t* dst_image = cairo_image_surface_create_for_data (dst,
                                                                     CAIRO_FORMAT_ARGB32,
                                                                     w,
                                                                     h,
                                                                     stride);
  cairo_surface_t* src_image = cairo_image_surface_create_for_data ((unsigned char*)src,
                                                                     CAIRO_FORMAT_ARGB32,
                                                                     w,
                                                                     h,
                                                                     stride);

  // Draw bg on surface
  cairo_set_source_surface (cr, dst_image, 0, 0);
  cairo_paint (cr);

  // Set source, blen mode and draw with current opacity
  frei0r_cairo_set_operator(cr, inst->blend_mode);
  cairo_set_source_surface (cr, src_image, 0, 0);
  cairo_paint_with_alpha (cr, inst->opacity);

  cairo_surface_destroy (out_image);
  cairo_surface_destroy (src_image);
  cairo_surface_destroy (dst_image);
  cairo_destroy (cr);
}

void f0r_update(f0r_instance_t instance, double time,
		const uint32_t* inframe, uint32_t* outframe)
{
  // not impl. for mixers
}

void f0r_update2(f0r_instance_t instance, double time, const uint32_t* inframe1,
		 const uint32_t* inframe2, const uint32_t* inframe3, uint32_t* outframe)
{
  assert(instance);
  cairo_blend_instance_t* inst = (cairo_blend_instance_t*) instance;

  unsigned char* src = (unsigned char*)inframe1;
  unsigned char* dst = (unsigned char*)inframe2;
  unsigned char* out = (unsigned char*)outframe;

  draw_composite (inst, out, src, dst, time);
}

