/* rgbsplit0r.c
 * Copyright (C) 2016 IDENT Software ~ http://identsoft.org
 * Inspired by the witch house and web culture
 * 
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

#include "frei0r.h"

typedef struct rgbsplit0r_instance
{
    unsigned int width;
    unsigned int height;
    unsigned int shiftX;
    unsigned int shiftY;

    uint32_t pxR, pxG, pxB;
} rgbsplit0r_instance_t;


inline static void rgbsplit0r_extract_color(uint32_t *pixelIn, uint32_t *pixelOut,
                                            short int colorIndex)
{
    uint8_t *pxIn  = (uint8_t *)pixelIn,
            *pxOut = (uint8_t *)pixelOut;

    switch (colorIndex)
    {
        case 0 :
            pxOut[0] = pxIn[0];
            pxOut[1] = 0;
            pxOut[2] = 0;
            break;

        case 1 :
            pxOut[1] = pxIn[1];
            pxOut[0] = 0;
            pxOut[2] = 0;
            break;

        case 2 :
            pxOut[2] = pxIn[2];
            pxOut[1] = 0;
            pxOut[0] = 0;
            break;
    }
}

int f0r_init()
{
    return 1;
}

void f0r_deinit()
{ /* no initialization required */ }

void f0r_get_plugin_info(f0r_plugin_info_t* rgbsplit0rInfo)
{
    rgbsplit0rInfo->name = "rgbsplit0r";
    rgbsplit0rInfo->author = "IDENT Software";
    rgbsplit0rInfo->plugin_type = F0R_PLUGIN_TYPE_FILTER;
    rgbsplit0rInfo->color_model = F0R_COLOR_MODEL_RGBA8888;
    rgbsplit0rInfo->frei0r_version = FREI0R_MAJOR_VERSION;
    rgbsplit0rInfo->major_version = 1;
    rgbsplit0rInfo->minor_version = 0;
    rgbsplit0rInfo->num_params =  2;
    rgbsplit0rInfo->explanation = "RGB splitting and shifting";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
    switch(param_index) {
        case 0:
        {
            info->name = "Vertical split distance";
            info->type = F0R_PARAM_DOUBLE;
            info->explanation = "How far should layers be moved vertically from each other";
            break;
        }

        case 1:
        {
            info->name = "Horizontal split distance";
            info->type = F0R_PARAM_DOUBLE;
            info->explanation = "How far should layers be moved horizontally from each other";
            break;
        }
    }
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
    rgbsplit0r_instance_t* inst = (rgbsplit0r_instance_t*)calloc(1, sizeof(*inst));
    inst->width = width; inst->height = height;
    inst->shiftY = 0;
    inst->shiftX = 0;

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
    rgbsplit0r_instance_t *inst = (rgbsplit0r_instance_t*)instance;

    switch (param_index)
    {

        case 0 : // vertical shift
        {
            // scale to [-1/16..1/16]
            double shiftY = *((double*)param) - 0.5;

            // Convert to range from 0 to one eighth of height
            shiftY = ((inst->height / 8) * shiftY);

            inst->shiftY = (unsigned int)shiftY;
            break;
        }

        case 1 : // horizontal shift
        {
            // scale to [-1/16..1/16]
            double shiftX = *((double*)param) - 0.5;
            
            // Convert to range from 0 to one eighth of width
            shiftX = ((inst->width / 8) * shiftX);

            inst->shiftX = (unsigned int)shiftX;
            break;
        }
    }
}

void f0r_get_param_value(f0r_instance_t instance,
			f0r_param_t param, int param_index)
{

    assert(instance);
    rgbsplit0r_instance_t *inst = (rgbsplit0r_instance_t*)instance;

    switch (param_index)
    {
        case 0 : // vertical shift
        {
            // convert plugin's param to frei0r range
            *((double*)param) = (inst->shiftY) / (inst->height / 8) + 0.5;
            break;
        }

        case 1 : // horizontal shift
        {
            // convert plugin's param to frei0r range
            *((double*)param) = (inst->shiftX) / (inst->width / 8) + 0.5;
            break;
        }
    }

}


void f0r_update(f0r_instance_t instance, double time,
		const uint32_t* inframe, uint32_t* outframe)
{
    assert(instance);
    rgbsplit0r_instance_t* inst = (rgbsplit0r_instance_t*)instance;
    unsigned int x, y;

    uint32_t* dst = outframe;
    const uint32_t* src = inframe;

    for (y = 0; y < inst->height; y++)
        for (x = 0; x < inst->width; x++)
        {

            // First make a blue layer shifted back
            if (((x - inst->shiftX) < inst->width) &&
                ((y - inst->shiftY) < inst->height))
            {
                rgbsplit0r_extract_color((uint32_t *)(src +
                    (x - inst->shiftX) +
                    (y - inst->shiftY)*inst->width),
                    &inst->pxB, 2);
            }
            else
                inst->pxB = 0;

            // The red layer is shifted forward
            if ((x + inst->shiftX < inst->width) &&
                (y + inst->shiftY < inst->height))
            {
                rgbsplit0r_extract_color((uint32_t *)(src +
                    (x + inst->shiftX) +
                    (y + inst->shiftY)*inst->width),
                    &inst->pxR, 0);
            }
            else
                inst->pxR = 0;

            // Green layer is on its place
            rgbsplit0r_extract_color((uint32_t *)(src + x + (y*inst->width)),
                    &inst->pxG, 1);

            // No need to save alpha because it will distort the resulting image
            *(dst + x + (y*inst->width)) = (inst->pxG | inst->pxB | inst->pxR);
        }
}

