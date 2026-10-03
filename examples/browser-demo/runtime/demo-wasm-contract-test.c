#include <math.h>
#include <stdalign.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <frei0r/bundle.h>

#include "frei0r-demo-contract.h"

static int check_parameter_round_trip(const f0r_plugin_descriptor_t *descriptor,
                                      f0r_instance_t instance, int index)
{
  f0r_param_info_t parameter = { 0 };

  descriptor->get_param_info(&parameter, index);
  if (!parameter.name || !frei0r_demo_contract_parameter_type_supported(parameter.type))
    return 1;
  if (parameter.type == F0R_PARAM_BOOL || parameter.type == F0R_PARAM_DOUBLE) {
    double value = NAN;
    descriptor->get_param_value(instance, &value, index);
    if (!isfinite(value) || value < 0.0 || value > 1.0)
      return 1;
    descriptor->set_param_value(instance, &value, index);
    value = NAN;
    descriptor->get_param_value(instance, &value, index);
    return !isfinite(value) || value < 0.0 || value > 1.0;
  }
  if (parameter.type == F0R_PARAM_COLOR) {
    f0r_param_color_t value = { NAN, NAN, NAN };
    descriptor->get_param_value(instance, &value, index);
    if (!isfinite(value.r) || !isfinite(value.g) || !isfinite(value.b) ||
        value.r < 0.0 || value.r > 1.0 || value.g < 0.0 || value.g > 1.0 ||
        value.b < 0.0 || value.b > 1.0)
      return 1;
    descriptor->set_param_value(instance, &value, index);
    return 0;
  }
  if (parameter.type == F0R_PARAM_POSITION) {
    f0r_param_position_t value = { NAN, NAN };
    descriptor->get_param_value(instance, &value, index);
    if (!isfinite(value.x) || !isfinite(value.y) || value.x < 0.0 ||
        value.x > 1.0 || value.y < 0.0 || value.y > 1.0)
      return 1;
    descriptor->set_param_value(instance, &value, index);
    return 0;
  }
  return 1;
}

int main(void)
{
  alignas(FREI0R_DEMO_FRAME_ALIGNMENT) uint32_t aligned_frame[64] = { 0 };
  size_t catalog_count = frei0r_demo_manifest_count();
  size_t catalog_index;

  if (catalog_count < 12 || catalog_count > 20 ||
      frei0r_demo_manifest_id(catalog_count) ||
      frei0r_demo_contract_validate_frame(NULL, 8, 8) !=
        FREI0R_DEMO_ERROR_ARGUMENT ||
      frei0r_demo_contract_validate_frame(aligned_frame, 7, 8) !=
        FREI0R_DEMO_ERROR_DIMENSIONS ||
      frei0r_demo_contract_validate_frame(aligned_frame, 8, 7) !=
        FREI0R_DEMO_ERROR_DIMENSIONS ||
      frei0r_demo_contract_validate_frame(aligned_frame, 648, 480) !=
        FREI0R_DEMO_ERROR_DIMENSIONS ||
      frei0r_demo_contract_validate_frame(aligned_frame, 640, 480) !=
        FREI0R_DEMO_OK ||
      frei0r_demo_contract_validate_frame((const unsigned char *)aligned_frame + 4,
                                          8, 8) != FREI0R_DEMO_ERROR_ALIGNMENT ||
      frei0r_demo_contract_color_model_supported(F0R_COLOR_MODEL_BGRA8888) ||
      !frei0r_demo_contract_color_model_supported(F0R_COLOR_MODEL_RGBA8888) ||
      !frei0r_demo_contract_color_model_supported(F0R_COLOR_MODEL_PACKED32) ||
      frei0r_demo_contract_parameter_type_supported(F0R_PARAM_STRING))
    return 1;

  if (f0r_bundle_plugin_count() != catalog_count)
    return 2;

  for (catalog_index = 0; catalog_index < catalog_count; ++catalog_index) {
    const char *id = frei0r_demo_manifest_id(catalog_index);
    const f0r_plugin_descriptor_t *descriptor;
    f0r_plugin_info_t info = { 0 };
    f0r_instance_t instance;
    size_t previous_index;
    int parameter_index;

    if (!id || !*id)
      return 3;
    for (previous_index = 0; previous_index < catalog_index; ++previous_index) {
      if (strcmp(id, frei0r_demo_manifest_id(previous_index)) == 0)
        return 4;
    }
    descriptor = f0r_bundle_plugin_by_index(catalog_index);
    if (!descriptor || strcmp(id, descriptor->id) != 0 ||
        f0r_bundle_plugin_by_id(id) != descriptor || !descriptor->init ||
        !descriptor->deinit || !descriptor->get_plugin_info ||
        !descriptor->get_param_info || !descriptor->construct ||
        !descriptor->destruct || !descriptor->set_param_value ||
        !descriptor->get_param_value || !descriptor->update || descriptor->update2)
      return 5;

    descriptor->get_plugin_info(&info);
    if (!info.name || !*info.name || !info.author || !info.explanation ||
        info.plugin_type != F0R_PLUGIN_TYPE_FILTER ||
        !frei0r_demo_contract_color_model_supported(info.color_model) ||
        info.num_params < 0)
      return 6;

    if (!descriptor->init())
      return 7;
    instance = descriptor->construct(320, 240);
    if (!instance)
      return 8;
    for (parameter_index = 0; parameter_index < info.num_params;
         ++parameter_index) {
      if (check_parameter_round_trip(descriptor, instance, parameter_index))
        return 9;
    }
    descriptor->destruct(instance);
    descriptor->deinit();
  }
  return 0;
}
