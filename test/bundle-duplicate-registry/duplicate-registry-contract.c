#include "frei0r/bundle.h"

const f0r_plugin_descriptor_t duplicate_one = {
  F0R_PLUGIN_DESCRIPTOR_SIZE, F0R_PLUGIN_DESCRIPTOR_VERSION, "duplicate",
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

const f0r_plugin_descriptor_t duplicate_two = {
  F0R_PLUGIN_DESCRIPTOR_SIZE, F0R_PLUGIN_DESCRIPTOR_VERSION, "duplicate",
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

int main(void)
{
  if (f0r_bundle_plugin_count() != 2 ||
      !f0r_bundle_plugin_by_index(0) || !f0r_bundle_plugin_by_index(1) ||
      f0r_bundle_plugin_by_id("duplicate") != NULL)
    return 1;
  return 0;
}
