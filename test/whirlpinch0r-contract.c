#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <frei0r.h>

enum { width = 64, height = 64, pixels = width * height };

typedef struct {
    void (*set)(f0r_instance_t, f0r_param_t, int);
    void (*update)(f0r_instance_t, double, const uint32_t *, uint32_t *);
} plugin_api_t;

static void parameters(const plugin_api_t *api, f0r_instance_t instance,
                       double whirl, double pinch, double radius,
                       double center_x, double center_y)
{
    double values[] = { whirl, pinch, radius, center_x, center_y };
    int i;
    for (i = 0; i < 5; ++i) api->set(instance, &values[i], i);
}

static int pixel_matches(const uint32_t *frame, unsigned int x, unsigned int y,
                         unsigned int b, unsigned int g,
                         unsigned int r, unsigned int a)
{
    const uint8_t *p = (const uint8_t *)&frame[y * width + x];
    if (p[0] == b && p[1] == g && p[2] == r && p[3] == a) return 0;
    fprintf(stderr, "Pixel (%u,%u): BGRA (%u,%u,%u,%u), expected (%u,%u,%u,%u)\n",
            x, y, p[0], p[1], p[2], p[3], b, g, r, a);
    return 1;
}

static int interpolation(const plugin_api_t *api, f0r_instance_t instance,
                         uint32_t *input, uint32_t *output)
{
    unsigned int i, transparent;
    int errors = 0;
    /* This maps output (2,0) exactly to source (3.125,0). */
    parameters(api, instance, 0.5, 1.0, 0.125, 0.0, 0.0);
    for (transparent = 0; transparent < 2; ++transparent) {
        for (i = 0; i < pixels; ++i) {
            uint8_t *p = (uint8_t *)&input[i];
            int left = i % width <= 3;
            p[0] = left ? 80 : 160;
            p[1] = left ? 40 : 220;
            p[2] = left ? 200 : 20;
            p[3] = transparent ? (left ? 64 : 192) : 255;
        }
        api->update(instance, 0.0, input, output);
        if (transparent)
            errors += pixel_matches(output, 2, 0, 104, 94, 146, 80);
        else
            errors += pixel_matches(output, 2, 0, 90, 63, 178, 255);
    }
    return errors;
}

static int hidden_colors(const plugin_api_t *api, f0r_instance_t instance,
                         uint32_t *input_a, uint32_t *input_b,
                         uint32_t *output_a, uint32_t *output_b)
{
    unsigned int i, mixed = 0;
    parameters(api, instance, 0.65, 0.5, 0.5, 0.5, 0.5);
    for (i = 0; i < pixels; ++i) {
        uint8_t *a = (uint8_t *)&input_a[i];
        uint8_t *b = (uint8_t *)&input_b[i];
        if (i % width < width / 2) {
            a[0] = a[1] = a[3] = 0;
            a[2] = 255; /* Invisible red must not bleed into visible white. */
            memset(b, 0, 4);
        } else {
            memset(a, 255, 4);
            memset(b, 255, 4);
        }
    }
    api->update(instance, 0.0, input_a, output_a);
    api->update(instance, 0.0, input_b, output_b);
    for (i = 0; i < pixels; ++i) {
        const uint8_t *a = (const uint8_t *)&output_a[i];
        const uint8_t *b = (const uint8_t *)&output_b[i];
        if (a[3] != b[3] ||
            (a[3] && (memcmp(a, b, 4) ||
                       a[0] != 255 || a[1] != 255 || a[2] != 255))) {
            fprintf(stderr, "Hidden RGB affects visible output at pixel %u\n", i);
            return 1;
        }
        if (a[3] > 0 && a[3] < 255) ++mixed;
    }
    if (!mixed) {
        fprintf(stderr, "Transparency case did not exercise a mixed-alpha pixel\n");
        return 1;
    }

    /* Exercise the scratch path with the same transparent-edge input. */
    memcpy(input_b, input_a, pixels * sizeof(*input_a));
    api->update(instance, 0.0, input_b, input_b);
    if (memcmp(input_b, output_a, pixels * sizeof(*input_b))) {
        fprintf(stderr, "In-place rendering differs from separate buffers\n");
        return 1;
    }

    /* A wholly transparent interpolated sample must not divide by zero. */
    for (i = 0; i < pixels; ++i) {
        uint8_t *p = (uint8_t *)&input_a[i];
        p[0] = 20; p[1] = 50; p[2] = 100; p[3] = 0;
    }
    api->update(instance, 0.0, input_a, output_a);
    for (i = 0; i < pixels; ++i) {
        if (((const uint8_t *)&output_a[i])[3]) {
            fprintf(stderr, "Transparent input produced visible output\n");
            return 1;
        }
    }
    return pixel_matches(output_a, 32, 32, 0, 0, 0, 0);
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
    void *storage;
    uint32_t *frames[4];
    unsigned int i;
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
    storage = malloc(4 * pixels * sizeof(uint32_t) + 15);
    if (!instance || !storage) {
        if (instance) destruct(instance);
        free(storage);
        deinit();
        dlclose(module);
        return 1;
    }
    frames[0] = (uint32_t *)(((uintptr_t)storage + 15) & ~(uintptr_t)15);
    for (i = 1; i < 4; ++i) frames[i] = frames[0] + i * pixels;

    /* Neutral parameters must retain all channels, including hidden RGB. */
    for (i = 0; i < pixels; ++i) frames[0][i] = i * 1234567u;
    api.update(instance, 0.0, frames[0], frames[1]);
    errors = memcmp(frames[0], frames[1], pixels * sizeof(uint32_t)) != 0;
    if (errors) fprintf(stderr, "Neutral rendering changed the frame\n");
    errors += interpolation(&api, instance, frames[0], frames[1]);
    errors += hidden_colors(&api, instance, frames[0], frames[1], frames[2], frames[3]);

    free(storage);
    destruct(instance);
    deinit();
    dlclose(module);
    return errors ? 1 : 0;
}
