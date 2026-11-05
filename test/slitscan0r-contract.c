/*
 * Contract test for slitscan0r: metadata, parameters, allocation failures,
 * the Scan image against a reference model, restarts, in-place rendering
 * and basic properties of the Stargate projection.
 */

#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <frei0r.h>

enum { width = 8, height = 6, pixels = width * height };
enum { p_mode, p_position, p_vertical, p_width, p_reverse, p_depth, p_fade, p_count };

static int fail_malloc, fail_calloc;

/* Only the test plugin object uses these allocators; the normal module does not. */
void *slitscan_test_malloc(size_t size)
{
    if (fail_malloc) { fail_malloc = 0; return NULL; }
    return malloc(size);
}

void *slitscan_test_calloc(size_t count, size_t size)
{
    if (fail_calloc) { fail_calloc = 0; return NULL; }
    return calloc(count, size);
}

static void set(f0r_instance_t instance, int index, double value)
{
    f0r_set_param_value(instance, &value, index);
}

static double get(f0r_instance_t instance, int index)
{
    double value = -1.0;
    f0r_get_param_value(instance, &value, index);
    return value;
}

/* A pixel that identifies its frame and its position. */
static uint32_t pixel(unsigned int frame, unsigned int x, unsigned int y)
{
    uint32_t value;
    uint8_t *c = (uint8_t *)&value;
    c[0] = (uint8_t)(10 * x + y);
    c[1] = (uint8_t)(frame + 1);
    c[2] = 0x33;
    c[3] = (uint8_t)(255 - frame);
    return value;
}

static void fill(uint32_t *frame, unsigned int number)
{
    unsigned int x, y;
    for (y = 0; y < height; ++y)
        for (x = 0; x < width; ++x)
            frame[y * width + x] = pixel(number, x, y);
}

static int compare(const uint32_t *got, const uint32_t *want, const char *what)
{
    int i;
    for (i = 0; i < pixels; ++i) {
        if (got[i] != want[i]) {
            const uint8_t *g = (const uint8_t *)&got[i];
            const uint8_t *w = (const uint8_t *)&want[i];
            fprintf(stderr, "%s: pixel (%d,%d) is (%u,%u,%u,%u), expected (%u,%u,%u,%u)\n",
                    what, i % width, i / width, g[0], g[1], g[2], g[3], w[0], w[1], w[2], w[3]);
            return 1;
        }
    }
    return 0;
}

/* Reference model of Scan mode: an image that scrolls by one line for
 * every pushed slit line, written independently of the plugin's ring. */
typedef struct {
    int vertical, reverse;
    unsigned int width;   /* slit width in pixels */
    double position;
    uint32_t image[pixels];
} model_t;

static void model_push_line(model_t *m, const uint32_t *frame, unsigned int line)
{
    unsigned int x, y;
    if (m->vertical) {
        for (y = 0; y < height; ++y) {
            uint32_t *row = m->image + y * width;
            if (m->reverse) {
                memmove(row + 1, row, (width - 1) * sizeof(uint32_t));
                row[0] = frame[y * width + line];
            } else {
                memmove(row, row + 1, (width - 1) * sizeof(uint32_t));
                row[width - 1] = frame[y * width + line];
            }
        }
    } else {
        if (m->reverse) {
            memmove(m->image + width, m->image, (pixels - width) * sizeof(uint32_t));
            for (x = 0; x < width; ++x) m->image[x] = frame[line * width + x];
        } else {
            memmove(m->image, m->image + width, (pixels - width) * sizeof(uint32_t));
            for (x = 0; x < width; ++x) m->image[(height - 1) * width + x] = frame[line * width + x];
        }
    }
}

static void model_push(model_t *m, const uint32_t *frame)
{
    unsigned int extent = m->vertical ? width : height;
    int start = (int)floor(m->position * extent - m->width / 2.0 + 0.5);
    unsigned int i;
    if (start < 0) start = 0;
    if (start > (int)(extent - m->width)) start = (int)(extent - m->width);
    /* Lines are pushed so that the slit appears unmirrored at the entry edge. */
    for (i = 0; i < m->width; ++i)
        model_push_line(m, frame, m->reverse ? start + m->width - 1 - i : start + i);
}

