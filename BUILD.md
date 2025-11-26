# Build instructions

Frei0r can be built using CMake.

The presence of optional libraries on the system will trigger compilation of extra plugins. These libraries are:

  + [Gavl](http://gmerlin.sourceforge.net) required for scale0tilt and vectorscope filters

  + [OpenCV](http://opencvlibrary.sourceforge.net) required for facebl0r filter

  + [Cairo](http://cairographics.org) required for cairo- filters and mixers

## Optional build flags

  + `-DWITHOUT_FACERECOGNITION=ON` - Disable face recognition plugins (facedetect and facebl0r) to avoid protobuf conflicts with applications like MLT

It is recommended to use a separate `build` sub-folder.

```
mkdir -p build
cd build && cmake ../
make
```

To disable face recognition plugins (recommended when using with MLT):
```
mkdir -p build
cd build && cmake -DWITHOUT_FACERECOGNITION=ON ../
make
```

Also ninja and nmake are supported through cmake:
```
cmake -G 'Ninja' ../
cmake -G 'NMake Makefiles' ../
```

