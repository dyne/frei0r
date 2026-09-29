#include <string.h>

#include "runtime-smoke.h"

extern const f0r_plugin_descriptor_t f0r_bundle_descriptor_brightness;
extern const f0r_plugin_descriptor_t f0r_bundle_descriptor_invert0r;

static const runtime_smoke_descriptor_t * const runtime_smoke_descriptors[] = {
  &f0r_bundle_descriptor_brightness,
  &f0r_bundle_descriptor_invert0r
};

size_t f0r_bundle_plugin_count(void)
{
  return sizeof(runtime_smoke_descriptors) / sizeof(runtime_smoke_descriptors[0]);
}

const f0r_plugin_descriptor_t *f0r_bundle_plugin_by_index(size_t index)
{
  if (index >= runtime_smoke_descriptor_count())
    return NULL;
  return runtime_smoke_descriptors[index];
}

const f0r_plugin_descriptor_t *f0r_bundle_plugin_by_id(const char *id)
{
  size_t index;
  const f0r_plugin_descriptor_t *match = NULL;

  if (!id)
    return NULL;
  for (index = 0; index < runtime_smoke_descriptor_count(); ++index)
    if (strcmp(id, runtime_smoke_descriptors[index]->id) == 0) {
      if (match)
        return NULL;
      match = runtime_smoke_descriptors[index];
    }
  return match;
}
