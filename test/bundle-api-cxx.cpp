#include <type_traits>

#include "frei0r/bundle.h"

static_assert(std::is_standard_layout<f0r_plugin_descriptor_t>::value,
              "bundle descriptors must have a C-compatible layout");
static_assert(std::is_same<decltype(f0r_plugin_descriptor_t::update2),
                           f0r_plugin_update2_fn>::value,
              "update2 must retain its typed optional slot");

int main()
{
  return F0R_PLUGIN_DESCRIPTOR_VERSION == 1 ? 0 : 1;
}
