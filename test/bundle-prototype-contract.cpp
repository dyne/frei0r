#include <cstring>

#include "frei0r/bundle.h"

int main()
{
  const f0r_plugin_descriptor_t *brightness = f0r_bundle_plugin_by_id("brightness");
  const f0r_plugin_descriptor_t *addition = f0r_bundle_plugin_by_id("addition");
  f0r_plugin_info_t info = {};

  if (f0r_bundle_plugin_count() != 2 || !brightness || !addition ||
      brightness->update2 != 0 || !brightness->update ||
      !addition->update || !addition->update2)
    return 1;

  addition->get_plugin_info(&info);
  if (!info.name || std::strcmp(info.name, "addition") != 0 ||
      info.plugin_type != F0R_PLUGIN_TYPE_MIXER2)
    return 2;
  return 0;
}
