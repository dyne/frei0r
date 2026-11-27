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
    uint32_t offset00;
    uint32_t dx_dy; /* bit 0: x1 > x0, bit 1: y1 > y0, bit 2: copy_src (r >= 1 or r < 1e-12) */
    double fx;
    double fy;
} wp_sample_t;

typedef struct {
    unsigned int x_start;
    unsigned int x_end;   /* exclusive */
    size_t sample_offset;
} wp_row_span_t;

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

    /* Cached coordinate map for unchanged warp parameters */
    int map_valid;
    int map_neutral;
    unsigned int y_start;
    unsigned int y_end; /* exclusive */
    wp_row_span_t *row_spans;
    wp_sample_t *samples;
    size_t samples_cap;
} whirl_pinch_t;

static double clamp01(double x)
{
    if (x < 0.0) return 0.0;
    if (x > 1.0) return 1.0;
    return x;
}

static inline uint8_t to_byte(double v)
{
    if (v < 0.0) v = 0.0;
    if (v > 255.0) v = 255.0;
    return (uint8_t)(v + 0.5);
}

/* Interpolate premultiplied color, then return straight BGRA8888 pixels. */
static inline uint32_t sample_bilinear_at(const uint32_t *src,
                                          unsigned int width,
                                          uint32_t offset00,
                                          uint32_t dx_dy,
                                          double fx,
                                          double fy)
{
    const uint32_t step_x = dx_dy & 1u;
    const uint32_t step_y = (dx_dy & 2u) ? width : 0u;
    const uint32_t w00 = src[offset00];
    const uint32_t w10 = src[offset00 + step_x];
    const uint32_t w01 = src[offset00 + step_y];
    const uint32_t w11 = src[offset00 + step_y + step_x];

    const uint8_t *p00 = (const uint8_t *)&w00;
    const uint8_t *p10 = (const uint8_t *)&w10;
    const uint8_t *p01 = (const uint8_t *)&w01;
    const uint8_t *p11 = (const uint8_t *)&w11;

    uint32_t out = 0;
    uint8_t *channels = (uint8_t *)&out;

    /* Fast path when all 4 neighbors share the same alpha (including opaque). */
    if (p00[3] == p10[3] && p00[3] == p01[3] && p00[3] == p11[3]) {
        if (p00[3] == 0) return 0;
        channels[3] = p00[3];
        for (unsigned int channel = 0; channel < 3; ++channel) {
            double a = (double)p00[channel];
            double b = (double)p10[channel];
            double c = (double)p01[channel];
            double d = (double)p11[channel];
            double top = a + (b - a) * fx;
            double bottom = c + (d - c) * fx;
            channels[channel] = to_byte(top + (bottom - top) * fy);
        }
        return out;
    }

    double a00 = (double)p00[3];
    double a10 = (double)p10[3];
    double a01 = (double)p01[3];
    double a11 = (double)p11[3];
    double top = a00 + (a10 - a00) * fx;
    double bottom = a01 + (a11 - a01) * fx;
    double alpha = top + (bottom - top) * fy;
    channels[3] = to_byte(alpha);
    if (channels[3] == 0) return 0;

    for (unsigned int channel = 0; channel < 3; ++channel) {
        double a = (double)p00[channel] * a00;
        double b = (double)p10[channel] * a10;
        double c = (double)p01[channel] * a01;
        double d = (double)p11[channel] * a11;
        top = a + (b - a) * fx;
        bottom = c + (d - c) * fx;
        channels[channel] = to_byte((top + (bottom - top) * fy) / alpha);
    }
    return out;
}

