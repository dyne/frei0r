/* glitch0r.c
 * Copyright (C) 2016 IDENT Software ~ http://identsoft.org
 * Inspired by the test video on frei0r.dune.org
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
#include <string.h>
#include <time.h>

#include "frei0r.h"
#include "frei0r/math.h"

/* cheap & fast randomizer (by Fukuchi Kentarou) */
static uint32_t randval;
inline static uint32_t fastrand() { return (randval=randval*1103515245+12345); };
static void fastsrand(uint32_t seed) { randval = seed; };

struct glitch0r_state // helps to save time when allocating in a loop
{
    unsigned int currentBlock;
    unsigned int currentPos;
    unsigned int currentY;
    unsigned int blkShift;

    uint32_t distortionSeed1;
    uint32_t distortionSeed2;

    short int howToDistort1;
    short int howToDistort2;
    short int passThisLine;
} g0r_state;

typedef struct glitch0r_instance
{
    unsigned int width;
    unsigned int height;

    unsigned int maxBlockSize;
    unsigned int maxBlockShift;

    short int colorGlitchIntensity;
    short int doColorDistortion;
    short int glitchChance;
} glitch0r_instance_t;


inline static unsigned int rnd (unsigned int min, unsigned int max)
{
    return fastrand() % (max - min + 1) + min;
}

inline static void glitch0r_state_reset(glitch0r_instance_t *inst)
{
    g0r_state.currentPos = 0;
    g0r_state.currentBlock = rnd(1, inst->maxBlockSize);
    g0r_state.blkShift = rnd(1, inst->maxBlockShift);
    g0r_state.passThisLine = (inst->glitchChance < rnd(1, 101)) ? 1 : 0;

    if (inst->doColorDistortion)
    {
        g0r_state.distortionSeed1 = rnd(0x00000000, 0xfffffffe);
        g0r_state.distortionSeed2 = rnd(0x00000000, 0xfffffffe);
        g0r_state.howToDistort1 = rnd (0, inst->colorGlitchIntensity);
        g0r_state.howToDistort2 = rnd (0, inst->colorGlitchIntensity);
    }
}

inline static void glitch0r_pixel_dist0rt (uint32_t *pixel,
            uint32_t distortionSeed, short int howToDistort)
{

    // Save alpha
    uint8_t *px = (uint8_t *)pixel;
    uint8_t alpha = px[3];

    // Choose from five levels of madness:
    switch (howToDistort)
    {
        case 0 : return; // ok, let this pixel live (just shift)

        case 1 : // lightest distortion: just invert
            *(pixel) = ~*(pixel);
            break;

        case 2 : // add some unneeded colors
            *(pixel) = distortionSeed | *(pixel);
            break;

        case 3 : // change some colors
            *(pixel) = distortionSeed ^ *(pixel);
             break;

        case 4 : // oh shi...
            *(pixel) = distortionSeed & *(pixel);
            break;
    }  

    // Restore alpha
    px[3] = alpha;
}

int f0r_init()
{
    fastsrand((uint32_t)time(0));
    return 1;
}

void f0r_deinit()
{ /* no initialization required */ }

