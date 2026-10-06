/* harris0r.c
 * Copyright (C) 2026 Marko Deinterlace
 * Implements a Harris shutter effect
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

// Frei0r parameter indices
#define HARRIS_PARAM_DELAY  0
#define HARRIS_PARAM_SIMPLE 1

// Delay boundaries
#define HARRIS_DELAY_MIN 1
#define HARRIS_DELAY_MAX 16 // maximum frames of Harris shutter delay

typedef struct harris0r_instance
{
    unsigned int width;
    unsigned int height;
    unsigned int size; // precalculated pixel size

    // Parameters
    short int simple;
    unsigned int delay;

    // Cross-mode properties
    uint32_t pos;

    // Simple effect properties
    uint32_t *cur_frame;

    // Smooth effect properties
    uint32_t *history;
    unsigned int history_frames;

} harris0r_instance_t;

void harris_mem_init(harris0r_instance_t *inst)
{
    inst->pos = 0; // reset position when switching modes
    
    if (inst->simple) { // if simple mode is chosen...

        // free the memory allocated by smooth mode if there was any
        if (inst->history != NULL) {
            free(inst->history);
            inst->history = NULL;
        }
        
        // allocate memory for frame buffer
        inst->cur_frame = (uint32_t*)malloc(inst->size * sizeof(uint32_t));
    } else {
        // free the memory allocated by simple mode
        if (inst->cur_frame != NULL) {
            free(inst->cur_frame);
            inst->cur_frame = NULL;
        }

        // in smooth mode, inst->history is allocated dynamically based on inst->delay 
        inst->history_frames = 0; // this will trigger a new memory allocation in f0r_update

    }
}

int f0r_init()
{
    return 1;
}

void f0r_deinit()
{ /* no initialization required */ }

void f0r_get_plugin_info(f0r_plugin_info_t* harris0rInfo)
{
    harris0rInfo->name = "harris0r";
    harris0rInfo->author = "Marko Deinterlace";
    harris0rInfo->plugin_type = F0R_PLUGIN_TYPE_FILTER;
    harris0rInfo->color_model = F0R_COLOR_MODEL_RGBA8888;
    harris0rInfo->frei0r_version = FREI0R_MAJOR_VERSION;
    harris0rInfo->major_version = 1;
    harris0rInfo->minor_version = 0;
    harris0rInfo->num_params =  2;
    harris0rInfo->explanation = "Harris shutter effect: dynamic RGB split with a delay";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
    switch(param_index) {
        case HARRIS_PARAM_DELAY:
        {
            info->name = "Delay";
            info->type = F0R_PARAM_DOUBLE;
            info->explanation = "Harris shutter color filtering delay";
            break;
        }

        case HARRIS_PARAM_SIMPLE:
        {
            info->name = "Simple";
            info->type = F0R_PARAM_BOOL;
            info->explanation = "Use simple effect instead of smooth. Reduces RAM usage, less eye-pleasing";
            break;
        }
    }
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
    harris0r_instance_t* inst = (harris0r_instance_t*)calloc(1, sizeof(*inst));
    inst->width = width;
    inst->height = height;
    inst->size = width * height;

    inst->simple = 0; // use smooth implementation by default

    inst->delay = 1; // Default delay of 1 frame
    inst->pos = 0;

    inst->cur_frame = NULL;
    
    inst->history = NULL;
    inst->history_frames = 0;

    // Important: inst->cur_frame is initialized when setting 'simple' param to true

    return (f0r_instance_t)inst;
}

void f0r_destruct(f0r_instance_t instance)
{
    harris0r_instance_t *inst = (harris0r_instance_t*)instance;

    free(inst->cur_frame);
    free(inst->history);
    
    free(instance);
}

void f0r_set_param_value(f0r_instance_t instance,
			 f0r_param_t param, int param_index)
{
    assert(instance);
    harris0r_instance_t *inst = (harris0r_instance_t*)instance;

    switch (param_index)
    {
        case HARRIS_PARAM_DELAY:
        {
            inst->delay = HARRIS_DELAY_MIN + (unsigned int)(*((double*)param) * (HARRIS_DELAY_MAX - HARRIS_DELAY_MIN));
            break;
        }

        case HARRIS_PARAM_SIMPLE:
        {
            short new_simple = *((double*)param) >= 0.5;

            if (new_simple != inst->simple) {
                inst->simple = new_simple;
                harris_mem_init(inst); // reallocate memory dynamically. TODO: test with keyframed usage
            }
            
            break;
        }
    }
}

