/* scale0tilt.c
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

#include <math.h>
#include "frei0r.h"
#include <gavl/gavl.h>
#include <stdlib.h>

typedef struct scale0tilt_instance {
	double cl, ct, cr, cb;
	double sx, sy;
	double tx, ty;
	int w, h;
	gavl_video_scaler_t* video_scaler;
	gavl_video_frame_t* frame_src;
	gavl_video_frame_t* frame_dst;
} scale0tilt_instance_t;

void update_scaler( scale0tilt_instance_t* inst )
{
	float dst_x, dst_y, dst_w, dst_h;
	float src_x, src_y, src_w, src_h;

	src_x = inst->w * inst->cl;
	src_y = inst->h * inst->ct;
	src_w = inst->w * (1.0 - inst->cl - inst->cr );
	src_h = inst->h * (1.0 - inst->ct - inst->cb );

	dst_x = inst->w * inst->cl * inst->sx + inst->tx * inst->w;
	dst_y = inst->h * inst->ct * inst->sy + inst->ty * inst->h;
	dst_w = inst->w * (1.0 - inst->cl - inst->cr) * inst->sx;
	dst_h = inst->h * (1.0 - inst->ct - inst->cb) * inst->sy;

	if ( dst_x + dst_w > inst->w ) {
		src_w = src_w * ( (inst->w-dst_x) / dst_w );
		dst_w = inst->w - dst_x;
	}
	if ( dst_y + dst_h > inst->h ) {
		src_h = src_h * ( (inst->h-dst_y) / dst_h );
		dst_h = inst->h - dst_y;
	}
	if ( dst_x < 0 ) {
		src_x = src_x - dst_x * ( src_w / dst_w );
		src_w = src_w * ( (dst_w+dst_x) / dst_w );
		dst_w = dst_w + dst_x;
		dst_x = 0;
	}
	if ( dst_y < 0 ) {
		src_y = src_y - dst_y * ( src_h / dst_h );
		src_h = src_h * ( (dst_h+dst_y) / dst_h );
		dst_h = dst_h + dst_y;
		dst_y = 0;
	}
	gavl_video_options_t* options = gavl_video_scaler_get_options( inst->video_scaler );

	gavl_video_format_t format_src;
	gavl_video_format_t format_dst;

	format_dst.frame_width  = inst->w;
	format_dst.frame_height = inst->h;
	format_dst.image_width  = inst->w;
	format_dst.image_height = inst->h;
	format_dst.pixel_width = 1;
	format_dst.pixel_height = 1;
	format_dst.pixelformat = GAVL_RGBA_32;
	
	format_src.frame_width  = inst->w;
	format_src.frame_height = inst->h;
	format_src.image_width  = inst->w;
	format_src.image_height = inst->h;
	format_src.pixel_width = 1;
	format_src.pixel_height = 1;
	format_src.pixelformat = GAVL_RGBA_32;

	gavl_rectangle_f_t src_rect;
	gavl_rectangle_i_t dst_rect;

	src_rect.x = src_x;
	src_rect.y = src_y;
	src_rect.w = src_w;
	src_rect.h = src_h;

	dst_rect.x = lroundf(dst_x);
	dst_rect.y = lroundf(dst_y);
	dst_rect.w = lroundf(dst_w);
	dst_rect.h = lroundf(dst_h);
	
	gavl_video_options_set_rectangles( options, &src_rect, &dst_rect );
	gavl_video_scaler_init( inst->video_scaler, &format_src, &format_dst );
}

int f0r_init()
{
  return 1;
}
void f0r_deinit()
{ /* empty */ }

