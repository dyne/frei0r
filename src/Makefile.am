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
	3dflippo.la \
	addition.la \
	addition_alpha.la \
	alpha0ps.la \
	alphaatop.la \
	alphagrad.la \
	alphain.la \
	alphainjection.la \
	alphaout.la \
	alphaover.la \
	alphaspot.la \
	alphaxor.la \
	B.la \
	balanc0r.la \
	baltan.la \
	blend.la \
	bluescreen0r.la \
	brightness.la \
	burn.la \
	bw0r.la \
	c0rners.la \
	cartoon.la \
	cluster.la \
	colgate.la \
	coloradj_RGB.la \
	colordistance.la \
	colorhalftone.la \
	colorize.la \
	colortap.la \
	color_only.la \
	composition.la \
	contrast0r.la \
	curves.la \
	d90stairsteppingfix.la \
	darken.la \
	defish0r.la \
	delay0r.la \
	delaygrab.la \
	difference.la \
	distort0r.la \
	dither.la \
	divide.la \
	dodge.la \
	edgeglow.la \
	emboss.la \
	equaliz0r.la \
	flippo.la \
	G.la \
	gamma.la \
	glow.la \
	grain_extract.la \
	grain_merge.la \
	hardlight.la \
	hqdn3d.la \
	hue.la \
	hueshift0r.la \
	IIRblur.la \
	invert0r.la \
	ising0r.la \
	keyspillm0pup.la \
	lenscorrection.la \
	letterb0xed.la \
	levels.la \
	lighten.la \
	lightgraffiti.la \
	lissajous0r.la \
	luminance.la \
	mask0mate.la \
	medians.la \
	multiply.la \
	nervous.la \
	nois0r.la \
	nosync0r.la \
	onecol0r.la \
	overlay.la \
	partik0l.la \
	perspective.la \
	pixeliz0r.la \
	plasma.la \
	posterize.la \
	pr0be.la \
	pr0file.la \
	primaries.la \
	R.la \
	RGB.la \
	rgbnoise.la \
	saturation.la \
	saturat0r.la \
	scanline0r.la \
	screen.la \
	select0r.la \
	sharpness.la \
	sigmoidaltransfer.la \
	sobel.la \
	softglow.la \
	softlight.la \
	sopsat.la \
	spillsupress.la \
	squareblur.la \
	subtract.la \
	tehroxx0r.la \
	test_pat_B.la \
	test_pat_C.la \
	test_pat_G.la \
	test_pat_I.la \
	test_pat_L.la \
	test_pat_R.la \
	three_point_balance.la \
	threshold0r.la \
	threelay0r.la \
	timeout.la \
	tint0r.la \
	transparency.la \
	twolay0r.la \
	uvmap.la \
	value.la \
	vertigo.la \
	vignette.la \
	xfade0r.la

if HAVE_GAVL
plugin_LTLIBRARIES += scale0tilt.la
scale0tilt_la_SOURCES = filter/scale0tilt/scale0tilt.c
scale0tilt_la_CFLAGS = @GAVL_CFLAGS@ @CFLAGS@
scale0tilt_la_LIBADD = @GAVL_LIBS@

plugin_LTLIBRARIES += vectorscope.la
vectorscope_la_SOURCES = filter/vectorscope/vectorscope.c filter/vectorscope/vectorscope_image.h
vectorscope_la_CFLAGS = @GAVL_CFLAGS@ @CFLAGS@
vectorscope_la_LIBADD = @GAVL_LIBS@

plugin_LTLIBRARIES += rgbparade.la
rgbparade_la_SOURCES = filter/rgbparade/rgbparade.c filter/rgbparade/rgbparade_image.h
rgbparade_la_CFLAGS = @GAVL_CFLAGS@ @CFLAGS@
rgbparade_la_LIBADD = @GAVL_LIBS@
endif

if HAVE_OPENCV
plugin_LTLIBRARIES += facebl0r.la
facebl0r_la_SOURCES = filter/facebl0r/facebl0r.cpp
facebl0r_la_CFLAGS = @OPENCV_CFLAGS@ @CFLAGS@ 
facebl0r_la_LIBADD = @OPENCV_LIBS@
plugin_LTLIBRARIES += facedetect.la
facedetect_la_SOURCES = filter/facedetect/facedetect.cpp
facedetect_la_CFLAGS = @OPENCV_CFLAGS@ @CFLAGS@
facedetect_la_LIBADD = @OPENCV_LIBS@
endif

if HAVE_CAIRO
plugin_LTLIBRARIES += cairogradient.la
cairogradient_la_SOURCES = filter/cairogradient/cairogradient.c
cairogradient_la_CFLAGS = @CAIRO_CFLAGS@ @CFLAGS@
cairogradient_la_LIBADD = @CAIRO_LIBS@

plugin_LTLIBRARIES += cairoimagegrid.la
cairoimagegrid_la_SOURCES = filter/cairoimagegrid/cairoimagegrid.c
cairoimagegrid_la_CFLAGS = @CAIRO_CFLAGS@ @CFLAGS@
cairoimagegrid_la_LIBADD = @CAIRO_LIBS@

