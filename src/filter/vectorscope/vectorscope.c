/* vectorscope.c
 * Copyright (C) 2008 Albert Frisch (albert.frisch AT gmail.com)
 * Copyright (C) 2008 Richard Spindler (richard.spindler AT gmail.com)
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
#include <stdio.h>

#include <gavl/gavl.h>

#include "vectorscope_image.h"

#define OFFSET_R        0
#define OFFSET_G        8
#define OFFSET_B        16
#define OFFSET_A        24

#define GRID_COLOR	0xFF8E8E8E //144D7D
#define SCOPE_COLOR	0xFFFF917F
#define SCOPE_STEPS	10

/* c99 seems to be extra clever, and removes the definition of M_PI,
 * this adds it again */
#define M_PI            3.14159265358979323846

typedef struct {
	double Y, Cb, Cr;
} YCbCr_t;

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
	unsigned char* scala;
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
{
	/* empty */
}
f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
	vectorscope_instance_t* inst = (vectorscope_instance_t*)malloc(sizeof(vectorscope_instance_t));
	inst->w = width;
	inst->h = height;


	inst->scala = (unsigned char*)malloc( width * height * 4 );

	gavl_video_scaler_t* video_scaler;
	gavl_video_frame_t* frame_src;
	gavl_video_frame_t* frame_dst;

	video_scaler = gavl_video_scaler_create();
	frame_src = gavl_video_frame_create( 0 );
	frame_dst = gavl_video_frame_create( 0 );
	frame_dst->strides[0] = width * 4;
	frame_src->strides[0] = vectorscope_image.width * 4;

	gavl_video_options_t* options = gavl_video_scaler_get_options( video_scaler );
	gavl_video_format_t format_src;
	gavl_video_format_t format_dst;

	format_dst.frame_width  = inst->w;
	format_dst.frame_height = inst->h;
	format_dst.image_width  = inst->w;
	format_dst.image_height = inst->h;
	format_dst.pixel_width = 1;
	format_dst.pixel_height = 1;
	format_dst.pixelformat = GAVL_RGBA_32;



	
	format_src.frame_width  = vectorscope_image.width;
	format_src.frame_height = vectorscope_image.height;
	format_src.image_width  = vectorscope_image.width;
	format_src.image_height = vectorscope_image.height;
	format_src.pixel_width = 1;
	format_src.pixel_height = 1;
	format_src.pixelformat = GAVL_RGBA_32;

	gavl_rectangle_f_t src_rect;
	gavl_rectangle_i_t dst_rect;

	src_rect.x = 0;
	src_rect.y = 0;
	src_rect.w = vectorscope_image.width;
	src_rect.h = vectorscope_image.height;

	float dst_x, dst_y, dst_w, dst_h;
	if ( (float)inst->w / inst->h > (float)vectorscope_image.width / vectorscope_image.height ) {
		dst_y = 0;
		dst_h = inst->h;
		dst_w = ((float)vectorscope_image.width / vectorscope_image.height) * inst->h;
		dst_x = ( inst->w - dst_w ) / 2.0;
	} else {
		dst_x = 0;
		dst_w = inst->w;
		dst_h = ((float)vectorscope_image.height / vectorscope_image.width) * inst->w;
		dst_y = ( inst->h - dst_h ) / 2.0;
	}
	dst_rect.x = (int)(dst_x);
	dst_rect.y = (int)(dst_y);
	dst_rect.w = (int)(dst_w);
	dst_rect.h = (int)(dst_h);


	gavl_video_options_set_rectangles( options, &src_rect, &dst_rect );
	gavl_video_scaler_init( video_scaler, &format_src, &format_dst );

	frame_src->planes[0] = (uint8_t *)vectorscope_image.pixel_data;
	frame_dst->planes[0] = (uint8_t *)inst->scala;

	float transparent[4] = { 0.0, 0.0, 0.0, 0.0 };
	gavl_video_frame_fill( frame_dst, &format_dst, transparent );
	//gavl_video_frame_clear( frame_dst, &format_dst );

	gavl_video_scaler_scale( video_scaler, frame_src, frame_dst );

	gavl_video_scaler_destroy(video_scaler);
	gavl_video_frame_null( frame_src );
	gavl_video_frame_destroy( frame_src );
	gavl_video_frame_null( frame_dst );
	gavl_video_frame_destroy( frame_dst );

	return (f0r_instance_t)inst;
}

void f0r_destruct(f0r_instance_t instance)
{
	vectorscope_instance_t* inst = (vectorscope_instance_t*)instance;
	free(inst->scala);
	free(instance);
}

void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index)
{   
	/* empty */
}

void f0r_set_param_value(f0r_instance_t instance, f0r_param_t param, int param_index)
{
	/* empty */
}

 /* RGB to YCbCr range 0-255 */
YCbCr_t rgb_to_YCbCr(rgb_t rgb)
{
	YCbCr_t dest;
	dest.Y = (float)((0.299 * (float)rgb.red + 0.587 * (float)rgb.green + 0.114 * (float)rgb.blue));
	dest.Cb = 128 + (float)((-0.16874 * (float)rgb.red - 0.33126 * (float)rgb.green + 0.5 * (float)rgb.blue));
	dest.Cr = 128 + (float)((0.5 * (float)rgb.red - 0.41869 * (float)rgb.green - 0.08131 * (float)rgb.blue));
	return dest;
}

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

