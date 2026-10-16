/* whirlpinch0r.c
 * frei0r Whirl and Pinch
 * A fast, inverse-mapped radial warp inspired by GEGL/GIMP's whirl-pinch.
 *
 * Normalized frei0r parameters:
 *   Whirl:   0..1 -> -720..+720 degrees (0.5 = neutral)
 *   Pinch:   0..1 -> -1..+1 (0.5 = neutral)
 *   Radius:  0..1 -> 0..2
 *   Center X/Y: normalized [0,1]
 *
 * This is a real-time-oriented approximation, not a bit-exact GEGL port.
 * Port triggered by shadowfax65 and done by OpenAI
 *
 * This file is a Frei0r plugin.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
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

#include <frei0r.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define WP_PARAM_COUNT 5

#ifndef WP_PI
#define WP_PI 3.1415926535897932384626433832795
#endif

typedef struct {
    unsigned int width;
    unsigned int height;
    double whirl;
    double pinch;
    double radius;
    double center_x;
    double center_y;
    uint32_t *scratch;
    size_t pixel_count;
} whirl_pinch_t;

static double clamp01(double x)
{
    if (x < 0.0) return 0.0;
    if (x > 1.0) return 1.0;
    return x;
}

static uint8_t to_byte(double v)
{
    if (v < 0.0) v = 0.0;
    if (v > 255.0) v = 255.0;
    return (uint8_t)(v + 0.5);
}

/* Bilinear interpolation of the four byte channels in BGRA8888 pixels. */
static uint32_t sample_bilinear(const uint32_t *src,
                                unsigned int width,
                                unsigned int height,
                                double x,
                                double y)
{
    unsigned int x0, y0, x1, y1, shift;
    double fx, fy;
    uint32_t p00, p10, p01, p11, out = 0;

    if (x < 0.0) x = 0.0;
    if (y < 0.0) y = 0.0;
    if (x > (double)(width - 1)) x = (double)(width - 1);
    if (y > (double)(height - 1)) y = (double)(height - 1);

    x0 = (unsigned int)x;
    y0 = (unsigned int)y;
    x1 = (x0 + 1 < width) ? x0 + 1 : width - 1;
    y1 = (y0 + 1 < height) ? y0 + 1 : height - 1;
    fx = x - (double)x0;
    fy = y - (double)y0;

    p00 = src[(size_t)y0 * width + x0];
    p10 = src[(size_t)y0 * width + x1];
    p01 = src[(size_t)y1 * width + x0];
    p11 = src[(size_t)y1 * width + x1];

    for (shift = 0; shift < 32; shift += 8) {
        double a = (double)((p00 >> shift) & 255u);
        double b = (double)((p10 >> shift) & 255u);
        double c = (double)((p01 >> shift) & 255u);
        double d = (double)((p11 >> shift) & 255u);
        double top = a + (b - a) * fx;
        double bottom = c + (d - c) * fx;
        out |= ((uint32_t)to_byte(top + (bottom - top) * fy)) << shift;
    }
    return out;
}

int f0r_init() { return 1; }
void f0r_deinit() { }

void f0r_get_plugin_info(f0r_plugin_info_t *info) {
    if (!info) return;

    info->name = "Whirl and Pinch";
    info->author = "shadowfax65";
    info->plugin_type = F0R_PLUGIN_TYPE_FILTER;
    info->color_model = F0R_COLOR_MODEL_BGRA8888;
    info->frei0r_version = FREI0R_MAJOR_VERSION;
    info->major_version = 1;
    info->minor_version = 2;
    info->num_params = WP_PARAM_COUNT;
    info->explanation =
        "Real-time radial whirl/pinch approximation. Whirl: -720..+720 degrees; "
        "Pinch: -1..+1; Radius: 0..2; Center X/Y: normalized.";
}

