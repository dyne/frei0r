#ifndef FREI0R_RUNTIME_SMOKE_H
#define FREI0R_RUNTIME_SMOKE_H

#include "frei0r/bundle.h"

/* Compatibility aliases for the archive smoke harness. */
typedef f0r_plugin_descriptor_t runtime_smoke_descriptor_t;

#define runtime_smoke_descriptor_by_index f0r_bundle_plugin_by_index
#define runtime_smoke_descriptor_by_id f0r_bundle_plugin_by_id
#define runtime_smoke_descriptor_count f0r_bundle_plugin_count

#endif
