# Develop a frei0r plugin

A frei0r plugin is a shared library implementing a small C ABI. The repository
also provides a C++ wrapper that handles exported functions and parameter
plumbing for common plugins.

Start with the maintained
[`src/filter/tutorial`](https://github.com/dyne/frei0r/tree/master/src/filter/tutorial)
example and keep
[`include/frei0r.h`](https://github.com/dyne/frei0r/blob/master/include/frei0r.h)
open as the specification.

## 1. Choose a plugin type

| Type | Frames in | Purpose |
| --- | ---: | --- |
| `F0R_PLUGIN_TYPE_SOURCE` | 0 | Generate a frame |
| `F0R_PLUGIN_TYPE_FILTER` | 1 | Transform a frame |
| `F0R_PLUGIN_TYPE_MIXER2` | 2 | Combine two frames |
| `F0R_PLUGIN_TYPE_MIXER3` | 3 | Combine three frames |

Choose the narrowest type that represents the effect. Hosts use this metadata
to decide how many inputs to connect.

## 2. Choose a color model

New plugins should normally use `F0R_COLOR_MODEL_RGBA8888`. The API also
defines BGRA and packed 32-bit models for compatibility. A frame is a
width-by-height array of 32-bit pixels in the declared model.

## 3. Implement the effect

The C++ wrapper provides base classes named `source`, `filter`, `mixer2` and
`mixer3`. A minimal filter stores its parameters and implements `update`:

```cpp
#include "frei0r.hpp"

class MyFilter : public frei0r::filter {
 public:
  MyFilter(unsigned int width, unsigned int height) {
    register_param(amount, "Amount", "Strength of the effect");
    amount = 0.5;
  }

  void update(double time, uint32_t* out, const uint32_t* in) override {
    // Transform width * height pixels from in to out.
  }

 private:
  double amount;
};

frei0r::construct<MyFilter> plugin(
    "My filter", "A short description", "Your name", 0, 1,
    F0R_COLOR_MODEL_RGBA8888);
```

The wrapper's constructor receives `(width, height)`, and its base class stores
both dimensions. Initialize every parameter in the constructor.

## 4. Register parameters

frei0r supports Boolean values, normalized doubles, colors, positions and
strings. Hosts generally present doubles in a normalized `0.0` to `1.0` range.
Convert that range to the useful domain inside the plugin and document any
special mapping in the parameter description.

## 5. Process frames safely

- Treat input frame pointers as read-only.
- Write every output pixel unless the effect first copies the input.
- Handle in-place operation only if the algorithm explicitly supports it.
- Keep allocation and expensive lookup-table setup in construction.
- Use `time` for deterministic animation rather than a private wall clock.
- Validate dimensions and arithmetic before calculating buffer offsets.

The tutorial demonstrates pixel iteration, parameter registration, lookup
tables and clamping. It is a collection of ideas, not an installed effect.

## 6. Add the CMake target

Create a directory under the matching `src` category with a `CMakeLists.txt`:

```cmake
set(SOURCES myfilter.cpp)
set(TARGET myfilter)

if(MSVC)
  set(SOURCES ${SOURCES} ${FREI0R_DEF})
endif()

add_library(${TARGET} MODULE ${SOURCES})
set_target_properties(${TARGET} PROPERTIES PREFIX "")
install(TARGETS ${TARGET} LIBRARY DESTINATION ${LIBDIR})
```

Add the directory with `add_subdirectory(myfilter)` in the parent category's
`CMakeLists.txt`.

## 7. Build and inspect it

```sh
cmake -S . -B build -G Ninja
cmake --build build --target myfilter

cd test
make frei0r-meta
./frei0r-meta ../build/src/filter/myfilter/myfilter.so
```

Adjust the library suffix and path for the platform. Check the displayed name,
author, explanation, type, color model, version and every parameter.

## 8. Run the plugin tests

```sh
cd test
make frei0r-asan
make check
```

New plugins should also include focused tests for their invariants where
practical. At minimum, exercise unusual frame dimensions, parameter boundaries
and repeated update calls.

## 9. Contribute

1. Keep the plugin focused on one reusable effect.
2. Use a clear, unique plugin name and human-readable metadata.
3. Confirm builds with and without optional dependencies as applicable.
4. Open a [pull request](https://github.com/dyne/frei0r/pulls).

Use [GitHub issues](https://github.com/dyne/frei0r/issues) for defects and
[Telegram](https://t.me/frei0r) for design discussion.