static f0r_instance_t scan_instance(const model_t *m, unsigned int slit)
{
    f0r_instance_t instance = f0r_construct(width, height);
    if (!instance) return NULL;
    set(instance, p_vertical, m->vertical);
    set(instance, p_reverse, m->reverse);
    set(instance, p_width, (slit - 1) / 63.0);
    set(instance, p_position, m->position);
    return instance;
}

/* Run a Scan scenario out of place and in place against the model. Frames
 * 4 to 6 are rendered in Stargate mode, which must not disturb the history. */
static int scan_case(int vertical, int reverse, unsigned int slit, double position)
{
    static uint32_t in[pixels], inplace[pixels], out[pixels], copy[pixels];
    model_t m;
    f0r_instance_t a, b;
    unsigned int frame;
    char what[128];
    int errors = 0;

    m.vertical = vertical;
    m.reverse = reverse;
    m.width = slit < (vertical ? width : height) ? slit : (vertical ? width : height);
    m.position = position;
    a = scan_instance(&m, slit);
    b = scan_instance(&m, slit);
    if (!a || !b) return 1;

    for (frame = 0; frame < 2 * width + 3; ++frame) {
        int stargate = frame >= 4 && frame <= 6;
        set(a, p_mode, stargate);
        set(b, p_mode, stargate);

        fill(in, frame);
        memcpy(copy, in, sizeof(in));
        memcpy(inplace, in, sizeof(in));
        if (frame == 0) memcpy(m.image, in, sizeof(in));
        else model_push(&m, in);

        f0r_update(a, frame / 25.0, in, out);
        f0r_update(b, frame / 25.0, inplace, inplace);

        snprintf(what, sizeof(what), "scan vertical=%d reverse=%d width=%u position=%g frame %u",
                 vertical, reverse, slit, position, frame);
        errors += compare(in, copy, what);  /* input left untouched */
        errors += compare(inplace, out, what);
        if (!stargate) errors += compare(out, m.image, what);
        if (errors) break;
    }

    f0r_destruct(a);
    f0r_destruct(b);
    return errors;
}

/* Seeking backwards and changing orientation restart from the input. */
static int restart_case(void)
{
    static uint32_t in[pixels], out[pixels];
    int errors = 0;
    unsigned int frame;
    f0r_instance_t instance = f0r_construct(width, height);
    if (!instance) return 1;

    for (frame = 0; frame < 5; ++frame) {
        fill(in, frame);
        f0r_update(instance, frame / 25.0, in, out);
    }
    fill(in, 9);
    f0r_update(instance, 1 / 25.0, in, out);
    errors += compare(out, in, "seek backwards restarts from the input");

    fill(in, 10);
    f0r_update(instance, 2 / 25.0, in, out);
    fill(in, 11);
    set(instance, p_vertical, 0.0);
    f0r_update(instance, 3 / 25.0, in, out);
    errors += compare(out, in, "orientation change restarts from the input");

    f0r_destruct(instance);
    return errors;
}

/* Paused redraws may change the projection, but must not consume history.
 * Compare with three instances that render each timestamp exactly once. */