static int build_warp_map(whirl_pinch_t *p)
{
    const unsigned int width = p->width;
    const unsigned int height = p->height;
    const double cx = p->center_x * (double)(width - 1);
    const double cy = p->center_y * (double)(height - 1);
    const double maxdim = (double)((width < height) ? width : height);
    const double radius_px = (p->radius * 2.0) * (maxdim * 0.5);
    const double inv_radius = (radius_px > 0.0) ? 1.0 / radius_px : 0.0;
    const double whirl_angle = (p->whirl * 1440.0 - 720.0) * (WP_PI / 180.0);
    const double pinch_amount = p->pinch * 2.0 - 1.0;

    if (radius_px < 0.5 ||
        (fabs(whirl_angle) < 1e-12 && fabs(pinch_amount) < 1e-12)) {
        p->map_neutral = 1;
        p->map_valid = 1;
        return 1;
    }

    p->map_neutral = 0;

    double y_lo_d = floor(cy - radius_px);
    double y_hi_d = ceil(cy + radius_px);
    unsigned int y_start = (y_lo_d > 0.0) ? (unsigned int)y_lo_d : 0u;
    unsigned int y_end = (y_hi_d + 1.0 < (double)height) ? (unsigned int)(y_hi_d + 1.0) : height;
    if (y_start > height) y_start = height;
    if (y_end < y_start) y_end = y_start;

    p->y_start = y_start;
    p->y_end = y_end;

    if (!p->row_spans) {
        p->row_spans = (wp_row_span_t *)malloc(height * sizeof(wp_row_span_t));
        if (!p->row_spans) return 0;
    }

    const double radius_sq = radius_px * radius_px;
    size_t total_samples = 0;

    for (unsigned int y = y_start; y < y_end; ++y) {
        double dy = (double)y - cy;
        double rem = radius_sq - dy * dy;
        if (rem <= 0.0) {
            p->row_spans[y].x_start = 0;
            p->row_spans[y].x_end = 0;
            p->row_spans[y].sample_offset = total_samples;
            continue;
        }
        double half_w = sqrt(rem) + 1.0;
        double x_lo_d = floor(cx - half_w);
        double x_hi_d = ceil(cx + half_w);
        unsigned int xs = (x_lo_d > 0.0) ? (unsigned int)x_lo_d : 0u;
        unsigned int xe = (x_hi_d + 1.0 < (double)width) ? (unsigned int)(x_hi_d + 1.0) : width;
        if (xs > width) xs = width;
        if (xe < xs) xe = xs;
        p->row_spans[y].x_start = xs;
        p->row_spans[y].x_end = xe;
        p->row_spans[y].sample_offset = total_samples;
        total_samples += (size_t)(xe - xs);
    }

    if (total_samples > p->samples_cap) {
        wp_sample_t *new_samples = (wp_sample_t *)realloc(p->samples, total_samples * sizeof(wp_sample_t));
        if (!new_samples) return 0;
        p->samples = new_samples;
        p->samples_cap = total_samples;
    }

    const double max_x = (double)(width - 1);
    const double max_y = (double)(height - 1);
    size_t idx = 0;

    for (unsigned int y = y_start; y < y_end; ++y) {
        unsigned int xs = p->row_spans[y].x_start;
        unsigned int xe = p->row_spans[y].x_end;
        double dy = (double)y - cy;
        double dy2 = dy * dy;

        for (unsigned int x = xs; x < xe; ++x) {
            double dx = (double)x - cx;
            double r = sqrt(dx * dx + dy2) * inv_radius;
            wp_sample_t *s = &p->samples[idx++];

            if (r >= 1.0 || r < 1e-12) {
                s->offset00 = 0;
                s->dx_dy = 4u; /* copy_src */
                s->fx = 0.0;
                s->fy = 0.0;
            } else {
                double falloff = 1.0 - r;
                double angle = atan2(dy, dx) + whirl_angle * falloff * falloff;
                double shaped_r = r * (1.0 + pinch_amount * falloff * falloff);
                double source_r = (shaped_r > 0.0 ? shaped_r : 0.0) * radius_px;
                double sx = cx + cos(angle) * source_r;
                double sy = cy + sin(angle) * source_r;

                if (sx < 0.0) sx = 0.0;
                if (sy < 0.0) sy = 0.0;
                if (sx > max_x) sx = max_x;
                if (sy > max_y) sy = max_y;

                unsigned int x0 = (unsigned int)sx;
                unsigned int y0 = (unsigned int)sy;
                uint32_t dx_flag = (x0 + 1 < width) ? 1u : 0u;
                uint32_t dy_flag = (y0 + 1 < height) ? 2u : 0u;

                s->offset00 = (uint32_t)((size_t)y0 * width + x0);
                s->dx_dy = dx_flag | dy_flag;
                s->fx = sx - (double)x0;
                s->fy = sy - (double)y0;
            }
        }
    }

    p->map_valid = 1;
    return 1;
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
    p->map_valid = 0;

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
    free(p->samples);
    free(p->row_spans);
    free(p->scratch);
    free(p);
}

