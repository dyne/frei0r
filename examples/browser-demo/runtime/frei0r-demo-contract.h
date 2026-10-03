/*
 * Browser demo host contract.
 *
 * The C adapter is the only owner of frei0r descriptors, plugin instances,
 * and frame storage. Browser code receives catalog indexes, primitive values,
 * and input/output addresses only; it never reads f0r_plugin_descriptor_t.
 */
#ifndef FREI0R_DEMO_CONTRACT_H
#define FREI0R_DEMO_CONTRACT_H

#include <stddef.h>
#include <stdint.h>

#include <frei0r.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FREI0R_DEMO_FRAME_ALIGNMENT 16u
#define FREI0R_DEMO_MIN_DIMENSION 8u
#define FREI0R_DEMO_MAX_PIXELS (640u * 480u)

/* This list is the browser product catalog, in stable presentation order. */
#define FREI0R_DEMO_FILTER_MANIFEST(X) \
  X(brightness) \
  X(bw0r) \
  X(colorize) \
  X(dither) \
  X(distort0r) \
  X(emboss) \
  X(glitch0r) \
  X(glow) \
  X(heatmap0r) \
  X(hueshift0r) \
  X(invert0r) \
  X(pixeliz0r) \
  X(posterize) \
  X(rgbsplit0r) \
  X(saturat0r) \
  X(threshold0r) \
  X(vertigo)

enum frei0r_demo_error {
  FREI0R_DEMO_OK = 0,
  FREI0R_DEMO_ERROR_ARGUMENT = 1,
  FREI0R_DEMO_ERROR_CATALOG_INDEX = 2,
  FREI0R_DEMO_ERROR_DIMENSIONS = 3,
  FREI0R_DEMO_ERROR_ALIGNMENT = 4,
  FREI0R_DEMO_ERROR_COLOR_MODEL = 5,
  FREI0R_DEMO_ERROR_PARAMETER_TYPE = 6,
  FREI0R_DEMO_ERROR_PARAMETER_INDEX = 7,
  FREI0R_DEMO_ERROR_INSTANCE = 8,
  FREI0R_DEMO_ERROR_UPDATE = 9,
  FREI0R_DEMO_ERROR_SHUTDOWN = 10
};

/* These are the only parameter types the initial browser catalog exposes. */
enum frei0r_demo_parameter_type {
  FREI0R_DEMO_PARAMETER_BOOL = F0R_PARAM_BOOL,
  FREI0R_DEMO_PARAMETER_DOUBLE = F0R_PARAM_DOUBLE,
  FREI0R_DEMO_PARAMETER_COLOR = F0R_PARAM_COLOR,
  FREI0R_DEMO_PARAMETER_POSITION = F0R_PARAM_POSITION
};

typedef union frei0r_demo_parameter_value {
  double scalar;
  f0r_param_color_t color;
  f0r_param_position_t position;
} frei0r_demo_parameter_value_t;

/* Shared checks used by the adapter before a plugin callback is entered. */
int frei0r_demo_contract_validate_frame(const void *frame, uint32_t width,
                                        uint32_t height);
int frei0r_demo_contract_color_model_supported(int color_model);
int frei0r_demo_contract_parameter_type_supported(int parameter_type);
size_t frei0r_demo_manifest_count(void);
const char *frei0r_demo_manifest_id(size_t index);

/*
 * Adapter ABI for the generated Emscripten module (implemented in L1.2).
 * All indexes are bounded by the corresponding catalog/parameter count. The
 * adapter owns returned frame storage; addresses stay valid only until a
 * successful select, shutdown, or an Emscripten memory-buffer replacement.
 */
size_t frei0r_demo_catalog_count(void);
const char *frei0r_demo_catalog_id(size_t catalog_index);
const char *frei0r_demo_catalog_name(size_t catalog_index);
const char *frei0r_demo_catalog_author(size_t catalog_index);
const char *frei0r_demo_catalog_explanation(size_t catalog_index);
int frei0r_demo_catalog_color_model(size_t catalog_index);
size_t frei0r_demo_catalog_parameter_count(size_t catalog_index);
int frei0r_demo_select(size_t catalog_index, uint32_t width, uint32_t height);
uintptr_t frei0r_demo_input_pointer(void);
uintptr_t frei0r_demo_output_pointer(void);
size_t frei0r_demo_parameter_count(void);
int frei0r_demo_parameter_type(size_t parameter_index);
const char *frei0r_demo_parameter_name(size_t parameter_index);
const char *frei0r_demo_parameter_explanation(size_t parameter_index);
int frei0r_demo_get_parameter(size_t parameter_index,
                              frei0r_demo_parameter_value_t *value);
int frei0r_demo_set_parameter(size_t parameter_index,
                              const frei0r_demo_parameter_value_t *value);
double frei0r_demo_get_parameter_scalar(size_t parameter_index);
int frei0r_demo_set_parameter_scalar(size_t parameter_index, double value);
double frei0r_demo_get_parameter_color_component(size_t parameter_index,
                                                 size_t component);
int frei0r_demo_set_parameter_color(size_t parameter_index, double red,
                                    double green, double blue);
double frei0r_demo_get_parameter_position_component(size_t parameter_index,
                                                    size_t component);
int frei0r_demo_set_parameter_position(size_t parameter_index, double x,
                                       double y);
int frei0r_demo_reset_parameters(void);
int frei0r_demo_update(double time);
int frei0r_demo_last_error(void);
void frei0r_demo_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif
