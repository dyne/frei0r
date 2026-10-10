/* Check that the cairo plugins do not write a const input frame.
 *
 * frei0r-run stores opaque alpha in the high byte of a uint32_t. On
 * little-endian that byte is 255, so frei0r_cairo_premultiply_rgba does
 * not change the buffer and the existing contract check stays green.
 * A non-opaque alpha byte is written on either endian.
 */

#include <dlfcn.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <frei0r.h>

#define FRAME_W 8
#define FRAME_H 8
#define FRAME_PIXELS (FRAME_W * FRAME_H)
#define FRAME_BYTES (FRAME_PIXELS * 4)

typedef int (*f0r_init_f)(void);
typedef void (*f0r_deinit_f)(void);
typedef void (*f0r_get_plugin_info_f)(f0r_plugin_info_t *info);
typedef f0r_instance_t (*f0r_construct_f)(unsigned int width, unsigned int height);
typedef void (*f0r_destruct_f)(f0r_instance_t instance);
typedef void (*f0r_update_f)(f0r_instance_t instance, double time,
                             const uint32_t *inframe, uint32_t *outframe);
typedef void (*f0r_update2_f)(f0r_instance_t instance, double time,
                              const uint32_t *inframe1, const uint32_t *inframe2,
                              const uint32_t *inframe3, uint32_t *outframe);

static void fill_non_opaque(unsigned char *frame)
{
  int i;

  for (i = 0; i < FRAME_PIXELS; i++) {
    frame[i * 4 + 0] = 200;
    frame[i * 4 + 1] = 100;
    frame[i * 4 + 2] = 50;
    frame[i * 4 + 3] = 128;
  }
}

static int frame_changed(const unsigned char *before, const unsigned char *after)
{
  return memcmp(before, after, FRAME_BYTES) != 0;
}

int main(int argc, char **argv)
{
  void *handle;
  f0r_init_f init;
  f0r_deinit_f deinit;
  f0r_get_plugin_info_f get_info;
  f0r_construct_f construct;
  f0r_destruct_f destruct;
  f0r_update_f update = NULL;
  f0r_update2_f update2 = NULL;
  f0r_plugin_info_t info;
  f0r_instance_t instance;
  unsigned char in1[FRAME_BYTES];
  unsigned char in2[FRAME_BYTES];
  unsigned char saved1[FRAME_BYTES];
  unsigned char saved2[FRAME_BYTES];
  unsigned char out[FRAME_BYTES];
  int failed = 0;
  const char *name;

  /* 200 * 128 >> 8 is 100, so an in-place premultiply must change byte 0. */
  if (((200 * 128) >> 8) == 200) {
    fprintf(stderr, "test pixel is not changed by premultiply\n");
    return 2;
  }

  if (argc != 2) {
    fprintf(stderr, "usage: %s <plugin>\n", argv[0]);
    return 2;
  }

  handle = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
  if (!handle) {
    fprintf(stderr, "dlopen %s: %s\n", argv[1], dlerror());
    return 2;
  }

  init = (f0r_init_f)dlsym(handle, "f0r_init");
  deinit = (f0r_deinit_f)dlsym(handle, "f0r_deinit");
  get_info = (f0r_get_plugin_info_f)dlsym(handle, "f0r_get_plugin_info");
  construct = (f0r_construct_f)dlsym(handle, "f0r_construct");
  destruct = (f0r_destruct_f)dlsym(handle, "f0r_destruct");
  update = (f0r_update_f)dlsym(handle, "f0r_update");
  update2 = (f0r_update2_f)dlsym(handle, "f0r_update2");
  if (!init || !deinit || !get_info || !construct || !destruct) {
    fprintf(stderr, "%s: missing plugin entry point\n", argv[1]);
    dlclose(handle);
    return 2;
  }

  if (!init()) {
    fprintf(stderr, "%s: f0r_init failed\n", argv[1]);
    dlclose(handle);
    return 2;
  }

  memset(&info, 0, sizeof(info));
  get_info(&info);
  name = info.name ? info.name : argv[1];
  instance = construct(FRAME_W, FRAME_H);
  if (!instance) {
    fprintf(stderr, "%s: f0r_construct failed\n", name);
    deinit();
    dlclose(handle);
    return 2;
  }

  fill_non_opaque(in1);
  fill_non_opaque(in2);
  memcpy(saved1, in1, FRAME_BYTES);
  memcpy(saved2, in2, FRAME_BYTES);
  memset(out, 0, FRAME_BYTES);

  if (info.plugin_type == F0R_PLUGIN_TYPE_FILTER) {
    if (!update) {
      fprintf(stderr, "%s: missing f0r_update\n", name);
      failed = 2;
    } else {
      update(instance, 0.0, (const uint32_t *)in1, (uint32_t *)out);
      if (frame_changed(saved1, in1)) {
        fprintf(stderr, "API contract violation in %s: modified input frame 1\n", name);
        failed = 1;
      }
    }
  } else if (info.plugin_type == F0R_PLUGIN_TYPE_MIXER2) {
    if (!update2) {
      fprintf(stderr, "%s: missing f0r_update2\n", name);
      failed = 2;
    } else {
      update2(instance, 0.0, (const uint32_t *)in1, (const uint32_t *)in2,
              NULL, (uint32_t *)out);
      if (frame_changed(saved1, in1)) {
        fprintf(stderr, "API contract violation in %s: modified input frame 1\n", name);
        failed = 1;
      }
      if (frame_changed(saved2, in2)) {
        fprintf(stderr, "API contract violation in %s: modified input frame 2\n", name);
        failed = 1;
      }
    }
  } else {
    fprintf(stderr, "%s: unexpected plugin type %d\n", name, info.plugin_type);
    failed = 2;
  }

  destruct(instance);
  deinit();
  dlclose(handle);
  return failed;
}
