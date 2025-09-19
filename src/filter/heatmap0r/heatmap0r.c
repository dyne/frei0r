/*
 * heatmap0r.c -- feel the heat, frei0r style.
 *
 * Copyright (C) 2025 Cynthia (cynthia2048@proton.me)
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

#include "frei0r.h"
#include "frei0r/math.h"

typedef struct {
    unsigned int width, height;
    uint8_t rlut[256], glut[256], blut[256];

    f0r_param_color_t black, grey, white;
    double gp; //< Grey point
} heatmap0r_t;

static void gen_heatmap0r_luts(heatmap0r_t *inst)
{
    double luma;
    double L0, L1, L2;
    double gp = inst->gp;
    double l0 = 1.0 / gp,
           l1 = 1.0 / (gp * (gp - 1.0)),
           l2 = 1.0 / (1.0 - gp);

    f0r_param_color_t black = inst->black,
                      grey = inst->grey,
                      white = inst->white;

    for (int i = 0; i < 256; ++i)
    {
        luma = 1.0 / 255.0 * (double)i;

        L0 = (luma - gp) * (luma - 1.0) * l0;
        L1 = luma * (luma - 1.0) * l1;
        L2 = luma * (luma - gp) * l2;

        inst->rlut[i] = CLAMP(255.0 * (L0 * black.r + L1 * grey.r + L2 * white.r), 0, 255);
        inst->glut[i] = CLAMP(255.0 * (L0 * black.g + L1 * grey.g + L2 * white.g), 0, 255);
        inst->blut[i] = CLAMP(255.0 * (L0 * black.b + L1 * grey.b + L2 * white.b), 0, 255);
    }
}

int f0r_init()
{
    return 1;
}

void f0r_deinit() {}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
    heatmap0r_t* inst = calloc(1, sizeof(*inst));

    inst->width = width; inst->height = height;
    inst->gp = 0.5;
    inst->white = (f0r_param_color_t) {/* Yellow */     1.0,  1.0, 0.0};
    inst->grey  = (f0r_param_color_t) {/* Deeppink */   1.0,  0.0, 0.5};
    inst->black = (f0r_param_color_t) {/* Darkviolet */ 0.27, 0.0, 0.5};

    gen_heatmap0r_luts (inst);

    return (f0r_instance_t)inst;
}

void f0r_destruct(f0r_instance_t instance)
{
    free(instance);
}

void f0r_get_plugin_info(f0r_plugin_info_t* info)
{
    info->name = "heatmap0r";
    info->author = "Cynthia";
    info->plugin_type = F0R_PLUGIN_TYPE_FILTER;
    info->color_model = F0R_COLOR_MODEL_RGBA8888;
    info->frei0r_version = FREI0R_MAJOR_VERSION;
    info->major_version = 0;
    info->minor_version = 1;
    info->num_params = 4;
    info->explanation = "Performs trichromatic tinting";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
    switch(param_index)
    {
        case 0:
            info->name = "Map black to";
            info->type = F0R_PARAM_COLOR;
            info->explanation = "The color to map source color with zero luminance";
            break;
        case 1:
            info->name = "Map grey to";
            info->type = F0R_PARAM_COLOR;
            info->explanation = "The color to map source color with mid luminance";
            break;
        case 2:
            info->name = "Map white to";
            info->type = F0R_PARAM_COLOR;
            info->explanation = "The color to map source color with full luminance";
            break;
        case 3:
            info->name = "Grey point";
            info->type = F0R_PARAM_DOUBLE;
            info->explanation = "Point in the luminance axis grey color is located";
            break;
    }
}

void f0r_set_param_value(f0r_instance_t instance,
                         f0r_param_t param, int param_index)
{
    assert(instance);
    heatmap0r_t* inst = (heatmap0r_t*)instance;

    switch(param_index)
    {
        case 0:
            inst->black = *((f0r_param_color_t *)param);
            break;
        case 1:
            inst->grey = *((f0r_param_color_t *)param);
            break;
        case 2:
            inst->white = *((f0r_param_color_t *)param);
            break;
        case 3:
            inst->gp = *((double *)param);
            break;
    }

    gen_heatmap0r_luts (inst);
}

void f0r_get_param_value(f0r_instance_t instance,
                         f0r_param_t param, int param_index)
{
    assert(instance);
    heatmap0r_t* inst = (heatmap0r_t*)instance;

    switch(param_index)
    {
        case 0:
            *((f0r_param_color_t*)param) = inst->black;
            break;
        case 1:
            *((f0r_param_color_t*)param) = inst->grey;
            break;
        case 2:
            *((f0r_param_color_t*)param) = inst->white;
            break;
        case 3:
            *((double *)param) = inst->gp;
            break;
    }
}

void f0r_update(f0r_instance_t instance, double time,
                const uint32_t* inframe, uint32_t* outframe)
{
    assert(instance);
    heatmap0r_t* inst = (heatmap0r_t*)instance;
    unsigned int len = inst->width * inst->height;

    const unsigned char* src = (void*)inframe;
          unsigned char* dst = (void*)outframe;

    unsigned char luma, r, g, b;
    while (len--)
    {
        r = *src++;
        g = *src++;
        b = *src++;

        luma = 0.299 * r + 0.587 * g + 0.114 * b;

        *dst++ = inst->rlut[luma];
        *dst++ = inst->glut[luma];
        *dst++ = inst->blut[luma];
        *dst++ = *src++;
    }
}
