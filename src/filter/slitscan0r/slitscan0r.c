/* slitscan0r.c
 * Copyright (C) 2026 phoebus-84
 *
 * Slit scan: every frame contributes only the pixels under a thin slit,
 * and the slits of consecutive frames are laid side by side, so one axis
 * of the output image becomes time.
 *
 * Modes:
 *   - Scan:     the classic slit scan / photo-finish image. The newest slit
 *               enters at one edge and older slits scroll towards the other.
 *   - Stargate: the same slit history is projected onto two walls receding
 *               to a vanishing point in the middle of the frame, recalling
 *               the slit-scan photography of the Star Gate sequence in
 *               "2001: A Space Odyssey". New slits appear in the distance
 *               and flow outwards, like a flight down a tunnel of light.
 *
 * The history holds exactly one output frame of slits and is allocated
 * when the instance is constructed, so rendering never allocates. It is
 * seeded with the first frame and restarts when time goes backwards.
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

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "frei0r.h"

#ifdef SLITSCAN_TEST_ALLOCATORS
/* Redirect allocations only after the system headers have been included. */
void *slitscan_test_malloc(size_t size);
void *slitscan_test_calloc(size_t count, size_t size);
#define malloc slitscan_test_malloc
#define calloc slitscan_test_calloc
#endif

enum {
    PARAM_MODE,
    PARAM_POSITION,
    PARAM_VERTICAL,
    PARAM_WIDTH,
    PARAM_REVERSE,
    PARAM_DEPTH,
    PARAM_FADE,
    PARAM_COUNT
};

#define MAX_SLIT_WIDTH 64 /* pixels sampled, and pushed, per frame */
#define MIN_DEPTH 2.0     /* distance of the newest slit in Stargate mode, */
#define MAX_DEPTH 64.0    /* in units of the distance of the screen edges   */

typedef struct slitscan0r_instance
{
    unsigned int width;
    unsigned int height;

    /* Parameters, stored as set so they read back unchanged. */
    double params[PARAM_COUNT];

    /* Slit history: `slots` slits of `length` pixels, used as a ring.
     * Vertical slits are columns (one slot per output column), horizontal
     * slits are rows (one slot per output row): either way the ring holds
     * width * height pixels. */
    uint32_t *history;
    unsigned int slots;
    unsigned int length;
    unsigned int head;   /* slot of the newest slit */
    int vertical;        /* orientation the history was filled with */

    int primed;
    double last_time;
} slitscan0r_instance_t;

static int flag(const slitscan0r_instance_t *inst, int param)
{
    return inst->params[param] >= 0.5;
}

/* Number of pixels sampled per frame. */
static unsigned int slit_width(const slitscan0r_instance_t *inst)
{
    unsigned int w = 1 + (unsigned int)(inst->params[PARAM_WIDTH] * (MAX_SLIT_WIDTH - 1) + 0.5);
    return w < inst->slots ? w : inst->slots;
}

/* Slot of the slit taken `age` frames (or slit widths) ago. */
static uint32_t *slot(const slitscan0r_instance_t *inst, unsigned int age)
{
    unsigned int index = (inst->head + inst->slots - age % inst->slots) % inst->slots;
    return inst->history + (size_t)index * inst->length;
}

/* Copy one column (vertical) or row of the frame into a slot. */
static void sample(const slitscan0r_instance_t *inst, const uint32_t *frame,
                   unsigned int line, uint32_t *dst)
{
    unsigned int i;
    if (inst->vertical) {
        for (i = 0; i < inst->height; ++i)
            dst[i] = frame[(size_t)i * inst->width + line];
    } else {
        memcpy(dst, frame + (size_t)line * inst->width, inst->width * sizeof(uint32_t));
    }
}

/* Screen line (column or row) where the slit of a given age is shown in
 * Scan mode. The newest slit is at the right (bottom) edge, or at the left
 * (top) edge when reversed. */
static unsigned int scan_line(const slitscan0r_instance_t *inst, unsigned int age)
{
    return flag(inst, PARAM_REVERSE) ? age : inst->slots - 1 - age;
}

/* Restart the history from a whole frame, so that Scan mode initially
 * shows the frame itself and builds the slit image over it. */
static void seed(slitscan0r_instance_t *inst, const uint32_t *frame)
{
    unsigned int age;
    inst->vertical = flag(inst, PARAM_VERTICAL);
    inst->slots = inst->vertical ? inst->width : inst->height;
    inst->length = inst->vertical ? inst->height : inst->width;
    inst->head = inst->slots - 1;
    for (age = 0; age < inst->slots; ++age)
        sample(inst, frame, scan_line(inst, age), slot(inst, age));
}

/* Push the pixels under the slit. The slit is `w` lines wide, centred on
 * Position; its lines are pushed so that Scan mode shows them unmirrored. */
