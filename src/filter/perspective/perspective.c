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


static void sub_vec2( f0r_param_position_t* r, f0r_param_position_t* a, f0r_param_position_t* b )
{
	r->x = a->x - b->x;
	r->y = a->y - b->y;
}
static void add_vec2( f0r_param_position_t* r, f0r_param_position_t* a, f0r_param_position_t* b )
{
	r->x = a->x + b->x;
	r->y = a->y + b->y;
}

static void mul_vec2( f0r_param_position_t* r, f0r_param_position_t* a, double scalar )
{
	r->x = a->x * scalar;
	r->y = a->y * scalar;
}

static void get_pixel_position( f0r_param_position_t* r, f0r_param_position_t* t, f0r_param_position_t* b, f0r_param_position_t* tl, f0r_param_position_t* bl,f0r_param_position_t* in )
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
#include <string.h>

typedef struct perspective_instance {
	int w, h;
	f0r_param_position_t tl;
	f0r_param_position_t tr;
	f0r_param_position_t bl;
	f0r_param_position_t br;
	int32_t *map;
	int map_dirty;
} perspective_instance_t;


static void rebuild_map(perspective_instance_t* inst)
{
	int w = inst->w;
	int h = inst->h;
	int32_t *map = inst->map;
	f0r_param_position_t top;
	f0r_param_position_t bot;
	f0r_param_position_t r;
	f0r_param_position_t in;
	double dw = (double)w;
	double dh = (double)h;
	float fw = (float)w;
	float fh = (float)h;
	int x, y;

	sub_vec2( &top, &inst->tr, &inst->tl );
	sub_vec2( &bot, &inst->br, &inst->bl );

	for ( y = 0; y < h; y++ ) {
		in.y = (double)y / dh;
		int32_t *row = map + (size_t)y * w;
		for ( x = 0; x < w; x++ ) {
			in.x = (double)x / dw;
			get_pixel_position( &r, &top, &bot, &inst->tl, &inst->bl, &in );
			int rx = (int)lrint(r.x * fw);
			int ry = (int)lrint(r.y * fh);
			if ( rx < 0 || rx >= w || ry < 0 || ry >= h ) {
				row[x] = -1;
			} else {
				row[x] = rx + w * ry;
			}
		}
	}
	inst->map_dirty = 0;
}

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
	inst->map = (int32_t*)malloc((size_t)width * height * sizeof(int32_t));
	inst->map_dirty = 1;
	return (f0r_instance_t)inst;
}
void f0r_destruct(f0r_instance_t instance)
{
	perspective_instance_t* inst = (perspective_instance_t*)instance;
	free(inst->map);
	free(inst);
}
void f0r_set_param_value(f0r_instance_t instance, 
                         f0r_param_t param, int param_index)
{
	perspective_instance_t* inst = (perspective_instance_t*)instance;
	f0r_param_position_t p = *((f0r_param_position_t*)param);
	switch ( param_index ) {
		case 0:
			if (inst->tl.x != p.x || inst->tl.y != p.y) { inst->tl = p; inst->map_dirty = 1; }
			break;
		case 1:
			if (inst->tr.x != p.x || inst->tr.y != p.y) { inst->tr = p; inst->map_dirty = 1; }
			break;
		case 2:
			if (inst->bl.x != p.x || inst->bl.y != p.y) { inst->bl = p; inst->map_dirty = 1; }
			break;
		case 3:
			if (inst->br.x != p.x || inst->br.y != p.y) { inst->br = p; inst->map_dirty = 1; }
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

	memset(dst, 0, (size_t)len * sizeof(uint32_t));

	if (inst->map_dirty) {
		rebuild_map(inst);
	}

	const int32_t *map = inst->map;
	int i;
	for ( i = 0; i < len; i++ ) {
		int32_t idx = map[i];
		if ( idx >= 0 ) {
			dst[idx] = src[i];
		}
	}
}