void f0r_get_param_info(f0r_param_info_t *info, int index) {
    if (!info || index < 0 || index >= WP_PARAM_COUNT) return;
    switch (index) {
        case 0: info->name = "Whirl";       info->type = F0R_PARAM_DOUBLE; info->explanation = "Whirl angle: -720 to +720 degrees; 0.5 is neutral."; break;
        case 1: info->name = "Pinch";       info->type = F0R_PARAM_DOUBLE; info->explanation = "Pinch amount: -1 to +1; 0.5 is neutral."; break;
        case 2: info->name = "Radius";      info->type = F0R_PARAM_DOUBLE; info->explanation = "Radius: 0 to 2; 0.5 gives radius 1."; break;
        case 3: info->name = "Center X";    info->type = F0R_PARAM_DOUBLE; info->explanation = "Horizontal center, normalized 0 to 1."; break;
        case 4: info->name = "Center Y";    info->type = F0R_PARAM_DOUBLE; info->explanation = "Vertical center, normalized 0 to 1."; break;
    }
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height) {
    whirl_pinch_t *p;
    size_t count;

    if (width == 0 || height == 0) return NULL;
    if ((size_t)width > SIZE_MAX / (size_t)height) return NULL;
    count = (size_t)width * (size_t)height;
    if (count > SIZE_MAX / sizeof(uint32_t)) return NULL;

    p = (whirl_pinch_t *)calloc(1, sizeof(*p));
    if (!p) return NULL;

    p->width = width;
    p->height = height;
    p->whirl = 0.5;
    p->pinch = 0.5;
    p->radius = 0.5;
    p->center_x = 0.5;
    p->center_y = 0.5;
    p->pixel_count = count;

    /*
     * Allocate scratch once during construction. This allows safe processing
     * if the host aliases input and output buffers, without frame-time malloc.
     */
    p->scratch = (uint32_t *)malloc(count * sizeof(uint32_t));
    if (!p->scratch) {
        free(p);
        return NULL;
    }
    return (f0r_instance_t)p;
}

void f0r_destruct(f0r_instance_t instance) {
    whirl_pinch_t *p = (whirl_pinch_t *)instance;
    if (!p) return;
    free(p->scratch);
    free(p);
}

void f0r_set_param_value(f0r_instance_t instance, f0r_param_t param, int index) {
    whirl_pinch_t *p = (whirl_pinch_t *)instance;
    double value;

    if (!p || !param) return;
    value = clamp01(*(double *)param);

    switch (index) {
        case 0: p->whirl = value; break;
        case 1: p->pinch = value; break;
        case 2: p->radius = value; break;
        case 3: p->center_x = value; break;
        case 4: p->center_y = value; break;
        default: break;
    }
}

void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int index) {
    const whirl_pinch_t *p = (const whirl_pinch_t *)instance;
    double value;

    if (!p || !param) return;

    switch (index) {
        case 0: value = p->whirl; break;
        case 1: value = p->pinch; break;
        case 2: value = p->radius; break;
        case 3: value = p->center_x; break;
        case 4: value = p->center_y; break;
        default: return;
    }
    *(double *)param = value;
}

void f0r_update(f0r_instance_t instance,
                double time,
                const uint32_t *inframe,
                uint32_t *outframe) {
    whirl_pinch_t *p = (whirl_pinch_t *)instance;
    const uint32_t *src;
    unsigned int x, y;
    size_t count;
    double cx, cy, maxdim, radius_px, inv_radius;
    double whirl_angle, pinch_amount;

    (void)time;
    if (!p || !inframe || !outframe) return;

    count = p->pixel_count;
    src = inframe;

    if (inframe == outframe) {
        memcpy(p->scratch, inframe, count * sizeof(uint32_t));
        src = p->scratch;
    }

    cx = p->center_x * (double)(p->width - 1);
    cy = p->center_y * (double)(p->height - 1);
    maxdim = (double)((p->width < p->height) ? p->width : p->height);
    radius_px = (p->radius * 2.0) * (maxdim * 0.5);
    inv_radius = (radius_px > 0.0) ? 1.0 / radius_px : 0.0;
    whirl_angle = (p->whirl * 1440.0 - 720.0) * (WP_PI / 180.0);
    pinch_amount = p->pinch * 2.0 - 1.0;

    if (radius_px < 0.5 ||
        (fabs(whirl_angle) < 1e-12 && fabs(pinch_amount) < 1e-12)) {
        memmove(outframe, src, count * sizeof(uint32_t));
        return;
    }

    for (y = 0; y < p->height; ++y) {
        double dy = (double)y - cy;
        size_t row = (size_t)y * p->width;

        for (x = 0; x < p->width; ++x) {
            double dx = (double)x - cx;
            double r = sqrt(dx * dx + dy * dy) * inv_radius;
            size_t i = row + x;

            if (r >= 1.0 || r < 1e-12) {
                outframe[i] = src[i];
            } else {
                double falloff = 1.0 - r;
                double angle = atan2(dy, dx) + whirl_angle * falloff * falloff;
                double shaped_r = r * (1.0 + pinch_amount * falloff * falloff);
                double source_r = (shaped_r > 0.0 ? shaped_r : 0.0) * radius_px;
                double sx = cx + cos(angle) * source_r;
                double sy = cy + sin(angle) * source_r;

                outframe[i] = sample_bilinear(src, p->width, p->height, sx, sy);
            }
        }
    }
}
