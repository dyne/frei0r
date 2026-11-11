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

#ifdef HARRIS_TEST_ALLOCATORS
/* Redirect allocations only after the system headers have been included. */
void *harris_test_malloc(size_t size);
void *harris_test_calloc(size_t count, size_t size);
void *harris_test_realloc(void *ptr, size_t size);
#define malloc harris_test_malloc
#define calloc harris_test_calloc
#define realloc harris_test_realloc
#endif

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
    size_t size; // precalculated pixel size

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

static void harris_mem_init(harris0r_instance_t *inst)
{
    inst->pos = 0; // reset position when switching modes
    free(inst->cur_frame);
    free(inst->history);
    inst->cur_frame = NULL;
    inst->history = NULL;
    inst->history_frames = 0;
    // Allocate on update so a failure can produce passthrough and be retried.
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
    if (width == 0 || height == 0 || (size_t)width > SIZE_MAX / height)
        return NULL;
    size_t size = (size_t)width * height;
    if (size > SIZE_MAX / sizeof(uint32_t)) return NULL;

    harris0r_instance_t* inst = (harris0r_instance_t*)calloc(1, sizeof(*inst));
    if (inst == NULL) return NULL;
    inst->width = width;
    inst->height = height;
    inst->size = size;

    inst->simple = 0; // use smooth implementation by default

    inst->delay = 1; // Default delay of 1 frame
    inst->pos = 0;

    inst->cur_frame = NULL;
    
    inst->history = NULL;
    inst->history_frames = 0;

    // Frame buffers are allocated on the first update in each mode.

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
            double value = *((double*)param);
            if (value < 0.0) value = 0.0;
            if (value > 1.0) value = 1.0;
            /* Store as double before truncating: x87 can otherwise keep the
             * product just below an integer. Volatile forces this rounding
             * even when the compiler ignores casts under excess precision. */
            volatile double frames = value * (HARRIS_DELAY_MAX - HARRIS_DELAY_MIN);
            inst->delay = HARRIS_DELAY_MIN + (unsigned int)frames;
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
            *((double*)param) = (double)(inst->delay - HARRIS_DELAY_MIN) /
                               (HARRIS_DELAY_MAX - HARRIS_DELAY_MIN);
            break;
        }

        case HARRIS_PARAM_SIMPLE:
        {
            *((double*)param) = inst->simple ? 1. : 0.;
            break;
        }
    }

}

inline static int harris_update_simple(harris0r_instance_t *inst, const uint32_t *src, uint32_t *dst)
{
    size_t size = inst->size;
    if (inst->cur_frame == NULL) {
        inst->cur_frame = (uint32_t*)malloc(size * sizeof(uint32_t));
        if (inst->cur_frame == NULL) {
            memmove(dst, src, size * sizeof(uint32_t));
            return 0;
        }
    }
    
    // First frame: initialize cur_frame with source and copy to destination
    if (inst->pos == 0) {
        memcpy(inst->cur_frame, src, size * sizeof(uint32_t));
        memcpy(dst, inst->cur_frame, size * sizeof(uint32_t));
        return 1;
    }
    
    unsigned int channel_index = ((inst->pos - 1) / inst->delay) % 3;
    uint8_t *cur = (uint8_t*)inst->cur_frame;
    const uint8_t *input = (const uint8_t*)src;

    for (size_t i = 0; i < size; i++) {
        cur[4 * i + channel_index] = input[4 * i + channel_index];
        cur[4 * i + 3] = input[4 * i + 3];
    }
    
    // Copy processed frame to destination
    memcpy(dst, inst->cur_frame, size * sizeof(uint32_t));
    return 1;
}

inline static int harris_update_smooth(harris0r_instance_t *inst, const uint32_t *src, uint32_t *dst)
{
    unsigned int needed = 2 * inst->delay + 1;
    if (inst->history_frames != needed) {

        if (inst->size > SIZE_MAX / sizeof(uint32_t) / needed) {
            memmove(dst, src, inst->size * sizeof(uint32_t));
            return 0;
        }
        size_t fb_size = (size_t)needed * inst->size * sizeof(uint32_t);

        uint32_t *tmp = realloc(inst->history, fb_size);

        if (!tmp) {
            memmove(dst, src, inst->size * sizeof(uint32_t)); // passthrough
            return 0;
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

    const uint8_t *b_frame = (const uint8_t*)(inst->history + (size_t)head * inst->size);
    const uint8_t *g_frame = (const uint8_t*)(inst->history + (size_t)((head + len - off_g) % len) * inst->size);
    const uint8_t *r_frame = (const uint8_t*)(inst->history + (size_t)((head + len - off_r) % len) * inst->size);
    const uint8_t *input = (const uint8_t*)src;
    uint8_t *output = (uint8_t*)dst;

    for (size_t i = 0; i < inst->size; i++) {
        output[4 * i]     = r_frame[4 * i];     // red from the oldest frame
        output[4 * i + 1] = g_frame[4 * i + 1]; // green from the middle one
        output[4 * i + 2] = b_frame[4 * i + 2]; // blue from the current frame
        output[4 * i + 3] = input[4 * i + 3];   // current alpha
    }
    return 1;
}


void f0r_update(f0r_instance_t instance, double time,
        const uint32_t* src, uint32_t* dst)
{
    assert(instance);
    harris0r_instance_t* inst = (harris0r_instance_t*)instance;

    int updated = inst->simple ? harris_update_simple(inst, src, dst)
                               : harris_update_smooth(inst, src, dst);
    if (updated) inst->pos++;
}