void f0r_get_plugin_info( f0r_plugin_info_t* info )
{
	info->name = "Scale0Tilt";
	info->author = "Richard Spindler";
	info->plugin_type = F0R_PLUGIN_TYPE_FILTER;
	info->color_model = F0R_COLOR_MODEL_RGBA8888;
	info->frei0r_version = FREI0R_MAJOR_VERSION;
	info->major_version = 0; 
	info->minor_version = 1; 
	info->num_params =  8; 
	info->explanation = "Scales, Tilts and Crops an Image";

}
void f0r_get_param_info( f0r_param_info_t* info, int param_index )
{
	switch ( param_index ) {
		case 0:
			info->name = "Clip left";
			info->type = F0R_PARAM_DOUBLE;
			info->explanation = "";
			break;
		case 1:
			info->name = "Clip right";
			info->type = F0R_PARAM_DOUBLE;
			info->explanation = "";
			break;
		case 2:
			info->name = "Clip top";
			info->type = F0R_PARAM_DOUBLE;
			info->explanation = "";
			break;
		case 3:
			info->name = "Clip bottom";
			info->type = F0R_PARAM_DOUBLE;
			info->explanation = "";
			break;
		case 4:
			info->name = "Scale X";
			info->type = F0R_PARAM_DOUBLE;
			info->explanation = "";
			break;
		case 5:
			info->name = "Scale Y";
			info->type = F0R_PARAM_DOUBLE;
			info->explanation = "";
			break;
		case 6:
			info->name = "Tilt X";
			info->type = F0R_PARAM_DOUBLE;
			info->explanation = "";
			break;
		case 7:
			info->name = "Tilt Y";
			info->type = F0R_PARAM_DOUBLE;
			info->explanation = "";
			break;
	}
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
	scale0tilt_instance_t* inst = (scale0tilt_instance_t*)malloc(sizeof(scale0tilt_instance_t));
	inst->w = width;
	inst->h = height;
	inst->cl = 0.0;
	inst->cr = 0.0;
	inst->ct = 0.0;
	inst->cb = 0.0;
	inst->tx = 0.0;
	inst->ty = 0.0;
	inst->sx = 1.0;
	inst->sy = 1.0;
	inst->video_scaler = gavl_video_scaler_create();
	inst->frame_src = gavl_video_frame_create( 0 );
	inst->frame_dst = gavl_video_frame_create( 0 );
	inst->frame_src->strides[0] = width * 4;
	inst->frame_dst->strides[0] = width * 4;
	update_scaler(inst);
	return (f0r_instance_t)inst;
}
void f0r_destruct(f0r_instance_t instance)
{
	scale0tilt_instance_t* inst = (scale0tilt_instance_t*)instance;
	gavl_video_scaler_destroy(inst->video_scaler);
	gavl_video_frame_null( inst->frame_src );
	gavl_video_frame_destroy( inst->frame_src );
	gavl_video_frame_null( inst->frame_dst );
	gavl_video_frame_destroy( inst->frame_dst );
	free(instance);
}
void f0r_set_param_value(f0r_instance_t instance, 
                         f0r_param_t param, int param_index)
{
	scale0tilt_instance_t* inst = (scale0tilt_instance_t*)instance;
	switch ( param_index ) {
		case 0:
			inst->cl = *((double*)param);
			break;
		case 1:
			inst->cr = *((double*)param);
			break;
		case 2:
			inst->ct = *((double*)param);
			break;
		case 3:
			inst->cb = *((double*)param);
			break;
		case 4:
			inst->sx = *((double*)param) * 2.0;
			break;
		case 5:
			inst->sy = *((double*)param) * 2.0;
			break;
		case 6:
			inst->tx = *((double*)param) * 2.0 - 1.0;
			break;
		case 7:
			inst->ty = *((double*)param) * 2.0 - 1.0;
			break;
	}
	update_scaler( inst );
}
void f0r_get_param_value(f0r_instance_t instance,
                         f0r_param_t param, int param_index)
{
	scale0tilt_instance_t* inst = (scale0tilt_instance_t*)instance;
	switch ( param_index ) {
		case 0:
			*((double*)param) = inst->cl;
			break;
		case 1:
			*((double*)param) = inst->cr;
			break;
		case 2:
			*((double*)param) = inst->ct;
			break;
		case 3:
			*((double*)param) = inst->cb;
			break;
		case 4:
			*((double*)param) = inst->sx / 2.0;
			break;
		case 5:
			*((double*)param) = inst->sy / 2.0;
			break;
		case 6:
			*((double*)param) = (inst->tx + 1.0) / 2.0;
			break;
		case 7:
			*((double*)param) = (inst->ty + 1.0) / 2.0;
			break;
	}
}
void f0r_update(f0r_instance_t instance, double time,
                const uint32_t* inframe, uint32_t* outframe)
{
	scale0tilt_instance_t* inst = (scale0tilt_instance_t*)instance;
	inst->frame_src->planes[0] = (uint8_t *)inframe;
	inst->frame_dst->planes[0] = (uint8_t *)outframe;
	int len = inst->w * inst->h;
	int i;
	for ( i = 0; i < len; i++ ) {
		outframe[i] = 0;
	}
	gavl_video_scaler_scale( inst->video_scaler, inst->frame_src, inst->frame_dst );
}

