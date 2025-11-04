/**
 * (c) Copyright 2025 Cynthia <cynthia2048@proton.me>
 *
 * This is based on Otsu's algorithm to segment an image into foreground
 * or background based on the shape of the histogram. Instead of doing a
 * hard threshold, we use the threshold obtained from Otsu's algorithm
 * as the base for a sigmoidal transfer with a high slope; this produces
 * a more eye-soothing threshold effect as seen in ImageMagick.
 *
 * This has the added benefit that, whereas the sigmoidal filter's base
 * requires manual tuning, here it is determined algorithmically and thus
 * can adapt on a frame-to-frame basis.
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

#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include "frei0r.h"
#include "frei0r/math.h"

#include "variance.h"

typedef struct {
    unsigned int width, height;
    float slope;

    uint8_t lut[256][256];
    uint8_t *lumaframe;
} s0ft0tsu_t;

static void gen_sigmoid_lut (uint8_t lut[256][256], float slope)
{
    float k = expf(slope) / 255.0;

    for (int j = 0; j < 256; ++j)
        for (int i = 0; i < 256; ++i)
            lut[j][i] = CLAMP (255.0 / (1.0 + expf(-k * (i - j))), 0, 255);
}

int f0r_init()
{
    return 0;
}

void f0r_deinit() {}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
    s0ft0tsu_t* s = calloc(1, sizeof(s0ft0tsu_t));

    s->width = width;
    s->height = height;
    s->slope = 4.0;
    s->lumaframe = malloc(s->width * s->height);

    gen_sigmoid_lut(s->lut, s->slope);

    return s;
}

void f0r_get_plugin_info(f0r_plugin_info_t* info)
{
    info->name = "autothreshold";
    info->author = "Cynthia";
    info->explanation = "Automatically threshold moving pictures";
    info->major_version = 0;
    info->minor_version = 1;
    info->frei0r_version = FREI0R_MAJOR_VERSION;
    info->plugin_type = F0R_PLUGIN_TYPE_FILTER;
    info->color_model = F0R_COLOR_MODEL_RGBA8888;
    info->num_params = 1;
}

void f0r_get_param_info(f0r_param_info_t *info, int index)
{
    switch (index)
    {
    case 0:
        info->name = "Slope";
        info->explanation = "Slope of sigmoidal transfer";
        info->type = F0R_PARAM_DOUBLE;
    break;
    }
}

void f0r_get_param_value(f0r_instance_t inst, f0r_param_t param, int index)
{
    s0ft0tsu_t* s = inst;

    switch (index)
    {
    case 0:
        *(double*)param = s->slope / 7.0;
    break;
    }
}

void f0r_set_param_value(f0r_instance_t inst, f0r_param_t param, int index)
{
    s0ft0tsu_t* s = inst;

    switch (index)
    {
    case 0:
        s->slope = *(double*)param * 7.0;

        gen_sigmoid_lut(s->lut, s->slope);
    break;
    }
}

void f0r_update(f0r_instance_t inst, double time,
                const uint32_t* inframe, uint32_t* outframe)
{
    s0ft0tsu_t *s = inst;

    uint8_t *src = (uint8_t*)inframe;
    uint8_t *dst = s->lumaframe;
    uint8_t r, g, b, luma;
    size_t len = s->width * s->height;

    // Normalised histogram (L = 256)
    float hist[256] = {0};

    for (int i = 0; i < len; ++i)
    {
        r = *src++;
        g = *src++;
        b = *src++;
        src++; // Ignore alpha

        luma = CLAMP(0.299 * r + 0.587 * g + 0.114 * b, 0, 255);
        *dst++ = luma;

        ++hist[luma]; // Add to histogram
    }

    // Normalise histogram; becomes a probability distribution
    for (int i = 0; i < 256; ++i)
        hist[i] /= len;

    uint8_t thresh = 0.0;
    float max_var = 0.0; // Maximum inter-class variance.

    for (int i = 1; i < 256; ++i)
    {
        float var = find_variance(hist, i);
        if (var > max_var)
        {
            thresh = i;
            max_var = var;
        }
    }

    src = (uint8_t*)s->lumaframe; // Replenish
    dst = (uint8_t*)outframe;

    for (int i = 0; i < len; ++i)
    {
        // Select the appropriate transfer
        uint8_t* lut = s->lut[thresh];

        luma = lut[*src++];

        *dst++ = luma;
        *dst++ = luma;
        *dst++ = luma;
        *dst++ = 0xFF; // TODO Copy alpha
    }
}

void f0r_destruct(f0r_instance_t s)
{
    free(s);
}
