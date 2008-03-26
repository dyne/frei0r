/* rgbparade.c
 * Copyright (C) 2008 Albert Frisch (albert.frisch@gmail.com)
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

#define GRID_COLOR	0xFF8E8E8E
#define SCOPE_R_COLOR	0xFF0000FF
#define SCOPE_G_COLOR	0xFF00FF00
#define SCOPE_B_COLOR	0xFFFF0000
#define SCOPE_STEPS	60

typedef struct {
	double red, green, blue;
} rgb_t;

typedef struct {
	double x, y;
} coord_rect_t;

typedef struct rgbparade {
	int w, h;
} rgbparade_t;

int f0r_init()
{
  return 1;
}
void f0r_deinit()
{ /* empty */ }

void f0r_get_plugin_info( f0r_plugin_info_t* info )
{
	info->name = "RGB-Parade";
	info->author = "Albert Frisch";
	info->plugin_type = F0R_PLUGIN_TYPE_FILTER;
	info->color_model = F0R_COLOR_MODEL_RGBA8888;
	info->frei0r_version = FREI0R_MAJOR_VERSION;
	info->major_version = 0; 
	info->minor_version = 1; 
	info->num_params =  0; 
	info->explanation = "Displays a histogram of R, G and B of the video-data";
}

void f0r_get_param_info( f0r_param_info_t* info, int param_index )
{ /* empty */ }

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
	rgbparade_t* inst = (rgbparade_t*)malloc(sizeof(rgbparade_t));
	inst->w = width;
	inst->h = height;
	return (f0r_instance_t)inst;
}

void f0r_destruct(f0r_instance_t instance)
{
	rgbparade_t* inst = (rgbparade_t*)instance;
	free(instance);
}

void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index)
{ /* empty */ }

void f0r_set_param_value(f0r_instance_t instance, f0r_param_t param, int param_index)
{ /* empty */ }

void pixel_draw(unsigned char* scope, coord_rect_t rect, double width, double height)
{
	long offset, inter_offset;
	long len;
	
	len = width*height;
	offset = (height-(int)(rect.y))*width+(int)(rect.x);
	if(scope[offset]<253) scope[offset]++;
	inter_offset = offset-width;
	if(inter_offset>=0) if(scope[inter_offset]<253) scope[inter_offset]++;
	inter_offset = offset+width;
	if(inter_offset<=len) if(scope[inter_offset]<253) scope[inter_offset]++;
}

void pixel_parade(unsigned char* scope, rgb_t pixel, double pixel_x, double width, double height)
{
	coord_rect_t rect;

	rect.x = pixel_x/3;
	rect.y = pixel.red/255*(height-1)+1;
	pixel_draw(scope, rect, width, height);
	rect.x = pixel_x/3+width/3;
	rect.y = pixel.green/255*(height-1)+1;
	pixel_draw(scope, rect, width, height);
	rect.x = pixel_x/3+2*width/3;
	rect.y = pixel.blue/255*(height-1)+1;
	pixel_draw(scope, rect, width, height);
}

void draw_grid(unsigned char* scope, double width, double height)
{
	double i, j;
	long offset;
	
	for(j=0;j<6;j++)
	{
		for(i=0;i<width;++i)
		{
			offset = j*(height-1)*width/5+i;
			scope[offset] = 255;
		}
	}
	for(j=0;j<2;j++)
	{
		for(i=0;i<height;i++)
		{
			offset = i*width+j*(width-1);
			scope[offset] = 255;
		}
	}
}

void f0r_update(f0r_instance_t instance, double time, const uint32_t* inframe, uint32_t* outframe)
{
	assert(instance);
	rgbparade_t* inst = (rgbparade_t*)instance;

	uint32_t* dst = outframe;
	const uint32_t* src = inframe;

	int width = inst->w;
	int height = inst->h;	
	int len = inst->w * inst->h;
	
	long i,j;
	rgb_t pixel;
	long offset;
	unsigned char scope[len];

	for(i=0;i<len;++i) scope[i] = 0;
	for(i=0;i<height;++i)
	{
		for(j=0;j<width;++j)
		{
			pixel.red = (((*src) & 0x000000FF) >> OFFSET_R);
			pixel.green = (((*src) & 0x0000FF00) >> OFFSET_G);
			pixel.blue = (((*src) & 0x00FF0000) >> OFFSET_B);
			src++;
			pixel_parade(scope, pixel, j, width, height);
		}
	}
	draw_grid(scope, width, height);
	for(i=0;i<height;++i)
	{
		for(j=0;j<width;++j)
		{
			offset = i*width+j;
			if(scope[offset]==255) dst[offset] = GRID_COLOR;
			else	
			{		
				if(j<width/3)
				{
					pixel.red = ((SCOPE_R_COLOR & 0x000000FF) >> OFFSET_R);
					pixel.green = ((SCOPE_R_COLOR & 0x0000FF00) >> OFFSET_G);
					pixel.blue = ((SCOPE_R_COLOR & 0x00FF0000) >> OFFSET_B);
				}
				else if(j<width*2/3)
				{
					pixel.red = ((SCOPE_G_COLOR & 0x000000FF) >> OFFSET_R);
					pixel.green = ((SCOPE_G_COLOR & 0x0000FF00) >> OFFSET_G);
					pixel.blue = ((SCOPE_G_COLOR & 0x00FF0000) >> OFFSET_B);
				}
				else
				{
					pixel.red = ((SCOPE_B_COLOR & 0x000000FF) >> OFFSET_R);
					pixel.green = ((SCOPE_B_COLOR & 0x0000FF00) >> OFFSET_G);
					pixel.blue = ((SCOPE_B_COLOR & 0x00FF0000) >> OFFSET_B);
				}
				if(scope[offset]>SCOPE_STEPS) scope[offset] = SCOPE_STEPS;
				pixel.red = scope[offset]*pixel.red/SCOPE_STEPS;
				pixel.green = scope[offset]*pixel.green/SCOPE_STEPS;
				pixel.blue = scope[offset]*pixel.blue/SCOPE_STEPS;			
				dst[offset] = 0xFF000000 | ((int)pixel.blue<<OFFSET_B) | ((int)pixel.green<<OFFSET_G) | ((int)pixel.red<<OFFSET_R);
			}
		}
	}
}

