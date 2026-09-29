#include <stddef.h>
#include <string.h>

#include <frei0r/bundle.h>

int f0r_bundle_emscripten_consumer_run(void)
{
  const f0r_plugin_descriptor_t *descriptor;
  size_t index;

  if (f0r_bundle_plugin_count() == 0 ||
      f0r_bundle_plugin_by_index(f0r_bundle_plugin_count()) != NULL ||
      f0r_bundle_plugin_by_id("missing") != NULL)
    return 1;
  for (index = 0; index < f0r_bundle_plugin_count(); ++index) {
    descriptor = f0r_bundle_plugin_by_index(index);
    if (!descriptor ||
        descriptor->descriptor_size != F0R_PLUGIN_DESCRIPTOR_SIZE ||
        descriptor->descriptor_version != F0R_PLUGIN_DESCRIPTOR_VERSION ||
        !descriptor->id || !descriptor->init || !descriptor->deinit ||
        !descriptor->get_plugin_info || !descriptor->get_param_info ||
        !descriptor->construct || !descriptor->destruct ||
        !descriptor->set_param_value || !descriptor->get_param_value ||
        f0r_bundle_plugin_by_id(descriptor->id) != descriptor)
      return 2;
  }
  return 0;
}
