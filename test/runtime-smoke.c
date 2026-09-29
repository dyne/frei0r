#include <stdio.h>
#include <string.h>

#include "runtime-smoke.h"

enum {
  runtime_smoke_width = 8,
  runtime_smoke_height = 8,
  runtime_smoke_pixels = runtime_smoke_width * runtime_smoke_height,
  runtime_smoke_metadata_stage = 1,
  runtime_smoke_parameter_stage = 2,
  runtime_smoke_init_stage = 3,
  runtime_smoke_construct_stage = 4,
  runtime_smoke_round_trip_stage = 5,
  runtime_smoke_update_stage = 6,
  runtime_smoke_pixel_stage = 7,
  runtime_smoke_lookup_stage = 8,
  runtime_smoke_index_stage = 9
};

static int failure(unsigned int plugin, unsigned int stage)
{
  return (int)(plugin * 100 + stage);
}

static int exercise_plugin(unsigned int plugin,
                           const runtime_smoke_descriptor_t *descriptor,
                           const char *expected_name,
                           unsigned int expected_params,
                           uint32_t expected_pixel)
{
  f0r_plugin_info_t plugin_info;
  f0r_param_info_t parameter_info;
  f0r_instance_t instance;
  uint32_t input[runtime_smoke_pixels];
  uint32_t output[runtime_smoke_pixels];
  unsigned int index;
  double value;

  memset(&plugin_info, 0, sizeof(plugin_info));
  descriptor->get_plugin_info(&plugin_info);
  if (!plugin_info.name || strcmp(plugin_info.name, expected_name) != 0 ||
      plugin_info.plugin_type != F0R_PLUGIN_TYPE_FILTER ||
      plugin_info.color_model != F0R_COLOR_MODEL_RGBA8888 ||
      plugin_info.num_params != expected_params)
    return failure(plugin, runtime_smoke_metadata_stage);

  for (index = 0; index < expected_params; ++index) {
    memset(&parameter_info, 0, sizeof(parameter_info));
    descriptor->get_param_info(&parameter_info, (int)index);
    if (!parameter_info.name || parameter_info.type != F0R_PARAM_DOUBLE)
      return failure(plugin, runtime_smoke_parameter_stage);
  }

  if (!descriptor->init())
    return failure(plugin, runtime_smoke_init_stage);
  instance = descriptor->construct(runtime_smoke_width, runtime_smoke_height);
  if (!instance) {
    descriptor->deinit();
    return failure(plugin, runtime_smoke_construct_stage);
  }

  if (expected_params) {
    value = 0.75;
    descriptor->set_param_value(instance, &value, 0);
    value = 0.0;
    descriptor->get_param_value(instance, &value, 0);
    if (value != 0.75) {
      descriptor->destruct(instance);
      descriptor->deinit();
      return failure(plugin, runtime_smoke_round_trip_stage);
    }
  }

  for (index = 0; index < runtime_smoke_pixels; ++index)
    input[index] = UINT32_C(0x80402010);
  memset(output, 0, sizeof(output));
  descriptor->update(instance, 0.0, input, output);
  for (index = 0; index < runtime_smoke_pixels; ++index)
    if (output[index] != expected_pixel) {
      descriptor->destruct(instance);
      descriptor->deinit();
      return failure(plugin, runtime_smoke_pixel_stage);
    }

  descriptor->destruct(instance);
  descriptor->deinit();
  return 0;
}

int runtime_smoke_run(void)
{
  const runtime_smoke_descriptor_t *descriptor;
  int status;

  if (runtime_smoke_descriptor_count() != 2 ||
      runtime_smoke_descriptor_by_index(2) != NULL) {
    fprintf(stderr, "runtime-smoke: invalid descriptor index check\n");
    return runtime_smoke_index_stage;
  }
  if (runtime_smoke_descriptor_by_id("missing") != NULL) {
    fprintf(stderr, "runtime-smoke: unknown descriptor lookup check\n");
    return runtime_smoke_lookup_stage;
  }

  descriptor = runtime_smoke_descriptor_by_id("brightness");
  if (!descriptor)
    return failure(1, runtime_smoke_lookup_stage);
  status = exercise_plugin(1, descriptor, "Brightness", 1, UINT32_C(0x80a09088));
  if (status)
    return status;

  descriptor = runtime_smoke_descriptor_by_id("invert0r");
  if (!descriptor)
    return failure(2, runtime_smoke_lookup_stage);
  status = exercise_plugin(2, descriptor, "Invert0r", 0, UINT32_C(0x80bfdfef));
  if (status)
    return status;

  return 0;
}

#ifndef RUNTIME_SMOKE_NO_MAIN
int main(void)
{
  return runtime_smoke_run();
}
#endif
