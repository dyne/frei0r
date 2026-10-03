#include "frei0r-demo-contract.h"

#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <frei0r/bundle.h>

#define FREI0R_DEMO_MANIFEST_COUNT_ENTRY(identifier) +1
enum { frei0r_demo_manifest_capacity = 0 FREI0R_DEMO_FILTER_MANIFEST(
  FREI0R_DEMO_MANIFEST_COUNT_ENTRY) };
#undef FREI0R_DEMO_MANIFEST_COUNT_ENTRY

typedef struct frei0r_demo_runtime {
  const f0r_plugin_descriptor_t *descriptors[frei0r_demo_manifest_capacity];
  size_t initialized_count;
  const f0r_plugin_descriptor_t *active_descriptor;
  f0r_instance_t active_instance;
  void *input_allocation;
  void *output_allocation;
  uint32_t *input;
  uint32_t *output;
  uint32_t width;
  uint32_t height;
  int initialized;
  int shutdown;
  int last_error;
} frei0r_demo_runtime_t;

static frei0r_demo_runtime_t runtime;

static int set_error(int error)
{
  runtime.last_error = error;
  return error;
}

static int validate_dimensions(uint32_t width, uint32_t height)
{
  if (width < FREI0R_DEMO_MIN_DIMENSION ||
      height < FREI0R_DEMO_MIN_DIMENSION || width % 8u || height % 8u ||
      (uint64_t)width * height > FREI0R_DEMO_MAX_PIXELS)
    return FREI0R_DEMO_ERROR_DIMENSIONS;
  return FREI0R_DEMO_OK;
}

static void free_buffers(void)
{
  free(runtime.input_allocation);
  free(runtime.output_allocation);
  runtime.input_allocation = NULL;
  runtime.output_allocation = NULL;
  runtime.input = NULL;
  runtime.output = NULL;
}

static void destroy_active_instance(void)
{
  if (runtime.active_instance)
    runtime.active_descriptor->destruct(runtime.active_instance);
  runtime.active_descriptor = NULL;
  runtime.active_instance = NULL;
  runtime.width = 0;
  runtime.height = 0;
}

static void deinitialize_descriptors(void)
{
  while (runtime.initialized_count) {
    --runtime.initialized_count;
    runtime.descriptors[runtime.initialized_count]->deinit();
  }
}

static int ensure_initialized(void)
{
  size_t index;

  if (runtime.shutdown)
    return set_error(FREI0R_DEMO_ERROR_SHUTDOWN);
  if (runtime.initialized)
    return FREI0R_DEMO_OK;
  if (frei0r_demo_manifest_count() != frei0r_demo_manifest_capacity ||
      f0r_bundle_plugin_count() != frei0r_demo_manifest_count())
    return set_error(FREI0R_DEMO_ERROR_CATALOG_INDEX);

  for (index = 0; index < frei0r_demo_manifest_count(); ++index) {
    const char *id = frei0r_demo_manifest_id(index);
    const f0r_plugin_descriptor_t *descriptor = f0r_bundle_plugin_by_id(id);
    f0r_plugin_info_t info = { 0 };

    if (!descriptor || !descriptor->id || strcmp(descriptor->id, id) != 0 ||
        !descriptor->init || !descriptor->deinit || !descriptor->get_plugin_info ||
        !descriptor->get_param_info || !descriptor->construct ||
        !descriptor->destruct || !descriptor->set_param_value ||
        !descriptor->get_param_value || !descriptor->update || descriptor->update2) {
      deinitialize_descriptors();
      return set_error(FREI0R_DEMO_ERROR_CATALOG_INDEX);
    }
    if (!descriptor->init()) {
      deinitialize_descriptors();
      return set_error(FREI0R_DEMO_ERROR_INSTANCE);
    }
    runtime.descriptors[index] = descriptor;
    ++runtime.initialized_count;
    descriptor->get_plugin_info(&info);
    if (info.plugin_type != F0R_PLUGIN_TYPE_FILTER ||
        !frei0r_demo_contract_color_model_supported(info.color_model) ||
        info.num_params < 0) {
      deinitialize_descriptors();
      return set_error(FREI0R_DEMO_ERROR_COLOR_MODEL);
    }
  }
  runtime.initialized = 1;
  return set_error(FREI0R_DEMO_OK);
}

