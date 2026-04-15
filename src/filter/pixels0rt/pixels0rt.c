/* pixels0rt.c
 * Copyright (C) 2026 Asterleen ~ https://asterleen.com
 * Implements a pixel sorting effect
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
#include <stdio.h>

#include "frei0r.h"

#define S0RT_PARAM_THRESHOLD 0
#define S0RT_PARAM_DIRECTION 1
#define S0RT_PARAM_REVERSED  2
#define S0RT_PARAM_ALPHA_THR 3

typedef enum pixels0rt_direction
{
    S0RT_DIRECTION_VER_TOP,    // vertical, from top to bottom
    S0RT_DIRECTION_VER_BOTTOM, // vertical, from bottom to top
    S0RT_DIRECTION_HOR_LEFT,   // horizontal, from left to right
    S0RT_DIRECTION_HOR_RIGHT,  // horizontal, from right to left

    S0RT_DIRECTION_COUNT       // total number of possible directions

} pixels0rt_direction_t;


typedef struct pixels0rt_instance
{
    unsigned int width;
    unsigned int height;

    uint8_t *grayscale_px; // the frame in grayscale, will be used for radix sort

    float threshold;
    float alpha_threshold;
    short int reversed;
    pixels0rt_direction_t direction;

} pixels0rt_instance_t;

inline static uint8_t pixel_grayscale(uint32_t px)
{
    uint8_t *pxSplit = (uint8_t *)&px;

    return ((pxSplit[0] & 0xFF) + 
            (pxSplit[1] & 0xFF) +
            (pxSplit[2] & 0xFF)) / 3;
}

inline static short pixel_alpha_threshold(uint32_t px, float threshold)
{
    uint8_t *pxSplit = (uint8_t *)&px;

    return pxSplit[3] / 0xFF >= threshold;
}

inline static double pixel_diff(uint32_t px1, uint32_t px2)
{
    uint8_t *px1_split = (uint8_t *)&px1;
    uint8_t *px2_split = (uint8_t *)&px2;

    return (
        abs(px1_split[0] - px2_split[0]) +
        abs(px1_split[1] - px2_split[1]) +
        abs(px1_split[2] - px2_split[2])
    ) / 765.0;
}

int f0r_init()
{
    return 1;
}

void f0r_deinit()
{ /* no initialization required */ }

void f0r_get_plugin_info(f0r_plugin_info_t* pixels0rtInfo)
{
    pixels0rtInfo->name = "pixels0rt";
    pixels0rtInfo->author = "Asterleen";
    pixels0rtInfo->plugin_type = F0R_PLUGIN_TYPE_FILTER;
    pixels0rtInfo->color_model = F0R_COLOR_MODEL_RGBA8888;
    pixels0rtInfo->frei0r_version = FREI0R_MAJOR_VERSION;
    pixels0rtInfo->major_version = 1;
    pixels0rtInfo->minor_version = 0;
    pixels0rtInfo->num_params =  4;
    pixels0rtInfo->explanation = "Pixel sorting effect";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
    switch(param_index) {
        case S0RT_PARAM_THRESHOLD:
        {
            info->name = "Threshold";
            info->type = F0R_PARAM_DOUBLE;
            info->explanation = "Relative size of the line that will be sorted";
            break;
        }

        case S0RT_PARAM_DIRECTION:
        {
            info->name = "Direction";
            info->type = F0R_PARAM_DOUBLE;
            info->explanation = "Sorting direction";
            break;
        }

        case S0RT_PARAM_REVERSED:
        {
            info->name = "Reversed";
            info->type = F0R_PARAM_BOOL;
            info->explanation = "Should be pixels sorted in reversed order";
            break;
        }

        case S0RT_PARAM_ALPHA_THR:
        {
            info->name = "Transparency threshold";
            info->type = F0R_PARAM_DOUBLE;
            info->explanation = "What transparency level should be bypassed by the filter. 0 processes all pixels, 1 ignores all transparent pixels";
            break;
        }
    }
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
    pixels0rt_instance_t* inst = (pixels0rt_instance_t*)calloc(1, sizeof(*inst));
    inst->width = width;
    inst->height = height;
    inst->threshold = 0.3;
    inst->alpha_threshold = 1.0;
    inst->direction = S0RT_DIRECTION_VER_TOP;
    inst->reversed = 0;

    inst->grayscale_px = malloc(width * height * sizeof(uint8_t));
    
    return (f0r_instance_t)inst;
}