plugin_LTLIBRARIES += cairoaffineblend.la
cairoaffineblend_la_SOURCES = mixer2/cairoaffineblend/cairoaffineblend.c
cairoaffineblend_la_CFLAGS = @CAIRO_CFLAGS@ @CFLAGS@
cairoaffineblend_la_LIBADD = @CAIRO_LIBS@

plugin_LTLIBRARIES += cairoblend.la
cairoblend_la_SOURCES = mixer2/cairoblend/cairoblend.c
cairoblend_la_CFLAGS = @CAIRO_CFLAGS@ @CFLAGS@
cairoblend_la_LIBADD = @CAIRO_LIBS@
endif

#
# FILTERS
#
3dflippo_la_SOURCES = filter/3dflippo/3dflippo.c
alpha0ps_la_SOURCES = filter/alpha0ps/alpha0ps.c filter/alpha0ps/fibe_f.h
alphagrad_la_SOURCES = filter/alpha0ps/alphagrad.c
alphaspot_la_SOURCES = filter/alpha0ps/alphaspot.c
B_la_SOURCES = filter/RGB/B.c
balanc0r_la_SOURCES = filter/balanc0r/balanc0r.c
baltan_la_SOURCES = filter/baltan/baltan.cpp
bluescreen0r_la_SOURCES = filter/bluescreen0r/bluescreen0r.cpp
brightness_la_SOURCES = filter/brightness/brightness.c
bw0r_la_SOURCES = filter/bw0r/bw0r.c
c0rners_la_SOURCES = filter/c0rners/c0rners.c filter/c0rners/interp.h
cartoon_la_SOURCES = filter/cartoon/cartoon.cpp
cluster_la_SOURCES = filter/cluster/cluster.c
colgate_la_SOURCES = filter/colgate/colgate.c
coloradj_RGB_la_SOURCES = filter/coloradj/coloradj_RGB.c
colordistance_la_SOURCES = filter/colordistance/colordistance.c
colorhalftone_la_SOURCES = filter/colorhalftone/colorhalftone.c
colorize_la_SOURCES = filter/colorize/colorize.c
colortap_la_SOURCES = filter/colortap/colortap.c
contrast0r_la_SOURCES = filter/contrast0r/contrast0r.c
curves_la_SOURCES = filter/curves/curves.c
d90stairsteppingfix_la_SOURCES = filter/d90stairsteppingfix/d90stairsteppingfix.cpp
defish0r_la_SOURCES = filter/defish0r/defish0r.c filter/defish0r/interp.h
delay0r_la_SOURCES = filter/delay0r/delay0r.cpp
delaygrab_la_SOURCES = filter/delaygrab/delaygrab.cpp
distort0r_la_SOURCES = filter/distort0r/distort0r.c
dither_la_SOURCES = filter/dither/dither.c
edgeglow_la_SOURCES = filter/edgeglow/edgeglow.cpp
emboss_la_SOURCES = filter/emboss/emboss.c
equaliz0r_la_SOURCES = filter/equaliz0r/equaliz0r.cpp
flippo_la_SOURCES = filter/flippo/flippo.c
G_la_SOURCES = filter/RGB/G.c
gamma_la_SOURCES = filter/gamma/gamma.c
glow_la_SOURCES = filter/glow/glow.c
hqdn3d_la_SOURCES = filter/denoise/hqdn3d.c
hueshift0r_la_SOURCES = filter/hueshift0r/hueshift0r.c filter/hueshift0r/matrix.h
IIRblur_la_SOURCES = filter/blur/IIRblur.c filter/blur/fibe.h
invert0r_la_SOURCES = filter/invert0r/invert0r.c
keyspillm0pup_la_SOURCES = filter/keyspillm0pup/keyspillm0pup.c
lenscorrection_la_SOURCES = filter/lenscorrection/lenscorrection.c
letterb0xed_la_SOURCES = filter/letterb0xed/letterb0xed.c
levels_la_SOURCES = filter/levels/levels.c
lightgraffiti_la_SOURCES = filter/lightgraffiti/lightgraffiti.cpp
luminance_la_SOURCES = filter/luminance/luminance.c
mask0mate_la_SOURCES = filter/mask0mate/mask0mate.c
medians_la_SOURCES = filter/medians/medians.c filter/medians/ctmf.h filter/medians/small_medians.h
nervous_la_SOURCES = filter/nervous/nervous.cpp
nosync0r_la_SOURCES = filter/nosync0r/nosync0r.cpp
partik0l_la_SOURCES = generator/partik0l/partik0l.cpp
perspective_la_SOURCES = filter/perspective/perspective.c
pixeliz0r_la_SOURCES = filter/pixeliz0r/pixeliz0r.c
posterize_la_SOURCES = filter/posterize/posterize.c
pr0be_la_SOURCES = filter/measure/pr0be.c filter/measure/measure.h filter/measure/font2.h
pr0file_la_SOURCES = filter/measure/pr0file.c filter/measure/measure.h filter/measure/font2.h
primaries_la_SOURCES = filter/primaries/primaries.cpp
R_la_SOURCES = filter/RGB/R.c
rgbnoise_la_SOURCES = filter/rgbnoise/rgbnoise.c
saturat0r_la_SOURCES = filter/saturat0r/saturat0r.c
scanline0r_la_SOURCES = filter/scanline0r/scanline0r.cpp
select0r_la_SOURCES = filter/select0r/select0r.c
sharpness_la_SOURCES = filter/sharpness/sharpness.c
sigmoidaltransfer_la_SOURCES = filter/sigmoidaltransfer/sigmoidaltransfer.c
sobel_la_SOURCES = filter/sobel/sobel.cpp
softglow_la_SOURCES = filter/softglow/softglow.c
sopsat_la_SOURCES = filter/sopsat/sopsat.cpp
spillsupress_la_SOURCES = filter/spillsupress/spillsupress.c
squareblur_la_SOURCES = filter/squareblur/squareblur.c
tehroxx0r_la_SOURCES = filter/tehroxx0r/tehRoxx0r.c
threelay0r_la_SOURCES = filter/threelay0r/threelay0r.cpp
three_point_balance_la_SOURCES = filter/three_point_balance/three_point_balance.c
threshold0r_la_SOURCES = filter/threshold0r/threshold0r.c
timeout_la_SOURCES = filter/timeout/timeout.cpp
tint0r_la_SOURCES = filter/tint0r/tint0r.c
transparency_la_SOURCES = filter/transparency/transparency.c
twolay0r_la_SOURCES = filter/twolay0r/twolay0r.cpp
vertigo_la_SOURCES = filter/vertigo/vertigo.c
vignette_la_SOURCES = filter/vignette/vignette.cpp

