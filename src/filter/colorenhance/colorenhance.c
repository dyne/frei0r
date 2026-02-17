/*
 * colorenhance.c -- enhance the color contrast for vivid pictures
 *
 * Copyright (C) 2026 Cynthia (cynthia2048@proton.me)
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

#include <frei0r.h>
#include <frei0r/math.h>

#include "oklab.h"

typedef struct
{
    unsigned int width, height;
    uint8_t a_lut[256], 
            b_lut[256];
    float a_base, a_slope, 
          b_base, b_slope;
} colorenhance_t;

static void make_sigmoid_lut(uint8_t *lut, float base, float steep)
{
  float k = expf(steep * 5.0) / 255.0;
  float b = (base - 0.5) * 63.0;

  for (int i = 0; i < 256; ++i)
    lut[i] = CLAMP (255.0 / (1.0 + expf(-k * (i - b - 127.0))), 0, 255.0);
}

int f0r_init(void) 
{
    return 0;
}

void f0r_deinit(void) {}

void f0r_get_plugin_info(f0r_plugin_info_t* sigmoidalInfo)
{
  sigmoidalInfo->name = "colorenhance";
  sigmoidalInfo->author = "Cynthia";
  sigmoidalInfo->plugin_type = F0R_PLUGIN_TYPE_FILTER;
  sigmoidalInfo->color_model = F0R_COLOR_MODEL_RGBA8888;
  sigmoidalInfo->frei0r_version = FREI0R_MAJOR_VERSION;
  sigmoidalInfo->major_version = 1;
  sigmoidalInfo->minor_version = 0;
  sigmoidalInfo->num_params = 4;
  sigmoidalInfo->explanation = "Enhances the color contrast in OkLab with sigmoidal transfers";
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
    colorenhance_t *inst = malloc(sizeof(colorenhance_t));

    if (inst == NULL)
        return NULL;

    inst->width   = width;
    inst->height  = height;
    inst->a_base  = 0.55;
    inst->a_slope = 0.4;
    inst->b_base  = 0.5;
    inst->b_slope = 0.4;

    make_sigmoid_lut(inst->a_lut, inst->a_base, inst->a_slope);
    make_sigmoid_lut(inst->b_lut, inst->b_base, inst->b_slope);
    return inst;
}

void f0r_destruct(f0r_instance_t instance)
{
  free(instance);
}

void f0r_get_param_info(f0r_param_info_t* info, int index)
{
	switch (index) {
		case 0:
            info->name = "a_base";
            info->type = F0R_PARAM_DOUBLE;
            info->explanation = "Midpoint of sigmoidal curve for A channel";
		break;
        case 1:
            info->name = "b_base";
            info->type = F0R_PARAM_DOUBLE;
            info->explanation = "Midpoint of sigmoidal curve for B channel";
        break;
        case 2:
            info->name = "a_slope";
            info->type = F0R_PARAM_DOUBLE;
            info->explanation = "Steepness of the sigmoidal curve for A channel";
        break;
        case 3:
            info->name = "b_slope";
            info->type = F0R_PARAM_DOUBLE;
            info->explanation = "Steepness of the sigmoidal curve for B channel";
        break;
	}
}

void f0r_set_param_value(f0r_instance_t instance, f0r_param_t param, int index)
{
	colorenhance_t* inst = (colorenhance_t*)instance;
	switch (index)
    {
        case 0:
            inst->a_base = *((double*)param);
            make_sigmoid_lut(inst->a_lut, inst->a_base, inst->a_slope);
        break;
        case 1:
            inst->b_base = *((double*)param);
            make_sigmoid_lut(inst->b_lut, inst->b_base, inst->b_slope);
        break;
        case 2:
            inst->a_slope = *((double*)param);
            make_sigmoid_lut(inst->a_lut, inst->a_base, inst->a_slope);
        break;
        case 3:
            inst->b_slope = *((double*)param);
            make_sigmoid_lut(inst->b_lut, inst->b_base, inst->b_slope);
        break;
    }
}

void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index)
{
	colorenhance_t* inst = (colorenhance_t*)instance;
	switch (param_index) 
    {
        case 0:
            *((double*)param) = inst->a_base;
        break;
        case 1:
            *((double*)param) = inst->b_base;
        break;
        case 2:
            *((double*)param) = inst->a_slope;
        break;
        case 3:
            *((double*)param) = inst->b_slope;
        break;
    }
}

void f0r_update(f0r_instance_t instance, double, const uint32_t* inframe, uint32_t* outframe) 
{
    colorenhance_t* inst = (colorenhance_t*)instance;
    const uint8_t* input = (const uint8_t*)inframe;
    uint8_t* output = (uint8_t*)outframe;

    long size = (long)inst->width * inst->height;
    while (size--)
    {
        uint8_t r, g, b;
        ciexyz_t u;
        oklab_t v;

        u.x = gamma_expand_table[*input++];
        u.y = gamma_expand_table[*input++];
        u.z = gamma_expand_table[*input++];

        #ifdef __SSE4_1__
        ciexyz_to_oklab_sse4_1(&u, &v);
        #else
        v = ciexyz_to_oklab(u);
        #endif

        v.l = CLAMP0255(v.l);
        v.a = inst->a_lut[CLAMP0255(v.a)];
        v.b = inst->b_lut[CLAMP0255(v.b)];

        #ifdef __SSE4_1__
        oklab_to_ciexyz_sse4_1(&v, &u);
        #else
        u = oklab_to_ciexyz(v);
        #endif

        r = CLAMP0255(u.x);
        g = CLAMP0255(u.y);
        b = CLAMP0255(u.z);

        *output++ = gamma_compress_table[r];
        *output++ = gamma_compress_table[g];
        *output++ = gamma_compress_table[b];
        *output++ = *input++;
    }
}
