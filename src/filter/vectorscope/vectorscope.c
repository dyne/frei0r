/* vectorscope.c
 * Copyright (C) 2007 Albert Frisch (albert.frisch@gmail.com)
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

#define OFFSET_R        0
#define OFFSET_G        8
#define OFFSET_B        16
#define OFFSET_A        24

typedef struct {
	double hue, sat, val;
} hsv_t;

typedef struct {
	double red, green, blue;
} rgb_t;

typedef struct {
	double x, y;
} coord_rect_t;

typedef struct {
	double r, phi;
} coord_pol_t;

typedef struct vectorscope_instance {
	int w, h;
} vectorscope_instance_t;

int f0r_init()
{
  return 1;
}
void f0r_deinit()
{ /* empty */ }

void f0r_get_plugin_info( f0r_plugin_info_t* info )
{
	info->name = "Vectorscope";
	info->author = "Albert Frisch";
	info->plugin_type = F0R_PLUGIN_TYPE_FILTER;
	info->color_model = F0R_COLOR_MODEL_RGBA8888;
	info->frei0r_version = FREI0R_MAJOR_VERSION;
	info->major_version = 0; 
	info->minor_version = 1; 
	info->num_params =  0; 
	info->explanation = "Displays the vectorscope of the video-data";
}

void f0r_get_param_info( f0r_param_info_t* info, int param_index )
{ /* empty */ }

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
	vectorscope_instance_t* inst = (vectorscope_instance_t*)malloc(sizeof(vectorscope_instance_t));
	inst->w = width;
	inst->h = height;
	return (f0r_instance_t)inst;
}

void f0r_destruct(f0r_instance_t instance)
{
	vectorscope_instance_t* inst = (vectorscope_instance_t*)instance;
	free(instance);
}

void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index)
{ /* empty */ }

void f0r_set_param_value(f0r_instance_t instance, f0r_param_t param, int param_index)
{ /* empty */ }

hsv_t rgb_to_hsv(rgb_t rgb)
{
	hsv_t hsv;
	double min, max, diff;
	double red, green, blue;

	red = rgb.red;
	green = rgb.green;
	blue = rgb.blue;

	if((red>green)&&(red>blue)) max = red;
	else if((green>red)&&(green>blue)) max = green;
	else if((blue>red)&&(blue>green)) max = blue;
	else max = 0;
	if((red<green)&&(red<blue)) min = red;
	else if((green<red)&&(green<blue)) min = green;
	else if((blue<red)&&(blue<green)) min = blue;
	else min = 0;

	diff = max-min;
	if(diff==0) hsv.hue = 0; 
	else
	{
		if(max==red) hsv.hue = 60*(0+(green-blue)/diff);
		else if(max==green) hsv.hue = 60*(2+(blue-red)/diff);
		else if(max==blue) hsv.hue = 60*(4+(red-green)/diff);
	}
	if(hsv.hue<0) hsv.hue += 360;

	if(max==0) hsv.sat = 0;
	else hsv.sat = 100*(max-min)/max;
	
	hsv.val = 100*max/255;

	return hsv;
}

coord_rect_t pol_to_rect(coord_pol_t pol)
{
	coord_rect_t rect;

	rect.x = pol.r*cos(pol.phi);
	rect.y = pol.r*sin(pol.phi);
	return rect;
}

void f0r_update(f0r_instance_t instance, double time, const uint32_t* inframe, uint32_t* outframe)
{
	assert(instance);
	vectorscope_instance_t* inst = (vectorscope_instance_t*)instance;

	uint32_t* dst = outframe;
	const uint32_t* src = inframe;

	int width = inst->w;
	int height = inst->h;	
	int len = inst->w * inst->h;
	
	long i;
	rgb_t rgb;
	hsv_t hsv;
	coord_pol_t pol;
	coord_rect_t rect;
	long offset;
	
	for(i=0;i<len;++i) dst[i] = 0xFF000000;
	for(i=0;i<len;++i)
	{
		rgb.red = (((*src) & 0x000000FF) >> OFFSET_R);
		rgb.green = (((*src) & 0x0000FF00) >> OFFSET_G);
		rgb.blue = (((*src) & 0x00FF0000) >> OFFSET_B);
		src++;

		hsv = rgb_to_hsv(rgb);
		pol.phi = hsv.hue/180*M_PI;
		pol.r = hsv.sat/100;
		rect = pol_to_rect(pol);
		rect.x = width/2+rect.x*height/2;
		rect.y = height/2+rect.y*height/2;
		offset = (height-(int)(rect.y))*width+(int)(rect.x);
		if(dst[offset]<0xFFFFFFFF) dst[offset] += 0x00333333;
	}
}

