#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <frei0r.h>

enum { width = 64, height = 32, pixels = width * height };
enum { p_offset, p_mode, p_blend, p_contrast, p_threshold, p_monochrome };

typedef struct {
    void (*set)(f0r_instance_t, f0r_param_t, int);
    void (*update)(f0r_instance_t, double, const uint32_t *, uint32_t *);
} plugin_api_t;

static void set(const plugin_api_t *api, f0r_instance_t instance, int index, double value)
{
    api->set(instance, &value, index);
}

/* Gray background (100) with an 8x8 square (250) starting at column x0. */
static void scene(uint32_t *frame, unsigned int x0)
{
    unsigned int x, y;
    for (y = 0; y < height; ++y) {
        for (x = 0; x < width; ++x) {
            uint8_t *p = (uint8_t *)&frame[y * width + x];
            uint8_t v = (x >= x0 && x < x0 + 8 && y >= 8 && y < 16) ? 250 : 100;
            p[0] = p[1] = p[2] = v;
            p[3] = 255;
        }
    }
}

static int expect(const uint32_t *frame, unsigned int x, unsigned int y, int channel,
                  unsigned int lo, unsigned int hi, const char *what)
{
    unsigned int v = ((const uint8_t *)&frame[y * width + x])[channel];
    if (v >= lo && v <= hi) return 0;
    fprintf(stderr, "%s: pixel (%u,%u) channel %d is %u, expected %u..%u\n",
            what, x, y, channel, v, lo, hi);
    return 1;
}

static int run(const plugin_api_t *api, f0r_instance_t instance, uint32_t *in, uint32_t *out)
{
    int errors = 0;
    unsigned int f;

    /* 0.1 selects a 200 ms offset; at 25 fps that is five frames. */
    set(api, instance, p_offset, 0.1);
    set(api, instance, p_mode, 0.0);
    set(api, instance, p_blend, 0.5);
    set(api, instance, p_contrast, 0.0);
    set(api, instance, p_threshold, 0.0);
    set(api, instance, p_monochrome, 0.0);

    /* The square moves one pixel per frame: frame 19 is compared with 14. */
    for (f = 0; f < 20; ++f) {
        scene(in, f);
        api->update(instance, f / 25.0, in, out);
    }
    errors += expect(out, 0, 0, 0, 127, 128, "overlay static area");
    errors += expect(out, 20, 10, 0, 127, 128, "overlay unchanged square");
    errors += expect(out, 24, 10, 0, 202, 203, "overlay new position");
    errors += expect(out, 15, 10, 0, 52, 53, "overlay old position");
    errors += expect(out, 0, 0, 3, 255, 255, "overlay alpha");

    set(api, instance, p_mode, 0.5);
    scene(in, 20);
    api->update(instance, 20 / 25.0, in, out);
    errors += expect(out, 0, 0, 0, 0, 0, "difference static area");
    errors += expect(out, 26, 10, 0, 150, 150, "difference moving area");

    set(api, instance, p_mode, 1.0);
    scene(in, 21);
    api->update(instance, 21 / 25.0, in, out);
    errors += expect(out, 0, 0, 3, 0, 0, "mask static alpha");
    errors += expect(out, 27, 10, 3, 150, 150, "mask moving alpha");
    errors += expect(out, 27, 10, 0, 250, 250, "mask keeps colors");

    /* Rendering the same time again gives the same result. */
    api->update(instance, 21 / 25.0, in, out);
    errors += expect(out, 27, 10, 3, 150, 150, "same-time render");

    /* Seeking backwards drops the history: nothing has changed yet. */
    scene(in, 5);
    api->update(instance, 0.0, in, out);
    errors += expect(out, 8, 10, 3, 0, 0, "seek backwards");

    /* Differences under the threshold are ignored. */
    set(api, instance, p_mode, 0.5);
    set(api, instance, p_threshold, 0.7);
    scene(in, 6);
    api->update(instance, 1 / 25.0, in, out);
    errors += expect(out, 13, 10, 0, 0, 0, "threshold");
    set(api, instance, p_threshold, 0.0);

    /* A zero offset compares with the previous frame. */
    set(api, instance, p_offset, 0.0);
    scene(in, 7);
    api->update(instance, 2 / 25.0, in, out);
    errors += expect(out, 14, 10, 0, 150, 150, "previous frame");

    /* In-place rendering keeps an intact copy of the input. */
    set(api, instance, p_mode, 0.0);
    scene(in, 8);
    api->update(instance, 3 / 25.0, in, in);
    scene(in, 8);
    api->update(instance, 4 / 25.0, in, out);
    errors += expect(out, 15, 10, 0, 127, 128, "in-place history");

    /* Luminance comparison with maximum contrast saturates the motion. */
    set(api, instance, p_monochrome, 1.0);
    set(api, instance, p_contrast, 1.0);
    scene(in, 9);
    api->update(instance, 5 / 25.0, in, out);
    errors += expect(out, 0, 0, 0, 127, 128, "monochrome static area");
    errors += expect(out, 16, 10, 0, 255, 255, "monochrome contrast");

    /* A long history at the maximum offset stays bounded. */
    set(api, instance, p_offset, 1.0);
    for (f = 0; f < 1000; ++f) {
        scene(in, f % 50);
        api->update(instance, 1.0 + f / 60.0, in, out);
    }
    return errors;
}

int main(int argc, char **argv)
{
    void *module;
    int (*init)(void);
    void (*deinit)(void);
    f0r_instance_t (*construct)(unsigned int, unsigned int);
    void (*destruct)(f0r_instance_t);
    plugin_api_t api;
    f0r_instance_t instance;
    uint32_t *in, *out;
    int errors;

    if (argc != 2) return 1;
    module = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!module) {
        fprintf(stderr, "Cannot load plugin: %s\n", dlerror());
        return 1;
    }
    init = (int (*)(void))dlsym(module, "f0r_init");
    deinit = (void (*)(void))dlsym(module, "f0r_deinit");
    construct = (f0r_instance_t (*)(unsigned int, unsigned int))dlsym(module, "f0r_construct");
    destruct = (void (*)(f0r_instance_t))dlsym(module, "f0r_destruct");
    api.set = (void (*)(f0r_instance_t, f0r_param_t, int))dlsym(module, "f0r_set_param_value");
    api.update = (void (*)(f0r_instance_t, double, const uint32_t *, uint32_t *))dlsym(module, "f0r_update");
    if (!init || !deinit || !construct || !destruct || !api.set || !api.update) {
        fprintf(stderr, "Missing plugin entry point\n");
        dlclose(module);
        return 1;
    }
    if (!init()) {
        dlclose(module);
        return 1;
    }
    instance = construct(width, height);
    in = (uint32_t *)malloc(pixels * sizeof(uint32_t));
    out = (uint32_t *)malloc(pixels * sizeof(uint32_t));
    if (!instance || !in || !out) {
        if (instance) destruct(instance);
        free(in);
        free(out);
        deinit();
        dlclose(module);
        return 1;
    }

    errors = run(&api, instance, in, out);

    free(in);
    free(out);
    destruct(instance);
    deinit();
    dlclose(module);
    return errors ? 1 : 0;
}
