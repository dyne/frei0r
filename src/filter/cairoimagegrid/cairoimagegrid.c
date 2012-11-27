/*
 * cairoimagegrid.c
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

#include "frei0r.h"
#include "frei0r_cairo.h"

#define MAX_ROWS    20
#define MAX_COLUMNS 20 

typedef struct cairo_imagegrid_instance
{
  unsigned int width;
  unsigned int height;
  double rows;
  double columns;
} cairo_imagegrid_instance_t;

int f0r_init()
{
  return 1;
}

void f0r_deinit()
{

}

void f0r_get_plugin_info(f0r_plugin_info_t* cairo_gradient_info)
{
  cairo_gradient_info->name = "cairoimagegrid";
  cairo_gradient_info->author = "Janne Liljeblad";
  cairo_gradient_info->plugin_type = F0R_PLUGIN_TYPE_FILTER;
  cairo_gradient_info->color_model = F0R_COLOR_MODEL_RGBA8888;
  cairo_gradient_info->frei0r_version = FREI0R_MAJOR_VERSION;
  cairo_gradient_info->major_version = 0; 
  cairo_gradient_info->minor_version = 9; 
  cairo_gradient_info->num_params =  2; 
  cairo_gradient_info->explanation = "Draws a grid of input images.";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
	switch(param_index) {
		case 0:
			info->name = "rows";
			info->type =  F0R_PARAM_DOUBLE;
			info->explanation = "Number of rows in the image grid. Input range 0 - 1 is interpreted as range 1 - 20";
			break;
		case 1:
			info->name = "columns";
			info->type = F0R_PARAM_DOUBLE;
			info->explanation = "Number of columns in the image grid. Input range 0 - 1 is interpreted as range 1 - 20";
      break;
	}  
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
  cairo_imagegrid_instance_t* inst = (cairo_imagegrid_instance_t*)calloc(1, sizeof(*inst));
  inst->width = width; 
  inst->height = height;
  inst->rows = 2.0 / (MAX_ROWS - 1);
  inst->columns = 2.0 / (MAX_COLUMNS - 1);
  return (f0r_instance_t)inst;
}

void f0r_destruct(f0r_instance_t instance)
{
  cairo_imagegrid_instance_t* inst = (cairo_imagegrid_instance_t*)instance;
  free(instance);
}

void f0r_set_param_value(f0r_instance_t instance, f0r_param_t param, int param_index)
{
	assert(instance);
  cairo_imagegrid_instance_t* inst = (cairo_imagegrid_instance_t*)instance;

	switch(param_index) {
		case 0:
			inst->rows = *((double*)param);
      break;
		case 1:
			inst->columns = *((double*)param);
      break;
  }
}

void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index)
{
	assert(instance);
  cairo_imagegrid_instance_t* inst = (cairo_imagegrid_instance_t*)instance;

	switch(param_index) {
		case 0:
			*((double*)param) = inst->rows;
			break;
		case 1:
			*((double*)param) = inst->columns;
			break;
	}
}

void draw_grid(cairo_imagegrid_instance_t* inst, unsigned char* dst, const unsigned char* src)
{
  int w = inst->width;
  int h = inst->height;
  int stride = cairo_format_stride_for_width (CAIRO_FORMAT_ARGB32, w);

  cairo_surface_t* dest_image = cairo_image_surface_create_for_data ((unsigned char*)dst,
                                                                       CAIRO_FORMAT_ARGB32,
                                                                       w,
                                                                       h,
                                                                       stride);
  cairo_t *cr = cairo_create (dest_image);

  cairo_surface_t *image = cairo_image_surface_create_for_data ((unsigned char*)src,
                                               CAIRO_FORMAT_ARGB32,
                                               w,
                                               h,
                                               stride);

  cairo_pattern_t *pattern = cairo_pattern_create_for_surface (image);
  cairo_pattern_set_extend (pattern, CAIRO_EXTEND_REPEAT);

  double rows = 1 + (MAX_ROWS - 1) * inst->rows;
  double columns = 1 + (MAX_ROWS - 1) * inst->columns;

  cairo_matrix_t   matrix;
  cairo_matrix_init_scale (&matrix, columns, rows);
  cairo_pattern_set_matrix (pattern, &matrix);

  cairo_set_source (cr, pattern);

  cairo_rectangle (cr, 0, 0, w, h);
  cairo_fill (cr);

  cairo_pattern_destroy (pattern);
  cairo_surface_destroy (image);
  cairo_surface_destroy (dest_image);
  cairo_destroy (cr);
}

void f0r_update(f0r_instance_t instance, double time,
		const uint32_t* inframe, uint32_t* outframe)
{
  assert(instance);
  cairo_imagegrid_instance_t* inst = (cairo_imagegrid_instance_t*) instance;
  const unsigned char* src = (unsigned char*)inframe;
  unsigned char* dst = (unsigned char*)outframe;

  draw_grid(inst, dst, src);
}