static int same_time_case(int vertical, int reverse)
{
    static uint32_t in[pixels], inplace[pixels], expected[3][pixels];
    f0r_instance_t reference[3] = { NULL, NULL, NULL };
    f0r_instance_t redraw = NULL;
    model_t m;
    unsigned int frame, view;
    int i, errors = 0;

    m.vertical = vertical;
    m.reverse = reverse;
    m.position = 0.5;
    for (i = 0; i < 3; ++i) {
        reference[i] = scan_instance(&m, 3);
        if (!reference[i]) { errors = 1; goto done; }
        set(reference[i], p_mode, i != 0);
        set(reference[i], p_fade, i == 2);
    }
    redraw = scan_instance(&m, 3);
    if (!redraw) { errors = 1; goto done; }

    for (frame = 0; frame < 2 * width + 3 && !errors; ++frame) {
        double time = frame / 25.0;
        fill(in, frame);
        for (i = 0; i < 3; ++i)
            f0r_update(reference[i], time, in, expected[i]);

        set(redraw, p_mode, 0.0);
        memcpy(inplace, in, sizeof(in));
        f0r_update(redraw, time, inplace, inplace);
        errors += compare(inplace, expected[0], "playback after paused redraws");

        /* Repeat Scan, switch to Stargate, change Fade, then return to Scan.
         * First-frame redraws and subsequent ring wraparound are both covered. */
        for (view = 0; view < 4 && !errors; ++view) {
            unsigned int image = view % 3;
            set(redraw, p_mode, image != 0);
            set(redraw, p_fade, image == 2);
            memcpy(inplace, in, sizeof(in));
            f0r_update(redraw, time, inplace, inplace);
            errors += compare(inplace, expected[image], "same-time projection change");
        }
    }

done:
    if (redraw) f0r_destruct(redraw);
    for (i = 0; i < 3; ++i)
        if (reference[i]) f0r_destruct(reference[i]);
    return errors;
}

/* Stargate is mirror-symmetric around the vanishing line, keeps an opaque
 * black hole in the middle and only shows (possibly darkened) history. */
static int stargate_case(int vertical)
{
    static uint32_t in[pixels], out[pixels];
    int errors = 0;
    unsigned int frame, x, y;
    f0r_instance_t instance = f0r_construct(width, height);
    if (!instance) return 1;

    set(instance, p_mode, 1.0);
    set(instance, p_vertical, vertical);
    set(instance, p_depth, 0.0);
    set(instance, p_fade, 0.0);

    for (frame = 0; frame < 6; ++frame) {
        fill(in, frame);
        f0r_update(instance, frame / 25.0, in, out);
    }

    for (y = 0; y < height && !errors; ++y) {
        for (x = 0; x < width && !errors; ++x) {
            uint32_t v = out[y * width + x];
            uint32_t mirror = vertical ? out[y * width + (width - 1 - x)]
                                       : out[(height - 1 - y) * width + x];
            double across = vertical ? fabs(x + 0.5 - width / 2.0) : fabs(y + 0.5 - height / 2.0);
            double edge = (vertical ? width : height) / 2.0;
            const uint8_t *c = (const uint8_t *)&v;

            if (v != mirror) {
                fprintf(stderr, "stargate vertical=%d: (%u,%u) is not mirrored\n", vertical, x, y);
                errors = 1;
            } else if (across * 2.0 <= edge) {
                if (c[0] || c[1] || c[2] || c[3] != 255) {
                    fprintf(stderr, "stargate vertical=%d: (%u,%u) should be opaque black\n",
                            vertical, x, y);
                    errors = 1;
                }
            } else if (c[2] != 0x33 || c[1] < 1 || c[1] > 6 || c[3] != 256 - c[1]) {
                fprintf(stderr, "stargate vertical=%d: (%u,%u) is not a history pixel\n",
                        vertical, x, y);
                errors = 1;
            }
        }
    }

    /* Newer slits (higher green) lie deeper, towards the black hole; with
     * Reverse the flow goes inwards and the edge shows the newest slits. */
    if (vertical && !errors) {
        const uint8_t *edge = (const uint8_t *)&out[0];
        const uint8_t *inner = (const uint8_t *)&out[1];
        if (!(inner[1] > edge[1])) {
            fprintf(stderr, "stargate: newer slits should be deeper\n");
            errors = 1;
        }
        set(instance, p_reverse, 1.0);
        fill(in, 6);
        f0r_update(instance, 6 / 25.0, in, out);
        if (!(inner[1] < edge[1])) {
            fprintf(stderr, "stargate reverse: newer slits should be nearer\n");
            errors = 1;
        }
        set(instance, p_reverse, 0.0);
    }

    /* Fade darkens colour but leaves alpha alone. */
    set(instance, p_fade, 1.0);
    fill(in, 6);
    f0r_update(instance, 6 / 25.0, in, out);
    for (x = 0; x < (unsigned int)pixels && !errors; ++x) {
        const uint8_t *c = (const uint8_t *)&out[x];
        if (c[2] > 0x33 || (c[3] != 255 && c[3] < 249)) {
            fprintf(stderr, "stargate vertical=%d: faded pixel %u out of range\n", vertical, x);
            errors = 1;
        }
    }

    f0r_destruct(instance);
    return errors;
}