#
# GENERATORS
#
ising0r_la_SOURCES = generator/ising0r/ising0r.c
lissajous0r_la_SOURCES = generator/lissajous0r/lissajous0r.cpp
nois0r_la_SOURCES = generator/nois0r/nois0r.cpp
onecol0r_la_SOURCES = generator/onecol0r/onecol0r.cpp
plasma_la_SOURCES = generator/dem0scene/plasma.cpp
test_pat_B_la_SOURCES = generator/test_pat/test_pat_B.c
test_pat_C_la_SOURCES = generator/test_pat/test_pat_C.c
test_pat_G_la_SOURCES = generator/test_pat/test_pat_G.c
test_pat_I_la_SOURCES = generator/test_pat/test_pat_I.c
test_pat_L_la_SOURCES = generator/test_pat/test_pat_L.c
test_pat_R_la_SOURCES = generator/test_pat/test_pat_R.c

#
# MIXERS
#
addition_la_SOURCES = mixer2/addition/addition.cpp
addition_alpha_la_SOURCES = mixer2/addition_alpha/addition_alpha.cpp
alphaatop_la_SOURCES = mixer2/alphaatop/alphaatop.cpp
alphainjection_la_SOURCES = mixer2/alphainjection/alphainjection.c
alphain_la_SOURCES = mixer2/alphain/alphain.cpp
alphaout_la_SOURCES = mixer2/alphaout/alphaout.cpp
alphaover_la_SOURCES = mixer2/alphaover/alphaover.cpp
alphaxor_la_SOURCES = mixer2/alphaxor/alphaxor.cpp
blend_la_SOURCES = mixer2/blend/blend.cpp
burn_la_SOURCES = mixer2/burn/burn.cpp
color_only_la_SOURCES = mixer2/color_only/color_only.cpp
composition_la_SOURCES = mixer2/composition/composition.c
darken_la_SOURCES = mixer2/darken/darken.cpp
difference_la_SOURCES = mixer2/difference/difference.cpp
divide_la_SOURCES = mixer2/divide/divide.cpp
dodge_la_SOURCES = mixer2/dodge/dodge.cpp
grain_extract_la_SOURCES = mixer2/grain_extract/grain_extract.cpp
grain_merge_la_SOURCES = mixer2/grain_merge/grain_merge.cpp
hardlight_la_SOURCES = mixer2/hardlight/hardlight.cpp
hue_la_SOURCES = mixer2/hue/hue.cpp
lighten_la_SOURCES = mixer2/lighten/lighten.cpp
multiply_la_SOURCES = mixer2/multiply/multiply.cpp
overlay_la_SOURCES = mixer2/overlay/overlay.cpp
RGB_la_SOURCES = mixer3/RGB/RGB.c
saturation_la_SOURCES = mixer2/saturation/saturation.cpp
screen_la_SOURCES = mixer2/screen/screen.cpp
softlight_la_SOURCES = mixer2/softlight/softlight.cpp
subtract_la_SOURCES = mixer2/subtract/subtract.cpp
uvmap_la_SOURCES = mixer2/uvmap/uvmap.c
value_la_SOURCES = mixer2/value/value.cpp
xfade0r_la_SOURCES = mixer2/xfade0r/xfade0r.cpp


AM_CPPFLAGS = -I@top_srcdir@/include 
AM_CFLAGS = -I@top_srcdir@/include 
AM_LDFLAGS = -module -avoid-version -lm -export-dynamic
AM_LIBTOOLFLAGS = --tag=disable-static

plugindir = @libdir@/frei0r-1


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