static const f0r_plugin_descriptor_t *catalog_descriptor(size_t catalog_index)
{
  if (ensure_initialized() != FREI0R_DEMO_OK)
    return NULL;
  if (catalog_index >= frei0r_demo_manifest_count()) {
    set_error(FREI0R_DEMO_ERROR_CATALOG_INDEX);
    return NULL;
  }
  return runtime.descriptors[catalog_index];
}

static int active_parameter(size_t parameter_index, f0r_param_info_t *parameter)
{
  f0r_plugin_info_t info = { 0 };

  if (runtime.shutdown)
    return set_error(FREI0R_DEMO_ERROR_SHUTDOWN);
  if (!runtime.active_instance)
    return set_error(FREI0R_DEMO_ERROR_INSTANCE);
  runtime.active_descriptor->get_plugin_info(&info);
  if (parameter_index > INT_MAX || parameter_index >= (size_t)info.num_params)
    return set_error(FREI0R_DEMO_ERROR_PARAMETER_INDEX);
  memset(parameter, 0, sizeof(*parameter));
  runtime.active_descriptor->get_param_info(parameter, (int)parameter_index);
  if (!frei0r_demo_contract_parameter_type_supported(parameter->type))
    return set_error(FREI0R_DEMO_ERROR_PARAMETER_TYPE);
  return FREI0R_DEMO_OK;
}

static int scalar_value_valid(double value)
{
  return isfinite(value) && value >= 0.0 && value <= 1.0;
}

static uint32_t *allocate_aligned_frame(size_t bytes, void **allocation)
{
  unsigned char *raw = malloc(bytes + FREI0R_DEMO_FRAME_ALIGNMENT - 1u);
  uintptr_t aligned;

  if (!raw)
    return NULL;
  aligned = ((uintptr_t)raw + FREI0R_DEMO_FRAME_ALIGNMENT - 1u) &
            ~(uintptr_t)(FREI0R_DEMO_FRAME_ALIGNMENT - 1u);
  *allocation = raw;
  return (uint32_t *)aligned;
}

size_t frei0r_demo_catalog_count(void)
{
  return frei0r_demo_manifest_count();
}

const char *frei0r_demo_catalog_id(size_t catalog_index)
{
  if (!catalog_descriptor(catalog_index))
    return NULL;
  return frei0r_demo_manifest_id(catalog_index);
}

const char *frei0r_demo_catalog_name(size_t catalog_index)
{
  const f0r_plugin_descriptor_t *descriptor = catalog_descriptor(catalog_index);
  f0r_plugin_info_t info = { 0 };

  if (!descriptor)
    return NULL;
  descriptor->get_plugin_info(&info);
  return info.name;
}

const char *frei0r_demo_catalog_author(size_t catalog_index)
{
  const f0r_plugin_descriptor_t *descriptor = catalog_descriptor(catalog_index);
  f0r_plugin_info_t info = { 0 };

  if (!descriptor)
    return NULL;
  descriptor->get_plugin_info(&info);
  return info.author;
}

const char *frei0r_demo_catalog_explanation(size_t catalog_index)
{
  const f0r_plugin_descriptor_t *descriptor = catalog_descriptor(catalog_index);
  f0r_plugin_info_t info = { 0 };

  if (!descriptor)
    return NULL;
  descriptor->get_plugin_info(&info);
  return info.explanation;
}

int frei0r_demo_catalog_color_model(size_t catalog_index)
{
  const f0r_plugin_descriptor_t *descriptor = catalog_descriptor(catalog_index);
  f0r_plugin_info_t info = { 0 };

  if (!descriptor)
    return -1;
  descriptor->get_plugin_info(&info);
  return info.color_model;
}