static void push(slitscan0r_instance_t *inst, const uint32_t *frame)
{
    unsigned int w = slit_width(inst);
    unsigned int extent = inst->vertical ? inst->width : inst->height;
    double centre = inst->params[PARAM_POSITION] * extent;
    int start = (int)floor(centre - w / 2.0 + 0.5);
    unsigned int i;

    if (start < 0) start = 0;
    if ((unsigned int)start > extent - w) start = (int)(extent - w);

    for (i = 0; i < w; ++i) {
        unsigned int line = flag(inst, PARAM_REVERSE) ? start + w - 1 - i : start + i;
        inst->head = (inst->head + 1) % inst->slots;
        sample(inst, frame, line, slot(inst, 0));
    }
}

static void render_scan(const slitscan0r_instance_t *inst, uint32_t *out)
{
    unsigned int age, i;
    for (age = 0; age < inst->slots; ++age) {
        const uint32_t *src = slot(inst, age);
        unsigned int line = scan_line(inst, age);
        if (inst->vertical) {
            for (i = 0; i < inst->height; ++i)
                out[(size_t)i * inst->width + line] = src[i];
        } else {
            memcpy(out + (size_t)line * inst->width, src, inst->width * sizeof(uint32_t));
        }
    }
}

/* Mirror an index into [0, n), so the walls repeat the slit without seams. */
static unsigned int reflect(int64_t v, unsigned int n)
{
    int64_t period = 2 * (int64_t)n;
    v %= period;
    if (v < 0) v += period;
    return (unsigned int)(v < (int64_t)n ? v : period - 1 - v);
}

static uint32_t dim(uint32_t pixel, double gain)
{
    uint8_t *c = (uint8_t *)&pixel;
    c[0] = (uint8_t)(c[0] * gain + 0.5);
    c[1] = (uint8_t)(c[1] * gain + 0.5);
    c[2] = (uint8_t)(c[2] * gain + 0.5);
    return pixel;
}

/* Project the slit history onto two walls. For vertical slits the walls
 * stand on the left and right, for horizontal slits they are the floor and
 * the ceiling. A wall pixel at screen distance `across` from the centre line
 * lies at depth edge / across; depth selects the age of the slit, and the
 * offset along the slit grows with depth, as in a perspective view. */
static void render_stargate(const slitscan0r_instance_t *inst, uint32_t *out)
{
    const double farthest = MIN_DEPTH + inst->params[PARAM_DEPTH] * (MAX_DEPTH - MIN_DEPTH);
    const double fade = inst->params[PARAM_FADE];
    const int reverse = flag(inst, PARAM_REVERSE);
    const unsigned int across_size = inst->vertical ? inst->width : inst->height;
    const unsigned int along_size = inst->length;
    const double edge = across_size / 2.0;
    uint32_t black;
    unsigned int x, y;

    {
        uint8_t *c = (uint8_t *)&black;
        c[0] = c[1] = c[2] = 0;
        c[3] = 255;
    }

    for (y = 0; y < inst->height; ++y) {
        for (x = 0; x < inst->width; ++x) {
            double dx = x + 0.5 - inst->width / 2.0;
            double dy = y + 0.5 - inst->height / 2.0;
            double across = fabs(inst->vertical ? dx : dy);
            double along = inst->vertical ? dy : dx;
            double depth, t, gain;
            unsigned int age, s;
            uint32_t *dst = out + (size_t)y * inst->width + x;

            if (across * farthest <= edge) {
                *dst = black; /* beyond the newest slit */
                continue;
            }
            depth = edge / across;                 /* 1 at the edges, far in the middle */
            t = (farthest - depth) / (farthest - 1.0); /* 0 for the newest slit, 1 for the oldest */
            if (reverse) t = 1.0 - t;
            age = (unsigned int)(t * (inst->slots - 1) + 0.5);
            s = reflect((int64_t)floor(along_size / 2.0 + along * depth * along_size /
                                    (double)(inst->vertical ? inst->height : inst->width)),
                        along_size);
            gain = 1.0 - fade * (depth - 1.0) / (farthest - 1.0);
            *dst = dim(slot(inst, age)[s], gain);
        }
    }
}

int f0r_init()
{
    return 1;
}

void f0r_deinit()
{ /* no initialization required */ }

void f0r_get_plugin_info(f0r_plugin_info_t *info)
{
    info->name = "slitscan0r";
    info->author = "phoebus-84";
    info->plugin_type = F0R_PLUGIN_TYPE_FILTER;
    info->color_model = F0R_COLOR_MODEL_RGBA8888;
    info->frei0r_version = FREI0R_MAJOR_VERSION;
    info->major_version = 1;
    info->minor_version = 0;
    info->num_params = PARAM_COUNT;
    info->explanation = "Slit scan: lays the slits of consecutive frames side by side, "
                        "flat (Scan) or as a tunnel of light (Stargate)";
}

