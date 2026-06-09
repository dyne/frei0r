# API overview

frei0r API **1.2** is specified by
[`include/frei0r.h`](https://github.com/dyne/frei0r/blob/master/include/frei0r.h).
That header is authoritative. This page is a map for readers; the
[generated API reference](https://dyne.org/frei0r/codedoc/html/) renders the complete declarations and
comments.

## Design

The interface is a C ABI for simple video sources, filters and mixers controlled
by a small set of parameter types. Hosts remain responsible for media decoding,
timelines, events, user interfaces and encoding.

Each plugin is a shared library. A host loads the library, checks metadata,
constructs an instance for a frame size, sets parameters, calls an update
function for each frame, and destroys the instance.

## Plugin lifecycle

| Function | Responsibility |
| --- | --- |
| `f0r_init` | Initialize process-wide plugin state |
| `f0r_get_plugin_info` | Return identity, type, color model, version and parameter count |
| `f0r_get_param_info` | Describe one parameter |
| `f0r_construct` | Create an instance for a width and height |
| `f0r_set_param_value` | Set one instance parameter |
| `f0r_get_param_value` | Read one instance parameter |
| `f0r_update` | Process a one-input API 1.0-compatible filter |
| `f0r_update2` | Process a source, filter, two-input mixer or three-input mixer |
| `f0r_destruct` | Destroy an instance |
| `f0r_deinit` | Release process-wide plugin state |

[Browse the generated function list](https://dyne.org/frei0r/codedoc/html/globals_func.html).

## Plugin types

- **Source:** no input frames; generates an output.
- **Filter:** one input frame.
- **Mixer2:** two input frames.
- **Mixer3:** three input frames.

[Generated plugin-type reference](https://dyne.org/frei0r/codedoc/html/group___p_l_u_g_i_n___t_y_p_e.html)

## Parameter types

The ABI supports Boolean, double, color, position and string parameters.
Parameters must be initialized during construction. Hosts use the metadata name,
type and explanation to build controls.

[Generated parameter-type reference](https://dyne.org/frei0r/codedoc/html/group___p_a_r_a_m___t_y_p_e.html)

## Color models

The plugin metadata declares how four bytes in each pixel are interpreted.
RGBA8888 is recommended for API 1.2 effects. BGRA8888 and packed 32-bit models
remain available for compatibility.

[Generated color-model reference](https://dyne.org/frei0r/codedoc/html/group___c_o_l_o_r___m_o_d_e_l.html)

## Plugin locations

Unix systems define standard system, local and user library locations under
`frei0r-1`. An optional vendor directory avoids name clashes.
`FREI0R_PATH` supplies a platform-separated search list when a host supports
custom locations.

[Generated plugin-location reference](https://dyne.org/frei0r/codedoc/html/group__pluglocations.html)

## Icons

An effect can install a PNG icon with the plugin name. Icon directories mirror
the library's vendor layout under the corresponding frei0r data directory.

[Generated icon reference](https://dyne.org/frei0r/codedoc/html/group__icons.html)

## Concurrency

Hosts may use separate instances concurrently. Plugin authors must follow the
header's rules for process-wide and instance state rather than assuming all
updates happen on one thread.

[Generated concurrency reference](https://dyne.org/frei0r/codedoc/html/group__concurrency.html)

## C++ wrapper

[`include/frei0r.hpp`](https://github.com/dyne/frei0r/blob/master/include/frei0r.hpp)
provides base classes and a `construct<T>` helper. It reduces boilerplate but
does not replace the C ABI.

Continue with the [plugin development guide](/develop-plugin).
