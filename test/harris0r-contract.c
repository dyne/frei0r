#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <frei0r.h>

enum { width = 8, height = 8, pixels = width * height };
static int fail_malloc, fail_calloc, fail_realloc;

/* Only the test plugin object uses these allocators; the normal module does not. */
void *harris_test_malloc(size_t size)
{
    if (fail_malloc) { fail_malloc = 0; return NULL; }
    return malloc(size);
}

void *harris_test_calloc(size_t count, size_t size)
{
    if (fail_calloc) { fail_calloc = 0; return NULL; }
    return calloc(count, size);
}

void *harris_test_realloc(void *ptr, size_t size)
{
    if (fail_realloc) { fail_realloc = 0; return NULL; }
    return realloc(ptr, size);
}

static void parameter(f0r_instance_t instance, int index, double value)
{
    f0r_set_param_value(instance, &value, index);
}

static uint8_t component(unsigned int frame, unsigned int pixel, unsigned int channel)
{
    if (channel == 3) return (uint8_t)(40 + (frame % 7) * 20);
    return (uint8_t)((10 + channel * 20 + frame * 3 + pixel % 8) % 251);
}

static int expect(const uint32_t *output, unsigned int red_frame,
                  unsigned int green_frame, unsigned int blue_frame,
                  unsigned int alpha_frame)
{
    const unsigned int frames[4] = { red_frame, green_frame, blue_frame, alpha_frame };
    unsigned int i, channel;
    const uint8_t *bytes = (const uint8_t *)output;
    for (i = 0; i < pixels; ++i) {
        for (channel = 0; channel < 4; ++channel) {
            uint8_t wanted = component(frames[channel], i, channel);
            if (bytes[4 * i + channel] != wanted) {
                fprintf(stderr, "Pixel %u RGBA channel %u: got %u, expected %u\n",
                        i, channel, bytes[4 * i + channel], wanted);
                return 1;
            }
        }
    }
    return 0;
}

static int step(f0r_instance_t instance, uint32_t *input, uint32_t *output,
                unsigned int frame, unsigned int red_frame,
                unsigned int green_frame, unsigned int blue_frame, int inplace)
{
    unsigned int i, channel;
    uint8_t *bytes = (uint8_t *)input;
    for (i = 0; i < pixels; ++i)
        for (channel = 0; channel < 4; ++channel)
            bytes[4 * i + channel] = component(frame, i, channel);
    memset(output, 0xa5, pixels * sizeof(*output));
    f0r_update(instance, frame / 30.0, input, inplace ? input : output);
    if (expect(inplace ? input : output, red_frame, green_frame, blue_frame, frame) ||
        (!inplace && expect(input, frame, frame, frame, frame))) {
        fprintf(stderr, "at input frame %u, in-place=%d\n", frame, inplace);
        return 1;
    }
    return 0;
}

static int mapping(void)
{
    unsigned int index;
    double value, again;
    f0r_instance_t instance = f0r_construct(width, height);
    if (!instance) return 1;
    f0r_get_param_value(instance, &value, 0);
    if (value != 0.0) goto failed;
    for (index = 0; index < 16; ++index) {
        double normalized = index / 15.0;
        parameter(instance, 0, normalized);
        f0r_get_param_value(instance, &value, 0);
        parameter(instance, 0, value);
        f0r_get_param_value(instance, &again, 0);
        if (fabs(value - normalized) > 1e-12 || fabs(again - value) > 1e-12)
            goto failed;
        /* Keep truncation on either side of each normalized frame setting;
         * adding a broad epsilon or rounding to nearest changes the bins. */
        if (index > 0) {
            parameter(instance, 0, normalized - 1e-9);
            f0r_get_param_value(instance, &value, 0);
            if (fabs(value - (index - 1) / 15.0) > 1e-12) goto failed;
        }
        if (index < 15) {
            parameter(instance, 0, normalized + 1e-9);
            f0r_get_param_value(instance, &value, 0);
            if (fabs(value - normalized) > 1e-12) goto failed;
        }
    }
    parameter(instance, 0, 0.5);
    f0r_get_param_value(instance, &value, 0);
    if (fabs(value - 7.0 / 15.0) > 1e-12) goto failed;
    parameter(instance, 0, -1.0);
    f0r_get_param_value(instance, &value, 0);
    if (value != 0.0) goto failed;
    parameter(instance, 0, 2.0);
    f0r_get_param_value(instance, &value, 0);
    if (value != 1.0) goto failed;
    f0r_destruct(instance);
    return 0;
failed:
    fprintf(stderr, "Delay did not return its normalized frame setting\n");
    f0r_destruct(instance);
    return 1;
}

