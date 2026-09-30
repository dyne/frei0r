# Build instructions

Frei0r can be built using CMake.

Minimum toolchain expectations:

  + C compiler
  + C++ compiler with C++11 support (required)
  + CMake
  + Ninja or Make

The presence of optional libraries on the system will trigger compilation of extra plugins. These libraries are:

  + [Gavl](http://gmerlin.sourceforge.net) required for scale0tilt and vectorscope filters

  + [OpenCV](http://opencvlibrary.sourceforge.net) required for facebl0r filter

  + [Cairo](http://cairographics.org) required for cairo- filters and mixers

## Optional build flags

  + `-DWITHOUT_FACERECOGNITION=ON` - Disable face recognition plugins (facedetect and facebl0r) to avoid protobuf conflicts with applications like MLT

It is recommended to use a separate `build` sub-folder.

```
cmake -S . -B build
cmake --build build
```

To disable face recognition plugins (recommended when using with MLT):
```
cmake -S . -B build -DWITHOUT_FACERECOGNITION=ON
cmake --build build
```

Ninja and nmake are also supported through CMake:
```
cmake -S . -B build -G 'Ninja'
cmake -S . -B build -G 'NMake Makefiles'
```

With CMake 3.20 or newer, the build variants can also be selected through
presets. Each preset uses a separate directory under `build/`, so switching
compilers or generators does not reuse an incompatible CMake cache.

List the available presets:
```
cmake --list-presets
```

Configure and build a preset:
```
cmake --preset release-gcc-ninja
cmake --build --preset release-gcc-ninja
```

The available variants are `release-gcc`, `release-gcc-ninja`,
`release-clang`, `release-clang-ninja`, `debug-gcc`, and
`debug-clang-ninja`. The debug presets enable AddressSanitizer.

Install a configured preset build with CMake's standard install command:
```
cmake --install build/release-gcc-ninja
```

Runtime test utilities:
```
cd test
make frei0r-asan   # builds ./frei0r-run with ASAN
make check         # loads and runs all built plugins under ../build/src
make frei0r-meta
make scan-meta
```

## Static plugin bundles

`FREI0R_BUILD_BUNDLE=ON` builds a registry-backed bundle in addition to the
ordinary frei0r MODULE targets.  It does not change the normal plugin install
directory or module ABI.  Native builds install `frei0r/bundle.h`, a static
archive named `libfrei0r-bundle-static`, a shared library named
`libfrei0r-bundle`, and a CMake package exposing `Frei0rBundle::static` and
`Frei0rBundle::shared`.

Build and install the default dependency-free `core` profile in fresh
directories.  The source tree also configures `shadert0y`; on Ubuntu install
`pkg-config`, `libgl-dev`, and `libegl1-mesa-dev` before configuring a native
bundle build.

```
cmake -S . -B build/bundle-native -G Ninja \
  -DFREI0R_BUILD_BUNDLE=ON -DBUILD_TESTING=ON \
  -DWITHOUT_OPENCV=ON -DWITHOUT_CAIRO=ON -DWITHOUT_GAVL=ON
cmake --build build/bundle-native --parallel 4
cmake --install build/bundle-native --prefix "$PWD/build/bundle-prefix"
ctest --test-dir build/bundle-native --output-on-failure -R \
  '^(frei0r-bundle-symbol-collisions|bundle-native-consumer|bundle-native-consumer-gc|bundle-registry-contract|frei0r-bundle-shared-exports|bundle-install-consumers)$'
```

The installed CMake targets encode the static archive's resolved link
requirements and link order.  A standalone consumer must use the installed
package rather than source-tree include or library paths.  The repository's
independent C and C++ samples demonstrate both targets:

```
cmake -S test/bundle-install-consumer -B build/bundle-consumer \
  -DCMAKE_PREFIX_PATH="$PWD/build/bundle-prefix"
cmake --build build/bundle-consumer --parallel 4
```

The public interface is `frei0r/bundle.h`, separate from the module entry
points in `frei0r.h`.  Look up a descriptor with
`f0r_bundle_plugin_by_id()` or `f0r_bundle_plugin_by_index()`, verify
`descriptor_size` and `descriptor_version`, call `init`, construct/use/destruct
instances, and finally call `deinit`.  IDs are canonical, NUL-terminated, and
unique within one bundle.  Registry order is stable only for that particular
bundle build; use IDs for durable selection.  Descriptors and IDs remain valid
for the bundle lifetime.

### Profiles and dependency classes

`FREI0R_BUNDLE_PROFILE=core` is the default and selects only plugins needing
the language runtimes and math library.  `FREI0R_BUNDLE_PROFILE=all` selects
every eligible plugin whose dependencies were resolved at configure time.
`FREI0R_BUNDLE_PLUGINS` overrides either profile with a semicolon-separated
target list.  To configure `all` after installing the desired development
packages:

```
cmake -S . -B build/bundle-all -G Ninja \
  -DFREI0R_BUILD_BUNDLE=ON -DFREI0R_BUNDLE_PROFILE=all
```

To configure an explicit list, quote it in a shell:

```
cmake -S . -B build/bundle-explicit -G Ninja \
  -DFREI0R_BUILD_BUNDLE=ON \
  -DFREI0R_BUNDLE_PLUGINS='brightness;invert0r'
```

An explicit target still requires its dependency to be available.  The
non-core classifications are:

| Class | Targets | Requirement |
| --- | --- | --- |
| `optional-opencv` | `facebl0r`, `facedetect` | OpenCV |
| `optional-cairo` | `cairoimagegrid`, `cairogradient`, `mirr0r`, `shake0scillate`, `cairoaffineblend`, `cairoblend` | Cairo |
| `optional-gavl` | `rgbparade`, `scale0tilt`, `vectorscope` | GAVL |
| `optional-opengl-egl` | `shadert0y` | OpenGL and EGL |
| `unsupported-dynamic-loading` | `colgate`, `ndvi` | Dynamic loading; not portable to static hosts |

Install the matching development packages, leave the relevant `WITHOUT_*`
option off, and use `all` or explicitly name the target.  The two
`unsupported-dynamic-loading` targets cannot be requested for a static bundle.

### WASI and Emscripten bundles

Wasm archives are build outputs, not native-installable packages.  They are
specific to the selected SDK, target, and feature variant; never mix a WASI
archive with Emscripten or native objects.  Use the public `frei0r/bundle.h`
from the same source revision and link each archive with its own toolchain.

WASI Preview 1 example (with a compatible WASI SDK and Node):

```
export WASI_SDK=/path/to/wasi-sdk
cmake -S . -B build/bundle-wasi-scalar -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$WASI_SDK/share/cmake/wasi-sdk-p1.cmake" \
  -DFREI0R_BUILD_BUNDLE=ON -DBUILD_TESTING=ON \
  -DFREI0R_BUNDLE_WASM_BASELINE=ON \
  -DWITHOUT_OPENCV=ON -DWITHOUT_CAIRO=ON -DWITHOUT_GAVL=ON
cmake --build build/bundle-wasi-scalar --target \
  bundle-registry-wasi bundle-wasi-consumer
ctest --test-dir build/bundle-wasi-scalar --output-on-failure -R \
  '^(bundle-registry-wasi|bundle-wasi-consumer)$'
```

For Emscripten, use a writable cache outside the SDK installation:

```
export EMSDK=/path/to/emsdk
source "$EMSDK/emsdk_env.sh"
export EM_CACHE="$PWD/build/bundle-emscripten-scalar/em-cache"
cmake -S . -B build/bundle-emscripten-scalar -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$EMSDK/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake" \
  -DFREI0R_BUILD_BUNDLE=ON -DBUILD_TESTING=ON \
  -DFREI0R_BUNDLE_WASM_BASELINE=ON \
  -DWITHOUT_OPENCV=ON -DWITHOUT_CAIRO=ON -DWITHOUT_GAVL=ON
cmake --build build/bundle-emscripten-scalar --target \
  bundle-registry-emscripten bundle-emscripten-consumer
ctest --test-dir build/bundle-emscripten-scalar --output-on-failure -R \
  '^(bundle-registry-emscripten|bundle-emscripten-consumer)$'
```

The baseline is scalar and single-threaded.  Build SIMD or pthread variants in
separate directories by replacing `FREI0R_BUNDLE_WASM_BASELINE=ON` with exactly
one of `FREI0R_BUNDLE_WASM_SIMD=ON` or
`FREI0R_BUNDLE_WASM_PTHREADS=ON`; these options are mutually exclusive and
valid only for WASI or Emscripten.  Pthread support remains experimental and
requires a toolchain/runtime configured for threads.  SIMD output is likewise
experimental.  Node contracts exercise supported profiles, but browser
deployment, cross-toolchain archive interchangeability, and optional external
dependencies on Wasm are not established compatibility promises.
