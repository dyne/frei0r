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
    unsigned bw;
    unsigned bw_scale;
    unsigned *bwk;
    } instance_t;

int f0r_init(void)
    {
    return 1;
    }

void f0r_deinit(void) {}

void f0r_get_plugin_info(f0r_plugin_info_t *info)
    {
    info->name = "wipe-up";
    info->author = "Vadim Druzhin";
    info->plugin_type = F0R_PLUGIN_TYPE_MIXER2;
    info->color_model = F0R_COLOR_MODEL_RGBA8888;
    info->frei0r_version = FREI0R_MAJOR_VERSION;
    info->major_version = 0;
    info->minor_version = 1;
    info->num_params = 1;
    info->explanation = "Wipe from bottom to top";
    }

void f0r_get_param_info(f0r_param_info_t *info, int index)
    {
    if(0 == index)
        {
        info->name = "position";
        info->type = F0R_PARAM_DOUBLE;
        info->explanation = "Edge position";
        }
    }

f0r_instance_t f0r_construct(unsigned width, unsigned height)
    {
    instance_t *inst;
    unsigned bw = height / 16;
    unsigned i;

    inst = malloc(sizeof(*inst) + bw * sizeof(*inst->bwk));
    if(NULL == inst)
        return NULL;

    inst->w = width;
    inst->h = height;
    inst->pos = 0.0;
    inst->bw = bw;
    inst->bw_scale = bw * bw;
    inst->bwk = (unsigned *)(inst + 1);

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
    unsigned y, x;
    int off;
    unsigned bw, dbw;
    uint8_t *c1, *c2, *co;

    (void)time; /* Unused */
    (void)inframe3; /* Unused */

    off = (int)((inst->h + inst->bw) * inst->pos + 0.5) - inst->bw;
    bw = inst->bw;
    dbw = 0;

    if(off < 0)
        {
        bw += off;
        off = 0;
        }
    else if(inst->h < off + bw)
        {
        bw = inst->h - off;
        dbw = inst->bw - bw;
        }

    memcpy(outframe, inframe1, inst->w * (inst->h - off - bw) * sizeof(*outframe));

    memcpy(outframe + inst->w * (inst->h - off), inframe2 + inst->w * (inst->h - off),
        inst->w * off * sizeof(*outframe));

    c1 = (uint8_t *)(inframe1 + inst->w * (inst->h - off - bw));
    c2 = (uint8_t *)(inframe2 + inst->w * (inst->h - off - bw));
    co = (uint8_t *)(outframe + inst->w * (inst->h - off - bw));
    for(y = 0; y < bw; ++y)
        {
        unsigned k = inst->bwk[y + dbw];

        for(x = 0; x < inst->w * 4; ++x)
            {
            *co = (*c1 * (inst->bw_scale - k) + *c2 * k + inst->bw_scale / 2) / inst->bw_scale;
            ++c1;
            ++c2;
            ++co;
            }

        }
    }



