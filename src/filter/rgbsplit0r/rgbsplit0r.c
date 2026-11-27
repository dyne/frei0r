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
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "frei0r.h"

typedef struct rgbsplit0r_instance
{
    unsigned int width;
    unsigned int height;
    int shiftX;
    int shiftY;

} rgbsplit0r_instance_t;

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
    rgbsplit0rInfo->minor_version = 1;
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
            if (inst->height > 0)
                shiftY = ((inst->height / 8) * shiftY);
            else
                shiftY = 0;

            inst->shiftY = (int)shiftY;
            break;
        }

        case 1 : // horizontal shift
        {
            // scale to [-1/16..1/16]
            double shiftX = *((double*)param) - 0.5;

            // Convert to range from 0 to one eighth of width
            if (inst->width > 0)
                shiftX = ((inst->width / 8) * shiftX);
            else
                shiftX = 0;

            inst->shiftX = (int)shiftX;
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
            if (inst->height > 0)
                *((double*)param) = (double)inst->shiftY /
                                   ((double)inst->height / 8.0) + 0.5;
            else
                *((double*)param) = 0.5;
            break;
        }

        case 1 : // horizontal shift
        {
            // convert plugin's param to frei0r range
            if (inst->width > 0)
                *((double*)param) = (double)inst->shiftX /
                                   ((double)inst->width / 8.0) + 0.5;
            else
                *((double*)param) = 0.5;
            break;
        }
    }

}

static inline void rgbsplit_border_px(uint32_t* __restrict__ drow,
                                      const uint32_t* __restrict__ g_row,
                                      const uint32_t* __restrict__ b_base,
                                      const uint32_t* __restrict__ r_base,
                                      int x, int w, int sx)
{
    uint32_t out = g_row[x] & 0xFF00FF00u;
    int bx = x - sx;
    if (b_base && (unsigned int)bx < (unsigned int)w)
        out |= b_base[bx] & 0xFFFF0000u;
    int rx = x + sx;
    if (r_base && (unsigned int)rx < (unsigned int)w)
        out |= r_base[rx] & 0xFF0000FFu;
    drow[x] = out;
}

void f0r_update(f0r_instance_t instance, double time,
                const uint32_t* src, uint32_t* dst)
{
    (void)time;
    assert(instance);
    rgbsplit0r_instance_t* inst = (rgbsplit0r_instance_t*)instance;
    const int w = (int)inst->width;
    const int h = (int)inst->height;
    const int sx = inst->shiftX;
    const int sy = inst->shiftY;

    if (sx == 0 && sy == 0)
    {
        memcpy(dst, src, (size_t)w * (size_t)h * sizeof(uint32_t));
        return;
    }

    const int abs_sx = (sx < 0) ? -sx : sx;
    const int x_start = (abs_sx < w) ? abs_sx : w;
    int x_end = (w > abs_sx) ? (w - abs_sx) : 0;
    if (x_end < x_start) x_end = x_start;

    for (int y = 0; y < h; ++y)
    {
        const int by = y - sy;
        const int ry = y + sy;
        const uint32_t* __restrict__ g_row = src + (size_t)y * w;
        const uint32_t* __restrict__ b_base = ((unsigned int)by < (unsigned int)h) ? (src + (size_t)by * w) : NULL;
        const uint32_t* __restrict__ r_base = ((unsigned int)ry < (unsigned int)h) ? (src + (size_t)ry * w) : NULL;
        uint32_t* __restrict__ drow = dst + (size_t)y * w;

        for (int x = 0; x < x_start; ++x)
            rgbsplit_border_px(drow, g_row, b_base, r_base, x, w, sx);

        if (x_start < x_end)
        {
            if (b_base && r_base)
            {
                const uint32_t* __restrict__ b_ptr = b_base - sx;
                const uint32_t* __restrict__ r_ptr = r_base + sx;
                for (int x = x_start; x < x_end; ++x)
                    drow[x] = (g_row[x] & 0xFF00FF00u)
                            | (b_ptr[x] & 0xFFFF0000u)
                            | (r_ptr[x] & 0xFF0000FFu);
            }
            else if (b_base)
            {
                const uint32_t* __restrict__ b_ptr = b_base - sx;
                for (int x = x_start; x < x_end; ++x)
                    drow[x] = (g_row[x] & 0xFF00FF00u)
                            | (b_ptr[x] & 0xFFFF0000u);
            }
            else if (r_base)
            {
                const uint32_t* __restrict__ r_ptr = r_base + sx;
                for (int x = x_start; x < x_end; ++x)
                    drow[x] = (g_row[x] & 0xFF00FF00u)
                            | (r_ptr[x] & 0xFF0000FFu);
            }
            else
            {
                for (int x = x_start; x < x_end; ++x)
                    drow[x] = g_row[x] & 0xFF00FF00u;
            }
        }

        for (int x = x_end; x < w; ++x)
            rgbsplit_border_px(drow, g_row, b_base, r_base, x, w, sx);
    }
}