size_t frei0r_demo_catalog_parameter_count(size_t catalog_index)
{
  const f0r_plugin_descriptor_t *descriptor = catalog_descriptor(catalog_index);
  f0r_plugin_info_t info = { 0 };

  if (!descriptor)
    return 0;
  descriptor->get_plugin_info(&info);
  return (size_t)info.num_params;
}

int frei0r_demo_select(size_t catalog_index, uint32_t width, uint32_t height)
{
  const f0r_plugin_descriptor_t *descriptor;
  void *new_input_allocation = NULL;
  void *new_output_allocation = NULL;
  uint32_t *new_input;
  uint32_t *new_output;
  f0r_instance_t new_instance;
  size_t bytes;
  int error = validate_dimensions(width, height);

  if (error != FREI0R_DEMO_OK)
    return set_error(error);
  descriptor = catalog_descriptor(catalog_index);
  if (!descriptor)
    return runtime.last_error;
  if (runtime.active_descriptor == descriptor && runtime.width == width &&
      runtime.height == height)
    return set_error(FREI0R_DEMO_OK);

  bytes = (size_t)width * height * sizeof(*new_input);
  new_input = allocate_aligned_frame(bytes, &new_input_allocation);
  new_output = allocate_aligned_frame(bytes, &new_output_allocation);
  if (!new_input || !new_output ||
      frei0r_demo_contract_validate_frame(new_input, width, height) !=
        FREI0R_DEMO_OK ||
      frei0r_demo_contract_validate_frame(new_output, width, height) !=
        FREI0R_DEMO_OK) {
    free(new_input_allocation);
    free(new_output_allocation);
    return set_error(FREI0R_DEMO_ERROR_INSTANCE);
  }
  new_instance = descriptor->construct(width, height);
  if (!new_instance) {
    free(new_input_allocation);
    free(new_output_allocation);
    return set_error(FREI0R_DEMO_ERROR_INSTANCE);
  }

  destroy_active_instance();
  free_buffers();
  runtime.active_descriptor = descriptor;
  runtime.active_instance = new_instance;
  runtime.input_allocation = new_input_allocation;
  runtime.output_allocation = new_output_allocation;
  runtime.input = new_input;
  runtime.output = new_output;
  runtime.width = width;
  runtime.height = height;
  return set_error(FREI0R_DEMO_OK);
}

uintptr_t frei0r_demo_input_pointer(void)
{
  if (!runtime.input) {
    set_error(runtime.shutdown ? FREI0R_DEMO_ERROR_SHUTDOWN : FREI0R_DEMO_ERROR_INSTANCE);
    return 0;
  }
  return (uintptr_t)runtime.input;
}

uintptr_t frei0r_demo_output_pointer(void)
{
  if (!runtime.output) {
    set_error(runtime.shutdown ? FREI0R_DEMO_ERROR_SHUTDOWN : FREI0R_DEMO_ERROR_INSTANCE);
    return 0;
  }
  return (uintptr_t)runtime.output;
}

size_t frei0r_demo_parameter_count(void)
{
  f0r_plugin_info_t info = { 0 };

  if (runtime.shutdown || !runtime.active_instance) {
    set_error(runtime.shutdown ? FREI0R_DEMO_ERROR_SHUTDOWN : FREI0R_DEMO_ERROR_INSTANCE);
    return 0;
  }
  runtime.active_descriptor->get_plugin_info(&info);
  return (size_t)info.num_params;
}

int frei0r_demo_parameter_type(size_t parameter_index)
{
  f0r_param_info_t parameter;

  if (active_parameter(parameter_index, &parameter) != FREI0R_DEMO_OK)
    return -1;
  return parameter.type;
}

const char *frei0r_demo_parameter_name(size_t parameter_index)
{
  f0r_param_info_t parameter;

  if (active_parameter(parameter_index, &parameter) != FREI0R_DEMO_OK)
    return NULL;
  return parameter.name;
}

