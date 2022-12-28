/* colordistance.c
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

#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "frei0r.h"
#include "frei0r_math.h"

typedef struct colordistance_instance
{
	unsigned int width;
	unsigned int height;
	f0r_param_color_t color;
} colordistance_instance_t;

int f0r_init()
{
	return 1;
}

void f0r_deinit()
{ /* no initialization required */ }

void f0r_get_plugin_info(f0r_plugin_info_t* colordistance_info)
{
	colordistance_info->name = "Color Distance";
	colordistance_info->author = "Richard Spindler";
	colordistance_info->plugin_type = F0R_PLUGIN_TYPE_FILTER;
	colordistance_info->color_model = F0R_COLOR_MODEL_RGBA8888;
	colordistance_info->frei0r_version = FREI0R_MAJOR_VERSION;
	colordistance_info->major_version = 0; 
	colordistance_info->minor_version = 2; 
	colordistance_info->num_params = 1; 
	colordistance_info->explanation = "Calculates the distance between the selected color and the current pixel and uses that value as new pixel value";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
	switch(param_index)
	{
		case 0:
			info->name = "Color";
			info->type = F0R_PARAM_COLOR;
			info->explanation = "The Source Color";
			break;
	}

}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
	colordistance_instance_t* inst = (colordistance_instance_t*)calloc(1, sizeof(*inst));
	inst->width = width; inst->height = height;
	inst->color.r = 0.5;
	inst->color.g = 0.5;
	inst->color.b = 0.5;
	return (f0r_instance_t)inst;
}

void f0r_destruct(f0r_instance_t instance)
{
	free(instance);
}

void f0r_set_param_value(f0r_instance_t instance, 
		f0r_param_t param, int param_index)
{
	assert(instance);
	colordistance_instance_t* inst = (colordistance_instance_t*)instance;

	switch(param_index) {
		case 0:
			inst->color = *((f0r_param_color_t*)param);
			break;
	}

}

void f0r_get_param_value(f0r_instance_t instance,
		f0r_param_t param, int param_index)
{
	assert(instance);
	colordistance_instance_t* inst = (colordistance_instance_t*)instance;

	switch(param_index) {
		case 0:
			*((f0r_param_color_t*)param) = inst->color;
			break;
	}

}

void f0r_update(f0r_instance_t instance, double time,
		const uint32_t* inframe, uint32_t* outframe)
{
	assert(instance);
	colordistance_instance_t* inst = (colordistance_instance_t*)instance;
	unsigned int len = inst->width * inst->height;


	unsigned char* dst = (unsigned char*)outframe;
	const unsigned char* src = (unsigned char*)inframe;

	float r1 = inst->color.r * 255.0;
	float g1 = inst->color.g * 255.0;
	float b1 = inst->color.b * 255.0;
	float r2, g2, b2;
	int l;
	while (len--) {
		r2 = *src++;
		g2 = *src++;
		b2 = *src++;
		l = (int)rint( sqrtf( powf( r1 - r2, 2 ) + powf( g1 - g2, 2 ) + powf( b1 - b2, 2 ) ) * 0.705724361914764  );
		/* Hint 0.35320727852735 == 255.0 / sqrt( (255)**2 + (255)**2 + (255)*2 )*/
		if ( r1 < 0 || r1 > 255 ||  g1 < 0 || g1 > 255 ||  b1 < 0 || b1	> 255 || r2 < 0 || r2 > 255 ||  g2 < 0 || g2 > 255 || b2 < 0 || b2 > 255 ) {
			printf ("%f %f %f\n", r2, g2, b2 );
		}


		*dst++ = (unsigned char) (l);
		*dst++ = (unsigned char) (l);
		*dst++ = (unsigned char) (l);

		*dst++ = *src++;  // copy alpha
	}
}

