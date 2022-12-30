/*
Copyright (C) 2013 Vadim Druzhin <cdslow@mail.ru>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <frei0r.h>

typedef struct instance_s
    {
    int w;
    int h;
    double pos;
    int r;
    int bw;
    int bw_scale;
    int *bwk;
    } instance_t;

int f0r_init(void)
    {
    return 1;
    }

void f0r_deinit(void) {}

void f0r_get_plugin_info(f0r_plugin_info_t *info)
    {
    info->name = "wipe-circle";
    info->author = "Vadim Druzhin";
    info->plugin_type = F0R_PLUGIN_TYPE_MIXER2;
    info->color_model = F0R_COLOR_MODEL_RGBA8888;
    info->frei0r_version = FREI0R_MAJOR_VERSION;
    info->major_version = 0;
    info->minor_version = 1;
    info->num_params = 1;
    info->explanation = "Circle wipe";
    }

void f0r_get_param_info(f0r_param_info_t *info, int index)
    {
    if(0 == index)
        {
        info->name = "position";
        info->type = F0R_PARAM_DOUBLE;
        info->explanation = "Circle size";
        }
    }

f0r_instance_t f0r_construct(unsigned width, unsigned height)
    {
    instance_t *inst;
    int r;
    int bw;
    int i;

    r = (int)(hypotf((float)height, (float)width) / 2.0f + 0.5f);

    bw = r / 16;

    inst = malloc(sizeof(*inst) + bw * sizeof(*inst->bwk));
    if(NULL == inst)
        return NULL;

    inst->w = width;
    inst->h = height;
    inst-> pos = 0.0;
    inst->r = r;
    inst->bw = bw;
    inst->bw_scale = bw * bw;
    inst->bwk = (int *)(inst + 1);

    for(i = 0; i < bw; ++i)
        {
        if(i < bw / 2)
            inst->bwk[i] = i * i * 2;
        else
            inst->bwk[i] = inst->bw_scale - (bw - i) * (bw - i) * 2;
        }

    return inst;
    }

void f0r_destruct(f0r_instance_t inst)
    {
    free(inst);
    }

void f0r_set_param_value(f0r_instance_t instance, f0r_param_t param, int param_index)
    {
    instance_t *inst = instance;

    if(0 == param_index)
        inst->pos = *(f0r_param_double *)param;
    }

void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index)
    {
    instance_t *inst = instance;

    if(0 == param_index)
        *(f0r_param_double *)param = inst->pos;
    }

void f0r_update2(
    f0r_instance_t instance,
    double time,
    const uint32_t* inframe1,
    const uint32_t* inframe2,
    const uint32_t* inframe3,
    uint32_t* outframe
    )
    {
    instance_t *inst = instance;
    int x0, y0, r0, r1;
    int y, x, dy, dx, ox, oy;

    (void)time; /* Unused */
    (void)inframe3; /* Unused */

    r1 = (int)((inst->r + inst->bw) * inst->pos + 0.5);
    r0 = r1 - inst->bw;

    x0 = inst->w / 2;
    y0 = inst->h / 2;

    dy = inst->h / 2 - r1;
    dx = inst->w / 2 - r1;

    if(r0 > 0)
        {
        ox = (int)((float)r0 * (float)M_SQRT1_2 + 0.5f);
        oy = ox;

        if(ox > inst->w / 2)
            ox = inst->w / 2;

        if(oy > inst->h / 2)
            oy = inst->h / 2;
        }
    else
        {
        ox = 0;
        oy = 0;
        }

    if(ox > 0 && oy > 0)
        {
        for(y = y0 - oy; y < y0 + oy; ++y)
            memcpy(outframe + inst->w * y + x0 - ox,
                inframe2 + inst->w * y + x0 - ox, ox * 2 * sizeof(*outframe));
        }

    if(dy > 0)
        {
        memcpy(outframe, inframe1, inst->w * dy * sizeof(*outframe));

        memcpy(outframe + inst->w * (y0 + r1), inframe1 + inst->w * (y0 + r1),
            inst->w * dy * sizeof(*outframe));

        inframe1 += dy * inst->w;
        inframe2 += dy * inst->w;
        outframe += dy * inst->w;
        }
    else
        dy = 0;

    if(dx > 0)
        {
        for(y = 0; y < inst->h - dy * 2; ++y)
            {
            memcpy(outframe + inst->w * y, inframe1 + inst->w * y, dx * sizeof(*outframe));

            memcpy(outframe + inst->w * y + inst->w - dx,
                inframe1 + inst->w * y + inst->w - dx, dx * sizeof(*outframe));
            }

        inframe1 += dx;
        inframe2 += dx;
        outframe += dx;
        }
    else
        dx = 0;

    for(y = dy; y < inst->h - dy; ++y)
        {
        for(x = dx; x < inst->w - dx; ++x)
            {
            if(y < y0 - oy || y >= y0 + oy || x < x0 - ox || x >= x0 + ox)
                {
                int r = (int)(hypotf((float)(x - x0), (float)(y - y0)) + 0.5f);

                if(r >= r1)
                    *outframe = *inframe1;
                else if(r < r0)
                    *outframe = *inframe2;
                else
                    {
                    int k1 = inst->bwk[r - r0];
                    int k2 = inst->bw_scale - k1;
                    uint8_t *c1 = (uint8_t *)inframe1;
                    uint8_t *c2 = (uint8_t *)inframe2;
                    uint8_t *co = (uint8_t *)outframe;

                    co[0] = (c1[0] * k1 + c2[0] * k2 + inst->bw_scale / 2) / inst->bw_scale;
                    co[1] = (c1[1] * k1 + c2[1] * k2 + inst->bw_scale / 2) / inst->bw_scale;
                    co[2] = (c1[2] * k1 + c2[2] * k2 + inst->bw_scale / 2) / inst->bw_scale;
                    co[3] = (c1[3] * k1 + c2[3] * k2 + inst->bw_scale / 2) / inst->bw_scale;
                    }
                }

            ++inframe1;
            ++inframe2;
            ++outframe;
            }

        inframe1 += dx * 2;
        inframe2 += dx * 2;
        outframe += dx * 2;
        }
    }

