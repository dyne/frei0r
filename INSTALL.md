# Installation Instructions

Frei0r can be built using either Autoconf or CMake.

The choice is open, CMake is mandatory only on Windowz.

The presence of optional libraries on the system will trigger compilation
of extra plugins. These libraries are:

  + [Gavl](http://gmerlin.sourceforge.net) required for scale0tilt and vectorscope filters

  + [OpenCV](http://opencvlibrary.sourceforge.net) required for facebl0r filter

  + [Cairo](http://cairographics.org) required for cairo- filters and mixers

## Autoconf build

```
./configure
make
```

## CMake build

```
cmake .
make
```

## Proceed with install

Default prefix is `/usr/local`, target directory is `frei0r-1`

A default `make install` as root will put the plugins into `/usr/local/lib/frei0r-1` unless the prefix path is specified. Most applications will look into that directory on GNU/Linux, or it should be possible to configure where to look for frei0r plugins.

When using Apple/OSX, the `dlopen()` mechanism (in FFMpeg for instance) will look for `.dylib` extensions and not the `.so` that frei0r plugins have by default. To fix this problem one can rename the plugins simply so:

```
for file in /usr/local/lib/frei0r-1/*.so ; do
  cp $file "${file%.*}.dylib"
done
```


