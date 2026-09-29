#include <string.h>

#include "frei0r/bundle.h"

extern const f0r_plugin_descriptor_t f0r_bundle_descriptor_brightness;
extern const f0r_plugin_descriptor_t f0r_bundle_descriptor_addition;

static const f0r_plugin_descriptor_t * const bundle_prototype_descriptors[] = {
  &f0r_bundle_descriptor_brightness,
  &f0r_bundle_descriptor_addition
};

size_t f0r_bundle_plugin_count(void)
{
  return sizeof(bundle_prototype_descriptors) /
         sizeof(bundle_prototype_descriptors[0]);
}

const f0r_plugin_descriptor_t *f0r_bundle_plugin_by_index(size_t index)
{
  if (index >= f0r_bundle_plugin_count())
    return NULL;
  return bundle_prototype_descriptors[index];
}

const f0r_plugin_descriptor_t *f0r_bundle_plugin_by_id(const char *id)
{
  size_t index;

  if (!id)
    return NULL;
  for (index = 0; index < f0r_bundle_plugin_count(); ++index)
    if (strcmp(id, bundle_prototype_descriptors[index]->id) == 0)
      return bundle_prototype_descriptors[index];
  return NULL;
}
