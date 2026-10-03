#include <math.h>
#include <stdalign.h>
#include <stdint.h>
#include <string.h>

#include "frei0r-demo-contract.h"

static uint32_t digest(const uint32_t *frame, size_t pixels)
{
  const unsigned char *bytes = (const unsigned char *)frame;
  uint32_t value = UINT32_C(2166136261);
  size_t index;

  for (index = 0; index < pixels * sizeof(*frame); ++index)
    value = (value ^ bytes[index]) * UINT32_C(16777619);
  return value;
}

int main(void)
{
  enum { width = 320, height = 240, pixels = width * height };
  uint32_t *input;
  uint32_t *output;
  size_t catalog_index;

  if (frei0r_demo_select(frei0r_demo_catalog_count(), width, height) !=
        FREI0R_DEMO_ERROR_CATALOG_INDEX ||
      frei0r_demo_select(0, 319, height) != FREI0R_DEMO_ERROR_DIMENSIONS ||
      frei0r_demo_catalog_count() != frei0r_demo_manifest_count())
    return 1;

  for (catalog_index = 0; catalog_index < frei0r_demo_catalog_count();
       ++catalog_index) {
    size_t parameter_index;

    if (!frei0r_demo_catalog_id(catalog_index) ||
        !frei0r_demo_catalog_name(catalog_index) ||
        !frei0r_demo_catalog_author(catalog_index) ||
        !frei0r_demo_catalog_explanation(catalog_index) ||
        !frei0r_demo_contract_color_model_supported(
          frei0r_demo_catalog_color_model(catalog_index)) ||
        frei0r_demo_select(catalog_index, width, height) != FREI0R_DEMO_OK)
      return 2;

    input = (uint32_t *)frei0r_demo_input_pointer();
    output = (uint32_t *)frei0r_demo_output_pointer();
    if (!input || !output || (uintptr_t)input % FREI0R_DEMO_FRAME_ALIGNMENT ||
        (uintptr_t)output % FREI0R_DEMO_FRAME_ALIGNMENT)
      return 3;
    for (parameter_index = 0; parameter_index < pixels; ++parameter_index)
      input[parameter_index] = UINT32_C(0xff000000) |
        (uint32_t)((parameter_index * 2654435761u) >> 8);
    for (parameter_index = 0;
         parameter_index < frei0r_demo_parameter_count(); ++parameter_index) {
      int type = frei0r_demo_parameter_type(parameter_index);

      if (!frei0r_demo_parameter_name(parameter_index) ||
          !frei0r_demo_parameter_explanation(parameter_index))
        return 4;
      if (type == F0R_PARAM_BOOL || type == F0R_PARAM_DOUBLE) {
        double value = frei0r_demo_get_parameter_scalar(parameter_index);
        if (!isfinite(value) || frei0r_demo_set_parameter_scalar(parameter_index,
            value < 0.5 ? 0.75 : 0.25) != FREI0R_DEMO_OK)
          return 5;
      } else if (type == F0R_PARAM_COLOR) {
        if (frei0r_demo_set_parameter_color(parameter_index, 0.25, 0.5, 0.75) !=
              FREI0R_DEMO_OK ||
            fabs(frei0r_demo_get_parameter_color_component(parameter_index, 1) - 0.5) >
              0.0001)
          return 6;
      } else if (type == F0R_PARAM_POSITION) {
        if (frei0r_demo_set_parameter_position(parameter_index, 0.25, 0.75) !=
              FREI0R_DEMO_OK ||
            fabs(frei0r_demo_get_parameter_position_component(parameter_index, 1) - 0.75) >
              0.000001)
          return 7;
      } else {
        return 8;
      }
    }
    if (frei0r_demo_update((double)catalog_index / 30.0) != FREI0R_DEMO_OK ||
        digest(output, pixels) == 0 || frei0r_demo_reset_parameters() != FREI0R_DEMO_OK)
      return 9;
  }
  frei0r_demo_shutdown();
  if (frei0r_demo_select(0, width, height) != FREI0R_DEMO_ERROR_SHUTDOWN)
    return 10;
  return 0;
}