void f0r_get_param_value(f0r_instance_t instance,
			f0r_param_t param, int param_index)
{

    assert(instance);
    harris0r_instance_t *inst = (harris0r_instance_t*)instance;

    switch (param_index)
    {
        case HARRIS_PARAM_DELAY:
        {
            *((double*)param) = inst->delay / (HARRIS_DELAY_MAX - HARRIS_DELAY_MIN);
            break;
        }

        case HARRIS_PARAM_SIMPLE:
        {
            *((double*)param) = inst->simple ? 1. : 0.;
            break;
        }
    }

}

inline static void harris_update_simple(harris0r_instance_t *inst, const uint32_t *src, uint32_t *dst) 
{
    unsigned int size = inst->width * inst->height;
    
    // First frame: initialize cur_frame with source and copy to destination
    if (inst->pos == 0) {
        memcpy(inst->cur_frame, src, size * sizeof(uint32_t));
        memcpy(dst, inst->cur_frame, size * sizeof(uint32_t));
        inst->pos = 1;
        return;
    }
    
    unsigned int channel_index = ((inst->pos - 1) / inst->delay) % 3;
    uint32_t channel_value;

    
    for (unsigned int i = 0; i < size; i++) {
        uint32_t cur_pixel = inst->cur_frame[i];
        uint32_t src_pixel = src[i];
        
        switch (channel_index) {
            case 0: // Red
                channel_value = (src_pixel & 0x00FF0000) >> 16;
                cur_pixel = (cur_pixel & 0xFF00FFFF) | (channel_value << 16);
                break;
                
            case 1: // Green
                channel_value = (src_pixel & 0x0000FF00) >> 8;
                cur_pixel = (cur_pixel & 0xFFFF00FF) | (channel_value << 8);
                break;
                
            case 2: // Blue
                channel_value = (src_pixel & 0x000000FF);
                cur_pixel = (cur_pixel & 0xFFFFFF00) | channel_value;
                break;
                
            default:
                break;
        }
        
        inst->cur_frame[i] = cur_pixel;
    }
    
    // Copy processed frame to destination
    memcpy(dst, inst->cur_frame, size * sizeof(uint32_t));
}

inline static void harris_update_smooth(harris0r_instance_t *inst, const uint32_t *src, uint32_t *dst)
{
    unsigned int needed = 2 * inst->delay + 1;
    if (inst->history_frames != needed) {

        size_t fb_size = (size_t)needed * inst->size * sizeof(uint32_t);

        uint32_t *tmp = realloc(inst->history, fb_size);

        if (!tmp) {
            memcpy(dst, src, (size_t)inst->size * sizeof(uint32_t)); // passthrough
            return;
        }
        
        inst->history = tmp;
        inst->history_frames = needed;
        inst->pos = 0;
    }

    unsigned int len  = inst->history_frames;
    unsigned int head = inst->pos % len;   // slot for the current frame

    memcpy(inst->history + (size_t)head * inst->size, src,
           (size_t)inst->size * sizeof(uint32_t));

    unsigned int off_g = (inst->delay      < inst->pos) ? inst->delay      : inst->pos;
    unsigned int off_r = (2 * inst->delay  < inst->pos) ? 2 * inst->delay  : inst->pos;

    const uint32_t *b_frame = inst->history + (size_t) head * inst->size;
    const uint32_t *g_frame = inst->history + (size_t)((head + len - off_g) % len) * inst->size;
    const uint32_t *r_frame = inst->history + (size_t)((head + len - off_r) % len) * inst->size;

    for (unsigned int i = 0; i < inst->size; i++) {
        dst[i] = (r_frame[i] & 0x00FF0000u)   // red   from the oldest frame
               | (g_frame[i] & 0x0000FF00u)   // green from the middle one
               | (b_frame[i] & 0x000000FFu)   // blue  from the current frame
               | (src[i]     & 0xFF000000u);  // alpha straight through
    }
}


void f0r_update(f0r_instance_t instance, double time,
        const uint32_t* src, uint32_t* dst)
{
    assert(instance);
    harris0r_instance_t* inst = (harris0r_instance_t*)instance;

    if (inst->simple) {
        harris_update_simple(inst, src, dst);
    } else {
        harris_update_smooth(inst, src, dst);
    }
    
    inst->pos++;
}