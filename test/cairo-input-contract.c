#include <dlfcn.h>
#include <cairo.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "frei0r.h"

int main(int argc, char **argv)
{
  void *module;
  int (*init)(void);
  void (*deinit)(void);
  void (*get_info)(f0r_plugin_info_t *);
  f0r_instance_t (*construct)(unsigned int, unsigned int);
  void (*destruct)(f0r_instance_t);
  void (*set_param)(f0r_instance_t, f0r_param_t, int);
  void (*update)(f0r_instance_t, double, const uint32_t *, uint32_t *);
  void (*update2)(f0r_instance_t, double, const uint32_t *, const uint32_t *,
                  const uint32_t *, uint32_t *);
  f0r_plugin_info_t info;
  f0r_instance_t instance;
  uint32_t storage[6 * 64 + 4];
  uint32_t *first = (uint32_t *)(((uintptr_t)storage + 15) & ~(uintptr_t)15);
  uint32_t *second = first + 64;
  uint32_t *saved_first = second + 64;
  uint32_t *saved_second = saved_first + 64;
  uint32_t *output = saved_second + 64;
  uint32_t *repeated = output + 64;
  const size_t frame_bytes = 64 * sizeof(uint32_t);
  unsigned int pass, index;
  int failed = 0;

  if (argc != 2 || !(module = dlopen(argv[1], RTLD_NOW))) {
    fprintf(stderr, "Cannot load Cairo plugin\n");
    return 1;
  }
#define LOAD(symbol, target) \
  do { \
    *(void **)(&(target)) = dlsym(module, symbol); \
    if (!(target)) { fprintf(stderr, "Missing %s\n", symbol); return 1; } \
  } while (0)
  LOAD("f0r_init", init);
  LOAD("f0r_deinit", deinit);
  LOAD("f0r_get_plugin_info", get_info);
  LOAD("f0r_construct", construct);
  LOAD("f0r_destruct", destruct);
  LOAD("f0r_set_param_value", set_param);
  get_info(&info);
  if (info.plugin_type == F0R_PLUGIN_TYPE_FILTER) {
    LOAD("f0r_update", update);
  } else {
    LOAD("f0r_update2", update2);
  }
#undef LOAD
  if (!init() || !(instance = construct(8, 8))) return 1;

  /* Opaque, translucent and transparent frames must all remain read-only. */
  for (pass = 0; pass < 3 && !failed; ++pass) {
    for (index = 0; index < 64; ++index) {
      unsigned char *a = (unsigned char *)&first[index];
      unsigned char *b = (unsigned char *)&second[index];
      a[0] = 16; a[1] = 32; a[2] = 64;
      b[0] = 200; b[1] = 100; b[2] = 50;
      a[3] = pass == 0 ? 255 : pass == 1 ? 64 : 0;
      b[3] = pass == 0 ? 255 : pass == 1 ? 128 : 0;
    }
    memcpy(saved_first, first, frame_bytes);
    memcpy(saved_second, second, frame_bytes);
    if (info.plugin_type == F0R_PLUGIN_TYPE_FILTER) {
      double opacity = 0.0;
      set_param(instance, &opacity, 2);
      set_param(instance, &opacity, 4);
      update(instance, 0.0, first, output);
      update(instance, 0.0, first, repeated);
    } else {
      update2(instance, 0.0, first, second, NULL, output);
      update2(instance, 0.0, first, second, NULL, repeated);
    }
    if (memcmp(first, saved_first, frame_bytes) ||
        memcmp(second, saved_second, frame_bytes) ||
        memcmp(output, repeated, frame_bytes)) {
      fprintf(stderr, "%s: input mutation or unstable output, alpha case %u\n",
              info.name, pass);
      failed = 1;
    }
    if (pass == 0) {
      const uint32_t *expected = info.plugin_type == F0R_PLUGIN_TYPE_FILTER
                                 ? saved_first : saved_second;
      if (memcmp(output, expected, frame_bytes)) {
        fprintf(stderr, "%s: opaque RGBA colors or alpha changed\n", info.name);
        failed = 1;
      }
    }
  }

  if (!failed && info.plugin_type == F0R_PLUGIN_TYPE_FILTER) {
    f0r_param_color_t color = {0.25, 0.5, 0.75};
    double opacity = 1.0;
    set_param(instance, &color, 1);
    set_param(instance, &color, 3);
    set_param(instance, &opacity, 2);
    set_param(instance, &opacity, 4);
    update(instance, 0.0, first, output);
    for (index = 0; index < 64; ++index) {
      const unsigned char *pixel = (const unsigned char *)&output[index];
      if (pixel[0] < 63 || pixel[0] > 64 || pixel[1] < 127 ||
          pixel[1] > 128 || pixel[2] < 191 || pixel[2] > 192 || pixel[3] != 255) {
        fprintf(stderr, "%s: gradient RGBA color or alpha changed\n", info.name);
        failed = 1;
        break;
      }
    }
  }

  destruct(instance);
  deinit();
  /* Release Cairo's process-wide caches before unloading it under leak checks. */
  cairo_debug_reset_static_data();
  dlclose(module);
  return failed;
}
