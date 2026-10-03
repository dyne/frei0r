#include "frei0r-demo-contract.h"

static const char *const manifest[] = {
#define FREI0R_DEMO_MANIFEST_ENTRY(identifier) #identifier,
  FREI0R_DEMO_FILTER_MANIFEST(FREI0R_DEMO_MANIFEST_ENTRY)
#undef FREI0R_DEMO_MANIFEST_ENTRY
};

int frei0r_demo_contract_validate_frame(const void *frame, uint32_t width,
                                        uint32_t height)
{
  if (!frame)
    return FREI0R_DEMO_ERROR_ARGUMENT;
  if (width < FREI0R_DEMO_MIN_DIMENSION ||
      height < FREI0R_DEMO_MIN_DIMENSION || width % 8u || height % 8u ||
      (uint64_t)width * height > FREI0R_DEMO_MAX_PIXELS)
    return FREI0R_DEMO_ERROR_DIMENSIONS;
  if ((uintptr_t)frame % FREI0R_DEMO_FRAME_ALIGNMENT)
    return FREI0R_DEMO_ERROR_ALIGNMENT;
  return FREI0R_DEMO_OK;
}

int frei0r_demo_contract_color_model_supported(int color_model)
{
  return color_model == F0R_COLOR_MODEL_RGBA8888 ||
         color_model == F0R_COLOR_MODEL_PACKED32;
}

int frei0r_demo_contract_parameter_type_supported(int parameter_type)
{
  return parameter_type == F0R_PARAM_BOOL || parameter_type == F0R_PARAM_DOUBLE ||
         parameter_type == F0R_PARAM_COLOR || parameter_type == F0R_PARAM_POSITION;
}

size_t frei0r_demo_manifest_count(void)
{
  return sizeof(manifest) / sizeof(manifest[0]);
}

const char *frei0r_demo_manifest_id(size_t index)
{
  return index < frei0r_demo_manifest_count() ? manifest[index] : NULL;
}