static int metadata_case(void)
{
    static const int types[p_count] = {
        F0R_PARAM_DOUBLE, F0R_PARAM_DOUBLE, F0R_PARAM_BOOL, F0R_PARAM_DOUBLE,
        F0R_PARAM_BOOL, F0R_PARAM_DOUBLE, F0R_PARAM_DOUBLE
    };
    static const double defaults[p_count] = { 0.0, 0.5, 1.0, 0.0, 0.0, 0.25, 1.0 };
    f0r_plugin_info_t info;
    f0r_param_info_t param;
    f0r_instance_t instance;
    int i, errors = 0;
    double nan = NAN;

    f0r_get_plugin_info(&info);
    if (strcmp(info.name, "slitscan0r") != 0 || info.num_params != p_count ||
        info.plugin_type != F0R_PLUGIN_TYPE_FILTER ||
        info.color_model != F0R_COLOR_MODEL_RGBA8888) {
        fprintf(stderr, "unexpected plugin info\n");
        return 1;
    }
    for (i = 0; i < p_count; ++i) {
        memset(&param, 0, sizeof(param));
        f0r_get_param_info(&param, i);
        if (!param.name || !param.explanation || param.type != types[i]) {
            fprintf(stderr, "parameter %d has unexpected info\n", i);
            errors++;
        }
    }

    instance = f0r_construct(width, height);
    if (!instance) return 1;
    for (i = 0; i < p_count; ++i) {
        if (get(instance, i) != defaults[i]) {
            fprintf(stderr, "parameter %d default is %g\n", i, get(instance, i));
            errors++;
        }
        set(instance, i, 0.3);
        if (get(instance, i) != 0.3) errors++;
        set(instance, i, 7.0);
        if (get(instance, i) != 1.0) errors++;
        set(instance, i, -2.0);
        if (get(instance, i) != 0.0) errors++;
        f0r_set_param_value(instance, &nan, i);
        if (get(instance, i) != 0.0) errors++;
    }
    if (errors) fprintf(stderr, "parameters do not round trip or clamp\n");
    f0r_destruct(instance);
    return errors;
}

static int construct_case(void)
{
    int errors = 0;
    f0r_instance_t instance;

    if (f0r_construct(0, height) || f0r_construct(width, 0) ||
        f0r_construct(UINT_MAX, UINT_MAX)) {
        fprintf(stderr, "invalid sizes were accepted\n");
        errors++;
    }
    fail_calloc = 1;
    if ((instance = f0r_construct(width, height)) != NULL) {
        fprintf(stderr, "instance allocation failure was ignored\n");
        f0r_destruct(instance);
        errors++;
    }
    fail_malloc = 1;
    if ((instance = f0r_construct(width, height)) != NULL) {
        fprintf(stderr, "history allocation failure was ignored\n");
        f0r_destruct(instance);
        errors++;
    }
    fail_calloc = fail_malloc = 0;
    return errors;
}

int main(void)
{
    int errors = 0;

    if (!f0r_init()) return 1;
    errors += metadata_case();
    errors += construct_case();
    errors += scan_case(1, 0, 1, 0.5);
    errors += scan_case(1, 1, 3, 0.2);
    errors += scan_case(1, 0, 64, 0.5);   /* wider than the frame: clamped */
    errors += scan_case(0, 0, 2, 0.0);
    errors += scan_case(0, 1, 1, 1.0);
    errors += restart_case();
    errors += same_time_case(1, 0);
    errors += same_time_case(1, 1);
    errors += same_time_case(0, 0);
    errors += same_time_case(0, 1);
    errors += stargate_case(1);
    errors += stargate_case(0);
    f0r_deinit();

    if (errors) fprintf(stderr, "%d slitscan0r contract failure(s)\n", errors);
    return errors ? 1 : 0;
}