long pixel_offset(coord_pol_t pol, int width, int height)
{
	coord_rect_t rect;
	long offset;
	
	rect = pol_to_rect(pol);
	rect.x = width/2+rect.x*height/2;
	rect.y = height/2+rect.y*height/2;
	offset = (height-(int)(rect.y))*width+(int)(rect.x);
	return offset;
}

void draw_grid(unsigned char* scope, int width, int height)
{
	int i, j;
	coord_pol_t pol;
	long offset;
	long len = width * height;
	

	for(j=1;j<6;j++)
	{
		pol.r = j*0.20;
		for(i=0;i<2000;++i)
		{
			pol.phi = (double)i/2000*2*M_PI;
			offset = pixel_offset(pol, width, height);
			if ( offset < len ) {
				scope[offset] = 255;
			}
		}
	}
	for(j=0;j<6;j++)
	{
		pol.phi = j*M_PI/3;
		for(i=0;i<1000;i++)
		{
			pol.r = (double)i/1000*0.7+0.3;
			offset = pixel_offset(pol, width, height);
			if ( offset < len ) {
				scope[offset] = 255;
			}
		}
	}
}

void f0r_update(f0r_instance_t instance, double time, const uint32_t* inframe, uint32_t* outframe)
{
	assert(instance);
	vectorscope_instance_t* inst = (vectorscope_instance_t*)instance;

	uint32_t* dst = outframe;
	uint32_t* dst_end;
	const uint32_t* src = inframe;
	const uint32_t* src_end;

	int width = inst->w;
	int height = inst->h;	
	int len = inst->w * inst->h;
	
	YCbCr_t YCbCr;
	rgb_t rgb;
	uint8_t* pixel;
	int x, y;
	dst_end = dst + len;
	src_end = src + len;

	while ( dst < dst_end ) {
		*(dst++) = 0xFF000000;
	}
	dst = outframe;

	while ( src < src_end ) {
		rgb.red = (((*src) & 0x000000FF) >> OFFSET_R);
		rgb.green = (((*src) & 0x0000FF00) >> OFFSET_G);
		rgb.blue = (((*src) & 0x00FF0000) >> OFFSET_B);
		src++;
		YCbCr = rgb_to_YCbCr(rgb);
		x = lrint((width/2)+((YCbCr.Cb/255.0-0.5) * height));
		y = lrint((height/2)-((YCbCr.Cr/255.0-0.5) * width));
		//printf ("Cb: %d, Cr: %d\n", x, y );
		if ( x >= 0 && x < width && y >= 0 && y < height ) {
			pixel = (uint8_t*)&dst[x+width*y];
			if ( pixel[0] < 240 ) {
				pixel[0]+=10;
				pixel[1]+=10;
				pixel[2]+=10;
			}
			//dst[x+width*y] += 1;//0xFFFFFFFF;
		}
	}

	unsigned char *scala8, *dst8, *dst8_end;

	scala8 = inst->scala;
	dst8 = (unsigned char*)outframe;
	dst8_end = dst8 + ( len * 4 );
	while ( dst8 < dst8_end ) {
		dst8[0] = ( ( ( scala8[0] - dst8[0] ) * 255 * scala8[3] ) >> 16 ) + dst8[0];
		dst8[1] = ( ( ( scala8[1] - dst8[1] ) * 255 * scala8[3] ) >> 16 ) + dst8[1];
		dst8[2] = ( ( ( scala8[2] - dst8[2] ) * 255 * scala8[3] ) >> 16 ) + dst8[2];
		scala8 += 4;
		dst8 += 4;

	}


	
/*

	long i,j;
	//hsv_t hsv;
	coord_pol_t pol;
	coord_rect_t rect;
	long offset;
	unsigned char scope[len];
	unsigned char pixel_val;

	for(i=0;i<len;++i) scope[i] = 0;
	for(i=0;i<len;++i)
	{
		rgb.red = (((*src) & 0x000000FF) >> OFFSET_R);
		rgb.green = (((*src) & 0x0000FF00) >> OFFSET_G);
		rgb.blue = (((*src) & 0x00FF0000) >> OFFSET_B);
		src++;

		YCbCr = rgb_to_YCbCr(rgb);
		//hsv = rgb_to_hsv(rgb);
		//pol.phi = hsv.hue/180*M_PI;
		//pol.r = hsv.sat/100;
		pol.phi = YCbCr.hue/180*M_PI;
		pol.r = YCbCr.sat/100;
		offset = pixel_offset(pol, width, height);
		if ( offset < len ) {
			scope[offset]++;
		}
	}
	draw_grid(&scope, width, height);
	rgb.red = ((SCOPE_COLOR & 0x000000FF) >> OFFSET_R);
	rgb.green = ((SCOPE_COLOR & 0x0000FF00) >> OFFSET_G);
	rgb.blue = ((SCOPE_COLOR & 0x00FF0000) >> OFFSET_B);
	for(i=0;i<len;++i)
	{
		if(scope[i]==255) dst[i] = GRID_COLOR;
		else		
		{		
			if(scope[i]>SCOPE_STEPS) scope[i] = SCOPE_STEPS;
			pixel.red = scope[i]*rgb.red/SCOPE_STEPS;
			pixel.green = scope[i]*rgb.green/SCOPE_STEPS;
			pixel.blue = scope[i]*rgb.blue/SCOPE_STEPS;			
			dst[i] = 0xFF000000 | ((int)pixel.blue<<OFFSET_B) | ((int)pixel.green<<OFFSET_G) | ((int)pixel.red<<OFFSET_R);
		}
	}
	*/
}

