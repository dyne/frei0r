```
   ___________              ._________
   \_   _____/______   ____ |__\   _  \_______
    |    __) \_  __ \_/ __ \|  /  /_\  \_  __ \
    |     \   |  | \/\  ___/|  \  \_/   \  | \/
    \___  /   |__|    \___  >__|\_____  /__|
        \/                \/          \/

```

*Minimalistic plugin API for video effects, by the Piksel Developers Union*

Updated info on https://frei0r.dyne.org

[![software by Dyne.org](https://www.dyne.org/wp-content/uploads/2015/12/software_by_dyne.png)](http://www.dyne.org)

[![Build Status](https://travis-ci.org/dyne/frei0r.svg?branch=master)](https://travis-ci.org/dyne/frei0r)

# What frei0r is 

Frei0r is a minimalistic plugin API for video effects.

The main emphasis is on simplicity for an API that will round up the
most common video effects into simple filters, sources and mixers that
can be controlled by parameters.

It's our hope that this way these simple effects can be shared between
many applications, avoiding their reimplementation by different
projects.

## What frei0r is not 

Frei0r is not meant as a competing standard to more ambitious efforts
that try to satisfy the needs of many different applications and more
complex effects.

It is not meant as a generic API for all kinds of video applications,
as it doesn't provides things like an extensive parameter mechanism or
event handling.

Eventually the frei0r API can be wrapped by higher level APIs
expanding its functionalities
(for instance as GStreamer, MLT, FFmpeg and others do).

## Current status 

Developers are sporadically contributing and we are happy if more
people like to get involved, so let us know about your creations! Code
and patches are well accepted, get in touch with us on the
mailinglist (see the section Communication below).


## History 

Frei0r has been around since 2004, born from yearly brainstormings
held at the Piksel conference with the participation of various free
and open source video software developers.

It works on all hardware platforms without the need for any particular
hardware acceleration (GNU/Linux, Apple/OSX and MS/Win) and consists
of more than 100 plugins. Among the free and open source video
application supporting frei0r are: KDEnLive, FFmpeg, MLT, PureData,
Open Movie Editor, DVEdit, Gephex, LiVES, FreeJ, VeeJay, Flowblade, and
Shotcut among the others.

Wikipedia page about frei0r: http://en.wikipedia.org/wiki/Frei0r


[Piksel]: http://www.piksel.no
[PureData]: http://www.artefacte.org/pd/
[Open  Movie  Editor]: http://openmovieeditor.sourceforge.net/
[DVEdit]: http://www.freenet.org.nz/dvedit
[Gephex]: http://www.gephex.org/
[LiVES]: http://lives.sf.net
[FreeJ]: http://freej.dyne.org
[MøB]: http://mob.bek.no/
[VeeJay]: http://veejayhq.net
[MLT]: http://www.mltframework.org/
[KDEnLive]: http://www.kdenlive.org/
[Flowblade]: http://code.google.com/p/flowblade/
[Shotcut]: https://www.shotcut.org/


# Downloads

## Source code 

Stable frei0r releases are packaged periodically and distributed on

 https://files.dyne.org/frei0r

Frei0r sourcecode is released under the terms of the GNU General Public License and, eventually other compatible Free Software licenses.

The latest source for frei0r plugins can be attained using git on https://github.com/dyne/frei0r

Make sure to get in touch with our mailinglist if you like to contribute.

## Build dependencies 

Frei0r can be built on GNU/Linux, M$/Windows and Apple/OSX platforms, possibly in even more environments like embedded devices.

For details see the [INSTALL](/INSTALL) file.

### GNU / Linux

Binary packages are mantained on various distributions, but they may not be completely up to date with latest release.

### Apple / OSX 

MacPorts provides packages for OSX:
[MacPorts]: http://www.macports.org
          $ sudo port install frei0r-plugins

Pre-compiled binaries are also uploaded on our website.

We encourage Apple/OSX application distributors to compile the plugins
directly and to include frei0r within their bundle.



### Microsoft / Windows

Pre-compiled binaries are often provided by third-parties, but they may not to be up to date.

We encourage MS/Win application distributors to compile the plugins directly and to include frei0r within their bundle.


# Documentation 


If you are new to frei0r (but not to programming) the best thing is probably to have a look at the [frei0r header](/include/frei0r.h), which is quite simple and well documented. The [doxyfied documentation](http://frei0r.dyne.org/codedoc/html) is also available for browsing on-line.


## C++ Filter example 

You could find a tutorial filter [here](/src/filter/tutorial) in the source code.
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


## Communication 

You can get in touch with our developer community, send your new effects and share your intentions with us.

We have a free mailinglist open to [subscription](https://mailinglists.dyne.org/cgi-bin/mailman/listinfo/frei0r) and we provide [public archives](http://lists.dyne.org/lurker/list/frei0r.en.html) of the discussions there that are also searchable and indexed online.

For bug reporting the mailinglist is preferred, but is also possible to use an [issue tracker](https://github.com/dyne/frei0r/issues).

## Acknowledgments 

Frei0r is the result of a collective effort in coordination with several software developers meeting to find a common standard for video effect plugins to be used among their applications.

For a full list of contributors and the project history, see the file [AUTHORS](/AUTHORS), the [ChangeLog](/ChangeLog) and the project web page: https://frei0r.dyne.org