void f0r_get_param_info(f0r_param_info_t *info, int param_index)
{
    switch (param_index) {
    case PARAM_MODE:
        info->name = "Mode";
        info->type = F0R_PARAM_DOUBLE;
        info->explanation = "Composition: < 0.5 Scan (flat slit image), >= 0.5 Stargate "
                            "(slits projected onto walls receding to a vanishing point)";
        break;
    case PARAM_POSITION:
        info->name = "Position";
        info->type = F0R_PARAM_DOUBLE;
        info->explanation = "Position of the slit across the input frame";
        break;
    case PARAM_VERTICAL:
        info->name = "Vertical";
        info->type = F0R_PARAM_BOOL;
        info->explanation = "Sample a vertical slit (a column) instead of a horizontal one "
                            "(a row); changing it restarts the image";
        break;
    case PARAM_WIDTH:
        info->name = "Width";
        info->type = F0R_PARAM_DOUBLE;
        info->explanation = "Slit width, from 1 to 64 pixels: how far the image advances "
                            "on every frame";
        break;
    case PARAM_REVERSE:
        info->name = "Reverse";
        info->type = F0R_PARAM_BOOL;
        info->explanation = "Scan: new slits enter at the left (top) edge instead of the "
                            "right (bottom) one. Stargate: slits flow inwards";
        break;
    case PARAM_DEPTH:
        info->name = "Depth";
        info->type = F0R_PARAM_DOUBLE;
        info->explanation = "Stargate: how far the tunnel reaches; larger values shrink "
                            "the dark hole in the middle";
        break;
    case PARAM_FADE:
        info->name = "Fade";
        info->type = F0R_PARAM_DOUBLE;
        info->explanation = "Stargate: darken the walls with distance, as in a long exposure";
        break;
    }
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
    slitscan0r_instance_t *inst;
    size_t pixels;

    if (width == 0 || height == 0 || (size_t)width > SIZE_MAX / height)
        return NULL;
    pixels = (size_t)width * height;
    if (pixels > SIZE_MAX / sizeof(uint32_t)) return NULL;

    inst = (slitscan0r_instance_t *)calloc(1, sizeof(*inst));
    if (inst == NULL) return NULL;
    inst->history = (uint32_t *)malloc(pixels * sizeof(uint32_t));
    if (inst->history == NULL) {
        free(inst);
        return NULL;
    }

    inst->width = width;
    inst->height = height;
    inst->params[PARAM_MODE] = 0.0;
    inst->params[PARAM_POSITION] = 0.5;
    inst->params[PARAM_VERTICAL] = 1.0;
    inst->params[PARAM_WIDTH] = 0.0;
    inst->params[PARAM_REVERSE] = 0.0;
    inst->params[PARAM_DEPTH] = 0.25;
    inst->params[PARAM_FADE] = 1.0;
    inst->primed = 0;
    return (f0r_instance_t)inst;
}

void f0r_destruct(f0r_instance_t instance)
{
    slitscan0r_instance_t *inst = (slitscan0r_instance_t *)instance;
    free(inst->history);
    free(inst);
}

void f0r_set_param_value(f0r_instance_t instance, f0r_param_t param, int param_index)
{
    slitscan0r_instance_t *inst = (slitscan0r_instance_t *)instance;
    double value;
    assert(instance);

    if (param_index < 0 || param_index >= PARAM_COUNT) return;
    value = *((double *)param);
    if (!(value >= 0.0)) value = 0.0; /* also rejects NaN */
    if (value > 1.0) value = 1.0;
    inst->params[param_index] = value;
}

void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index)
{
    slitscan0r_instance_t *inst = (slitscan0r_instance_t *)instance;
    assert(instance);

    if (param_index < 0 || param_index >= PARAM_COUNT) return;
    *((double *)param) = inst->params[param_index];
}

void f0r_update(f0r_instance_t instance, double time,
                const uint32_t *inframe, uint32_t *outframe)
{
    slitscan0r_instance_t *inst = (slitscan0r_instance_t *)instance;
    assert(instance);

    /* Restart on the first frame, after seeking backwards and when the
     * orientation changes; otherwise add a slit only when time advances.
     * Paused redraws can change the projection without consuming history.
     * The input is fully read before output is written, so in-place is safe. */
    if (!inst->primed || time < inst->last_time || inst->vertical != flag(inst, PARAM_VERTICAL)) {
        seed(inst, inframe);
        inst->primed = 1;
    } else if (time > inst->last_time) {
        push(inst, inframe);
    }
    inst->last_time = time;

    if (flag(inst, PARAM_MODE))
        render_stargate(inst, outframe);
    else
        render_scan(inst, outframe);
}
