#include <stdint.h>
#include <stdlib.h>

#include <frei0r.h>

enum { frame_width = 320, frame_height = 240 };

int measure_pr0file_wasi_run(void)
{
  uint32_t *input = calloc(frame_width * frame_height, sizeof(*input));
  uint32_t *output = calloc(frame_width * frame_height, sizeof(*output));
  f0r_instance_t instance;
  if (!input || !output) return 1;
  if (!f0r_init()) return 2;
  instance = f0r_construct(frame_width, frame_height);
  if (!instance) return 3;
  f0r_update(instance, 0.0, input, output);
  f0r_destruct(instance);
  f0r_deinit();
  free(input);
  free(output);
  return 0;
}
