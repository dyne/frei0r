/*
 * Public interface for statically linked frei0r plugin bundles.
 *
 * This interface is separate from the frei0r module ABI in frei0r.h.  A
 * bundle exposes immutable descriptors instead of selecting a plugin through
 * process-global state.
 */
#ifndef INCLUDED_FREI0R_BUNDLE_H
#define INCLUDED_FREI0R_BUNDLE_H

#include <stddef.h>
#include <stdint.h>

#include "frei0r.h"

/*
 * Define FREI0R_BUNDLE_BUILD while compiling a shared bundle.  Consumers of
 * static bundles need no visibility definition.
 */
#ifndef FREI0R_BUNDLE_PUBLIC
#if defined(_WIN32) || defined(__CYGWIN__)
#if defined(FREI0R_BUNDLE_BUILD)
#define FREI0R_BUNDLE_PUBLIC __declspec(dllexport)
#else
#define FREI0R_BUNDLE_PUBLIC
#endif
#elif defined(__GNUC__) && __GNUC__ >= 4
#define FREI0R_BUNDLE_PUBLIC __attribute__((visibility("default")))
#else
#define FREI0R_BUNDLE_PUBLIC
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* The initial, append-only descriptor layout. */
#define F0R_PLUGIN_DESCRIPTOR_VERSION UINT32_C(1)

typedef int (*f0r_plugin_init_fn)(void);
typedef void (*f0r_plugin_deinit_fn)(void);
typedef void (*f0r_plugin_get_plugin_info_fn)(f0r_plugin_info_t *info);
typedef void (*f0r_plugin_get_param_info_fn)(f0r_param_info_t *info,
                                              int param_index);
typedef f0r_instance_t (*f0r_plugin_construct_fn)(unsigned int width,
                                                   unsigned int height);
typedef void (*f0r_plugin_destruct_fn)(f0r_instance_t instance);
typedef void (*f0r_plugin_set_param_value_fn)(f0r_instance_t instance,
                                               f0r_param_t param,
                                               int param_index);
typedef void (*f0r_plugin_get_param_value_fn)(f0r_instance_t instance,
                                               f0r_param_t param,
                                               int param_index);
typedef void (*f0r_plugin_update_fn)(f0r_instance_t instance, double time,
                                     const uint32_t *inframe,
                                     uint32_t *outframe);
typedef void (*f0r_plugin_update2_fn)(f0r_instance_t instance, double time,
                                      const uint32_t *inframe1,
                                      const uint32_t *inframe2,
                                      const uint32_t *inframe3,
                                      uint32_t *outframe);

/*
 * A descriptor and its ID remain valid for the lifetime of the bundle.  The
 * fields through update2 form version 1 and are never reordered or assigned a
 * different meaning; later versions append fields.  Callers must check both
 * descriptor_size and descriptor_version before reading an extension.
 *
 * id is a non-null, canonical, NUL-terminated identifier.  update is null
 * only when the plugin kind does not provide f0r_update; update2 is null only
 * when the plugin kind does not provide f0r_update2.  All other entry points
 * are required by frei0r 1.2 and are non-null.
 */
typedef struct f0r_plugin_descriptor {
  uint32_t descriptor_size;
  uint32_t descriptor_version;
  const char *id;
  f0r_plugin_init_fn init;
  f0r_plugin_deinit_fn deinit;
  f0r_plugin_get_plugin_info_fn get_plugin_info;
  f0r_plugin_get_param_info_fn get_param_info;
  f0r_plugin_construct_fn construct;
  f0r_plugin_destruct_fn destruct;
  f0r_plugin_set_param_value_fn set_param_value;
  f0r_plugin_get_param_value_fn get_param_value;
  f0r_plugin_update_fn update;
  f0r_plugin_update2_fn update2;
} f0r_plugin_descriptor_t;

#define F0R_PLUGIN_DESCRIPTOR_SIZE ((uint32_t)sizeof(f0r_plugin_descriptor_t))

/*
 * The registry order is stable for a particular bundle build.  It contains
 * only descriptors with unique canonical IDs.  Bundle construction must
 * reject a duplicate ID; if a malformed bundle nevertheless contains one,
 * f0r_bundle_plugin_by_id returns null rather than choosing arbitrarily.
 *
 * A descriptor and its ID are immutable for the lifetime of the bundle.  A
 * host obtains a descriptor by index or ID, checks its size and version, then
 * calls init before construct.  It must destruct every constructed instance
 * before calling deinit.  The remaining descriptor callbacks have the same
 * per-plugin lifecycle and parameter rules as the frei0r 1.2 ABI in
 * frei0r.h; a bundle does not expose process-global f0r_* entry points.
 */
FREI0R_BUNDLE_PUBLIC size_t f0r_bundle_plugin_count(void);
FREI0R_BUNDLE_PUBLIC const f0r_plugin_descriptor_t *
f0r_bundle_plugin_by_index(size_t index);
FREI0R_BUNDLE_PUBLIC const f0r_plugin_descriptor_t *
f0r_bundle_plugin_by_id(const char *id);

#ifdef __cplusplus
}
#endif

#endif