void f0r_destruct(f0r_instance_t instance)
{
    pixels0rt_instance_t *inst = (pixels0rt_instance_t*)instance;

    free(inst->grayscale_px);

    free(instance);
}

void f0r_set_param_value(f0r_instance_t instance,
			 f0r_param_t param, int param_index)
{
    assert(instance);
    pixels0rt_instance_t *inst = (pixels0rt_instance_t*)instance;

    switch (param_index)
    {
        case S0RT_PARAM_THRESHOLD:
        {
            inst->threshold = *((double*)param);
            break;
        }

        case S0RT_PARAM_DIRECTION:
        {
            inst->direction = (pixels0rt_direction_t)(*((double*)param) * S0RT_DIRECTION_COUNT);
            break;
        }

        case S0RT_PARAM_REVERSED:
        {
            inst->reversed = *((double*)param) >= 0.5;
            break;
        }

        case S0RT_PARAM_ALPHA_THR:
        {
            inst->alpha_threshold = *((double*)param);
            break;
        }
    }
}

void f0r_get_param_value(f0r_instance_t instance,
			f0r_param_t param, int param_index)
{

    assert(instance);
    pixels0rt_instance_t *inst = (pixels0rt_instance_t*)instance;

    switch (param_index)
    {
        case S0RT_PARAM_THRESHOLD:
        {
            *((double*)param) = inst->threshold;
            break;
        }

        case S0RT_PARAM_DIRECTION:
        {
            *((double*)param) = inst->direction / S0RT_DIRECTION_COUNT;
            break;
        }

        case S0RT_PARAM_REVERSED:
        {
            *((double*)param) = inst->reversed ? 1. : 0.;
            break;
        }

        case S0RT_PARAM_ALPHA_THR:
        {
            *((double*)param) = inst->alpha_threshold;
            break;
        }
    }

}

inline static void s0rt_line_horizontal(
    const uint32_t *src,
    uint32_t *dst, 
    uint8_t *gray,
    unsigned int line,
    unsigned int start,
    unsigned int end) 
{
    unsigned int length = end - start;
    unsigned int line_start = line + start;
    
    uint32_t histogram[256] = {0};

    for (int i = 0; i < length; i++)
        histogram[gray[line_start + i]]++;

    for (int i = 1; i < 256; i++)
        histogram[i] += histogram[i - 1];

    for (int i = length; i-- > 0;)
    {
        uint8_t key = gray[line_start + i];
        dst[line_start + (--histogram[key])] = src[line_start + i];
    }
}

inline static void s0rt_line_vertical(
    const uint32_t *src,
    uint32_t *dst, 
    uint8_t *gray,
    unsigned int width,
    unsigned int column,
    unsigned int start,
    unsigned int end) 
{
    unsigned int length = end - start;
    
    uint32_t histogram[256] = {0};

    for (int i = 0; i < length; i++)
        histogram[gray[(start + i) * width + column]]++;

    for (int i = 1; i < 256; i++)
        histogram[i] += histogram[i - 1];

    for (int i = length; i-- > 0;)
    {
        uint8_t key = gray[(start + i) * width + column];
        dst[(start + (--histogram[key])) * width + column] = src[(start + i) * width + column];
    }
}


