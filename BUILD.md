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

Top-level shorthand targets are available through `GNUmakefile`:
```
make release-gcc-ninja
make debug-gcc
```

Runtime test utilities:
```
cd test
make frei0r-asan   # builds ./frei0r-run with ASAN
make check         # loads and runs all built plugins under ../build/src
make frei0r-meta
make scan-meta
```
