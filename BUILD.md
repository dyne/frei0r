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

## Experimental archive-backed WebAssembly smoke test

The commands below exercise a small, test-only archive registry containing the
dependency-free `brightness` and `invert0r` filters.  It is not yet a public
bundle API or a replacement for normal frei0r plugin modules.  The smoke
contract queries metadata and parameters, creates 8x8 instances, processes
deterministic frames, and reports a nonzero plugin/stage status on failure.

All commands use a new build directory.  They intentionally disable optional
plugin dependencies because this experiment links only the two archive members.

Native GCC uses CMake's selected host archiver:

```
cmake -S . -B build/runtime-smoke-native \
  -DWITHOUT_OPENCV=ON -DWITHOUT_CAIRO=ON -DWITHOUT_GAVL=ON \
  -DCMAKE_BUILD_TYPE=Release -DFREI0R_RUNTIME_SMOKE_TEST=ON
cmake --build build/runtime-smoke-native --target runtime-smoke
ctest --test-dir build/runtime-smoke-native --output-on-failure -R '^runtime-smoke$'
```

For WASI Preview 1, point `WASI_SDK` at a compatible SDK installation.  CMake
selects its LLVM archiver (`llvm-ar`) for `libruntime-smoke-plugins.a`.  The
resulting module has no command entry point; its only smoke function export is
`runtime_smoke_run`.  The Node runner compiles the module, supplies only its
declared WASI function imports, and invokes that export.

```
export WASI_SDK=/path/to/wasi-sdk
cmake -S . -B build/runtime-smoke-wasi \
  -DCMAKE_TOOLCHAIN_FILE="$WASI_SDK/share/cmake/wasi-sdk-p1.cmake" \
  -DWITHOUT_OPENCV=ON -DWITHOUT_CAIRO=ON -DWITHOUT_GAVL=ON \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build/runtime-smoke-wasi --target runtime-smoke-wasi
node test/run-runtime-smoke-wasi.mjs \
  build/runtime-smoke-wasi/test/runtime-smoke-wasi.wasm

# Repeat with explicit linker section garbage collection.
cmake -S . -B build/runtime-smoke-wasi-gc \
  -DCMAKE_TOOLCHAIN_FILE="$WASI_SDK/share/cmake/wasi-sdk-p1.cmake" \
  -DWITHOUT_OPENCV=ON -DWITHOUT_CAIRO=ON -DWITHOUT_GAVL=ON \
  -DCMAKE_BUILD_TYPE=Release -DFREI0R_RUNTIME_SMOKE_GC=ON
cmake --build build/runtime-smoke-wasi-gc --target runtime-smoke-wasi
node test/run-runtime-smoke-wasi.mjs \
  build/runtime-smoke-wasi-gc/test/runtime-smoke-wasi.wasm
```

For Emscripten, source the selected SDK environment and put `EM_CACHE` in the
build directory so the SDK installation remains read-only.  CMake selects
`emar` for the same archive.  The generated Node-oriented modular factory
exports `_runtime_smoke_run` through its module API; no plugin entry points are
public Wasm exports.

```
export EMSDK=/path/to/emsdk
source "$EMSDK/emsdk_env.sh"
mkdir -p build/runtime-smoke-emscripten/em-cache
export EM_CACHE="$PWD/build/runtime-smoke-emscripten/em-cache"
emcmake cmake -S . -B build/runtime-smoke-emscripten \
  -DWITHOUT_OPENCV=ON -DWITHOUT_CAIRO=ON -DWITHOUT_GAVL=ON \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build/runtime-smoke-emscripten --target runtime-smoke-emscripten
node test/run-runtime-smoke-emscripten.mjs \
  build/runtime-smoke-emscripten/test/runtime-smoke-emscripten.js
```

The current WASI module imports only `fd_close`, `fd_seek`, and `fd_write`
from `wasi_snapshot_preview1`, and exports memory plus `runtime_smoke_run`.
The Emscripten artifact exports its runtime memory/table support and the
mapped smoke runner, but not any `brightness_f0r_*` or `invert0r_f0r_*`
function.  With WASI SDK 20.1.8 and Emscripten 4.0.1, Release artifacts were
85,829 bytes for the WASI module, and 11,249 bytes Wasm plus 9,116 bytes JS for
the Emscripten module.  Sizes are observations, not compatibility thresholds.

Both toolchains proceed to the next experiment: each built the CMake-selected
archive and completed the same runtime contract, including the WASI
garbage-collection link.  This evidence covers only scalar, single-threaded,
dependency-free C filters and Node execution.  It does not yet establish a
browser deployment profile, public static-bundle API, external-dependency
plugin support, or the full plugin set.
