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
