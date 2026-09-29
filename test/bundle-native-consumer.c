#include <stddef.h>
#include <string.h>

#include <frei0r/bundle.h>

static int check_descriptor(const f0r_plugin_descriptor_t *descriptor,
                            const char *id, int expects_update,
                            int expects_update2)
{
  if (!descriptor || descriptor->descriptor_size != F0R_PLUGIN_DESCRIPTOR_SIZE ||
      descriptor->descriptor_version != F0R_PLUGIN_DESCRIPTOR_VERSION ||
      !descriptor->id || strcmp(descriptor->id, id) != 0 || !descriptor->init ||
      !descriptor->deinit || !descriptor->get_plugin_info ||
      !descriptor->get_param_info || !descriptor->construct ||
      !descriptor->destruct || !descriptor->set_param_value ||
      !descriptor->get_param_value || (!!descriptor->update != expects_update) ||
      (!!descriptor->update2 != expects_update2))
    return 1;
  return 0;
}

int main(void)
{
  static const char * const ids[] = {
    "brightness", "invert0r", "premultiply", "RGB"
  };
  const f0r_plugin_descriptor_t *descriptor;
  size_t index;

  if (f0r_bundle_plugin_count() != sizeof(ids) / sizeof(ids[0]) ||
      f0r_bundle_plugin_by_index(sizeof(ids) / sizeof(ids[0])) != NULL ||
      f0r_bundle_plugin_by_id("missing") != NULL)
    return 1;
  for (index = 0; index < sizeof(ids) / sizeof(ids[0]); ++index) {
    descriptor = f0r_bundle_plugin_by_index(index);
    if (check_descriptor(descriptor, ids[index], index != 3,
                         index == 2 || index == 3) ||
        f0r_bundle_plugin_by_id(ids[index]) != descriptor)
      return 2;
  }
  return 0;
}
