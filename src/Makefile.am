# Copyright (C) 2007 Richard Spindler <richard.spindler AT gmail.com>
#  
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without 
# modifications, as long as this notice is preserved.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

plugin_LTLIBRARIES = \
3dflippo.la   brightness.la  distort0r.la      grain_merge.la  mask0mate.la  saturation.la  tehroxx0r.la\
addition.la   burn.la        divide.la         hardlight.la    multiply.la     threshold0r.la\
alphaatop.la  bw0r.la        dodge.la          hueshift0r.la   nois0r.la     scanline0r.la  twolay0r.la\
alphain.la         equaliz0r.la      hue.la          nosync0r.la   screen.la      value.la\
alphaout.la   color_only.la  flippo.la         invert0r.la     onecol0r.la   sobel.la       \
alphaover.la  darken.la      gamma.la          ising0r.la      overlay.la    softlight.la   xfade0r.la\
alphaxor.la   delay0r.la                lighten.la      pixeliz0r.la  squareblur.la  contrast0r.la\
blend.la      difference.la  grain_extract.la  lissajous0r.la  saturat0r.la\
subtract.la letterb0xed.la R.la G.la B.la RGB.la composition.la alphainjection.la\
water.la cartoon.la  glow.la lenscorrection.la transparency.la

if HAVE_GAVL
plugin_LTLIBRARIES += scale0tilt.la
scale0tilt_la_SOURCES = filter/scale0tilt/scale0tilt.c
scale0tilt_la_CFLAGS = @GAVL_CFLAGS@ @CFLAGS@
endif

if HAVE_OPENCV
plugin_LTLIBRARIES += facedetect.la
facedetect_la_SOURCES = filter/facedetect/facedetect.c
facedetect_la_CFLAGS = @OPENCV_CFLAGS@ @CFLAGS@ 
endif

transparency_la_SOURCES = filter/transparency/transparency.c
lenscorrection_la_SOURCES = filter/lenscorrection/lenscorrection.c
glow_la_SOURCES = filter/glow/glow.c filter/glow/blur.h
cartoon_la_SOURCES = filter/cartoon/cartoon.cpp
water_la_SOURCES = filter/water/water.cpp
R_la_SOURCES = filter/RGB/R.c
G_la_SOURCES = filter/RGB/G.c
B_la_SOURCES = filter/RGB/B.c
letterb0xed_la_SOURCES = filter/letterb0xed/letterb0xed.c
contrast0r_la_SOURCES = filter/contrast0r/contrast0r.c
3dflippo_la_SOURCES = filter/3dflippo/3dflippo.c
equaliz0r_la_SOURCES = filter/equaliz0r/equaliz0r.cpp
gamma_la_SOURCES = filter/gamma/gamma.c
mask0mate_la_SOURCES = filter/mask0mate/mask0mate.c filter/mask0mate/blur.h
saturat0r_la_SOURCES = filter/saturat0r/saturat0r.c
sobel_la_SOURCES = filter/sobel/sobel.cpp
threshold0r_la_SOURCES = filter/threshold0r/threshold0r.c
brightness_la_SOURCES = filter/brightness/brightness.c
delay0r_la_SOURCES = filter/delay0r/delay0r.cpp
hueshift0r_la_SOURCES = filter/hueshift0r/hueshift0r.c filter/hueshift0r/matrix.h
nosync0r_la_SOURCES = filter/nosync0r/nosync0r.cpp
squareblur_la_SOURCES = filter/squareblur/squareblur.c
twolay0r_la_SOURCES = filter/twolay0r/twolay0r.cpp
bw0r_la_SOURCES = filter/bw0r/bw0r.c
distort0r_la_SOURCES = filter/distort0r/distort0r.c
flippo_la_SOURCES = filter/flippo/flippo.c
invert0r_la_SOURCES = filter/invert0r/invert0r.c
pixeliz0r_la_SOURCES = filter/pixeliz0r/pixeliz0r.c
scanline0r_la_SOURCES = filter/scanline0r/scanline0r.cpp
tehroxx0r_la_SOURCES = filter/tehroxx0r/tehRoxx0r.c
ising0r_la_SOURCES = generator/ising0r/ising0r.c
lissajous0r_la_SOURCES = generator/lissajous0r/lissajous0r.cpp
nois0r_la_SOURCES = generator/nois0r/nois0r.cpp
onecol0r_la_SOURCES = generator/onecol0r/onecol0r.cpp
composition_la_SOURCES = mixer2/composition/composition.c
alphainjection_la_SOURCES = mixer2/alphainjection/alphainjection.c
addition_la_SOURCES = mixer2/addition/addition.cpp
alphaout_la_SOURCES = mixer2/alphaout/alphaout.cpp
blend_la_SOURCES = mixer2/blend/blend.cpp
darken_la_SOURCES = mixer2/darken/darken.cpp
dodge_la_SOURCES = mixer2/dodge/dodge.cpp
hardlight_la_SOURCES = mixer2/hardlight/hardlight.cpp
multiply_la_SOURCES = mixer2/multiply/multiply.cpp
screen_la_SOURCES = mixer2/screen/screen.cpp
value_la_SOURCES = mixer2/value/value.cpp
alphaatop_la_SOURCES = mixer2/alphaatop/alphaatop.cpp
alphaover_la_SOURCES = mixer2/alphaover/alphaover.cpp
burn_la_SOURCES = mixer2/burn/burn.cpp
difference_la_SOURCES = mixer2/difference/difference.cpp
grain_extract_la_SOURCES = mixer2/grain_extract/grain_extract.cpp
hue_la_SOURCES = mixer2/hue/hue.cpp
overlay_la_SOURCES = mixer2/overlay/overlay.cpp
softlight_la_SOURCES = mixer2/softlight/softlight.cpp
xfade0r_la_SOURCES = mixer2/xfade0r/xfade0r.cpp
alphain_la_SOURCES = mixer2/alphain/alphain.cpp
alphaxor_la_SOURCES = mixer2/alphaxor/alphaxor.cpp
color_only_la_SOURCES = mixer2/color_only/color_only.cpp
divide_la_SOURCES = mixer2/divide/divide.cpp
grain_merge_la_SOURCES = mixer2/grain_merge/grain_merge.cpp
lighten_la_SOURCES = mixer2/lighten/lighten.cpp
saturation_la_SOURCES = mixer2/saturation/saturation.cpp
subtract_la_SOURCES = mixer2/subtract/subtract.cpp
RGB_la_SOURCES = mixer3/RGB/RGB.c


AM_CPPFLAGS = -I@top_srcdir@/include 
AM_CFLAGS = -I@top_srcdir@/include 
AM_LDFLAGS = -module -avoid-version -Wc,-nostartfiles

plugindir = @prefix@/lib/frei0r-1


install-pluginLTLIBRARIES: $(plugin_LTLIBRARIES)
	mkdir -p $(DESTDIR)/$(plugindir)
	list='$(plugin_LTLIBRARIES)'; \
	for file in $$list; do \
	  sofile=`basename $$file .la`.so; \
	  $(INSTALL_PROGRAM) .libs/$$sofile $(DESTDIR)/$(plugindir); \
	done

uninstall-pluginLTLIBRARIES:
	list='$(plugin_LTLIBRARIES)'; \
	for file in $$list; do \
	  sofile=`basename $$file .la`.so; \
	  rm -f $(DESTDIR)/$(plugindir)/$$sofile; \
	done


