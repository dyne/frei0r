/*
 * cairogradient.c
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
#include <string.h>
#include <assert.h>
#include <cairo.h>
#include <stdio.h>
#include <math.h>

#include "frei0r.h"
#include "frei0r_cairo.h"

typedef struct cairo_gradient_instance
{
  unsigned int width;
  unsigned int height;
  char *pattern;
	f0r_param_color_t start_color;
	double start_opacity;
	f0r_param_color_t end_color;
	double end_opacity;
  double start_x;
  double start_y;
  double end_x;
  double end_y;
  char *end_point;
  double offset;
  char *blend_mode;
} cairo_gradient_instance_t;

int f0r_init()
{
  return 1;
}

void f0r_deinit()
{

}

void f0r_get_plugin_info(f0r_plugin_info_t* info)
{
  info->name = "cairogradient";
  info->author = "Janne Liljeblad";
  info->plugin_type = F0R_PLUGIN_TYPE_FILTER;
  info->color_model = F0R_COLOR_MODEL_RGBA8888;
  info->frei0r_version = FREI0R_MAJOR_VERSION;
  info->major_version = 0; 
  info->minor_version = 9; 
  info->num_params = 11; 
  info->explanation = "Draws a gradient on top of image. Filter is given gradient start and end points, colors and opacities.";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
	switch(param_index) {
		case 0:
			info->name = "pattern";
      info->type = F0R_PARAM_STRING;
			info->explanation = "Linear or radial gradient";// Accepted values: 'gradient_linear' and 'gradient_radial
			break;
		case 1:
			info->name = "start color";
			info->type = F0R_PARAM_COLOR;
			info->explanation = "First color of the gradient";
			break;
		case 2:
			info->name = "start opacity";
			info->type = F0R_PARAM_DOUBLE;
			info->explanation = "Opacity of the first color of the gradient";
      break;
		case 3:
			info->name = "end color";
			info->type = F0R_PARAM_COLOR;
			info->explanation = "Second color of the gradient";
			break;
		case 4:
			info->name = "end opacity";
			info->type = F0R_PARAM_DOUBLE;
			info->explanation = "Opacity of the second color of the gradient";
			break;
		case 5:
			info->name = "start x";
      info->type = F0R_PARAM_DOUBLE;
			info->explanation = "X position of the start point of the gradient";
			break;
		case 6:
			info->name = "start y";
      info->type = F0R_PARAM_DOUBLE;
			info->explanation = "Y position of the start point of the gradient";
			break;
    case 7:
			info->name = "end x";
      info->type = F0R_PARAM_DOUBLE;
			info->explanation = "X position of the end point of the gradient";
			break;
    case 8:
			info->name = "end y";
      info->type = F0R_PARAM_DOUBLE;
			info->explanation = "Y position of the end point of the gradient";
			break;
    case 9:
			info->name = "offset";
			info->type = F0R_PARAM_DOUBLE;
			info->explanation = "Position of first color in the line connecting gradient ends, really useful only for radial gradient";
			break;
    case 10:
			info->name = "blend mode";
      info->type = F0R_PARAM_STRING;
			info->explanation = "Blend mode used to compose gradient on image. Accepted values: 'normal', 'add', 'saturate', 'multiply', 'screen', 'overlay', 'darken', 'lighten', 'colordodge', 'colorburn', 'hardlight', 'softlight', 'difference', 'exclusion', 'hslhue', 'hslsaturation', 'hslcolor', 'hslluminosity'";
			break;
	}  
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
  cairo_gradient_instance_t* inst = (cairo_gradient_instance_t*) calloc (1, sizeof(*inst));
  inst->width = width; 
  inst->height = height;

	const char* pattern_val = GRADIENT_LINEAR;
  inst->pattern = (char*) malloc (strlen(pattern_val) + 1);
	strcpy (inst->pattern, pattern_val);

  inst->start_color.r = 0.0;
  inst->start_color.g = 0.0;
  inst->start_color.b = 0.0;
  inst->start_opacity = 0.5;
  inst->end_color.r = 1.0;
  inst->end_color.g = 1.0;
  inst->end_color.b = 1.0;
  inst->end_opacity = 0.5;
  inst->start_x = 0.5;
  inst->start_y = 0.0;
  inst->end_x = 0.5;
  inst->end_y = 1.0;
  inst->offset = 0.0;

	const char* blend_val = NORMAL;
  inst->blend_mode  = (char*) malloc (strlen(blend_val) + 1 );
	strcpy (inst->blend_mode, blend_val);

  return (f0r_instance_t)inst;
}

void f0r_destruct(f0r_instance_t instance)
{
  cairo_gradient_instance_t* inst = (cairo_gradient_instance_t*)instance;
  free(instance);
}

void f0r_set_param_value(f0r_instance_t instance, f0r_param_t param, int param_index)
{
	assert(instance);
  cairo_gradient_instance_t* inst = (cairo_gradient_instance_t*)instance;
  char* sval;
	switch(param_index) {
		case 0:
			sval = (*(char**)param);
			inst->pattern = (char*)realloc (inst->pattern, strlen(sval) + 1);
			strcpy( inst->pattern, sval );
      break;
		case 1:
			inst->start_color = *((f0r_param_color_t*)param);
      break;
		case 2:
			inst->start_opacity = *((double*)param);
      break;
		case 3:
			inst->end_color = *((f0r_param_color_t*)param);
      break;
		case 4:
			inst->end_opacity = *((double*)param);
      break;
		case 5:
			inst->start_x = *((double*)param);
      break;
		case 6:
			inst->start_y = *((double*)param);
      break;
		case 7:
			inst->end_x = *((double*)param);
      break;
		case 8:
			inst->end_y = *((double*)param);
      break;
		case 9:
			inst->offset = *((double*)param);
      break;
		case 10:
			sval = (*(char**)param);
			inst->blend_mode = (char*) realloc (inst->blend_mode, strlen(sval) + 1);
			strcpy (inst->blend_mode, sval);
      break;
  }
}

void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index)
{
	assert(instance);
  cairo_gradient_instance_t* inst = (cairo_gradient_instance_t*)instance;

	switch(param_index) {
		case 0:
      *((f0r_param_string *)param) = inst->pattern;
			break;
		case 1:
			*((f0r_param_color_t*)param) = inst->start_color;
			break;
		case 2:
			*((double*)param) = inst->start_opacity;
			break;
		case 3:
			*((f0r_param_color_t*)param) = inst->end_color;
			break;
		case 4:
			*((double*)param) = inst->end_opacity;
			break;
		case 5:
			*((double*)param) = inst->start_x;
			break;
		case 6:
			*((double*)param) = inst->start_y;
			break;
		case 7:
			*((double*)param) = inst->end_x;
			break;
		case 8:
			*((double*)param) = inst->end_y;
			break;
		case 9:
			*((double*)param) = inst->offset;
			break;
		case 10:
      *((f0r_param_string *)param) = inst->blend_mode;
			break;
	}
}

void draw_gradient(cairo_gradient_instance_t* inst, unsigned char* dst, const unsigned char* src, double time)
{
  int stride = cairo_format_stride_for_width (CAIRO_FORMAT_ARGB32,  inst->width);
  cairo_surface_t *surface = cairo_image_surface_create_for_data (dst, CAIRO_FORMAT_ARGB32, inst->width, inst->height, stride);
  cairo_t *cr = cairo_create (surface);

  cairo_surface_t* src_image = cairo_image_surface_create_for_data ((unsigned char*)src, 
                                              CAIRO_FORMAT_ARGB32, inst->width, inst->height, stride);

  cairo_set_source_surface (cr, src_image, 0, 0);
  cairo_paint (cr);

  cairo_pattern_t *pat;
  double sx = inst->start_x;
  double sy = inst->start_y;
  double ex = inst->end_x;
  double ey = inst->end_y;
  if (strcmp(inst->pattern, GRADIENT_RADIAL) == 0)
  {


    double distance_x = (sx - ex) * inst->width;
    double distance_y = (sy - ey) * inst->height;
    double distance = sqrt( (distance_x * distance_x) + (distance_y * distance_y));
    pat = cairo_pattern_create_radial (sx * inst->width,
                                       sy * inst->height,
                                       0.0,
                                       sx * inst->width,
                                       sy * inst->height,
                                       distance);
  }
  else
  {
    pat = cairo_pattern_create_linear ( sx * inst->width,
                                        sy * inst->height,
                                        ex * inst->width,
                                        ey * inst->height);
  }

  freior_cairo_set_color_stop_rgba_LITLLE_ENDIAN (pat,
                                                  1.0,
                                                  inst->start_color.b,
                                                  inst->start_color.g,
                                                  inst->start_color.r,
                                                  inst->start_opacity);

  freior_cairo_set_color_stop_rgba_LITLLE_ENDIAN (pat,
                                                  inst->offset,                                    
                                                  inst->end_color.b,
                                                  inst->end_color.g,
                                                  inst->end_color.r,
                                                  inst->end_opacity);


  cairo_set_source (cr, pat);
  frei0r_cairo_set_operator(cr, inst->blend_mode);
  cairo_rectangle (cr, 0, 0, inst->width, inst->height);

  cairo_fill (cr);

  cairo_pattern_destroy (pat);
  cairo_destroy (cr);
  cairo_surface_destroy (surface);
  cairo_surface_destroy (src_image);  
}

void f0r_update(f0r_instance_t instance, double time,
		const uint32_t* inframe, uint32_t* outframe)
{
  assert(instance);
  cairo_gradient_instance_t* inst = (cairo_gradient_instance_t*)instance;

  unsigned char* dst = (unsigned char*)outframe;
  const unsigned char* src = (unsigned char*)inframe;

  draw_gradient(inst, dst, src, time);
}