void f0r_update(f0r_instance_t instance, double time,
		const uint32_t* src, uint32_t* dst)
{
    assert(instance);
    pixels0rt_instance_t* inst = (pixels0rt_instance_t*)instance;

    unsigned int size = inst->width * inst->height;

    double px_diff = 0.0;
    unsigned int line_idx;
    unsigned int next_pos = 0;

    if (inst->reversed) {
        for (int i = 0; i < size; i++)
            inst->grayscale_px[i] = 255 - pixel_grayscale(src[i]);
    } else {
        for (int i = 0; i < size; i++)
            inst->grayscale_px[i] = pixel_grayscale(src[i]);
    }

    switch (inst->direction) {
        case S0RT_DIRECTION_VER_TOP:
        {
            for (int column = 0; column < inst->width; column++) {
                next_pos = 0;

                for (int i = 0; i < inst->height; i++) {
                    if (pixel_alpha_threshold(src[i * inst->width + column], inst->alpha_threshold)) {
                        px_diff = pixel_diff(src[next_pos * inst->width + column], src[i * inst->width + column]);

                        if (px_diff > inst->threshold ||
                            (i < inst->height - 2 && !pixel_alpha_threshold(src[(i + 1) * inst->width + column], inst->alpha_threshold)) ||
                            i == inst->height - 1) {
                            s0rt_line_vertical(src, dst, inst->grayscale_px, inst->width, column, next_pos, i);
                            next_pos = i;
                        }
                    } else {
                        dst[i * inst->width + column] = src[i * inst->width + column];
                        next_pos = i;
                    }
                }
            }

            break;
        }

        case S0RT_DIRECTION_VER_BOTTOM:
        {
            for (int column = 0; column < inst->width; column++) {
                next_pos = inst->height - 1;

                for (int i = inst->height - 1; i >= 0; i--) {

                    if (pixel_alpha_threshold(src[i * inst->width + column], inst->alpha_threshold)) {
                        px_diff = pixel_diff(src[next_pos * inst->width + column], src[i * inst->width + column]);

                        if (px_diff > inst->threshold ||
                            (i > 0 && !pixel_alpha_threshold(src[(i - 1) * inst->width + column], inst->alpha_threshold)) ||
                            i == 0) {
                            s0rt_line_vertical(src, dst, inst->grayscale_px, inst->width, column, i, next_pos);
                            next_pos = i;
                        }
                    } else {
                        dst[i * inst->width + column] = src[i * inst->width + column];
                        next_pos = i;
                    }
                }
            }

            break;
        }


        case S0RT_DIRECTION_HOR_LEFT:
        {
            for (int line = 0; line < inst->height; line++) {
                next_pos = 0;
                line_idx = line * inst->width;

                for (int i = 0; i < inst->width; i++) {
                    if (pixel_alpha_threshold(src[line_idx + i], inst->alpha_threshold)) {
                        px_diff = pixel_diff(src[line_idx + next_pos], src[line_idx + i]);

                        if (px_diff > inst->threshold ||
                            (i < inst->width - 2 && !pixel_alpha_threshold(src[line_idx + i + 1], inst->alpha_threshold)) ||
                            i == inst->width - 1) {
                            s0rt_line_horizontal(src, dst, inst->grayscale_px, line_idx, next_pos, i);
                            next_pos = i;
                        }
                    } else {
                        dst[line_idx + i] = src[line_idx + i];
                        next_pos = i;
                    }
                }
            }

            break;
        }

        case S0RT_DIRECTION_HOR_RIGHT:
        {
            for (int line = 0; line < inst->height; line++) {
                next_pos = inst->width - 1;
                line_idx = line * inst->width;

                for (int i = inst->width - 1; i >= 0; i--) {
                    if (pixel_alpha_threshold(src[line_idx + i], inst->alpha_threshold)) {
                        px_diff = pixel_diff(src[line_idx + next_pos], src[line_idx + i]);

                        if (px_diff > inst->threshold ||
                            (i > 0 && !pixel_alpha_threshold(src[line_idx + i - 1], inst->alpha_threshold)) ||
                            i == 0) {
                            s0rt_line_horizontal(src, dst, inst->grayscale_px, line_idx, i, next_pos);
                            next_pos = i;
                        }
                    } else {
                        dst[line_idx + i] = src[line_idx + i];
                        next_pos = i;
                    }
                }
            }

            break;
        }
    }
}
