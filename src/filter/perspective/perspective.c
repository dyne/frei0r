/* perspective.c
 * Copyright (C) 2008 Richard Spindler (richard.spindler@gmail.com)
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


#include "frei0r.h"


void sub_vec2( f0r_param_position_t* r, f0r_param_position_t* a, f0r_param_position_t* b ) 
{
	r->x = a->x - b->x;
	r->y = a->y - b->y;
}
void add_vec2( f0r_param_position_t* r, f0r_param_position_t* a, f0r_param_position_t* b ) 
{
	r->x = a->x + b->x;
	r->y = a->y + b->y;
}

void mul_vec2( f0r_param_position_t* r, f0r_param_position_t* a, double scalar )
{
	r->x = a->x * scalar;
	r->y = a->y * scalar;
}

void get_pixel_position( f0r_param_position_t* r, f0r_param_position_t* t, f0r_param_position_t* b, f0r_param_position_t* tl, f0r_param_position_t* bl,f0r_param_position_t* in )
{
	f0r_param_position_t t_x;
	f0r_param_position_t b_x;
	f0r_param_position_t k;
	mul_vec2( &t_x, t, in->x );
	mul_vec2( &b_x, b, in->x );

	add_vec2( &t_x, &t_x, tl );
	add_vec2( &b_x, &b_x, bl );

	sub_vec2( &k, &b_x, &t_x );
	mul_vec2( &k, &k, in->y );

	add_vec2( r, &k, &t_x );

}


#include <math.h>
#include <stdlib.h>

typedef struct perspective_instance {
	int w, h;
	f0r_param_position_t tl;
	f0r_param_position_t tr;
	f0r_param_position_t bl;
	f0r_param_position_t br;
} perspective_instance_t;


int f0r_init()
{
  return 1;
}
void f0r_deinit()
{ /* empty */ }

void f0r_get_plugin_info( f0r_plugin_info_t* info )
{
	info->name = "Perspective";
	info->author = "Richard Spindler";
	info->plugin_type = F0R_PLUGIN_TYPE_FILTER;
	info->color_model = F0R_COLOR_MODEL_RGBA8888;
	info->frei0r_version = FREI0R_MAJOR_VERSION;
	info->major_version = 0; 
	info->minor_version = 1; 
	info->num_params =  4; 
	info->explanation = "Distorts the image for a pseudo perspective";

}
void f0r_get_param_info( f0r_param_info_t* info, int param_index )
{
	switch ( param_index ) {
		case 0:
			info->name = "Top Left";
			info->type = F0R_PARAM_POSITION;
			info->explanation = "";
			break;
		case 1:
			info->name = "Top Right";
			info->type = F0R_PARAM_POSITION;
			info->explanation = "";
			break;
		case 2:
			info->name = "Bottom Left";
			info->type = F0R_PARAM_POSITION;
			info->explanation = "";
			break;
		case 3:
			info->name = "Bottom Right";
			info->type = F0R_PARAM_POSITION;
			info->explanation = "";
			break;
	}
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
	perspective_instance_t* inst = (perspective_instance_t*)calloc(1, sizeof(*inst));
	inst->w = width;
	inst->h = height;
	inst->tl.x = 0.0;
	inst->tl.y = 0.0;
	inst->tr.x = 1.0;
	inst->tr.y = 0.0;
	inst->bl.x = 0.0;
	inst->bl.y = 1.0;
	inst->br.x = 1.0;
	inst->br.y = 1.0;
	return (f0r_instance_t)inst;
}
void f0r_destruct(f0r_instance_t instance)
{
	perspective_instance_t* inst = (perspective_instance_t*)instance;
	free(inst);
}
void f0r_set_param_value(f0r_instance_t instance, 
                         f0r_param_t param, int param_index)
{
	perspective_instance_t* inst = (perspective_instance_t*)instance;
	switch ( param_index ) {
		case 0:
			inst->tl = *((f0r_param_position_t*)param);
			break;
		case 1:
			inst->tr = *((f0r_param_position_t*)param);
			break;
		case 2:
			inst->bl = *((f0r_param_position_t*)param);
			break;
		case 3:
			inst->br = *((f0r_param_position_t*)param);
			break;
		}
}
void f0r_get_param_value(f0r_instance_t instance,
                         f0r_param_t param, int param_index)
{
	perspective_instance_t* inst = (perspective_instance_t*)instance;
	switch ( param_index ) {
		case 0:
			*((f0r_param_position_t*)param) = inst->tl;
			break;
		case 1:
			*((f0r_param_position_t*)param) = inst->tr;
			break;
		case 2:
			*((f0r_param_position_t*)param) = inst->bl;
			break;
		case 3:
			*((f0r_param_position_t*)param) = inst->br;
			break;
	}
}

void f0r_update(f0r_instance_t instance, double time,
                const uint32_t* inframe, uint32_t* outframe)
{
	perspective_instance_t* inst = (perspective_instance_t*)instance;

	uint32_t* dst = outframe;
	const uint32_t* src = inframe;
	int len = inst->w * inst->h;
	
	int i;
	for ( i = 0; i < len; i++ ) {
		*dst = 0x00000000;
		dst++;
	}
	dst = outframe;

	int w = inst->w;
	int h = inst->h;
	
	int rx;
	int ry;
	int x;
	int y;
	f0r_param_position_t top;
	f0r_param_position_t bot;
	f0r_param_position_t r;
	f0r_param_position_t in;
	sub_vec2( &top, &inst->tr, &inst->tl );
	sub_vec2( &bot, &inst->br, &inst->bl );
	for( y = 0; y < h; y++ ) {
		for ( x = 0; x < w; x++ ) {
			in.x = (double)x / (double)w;
			in.y = (double)y / (double)h;
			get_pixel_position( &r, &top, &bot, &inst->tl, &inst->bl, &in );
			rx = lrint(r.x * (float)w);
			ry = lrint(r.y * (float)h);
			if ( rx < 0 || rx >= w || ry < 0 || ry >= h ) {
				src++;
				continue;
			}
			dst[rx + w * ry] = *src;
			src++;
		}
	}

}

