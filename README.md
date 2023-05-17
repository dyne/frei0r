[![Frei0r logo](https://github.com/dyne/frei0r/raw/gh_pages/pics/frei0r.png)](https://frei0r.dyne.org)

<img src="https://files.dyne.org/software_by_dyne.png" width="300">

[![frei0r](https://github.com/dyne/frei0r/actions/workflows/test.yml/badge.svg)](https://github.com/dyne/frei0r/actions/workflows/test.yml)
[![frei0r](https://github.com/dyne/frei0r/actions/workflows/release.yml/badge.svg)](https://github.com/dyne/frei0r/actions/workflows/release.yml)


# What frei0r is 

The frei0r project is a collection of free and open source video effects plugins that can be used with a variety of video editing and processing software.

[For an extensive introduction to frei0r please read this story.](https://jaromil.medium.com/frei0r-the-free-and-open-source-video-effect-preservation-project-604134dde8b3?source=friends_link&sk=c83a054b979d421279f5fc3d2ea1acd8)

The frei0r project welcomes contributions by people who are passionate about video effects, its collection consists of more than 100 plugins made to work on any target platform (GNU/Linux, Apple/OSX and MS/Win) without the need for special video hardware. These plugins can be used to add a wide range of effects to video, such as color correction, blurring, and distortion.

The frei0r project is a great resource for anyone interested in algorithms for video transformation and effects, as it provides a wide range of open source formulas available for free and can be easily integrated into a variety of software. 


## What frei0r is not 

Frei0r itself is just a C/C++ header and a collection of small programs using it to accept an input frame, change it in any possible way and returning an output frame.

It is not meant as a generic API for all kinds of video applications, as it doesn't provides things like an extensive parameter mechanism or event handling.

Eventually the frei0r API can be wrapped by higher level APIs expanding its functionalities, for instance GStreamer, MLT, FFmpeg and Pure Data do.

## Links

Wikipedia page about frei0r: http://en.wikipedia.org/wiki/Frei0r

Some applications using frei0r, sorted in order of most recent activity

- [MLT](http://www.mltframework.org/)
- [KDEnLive](http://www.kdenlive.org/)
- [Shotcut](https://www.shotcut.org/)
- [FFMpeg](https://g=ffmpeg.org)
- [PureData](http://www.artefacte.org/pd/)
- [Open  Movie  Editor](http://openmovieeditor.sourceforge.net/)
- [DVEdit](http://www.freenet.org.nz/dvedit)
- [Gephex](http://www.gephex.org/)
- [LiVES](http://lives.sf.net)
- [FreeJ](http://freej.dyne.org)
- [MøB](http://mob.bek.no/)
- [VeeJay](http://veejayhq.net)
- [Flowblade](http://code.google.com/p/flowblade/)


# Downloads

Stable frei0r releases are built automatically and made available on

## https://github.com/dyne/frei0r/releases

Frei0r sourcecode is released under the terms of the GNU General Public License and, eventually other compatible Free Software licenses.

## Build dependencies 

Frei0r can be built on GNU/Linux, M$/Windows and Apple/OSX platforms, possibly in even more environments like embedded devices.

For details see the [BUILD](/BUILD.md) file.

### MS / Windows

We distribute official builds of frei0r plugins as .dll for the Win64 platform from the releases page.

### BSD

Ports of frei0r are included in all major BSD distros:
- FreeBSD https://www.freshports.org/graphics/frei0r
- OpenBSD https://openports.se/multimedia/frei0r-plugins
- NetBSD https://pkgsrc.se/multimedia/frei0r

### GNU / Linux

Binary packages are mantained on various distributions, but they may not be completely up to date with latest release.

- [frei0r*](https://repology.org/project/frei0r/versions)
- [frei0r-plugins*](https://repology.org/project/frei0r-plugins/versions)
- [ocaml:frei0r*](https://repology.org/project/ocaml:frei0r/versions)

### Apple / OSX 

A [frei0r Brew formula](https://formulae.brew.sh/formula/frei0r) is available.

Official builds of frei0r plugins as .dlsym for the Apple/OSX platform will be soon included in the releases page.

# Documentation 


If you are new to frei0r (but not to programming) the best thing is probably to have a look at the [frei0r header](/include/frei0r.h), which is quite simple and well documented. The [doxyfied documentation](http://frei0r.dyne.org/codedoc/html) is also available for browsing on-line.


## C++ Filter example 

You could find a tutorial filter [here](https://github.com/dyne/frei0r/tree/master/src/filter/tutorial) in the source code.
A simple skeleton for a frei0r video filter looks like this:

```c++
  #include <frei0r.hpp>
  
  typedef struct {
    int16_t w, h;
    uint8_t bpp;
    uint32_t size;
  } ScreenGeometry;
  
  class MyExample: public frei0r::filter {
  public:
    MyExample(int wdt, int hgt);
    ~MyExample();
    virtual void update();
  private:
    ScreenGeometry geo;
    void _init(int wdt, int hgt);
  }
  
  MyExample::MyExample() { /* constructor */ }
  MyExample::~MyExample() { /* destructor */ }
  
  void MyExample::_init(int wdt, int hgt) {
    geo.w = wdt;
    geo.h = hgt;
    geo.bpp = 32; // this filter works only in RGBA 32bit
    geo.size = geo.w*geo.h*(geo.bpp/8); // calculate the size in bytes
  }
  
  void MyExample::update() {
    // we get video input via buffer pointer (void*)in 
    uint32_t *src = (uint32_t*)in;
    // and we give video output via buffer pointer (void*)out
    uint32_t *dst = (uint32_t*)out;
    // this example here does just a copy of input to output
    memcpy(dst, src, geo.size);
  }
    
  frei0r::construct<MyExample>
          plugin("MyExample", "short and simple description for my example",
                 "Who did it", 1, 0);
```

## Join us 

To contribute your plugin please open a [pull request](https://github.com/dyne/frei0r/pulls).

For bug reporting please use our [issue tracker](https://github.com/dyne/frei0r/issues).

You can get in touch with some developers over various dyne.org chat channels, for instance

### https://t.me/frei0r

We als have an (old) mailinglist open to [subscription](https://mailinglists.dyne.org/cgi-bin/mailman/listinfo/frei0r) and we provide [public archives](https://lists.dyne.org/lurker/list/frei0r.en.html) of discussions, searchable and indexed online.

## Acknowledgments 

Frei0r is the result of a collective effort in coordination with several software developers meeting to find a common standard for video effect plugins to be used among their applications.

For a full list of contributors and the project history, see the file [AUTHORS](/AUTHORS), the [ChangeLog](/ChangeLog) and the project web page: https://frei0r.dyne.org


