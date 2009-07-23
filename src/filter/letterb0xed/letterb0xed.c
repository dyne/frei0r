/* letterb0xed.c
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
#include <stdlib.h>
#include <stdio.h>

typedef struct letterb0xed_instance {
	double value;
	double bg_transparent;
	int w, h;
	int top, bottom;
	int len;
	uint32_t background;
} letterb0xed_instance_t;

int f0r_init()
{
  return 1;
}
void f0r_deinit()
{ /* empty */ }

void f0r_get_plugin_info( f0r_plugin_info_t* info )
{
	info->name = "LetterB0xed";
	info->author = "Richard Spindler";
	info->plugin_type = F0R_PLUGIN_TYPE_FILTER;
	info->color_model = F0R_COLOR_MODEL_RGBA8888;
	info->frei0r_version = FREI0R_MAJOR_VERSION;
	info->major_version = 0; 
	info->minor_version = 1; 
	info->num_params =  2; 
	info->explanation = "Adds Black Borders at top and bottom for Cinema Look";

}
void f0r_get_param_info( f0r_param_info_t* info, int param_index )
{
	switch ( param_index ) {
		case 0:
			info->name = "Border Width";
			info->type = F0R_PARAM_DOUBLE;
			info->explanation = "";
			break;
		case 1:
			info->name = "Transparency";
			info->type = F0R_PARAM_BOOL;
			info->explanation = "";
			break;
	}
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
	letterb0xed_instance_t* inst = calloc(1, sizeof(*inst));
	inst->w = width;
	inst->h = height;
	inst->len = width * height;
	inst->value = 0.4;
	inst->bg_transparent = 0.0;
	inst->top = (int)( ( inst->h / 2 ) * inst->value );
	inst->bottom = inst->h - inst->top;
	inst->top *= inst->w;
	inst->bottom *= inst->w;
	inst->background = 0xFF000000;


	return (f0r_instance_t)inst;
}
void f0r_destruct(f0r_instance_t instance)
{
	letterb0xed_instance_t* inst = (letterb0xed_instance_t*)instance;
	free(inst);
}
void f0r_set_param_value(f0r_instance_t instance, 
                         f0r_param_t param, int param_index)
{
	letterb0xed_instance_t* inst = (letterb0xed_instance_t*)instance;
	switch ( param_index ) {
		case 0:
			inst->value = *((double*)param);
			break;
		case 1:
			inst->bg_transparent = *((double*)param);
			break;
	}
	inst->top = (int)( ( inst->h / 2 ) * inst->value );
	inst->bottom = inst->h - inst->top;
	inst->top *= inst->w;
	inst->bottom *= inst->w;
	inst->background = 0x00000000;
	if ( inst->bg_transparent < 0.5 ) {
		((uint8_t*)(&inst->background))[3] = 0xFF;
	}
}
void f0r_get_param_value(f0r_instance_t instance,
                         f0r_param_t param, int param_index)
{
	letterb0xed_instance_t* inst = (letterb0xed_instance_t*)instance;
	switch ( param_index ) {
		case 0:
			*((double*)param) = inst->value;
			break;
		case 1:
			*((double*)param) = inst->bg_transparent;
			break;
	}
}
void f0r_update(f0r_instance_t instance, double time,
                const uint32_t* inframe, uint32_t* outframe)
{
	letterb0xed_instance_t* inst = (letterb0xed_instance_t*)instance;
	int i;
	for ( i = 0; i < inst->top; i++ ) {
		outframe[i] = inst->background;
	}
	for ( i = inst->top; i < inst->bottom; i++ ) {
		outframe[i] = inframe[i];
	}
	for ( i = inst->bottom; i < inst->len; i++ ) {
		outframe[i] = inst->background;
	}
	
}

