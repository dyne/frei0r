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
#include <frei0r.h>

typedef struct instance_s
    {
    unsigned w;
    unsigned h;
    double pos;
    } instance_t;

int f0r_init(void)
    {
    return 1;
    }

void f0r_deinit(void) {}

void f0r_get_plugin_info(f0r_plugin_info_t *info)
    {
    info->name = "slide-right";
    info->author = "Vadim Druzhin";
    info->plugin_type = F0R_PLUGIN_TYPE_MIXER2;
    info->color_model = F0R_COLOR_MODEL_RGBA8888;
    info->frei0r_version = FREI0R_MAJOR_VERSION;
    info->major_version = 0;
    info->minor_version = 1;
    info->num_params = 1;
    info->explanation = "Slide from left to right";
    }

void f0r_get_param_info(f0r_param_info_t *info, int index)
    {
    if(0 == index)
        {
        info->name = "position";
        info->type = F0R_PARAM_DOUBLE;
        info->explanation = "Slide position";
        }
    }

f0r_instance_t f0r_construct(unsigned width, unsigned height)
    {
    instance_t *inst;

    inst = calloc(1, sizeof(*inst));
    if(NULL == inst)
        return NULL;

    inst->w = width;
    inst->h = height;
    inst->pos = 0.0;

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
    unsigned y;
    int off;
    unsigned bw, x;
    double t;
    uint32_t px;

    (void)time; /* Unused */
    (void)inframe3; /* Unused */

    if(inst->pos < 0.5)
        {
        t = inst->pos;
        t = t * t * 2.0;
        }
    else
        {
        t = 1.0 - inst->pos;
        t = 1.0 - t * t * 2.0;
        }

    bw = inst->w / 64;

    off = (int)((inst->w + bw) * t + 0.5) - bw;

    if(off < 0)
        {
        bw += off;
        off = 0;
        }
    else if(inst->w < off + bw)
        bw = inst->w - off;

    for(y = 0; y < inst->h; ++y)
        {
        memcpy(outframe + inst->w * y, inframe2 + inst->w * y + inst->w - off,
            off * sizeof(*outframe));

        for(x = off; x < off + bw; ++x)
            {
            px = (inframe1[inst->w * y + x] >> 2) & 0x3F3F3F3F;
            ((uint8_t *)(&px))[3] = ((uint8_t *)(inframe1 + inst->w * y + x))[3];
            outframe[inst->w * y + x] = px;
            }

        memcpy(outframe + inst->w * y + off + bw, inframe1 + inst->w * y + off + bw,
            (inst->w - off - bw) * sizeof(*outframe));
        }
    }