const char *frei0r_demo_parameter_explanation(size_t parameter_index)
{
  f0r_param_info_t parameter;

  if (active_parameter(parameter_index, &parameter) != FREI0R_DEMO_OK)
    return NULL;
  return parameter.explanation;
}

int frei0r_demo_get_parameter(size_t parameter_index,
                              frei0r_demo_parameter_value_t *value)
{
  f0r_param_info_t parameter;
  int error;

  if (!value)
    return set_error(FREI0R_DEMO_ERROR_ARGUMENT);
  error = active_parameter(parameter_index, &parameter);
  if (error != FREI0R_DEMO_OK)
    return error;
  runtime.active_descriptor->get_param_value(runtime.active_instance, value,
                                              (int)parameter_index);
  return set_error(FREI0R_DEMO_OK);
}

int frei0r_demo_set_parameter(size_t parameter_index,
                              const frei0r_demo_parameter_value_t *value)
{
  f0r_param_info_t parameter;
  int error;

  if (!value)
    return set_error(FREI0R_DEMO_ERROR_ARGUMENT);
  error = active_parameter(parameter_index, &parameter);
  if (error != FREI0R_DEMO_OK)
    return error;
  if ((parameter.type == F0R_PARAM_BOOL || parameter.type == F0R_PARAM_DOUBLE) &&
      !scalar_value_valid(value->scalar))
    return set_error(FREI0R_DEMO_ERROR_ARGUMENT);
  if (parameter.type == F0R_PARAM_COLOR &&
      (!scalar_value_valid(value->color.r) || !scalar_value_valid(value->color.g) ||
       !scalar_value_valid(value->color.b)))
    return set_error(FREI0R_DEMO_ERROR_ARGUMENT);
  if (parameter.type == F0R_PARAM_POSITION &&
      (!scalar_value_valid(value->position.x) || !scalar_value_valid(value->position.y)))
    return set_error(FREI0R_DEMO_ERROR_ARGUMENT);
  runtime.active_descriptor->set_param_value(runtime.active_instance,
                                              (f0r_param_t)value,
                                              (int)parameter_index);
  return set_error(FREI0R_DEMO_OK);
}

double frei0r_demo_get_parameter_scalar(size_t parameter_index)
{
  frei0r_demo_parameter_value_t value;
  int type = frei0r_demo_parameter_type(parameter_index);

  if (type != F0R_PARAM_BOOL && type != F0R_PARAM_DOUBLE) {
    if (type >= 0)
      set_error(FREI0R_DEMO_ERROR_PARAMETER_TYPE);
    return NAN;
  }
  if (frei0r_demo_get_parameter(parameter_index, &value) != FREI0R_DEMO_OK)
    return NAN;
  return value.scalar;
}

int frei0r_demo_set_parameter_scalar(size_t parameter_index, double value)
{
  frei0r_demo_parameter_value_t parameter_value;
  int type = frei0r_demo_parameter_type(parameter_index);

  if (type != F0R_PARAM_BOOL && type != F0R_PARAM_DOUBLE)
    return type < 0 ? runtime.last_error : set_error(FREI0R_DEMO_ERROR_PARAMETER_TYPE);
  parameter_value.scalar = value;
  return frei0r_demo_set_parameter(parameter_index, &parameter_value);
}

double frei0r_demo_get_parameter_color_component(size_t parameter_index,
                                                 size_t component)
{
  frei0r_demo_parameter_value_t value;
  int type;

  if (component > 2) {
    set_error(FREI0R_DEMO_ERROR_ARGUMENT);
    return NAN;
  }
  type = frei0r_demo_parameter_type(parameter_index);
  if (type != F0R_PARAM_COLOR) {
    if (type >= 0)
      set_error(FREI0R_DEMO_ERROR_PARAMETER_TYPE);
    return NAN;
  }
  if (frei0r_demo_get_parameter(parameter_index, &value) != FREI0R_DEMO_OK)
    return NAN;
  return component == 0 ? value.color.r : component == 1 ? value.color.g : value.color.b;
}

