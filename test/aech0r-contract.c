#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <frei0r.h>

enum { width = 8, height = 8, pixels = width * height };

static void parameter(f0r_instance_t instance, int index, double value)
{
  f0r_set_param_value(instance, &value, index);
}

static void fill(uint32_t *frame, const uint8_t bgra[4])
{
  unsigned int i;
  for (i = 0; i < pixels; ++i) memcpy(&frame[i], bgra, 4);
}

static int expect(const uint32_t *frame, const uint8_t bgra[4])
{
  unsigned int i;
  for (i = 0; i < pixels; ++i) {
    const uint8_t *p = (const uint8_t *)&frame[i];
    if (memcmp(p, bgra, 4)) {
      fprintf(stderr, "Pixel %u: BGRA (%u,%u,%u,%u), expected (%u,%u,%u,%u)\n",
              i, p[0], p[1], p[2], p[3], bgra[0], bgra[1], bgra[2], bgra[3]);
      return 1;
    }
  }
  return 0;
}

static int channel_sequences(uint32_t *input, uint32_t *output)
{
  const uint8_t seed[3] = { 60, 110, 200 };
  const uint8_t alpha[3] = { 80, 140, 35 };
  unsigned int bright, fade, mask, inplace, frame, channel;

  for (bright = 0; bright < 2; ++bright)
    for (fade = 0; fade < 2; ++fade)
      for (mask = 0; mask < 8; ++mask)
        for (inplace = 0; inplace < 2; ++inplace) {
          f0r_instance_t instance = f0r_construct(width, height);
          uint32_t *destination = inplace ? input : output;
          if (!instance) return 1;
          parameter(instance, 0, fade ? 0.25 : 0.0); /* 0.25 decays by 31. */
          parameter(instance, 1, bright);
          parameter(instance, 2, (mask & 1) != 0); /* Keep RED */
          parameter(instance, 3, (mask & 2) != 0); /* Keep GREEN */
          parameter(instance, 4, (mask & 4) != 0); /* Keep BLUE */

          for (frame = 0; frame < 3; ++frame) {
            uint8_t source[4], expected[4];
            for (channel = 0; channel < 3; ++channel) {
              int kept = (mask & (1u << (2 - channel))) != 0;
              int decay = kept || !fade ? 0 : 31;
              int value = seed[channel] + (bright ? -1 : 1) * (int)frame * decay;
              source[channel] = frame ? (bright ? 0 : 255) : seed[channel];
              expected[channel] = value < 0 ? 0 : (value > 255 ? 255 : value);
            }
            source[3] = expected[3] = alpha[frame];
            fill(input, source);
            if (!inplace) memset(output, 0xa5, pixels * sizeof(*output));
            f0r_update(instance, frame / 30.0, input, destination);
            if (expect(destination, expected) || (!inplace && expect(input, source))) {
              fprintf(stderr, "bright=%u fade=%u keep-mask=%u in-place=%u frame=%u\n",
                      bright, fade, mask, inplace, frame);
              f0r_destruct(instance);
              return 1;
            }
          }
          f0r_destruct(instance);
        }
  return 0;
}

static int strobe_sequence(uint32_t *input, uint32_t *output)
{
  const uint8_t seed[4] = { 60, 110, 200, 80 };
  const uint8_t black[4] = { 0, 0, 0, 35 };
  const uint8_t faded[4] = { 29, 79, 169, 35 };
  unsigned int frame;
  int errors = 0;
  f0r_instance_t instance = f0r_construct(width, height);
  if (!instance) return 1;
  parameter(instance, 0, 0.25);
  parameter(instance, 1, 1.0);
  parameter(instance, 5, 0.125); /* Skip one frame between processed frames. */

  for (frame = 0; frame < 3; ++frame) {
    fill(input, frame ? black : seed);
    memset(output, 0xa5, pixels * sizeof(*output));
    f0r_update(instance, frame / 30.0, input, output);
    errors += expect(output, frame < 2 ? seed : faded);
  }
  f0r_destruct(instance);
  return errors;
}

int main(void)
{
  void *storage;
  uint32_t *input, *output;
  int errors;
  if (!f0r_init()) return 1;
  storage = malloc(2 * pixels * sizeof(uint32_t) + 15);
  if (!storage) {
    f0r_deinit();
    return 1;
  }
  input = (uint32_t *)(((uintptr_t)storage + 15) & ~(uintptr_t)15);
  output = input + pixels;
  errors = channel_sequences(input, output);
  errors += strobe_sequence(input, output);
  free(storage);
  f0r_deinit();
  return errors ? 1 : 0;
}
