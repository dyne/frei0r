#ifndef FREI0R_RUNTIME_SMOKE_H
#define FREI0R_RUNTIME_SMOKE_H

#include "frei0r.h"

typedef struct runtime_smoke_descriptor {
  const char *id;
  int (*init)(void);
  void (*deinit)(void);
  void (*get_plugin_info)(f0r_plugin_info_t *info);
  void (*get_param_info)(f0r_param_info_t *info, int param_index);
  f0r_instance_t (*construct)(unsigned int width, unsigned int height);
  void (*destruct)(f0r_instance_t instance);
  void (*set_param_value)(f0r_instance_t instance, f0r_param_t param,
                          int param_index);
  void (*get_param_value)(f0r_instance_t instance, f0r_param_t param,
                          int param_index);
  void (*update)(f0r_instance_t instance, double time,
                 const uint32_t *inframe, uint32_t *outframe);
} runtime_smoke_descriptor_t;

const runtime_smoke_descriptor_t *runtime_smoke_descriptor_by_index(
  unsigned int index);
const runtime_smoke_descriptor_t *runtime_smoke_descriptor_by_id(
  const char *id);
unsigned int runtime_smoke_descriptor_count(void);

#endif