void f0r_get_plugin_info(f0r_plugin_info_t* glitch0rInfo)
{
    glitch0rInfo->name = "Glitch0r";
    glitch0rInfo->author = "IDENT Software";
    glitch0rInfo->plugin_type = F0R_PLUGIN_TYPE_FILTER;
    glitch0rInfo->color_model = F0R_COLOR_MODEL_RGBA8888;
    glitch0rInfo->frei0r_version = FREI0R_MAJOR_VERSION;
    glitch0rInfo->major_version = 0; 
    glitch0rInfo->minor_version = 1; 
    glitch0rInfo->num_params =  4; 
    glitch0rInfo->explanation = "Adds glitches and block shifting";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
    switch(param_index) {
        case 0:
        {
            info->name = "Glitch frequency";
            info->type = F0R_PARAM_DOUBLE;
            info->explanation = "How frequently the glitch should be applied";
            break;
        }

        case 1:
        {
            info->name = "Block height";
            info->type = F0R_PARAM_DOUBLE;
            info->explanation = "Height range of the block that will be shifted/glitched";
            break;
        }

        case 2:
        {
            info->name = "Shift intensity";
            info->type = F0R_PARAM_DOUBLE;
            info->explanation = "How much we should move blocks when glitching";
            break;
        }

        case 3:
        {
            info->name = "Color glitching intensity";
            info->type = F0R_PARAM_DOUBLE;
            info->explanation = "How intensive should be color distortion";
            break;
        }
    }
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
    glitch0r_instance_t* inst = (glitch0r_instance_t*)calloc(1, sizeof(*inst));
    inst->width = width; inst->height = height;
    inst->maxBlockSize = (unsigned int)MAX(1, inst->height / 2);
    inst->maxBlockShift = (unsigned int)MAX(1, inst->width / 2);
    inst->colorGlitchIntensity = 3;
    inst->doColorDistortion = 1;

    glitch0r_state_reset(inst);

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
    glitch0r_instance_t * inst = (glitch0r_instance_t*)instance;

    switch (param_index)
    {

        case 0 : // glitch frequency
        {
            double glitchChance = *((double*)param);
            // convert frei0r range [0, 1] to range between 0 and 100
            glitchChance = (100 * glitchChance);
            inst->glitchChance = (short int)glitchChance;
            break;
        }

        case 1 : // block height
        {
            double mbHeight = *((double*)param);
            // convert frei0r range [0, 1] to range between 1 and maximal height
            mbHeight = (1 + (inst->height - 1) * mbHeight);
            inst->maxBlockSize = (unsigned int)mbHeight;

            // zero size is a bad idea
            if (inst->maxBlockSize == 0)
                inst->maxBlockSize = (unsigned int)MAX(1, inst->height / 2);
            break;
        }

        case 2 : // shift intensity
        {
            double intensity = *((double*)param);
            // convert frei0r range [0, 1] to range between 1 and maximal width
            intensity = (1 + (inst->width - 1) * intensity);
            inst->maxBlockShift = (unsigned int)intensity;

            // zero shift is a bad idea
            if (inst->maxBlockShift == 0)
                inst->maxBlockShift = (unsigned int)MAX(1, inst->width / 2);
            break;
        }

        case 3 : // color intensity
        {
            double intensity = *((double*)param);
            // convert frei0r range [0, 1] to range between 0 and 5
            intensity = (5 * intensity);
            inst->colorGlitchIntensity = (short int)intensity;

            if (inst->colorGlitchIntensity > 0)
            {
                inst->doColorDistortion = 1;
            }
            else
                inst->doColorDistortion = 0;

            break;
        }
    }
}

void f0r_get_param_value(f0r_instance_t instance,
			f0r_param_t param, int param_index)
{

    assert(instance);
    glitch0r_instance_t * inst = (glitch0r_instance_t*)instance;

    switch (param_index)
    {
        case 0 : // glitch frequency
        {
            // convert plugin's param to frei0r range
            *((double*)param) = (inst->glitchChance) / 100;
            break;
        }

        case 1 : // block height
        {
            // convert plugin's param to frei0r range
            *((double*)param) = (inst->maxBlockSize - 1.0) / MAX(1, inst->height - 1.0);
            break;
        }

        case 2 : // block shift intensity
        {
            *((double*)param) = (inst->maxBlockShift - 1.0) / MAX(1, inst->width - 1.0);
            break;
        }

        case 3 : // color glitch intensity
        {
            *((double*)param) = (inst->colorGlitchIntensity) / 5; // 5 levels of madness
            break;
        }
    }

}

void f0r_update(f0r_instance_t instance, double time,
		const uint32_t* inframe, uint32_t* outframe)
{
    assert(instance);
    glitch0r_instance_t* inst = (glitch0r_instance_t*)instance;
    unsigned int x, y;

    uint32_t* dst = outframe;
    const uint32_t* src = inframe;
    uint32_t *pixel; 

    g0r_state.currentBlock = rnd(1, inst->maxBlockSize);

    for (y = 0; y < inst->height; y++)
    {

        if (g0r_state.currentPos > g0r_state.currentBlock)
        {
            glitch0r_state_reset(inst);
        }
        else
            g0r_state.currentPos++;

        g0r_state.currentY = y*inst->width;
        pixel = dst + g0r_state.currentY;

        if (g0r_state.passThisLine)
        {
            memcpy((uint32_t *)(dst + g0r_state.currentY),
                   (uint32_t *)(src + g0r_state.currentY),
                   (inst->width) * sizeof(uint32_t));
            continue;
        }

        for (x = g0r_state.blkShift; x < (inst->width); x++)
        {
            *(pixel) = *(src + g0r_state.currentY + x);

            if (inst->doColorDistortion)
                    glitch0r_pixel_dist0rt(pixel,
                        g0r_state.distortionSeed1, g0r_state.howToDistort1);

            pixel++;
        }

        for (x = 0; x < g0r_state.blkShift; x++)
        {
            *(pixel) = *(src + g0r_state.currentY + x);

            if (inst->doColorDistortion)
                    glitch0r_pixel_dist0rt(pixel,
                        g0r_state.distortionSeed2, g0r_state.howToDistort2);

            pixel++;
        }

    }
}