static int sequence(uint32_t *input, uint32_t *output,
                    unsigned int delay, int simple, int inplace)
{
    unsigned int frame, phase = 0, phase_frames = 0;
    unsigned int channel_frames[3] = { 0, 0, 0 };
    f0r_instance_t instance = f0r_construct(width, height);
    if (!instance) return 1;
    parameter(instance, 0, (delay - 1) / 15.0);
    parameter(instance, 1, simple);
    for (frame = 0; frame < 3 * delay + 5; ++frame) {
        if (simple) {
            if (frame) {
                channel_frames[phase] = frame;
                if (++phase_frames == delay) {
                    phase_frames = 0;
                    phase = (phase + 1) % 3;
                }
            }
        } else {
            channel_frames[0] = frame > 2 * delay ? frame - 2 * delay : 0;
            channel_frames[1] = frame > delay ? frame - delay : 0;
            channel_frames[2] = frame;
        }
        if (step(instance, input, output, frame, channel_frames[0],
                 channel_frames[1], channel_frames[2], inplace)) {
            fprintf(stderr, "delay=%u simple=%d\n", delay, simple);
            f0r_destruct(instance);
            return 1;
        }
    }
    f0r_destruct(instance);
    return 0;
}

static int mode_changes(uint32_t *input, uint32_t *output)
{
    int errors = 0;
    f0r_instance_t instance = f0r_construct(width, height);
    if (!instance) return 1;
    errors += step(instance, input, output, 0, 0, 0, 0, 0);
    errors += step(instance, input, output, 1, 0, 0, 1, 0);
    parameter(instance, 1, 1.0);
    errors += step(instance, input, output, 2, 2, 2, 2, 0);
    parameter(instance, 1, 1.0); /* Setting the same mode must preserve its state. */
    errors += step(instance, input, output, 3, 3, 2, 2, 0);
    parameter(instance, 1, 0.0);
    errors += step(instance, input, output, 4, 4, 4, 4, 0);
    errors += step(instance, input, output, 5, 4, 4, 5, 0);
    parameter(instance, 0, 1.0 / 15.0); /* Delay becomes two frames. */
    errors += step(instance, input, output, 6, 6, 6, 6, 0);
    parameter(instance, 0, 0.0);
    errors += step(instance, input, output, 7, 7, 7, 7, 0);
    f0r_destruct(instance);
    return errors;
}

static int allocation_failures(uint32_t *input, uint32_t *output)
{
    int errors = 0;
    f0r_instance_t instance;
    fail_calloc = 1;
    instance = f0r_construct(width, height);
    if (instance || fail_calloc) {
        fprintf(stderr, "Instance allocation failure was not returned as NULL\n");
        if (instance) f0r_destruct(instance);
        return 1;
    }
    instance = f0r_construct(UINT_MAX, UINT_MAX);
    if (instance) {
        fprintf(stderr, "Overflowing frame allocation was accepted\n");
        f0r_destruct(instance);
        return 1;
    }
    instance = f0r_construct(width, height);
    if (!instance) return 1;
    fail_realloc = 1;
    errors += step(instance, input, output, 0, 0, 0, 0, 0);
    errors += fail_realloc != 0;
    errors += step(instance, input, output, 1, 1, 1, 1, 0);
    errors += step(instance, input, output, 2, 1, 1, 2, 0);
    parameter(instance, 0, 1.0 / 15.0);
    fail_realloc = 1;
    errors += step(instance, input, output, 3, 3, 3, 3, 0);
    errors += fail_realloc != 0;
    errors += step(instance, input, output, 4, 4, 4, 4, 0);
    errors += step(instance, input, output, 5, 4, 4, 5, 0);
    parameter(instance, 1, 1.0);
    parameter(instance, 0, 0.0);
    fail_malloc = 1;
    errors += step(instance, input, output, 6, 6, 6, 6, 0);
    errors += fail_malloc != 0;
    errors += step(instance, input, output, 7, 7, 7, 7, 0);
    errors += step(instance, input, output, 8, 8, 7, 7, 0);
    f0r_destruct(instance);
    return errors;
}

int main(void)
{
    unsigned int delay;
    int simple, inplace, errors;
    void *storage;
    uint32_t *input, *output;
    if (!f0r_init()) return 1;
    storage = malloc(2 * pixels * sizeof(uint32_t) + 15);
    if (!storage) { f0r_deinit(); return 1; }
    input = (uint32_t *)(((uintptr_t)storage + 15) & ~(uintptr_t)15);
    output = input + pixels;
    errors = mapping();
    for (delay = 1; delay <= 16; ++delay)
        for (simple = 0; simple < 2; ++simple)
            for (inplace = 0; inplace < 2; ++inplace)
                errors += sequence(input, output, delay, simple, inplace);
    errors += mode_changes(input, output);
    errors += allocation_failures(input, output);
    free(storage);
    f0r_deinit();
    return errors ? 1 : 0;
}