int frei0r_demo_set_parameter_color(size_t parameter_index, double red,
                                    double green, double blue)
{
  frei0r_demo_parameter_value_t value;
  int type = frei0r_demo_parameter_type(parameter_index);

  if (type != F0R_PARAM_COLOR)
    return type < 0 ? runtime.last_error :
      set_error(FREI0R_DEMO_ERROR_PARAMETER_TYPE);
  value.color.r = (float)red;
  value.color.g = (float)green;
  value.color.b = (float)blue;
  return frei0r_demo_set_parameter(parameter_index, &value);
}

double frei0r_demo_get_parameter_position_component(size_t parameter_index,
                                                    size_t component)
{
  frei0r_demo_parameter_value_t value;
  int type;

  if (component > 1) {
    set_error(FREI0R_DEMO_ERROR_ARGUMENT);
    return NAN;
  }
  type = frei0r_demo_parameter_type(parameter_index);
  if (type != F0R_PARAM_POSITION) {
    if (type >= 0)
      set_error(FREI0R_DEMO_ERROR_PARAMETER_TYPE);
    return NAN;
  }
  if (frei0r_demo_get_parameter(parameter_index, &value) != FREI0R_DEMO_OK)
    return NAN;
  return component == 0 ? value.position.x : value.position.y;
}

int frei0r_demo_set_parameter_position(size_t parameter_index, double x,
                                       double y)
{
  frei0r_demo_parameter_value_t value;
  int type = frei0r_demo_parameter_type(parameter_index);

  if (type != F0R_PARAM_POSITION)
    return type < 0 ? runtime.last_error :
      set_error(FREI0R_DEMO_ERROR_PARAMETER_TYPE);
  value.position.x = x;
  value.position.y = y;
  return frei0r_demo_set_parameter(parameter_index, &value);
}

int frei0r_demo_reset_parameters(void)
{
  f0r_instance_t instance;

  if (runtime.shutdown)
    return set_error(FREI0R_DEMO_ERROR_SHUTDOWN);
  if (!runtime.active_instance)
    return set_error(FREI0R_DEMO_ERROR_INSTANCE);
  instance = runtime.active_descriptor->construct(runtime.width, runtime.height);
  if (!instance)
    return set_error(FREI0R_DEMO_ERROR_INSTANCE);
  runtime.active_descriptor->destruct(runtime.active_instance);
  runtime.active_instance = instance;
  return set_error(FREI0R_DEMO_OK);
}

int frei0r_demo_update(double time)
{
  int input_status;
  int output_status;

  if (runtime.shutdown)
    return set_error(FREI0R_DEMO_ERROR_SHUTDOWN);
  if (!runtime.active_instance)
    return set_error(FREI0R_DEMO_ERROR_INSTANCE);
  if (!isfinite(time))
    return set_error(FREI0R_DEMO_ERROR_ARGUMENT);
  input_status = frei0r_demo_contract_validate_frame(runtime.input, runtime.width,
                                                      runtime.height);
  output_status = frei0r_demo_contract_validate_frame(runtime.output, runtime.width,
                                                       runtime.height);
  if (input_status != FREI0R_DEMO_OK || output_status != FREI0R_DEMO_OK)
    return set_error(input_status != FREI0R_DEMO_OK ? input_status : output_status);
  runtime.active_descriptor->update(runtime.active_instance, time, runtime.input,
                                    runtime.output);
  return set_error(FREI0R_DEMO_OK);
}

int frei0r_demo_last_error(void)
{
  return runtime.last_error;
}

void frei0r_demo_shutdown(void)
{
  if (runtime.shutdown)
    return;
  destroy_active_instance();
  free_buffers();
  deinitialize_descriptors();
  runtime.initialized = 0;
  runtime.shutdown = 1;
  runtime.last_error = FREI0R_DEMO_OK;
}
