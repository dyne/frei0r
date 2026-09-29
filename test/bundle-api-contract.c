#include <stdint.h>
#include <string.h>

#include "frei0r/bundle.h"

static int check_descriptors(const f0r_plugin_descriptor_t *first,
                             const f0r_plugin_descriptor_t *second)
{
  if (!first || !second || first == second ||
      first->descriptor_size != F0R_PLUGIN_DESCRIPTOR_SIZE ||
      second->descriptor_size != F0R_PLUGIN_DESCRIPTOR_SIZE ||
      first->descriptor_version != F0R_PLUGIN_DESCRIPTOR_VERSION ||
      second->descriptor_version != F0R_PLUGIN_DESCRIPTOR_VERSION ||
      !first->id || !second->id || strcmp(first->id, second->id) == 0 ||
      !first->init || !first->deinit || !first->get_plugin_info ||
      !first->get_param_info || !first->construct || !first->destruct ||
      !first->set_param_value || !first->get_param_value || !first->update ||
      !second->init || !second->deinit || !second->get_plugin_info ||
      !second->get_param_info || !second->construct || !second->destruct ||
      !second->set_param_value || !second->get_param_value || !second->update ||
      first->update2 || second->update2)
    return 1;
  return 0;
}

int main(void)
{
  const f0r_plugin_descriptor_t *brightness;
  const f0r_plugin_descriptor_t *invert0r;
  f0r_instance_t brightness_instance;
  f0r_instance_t invert0r_instance;
  uint32_t input[64];
  uint32_t brightness_output[64];
  uint32_t invert0r_output[64];
  size_t index;
  double value = 0.75;

  if (f0r_bundle_plugin_count() != 2 ||
      f0r_bundle_plugin_by_index(2) != NULL ||
      f0r_bundle_plugin_by_id(NULL) != NULL ||
      f0r_bundle_plugin_by_id("missing") != NULL)
    return 2;

  brightness = f0r_bundle_plugin_by_index(0);
  invert0r = f0r_bundle_plugin_by_index(1);
  if (check_descriptors(brightness, invert0r) ||
      f0r_bundle_plugin_by_index(0) != brightness ||
      f0r_bundle_plugin_by_index(1) != invert0r ||
      f0r_bundle_plugin_by_id(brightness->id) != brightness ||
      f0r_bundle_plugin_by_id(invert0r->id) != invert0r)
    return 3;

  if (!brightness->init() || !invert0r->init())
    return 4;
  brightness_instance = brightness->construct(8, 8);
  invert0r_instance = invert0r->construct(8, 8);
  if (!brightness_instance || !invert0r_instance)
    return 5;

  brightness->set_param_value(brightness_instance, &value, 0);
  for (index = 0; index < 64; ++index)
    input[index] = UINT32_C(0x80402010);
  brightness->update(brightness_instance, 0.0, input, brightness_output);
  invert0r->update(invert0r_instance, 0.0, input, invert0r_output);
  if (brightness_output[0] != UINT32_C(0x80a09088) ||
      invert0r_output[0] != UINT32_C(0x80bfdfef))
    return 6;

  brightness->destruct(brightness_instance);
  invert0r->destruct(invert0r_instance);
  brightness->deinit();
  invert0r->deinit();
  return 0;
}