void f0r_set_param_value(f0r_instance_t instance, f0r_param_t param, int index) {
    whirl_pinch_t *p = (whirl_pinch_t *)instance;
    double value;

    if (!p || !param) return;
    value = clamp01(*(double *)param);

    switch (index) {
        case 0: if (p->whirl != value)    { p->whirl = value;    p->map_valid = 0; } break;
        case 1: if (p->pinch != value)    { p->pinch = value;    p->map_valid = 0; } break;
        case 2: if (p->radius != value)   { p->radius = value;   p->map_valid = 0; } break;
        case 3: if (p->center_x != value) { p->center_x = value; p->map_valid = 0; } break;
        case 4: if (p->center_y != value) { p->center_y = value; p->map_valid = 0; } break;
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
    size_t count;

    (void)time;
    if (!p || !inframe || !outframe) return;

    count = p->pixel_count;
    if (!p->map_valid) {
        if (!build_warp_map(p)) {
            /* Allocation failure fallback: neutral copy */
            if (inframe != outframe)
                memmove(outframe, inframe, count * sizeof(uint32_t));
            return;
        }
    }

    if (p->map_neutral) {
        if (inframe != outframe)
            memmove(outframe, inframe, count * sizeof(uint32_t));
        return;
    }

    src = inframe;
    if (inframe == outframe) {
        memcpy(p->scratch, inframe, count * sizeof(uint32_t));
        src = p->scratch;
    }

    const unsigned int width = p->width;
    const unsigned int height = p->height;
    const unsigned int y_start = p->y_start;
    const unsigned int y_end = p->y_end;

    /* Copy rows above the warp circle */
    if (y_start > 0) {
        memcpy(outframe, src, (size_t)y_start * width * sizeof(uint32_t));
    }

    for (unsigned int y = y_start; y < y_end; ++y) {
        const size_t row = (size_t)y * width;
        const unsigned int xs = p->row_spans[y].x_start;
        const unsigned int xe = p->row_spans[y].x_end;

        if (xs >= xe) {
            memcpy(outframe + row, src + row, (size_t)width * sizeof(uint32_t));
            continue;
        }

        if (xs > 0) {
            memcpy(outframe + row, src + row, (size_t)xs * sizeof(uint32_t));
        }

        const wp_sample_t *s = p->samples + p->row_spans[y].sample_offset;
        uint32_t *out_row = outframe + row;
        const uint32_t *src_row = src + row;

        for (unsigned int x = xs; x < xe; ++x, ++s) {
            if (s->dx_dy & 4u) {
                out_row[x] = src_row[x];
            } else {
                out_row[x] = sample_bilinear_at(src, width, s->offset00, s->dx_dy, s->fx, s->fy);
            }
        }

        if (xe < width) {
            memcpy(outframe + row + xe, src + row + xe, (size_t)(width - xe) * sizeof(uint32_t));
        }
    }

    /* Copy rows below the warp circle */
    if (y_end < height) {
        size_t tail_off = (size_t)y_end * width;
        memcpy(outframe + tail_off, src + tail_off, (size_t)(height - y_end) * width * sizeof(uint32_t));
    }
}
