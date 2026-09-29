#include <string.h>

#include "runtime-smoke.h"

#define DECLARE_PLUGIN(prefix) \
  int prefix##_f0r_init(void); \
  void prefix##_f0r_deinit(void); \
  void prefix##_f0r_get_plugin_info(f0r_plugin_info_t *info); \
  void prefix##_f0r_get_param_info(f0r_param_info_t *info, int param_index); \
  f0r_instance_t prefix##_f0r_construct(unsigned int width, unsigned int height); \
  void prefix##_f0r_destruct(f0r_instance_t instance); \
  void prefix##_f0r_set_param_value(f0r_instance_t instance, f0r_param_t param, int param_index); \
  void prefix##_f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index); \
  void prefix##_f0r_update(f0r_instance_t instance, double time, const uint32_t *inframe, uint32_t *outframe)

DECLARE_PLUGIN(brightness);
DECLARE_PLUGIN(invert0r);

#define DESCRIPTOR(id, prefix) \
  { id, prefix##_f0r_init, prefix##_f0r_deinit, \
    prefix##_f0r_get_plugin_info, prefix##_f0r_get_param_info, \
    prefix##_f0r_construct, prefix##_f0r_destruct, \
    prefix##_f0r_set_param_value, prefix##_f0r_get_param_value, \
    prefix##_f0r_update }

static const runtime_smoke_descriptor_t runtime_smoke_descriptors[] = {
  DESCRIPTOR("brightness", brightness),
  DESCRIPTOR("invert0r", invert0r)
};

unsigned int runtime_smoke_descriptor_count(void)
{
  return sizeof(runtime_smoke_descriptors) / sizeof(runtime_smoke_descriptors[0]);
}

const runtime_smoke_descriptor_t *runtime_smoke_descriptor_by_index(
  unsigned int index)
{
  if (index >= runtime_smoke_descriptor_count())
    return NULL;
  return &runtime_smoke_descriptors[index];
}

const runtime_smoke_descriptor_t *runtime_smoke_descriptor_by_id(const char *id)
{
  unsigned int index;

  if (!id)
    return NULL;
  for (index = 0; index < runtime_smoke_descriptor_count(); ++index)
    if (strcmp(id, runtime_smoke_descriptors[index].id) == 0)
      return &runtime_smoke_descriptors[index];
  return NULL;
}
