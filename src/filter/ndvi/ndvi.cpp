/*
 * Copyright (C) 2014 Brian Matherly (pez4brian@yahoo.com)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifdef HAVE_CAIRO
#include <cairo.h>
#endif
#include "frei0r.hpp"
#include "frei0r/math.h"
#include "gradientlut.hpp"
#include <string>
#include <stdlib.h>

/**
This filter calculates NDVI or VI values from near-infrared + visible video.
The index values are mapped to a false color image.
*/

static inline double N2P(double ndvi) {
    // Convert an index value (-1.0 to 1.0) to position (0.0 to 1.0).
    return (ndvi + 1.0) / 2.0;
}

static unsigned int ColorIndex(const std::string& str) {
    // Convert a color initial to a component index.
    if(str == "r" || str == "R") {
        return 0;
    } else if(str == "g" || str == "G") {
        return 1;
    } else { // "b"
        return 2;
    }
}

class Ndvi : public frei0r::filter
{
public:
    Ndvi(unsigned int width, unsigned int height);
    virtual void update(double time,
                        uint32_t* out,
                        const uint32_t* in);

private:
    void initLut();
    double getComponent(uint8_t *sample, unsigned int chan, double offset, double scale);
    void setColor(uint8_t *sample, double index);
    void drawLegend(uint32_t *out);
    void drawRect( uint32_t* out, uint8_t r, uint8_t g, uint8_t b, unsigned int x, unsigned int y, unsigned int w, unsigned int h );
    void drawGradient( uint32_t* out, unsigned int x, unsigned int y, unsigned int w, unsigned int h );
    void drawText( uint32_t* out, std::string text, unsigned int x, unsigned int y, unsigned int textHeight );

    double paramLutLevels;
    std::string paramColorMap;
    double paramVisScale;
    double paramVisOffset;
    double paramNirScale;
    double paramNirOffset;
    std::string paramVisChan;
    std::string paramNirChan;
    std::string paramIndex;
    std::string paramLegend;
    unsigned int lutLevels;
    std::string colorMap;
    GradientLut gradient;
};

Ndvi::Ndvi(unsigned int width, unsigned int height)
 : paramLutLevels(256.0 / 1000.0)
 , paramColorMap("grayscale")
 , paramVisScale(0.1)
 , paramVisOffset(0.5)
 , paramNirScale(0.1)
 , paramNirOffset(0.5)
 , paramVisChan("b")
 , paramNirChan("r")
 , paramIndex("ndvi")
 , paramLegend("off")
 , lutLevels(0)
 , colorMap("")
 , gradient()
{
    register_param(paramColorMap,  "Color Map",
            "The color map to use. One of 'earth', 'grayscale', 'heat' or 'rainbow'.");
    register_param(paramLutLevels, "Levels",
            "The number of color levels to use in the false image (divided by 1000).");
    register_param(paramVisScale, "VIS Scale",
            "A scaling factor to be applied to the visible component (divided by 10).");
    register_param(paramVisOffset, "VIS Offset",
            "An offset to be applied to the visible component (mapped to [-100%, 100%].");
    register_param(paramNirScale, "NIR Scale",
            "A scaling factor to be applied to the near-infrared component (divided by 10).");
    register_param(paramNirOffset, "NIR Offset",
            "An offset to be applied to the near-infrared component (mapped to [-100%, 100%].");
    register_param(paramVisChan,  "Visible Channel",
            "The channel to use for the visible component. One of 'r', 'g', or 'b'.");
    register_param(paramNirChan,  "NIR Channel",
            "The channel to use for the near-infrared component. One of 'r', 'g', or 'b'.");
    register_param(paramIndex,  "Index Calculation",
            "The index calculation to use. One of 'ndvi' or 'vi'.");
    register_param(paramLegend,  "Legend",
            "Control legend display. One of 'off' or 'bottom'.");
}

void Ndvi::update(double time,
                  uint32_t* out,
                  const uint32_t* in) {
    uint8_t *inP = (uint8_t*)in;
    uint8_t *outP = (uint8_t*)out;
    double visScale = paramVisScale * 10.0;
    double visOffset =  (paramVisOffset * 510) - 255;
    double nirScale = paramNirScale * 10.0;
    double nirOffset = (paramNirOffset * 510) - 255;
    unsigned int visChan = ColorIndex(paramVisChan);
    unsigned int nirChan = ColorIndex(paramNirChan);

    initLut();

    if (paramIndex == "vi") {
        for (unsigned int i = 0; i < size; i++) {
            double vis =  getComponent(inP, visChan, visOffset, visScale);
            double nir =  getComponent(inP, nirChan, nirOffset, nirScale);
            double vi = (nir - vis) / 255.0;
            setColor(outP, vi);
            inP += 4;
            outP += 4;
        }
    } else { // ndvi
        for (unsigned int i = 0; i < size; i++) {
            double vis =  getComponent(inP, visChan, visOffset, visScale);
            double nir =  getComponent(inP, nirChan, nirOffset, nirScale);
            double ndvi = (nir - vis) / (nir + vis);
            setColor(outP, ndvi);
            inP += 4;
            outP += 4;
        }
    }

    if( paramLegend == "bottom" ) {
        drawLegend(out);
    }
}

void Ndvi::initLut() {
    // Only update the LUT if a parameter has changed.
    unsigned int paramLutLevelsInt = paramLutLevels * 1000.0 + 0.5;
    if (paramLutLevelsInt < 2) paramLutLevelsInt = 2;
    if (paramLutLevelsInt > 1000) paramLutLevelsInt = 1000;
    if (lutLevels == paramLutLevelsInt &&
        colorMap == paramColorMap) {
        return;
    } else {
        lutLevels = paramLutLevelsInt;
        colorMap = paramColorMap;
    }

    gradient.setDepth(lutLevels);

    if(colorMap == "earth") {
        GradientLut::Color water   = {0x30, 0x70, 0xd0};
        GradientLut::Color desert  = {0xd0, 0xc0, 0x90};
        GradientLut::Color grass   = {0x00, 0xc0, 0x20};
        GradientLut::Color forrest = {0x00, 0x30, 0x00};
        gradient.fillRange( N2P(-1.0), water,  N2P(-0.2), water  );
        gradient.fillRange( N2P(-0.2), water,  N2P(-0.1), desert );
        gradient.fillRange( N2P(-0.1), desert, N2P( 0.1), desert );
        gradient.fillRange( N2P( 0.1), desert, N2P( 0.4), grass );
        gradient.fillRange( N2P( 0.4), grass,  N2P( 1.0), forrest );
    } else if(colorMap == "heat") {
        GradientLut::Color n10 = {0x00, 0x00, 0x00};
        GradientLut::Color n08 = {0x10, 0x10, 0x70};
        GradientLut::Color n06 = {0x10, 0x20, 0xf0};
        GradientLut::Color n04 = {0x10, 0x60, 0xf0};
        GradientLut::Color n02 = {0x20, 0xa0, 0xc0};
        GradientLut::Color zer = {0x20, 0xb0, 0x20};
        GradientLut::Color p02 = {0x90, 0xf0, 0x10};
        GradientLut::Color p04 = {0xf0, 0xb0, 0x10};
        GradientLut::Color p06 = {0xf0, 0xa0, 0x10};
        GradientLut::Color p08 = {0xf0, 0x50, 0x10};
        GradientLut::Color p10 = {0xff, 0x00, 0x00};
        gradient.fillRange( N2P(-1.0), n10, N2P(-0.8), n08 );
        gradient.fillRange( N2P(-0.8), n08, N2P(-0.6), n06 );
        gradient.fillRange( N2P(-0.6), n06, N2P(-0.4), n04 );
        gradient.fillRange( N2P(-0.4), n04, N2P(-0.2), n02 );
        gradient.fillRange( N2P(-0.2), n02, N2P( 0.0), zer );
        gradient.fillRange( N2P( 0.0), zer, N2P( 0.2), p02 );
        gradient.fillRange( N2P( 0.2), p02, N2P( 0.4), p04 );
        gradient.fillRange( N2P( 0.4), p04, N2P( 0.6), p06 );
        gradient.fillRange( N2P( 0.6), p06, N2P( 0.8), p08 );
        gradient.fillRange( N2P( 0.8), p08, N2P( 1.0), p10 );
    } else if(colorMap == "rainbow") {
        GradientLut::Color violet = {0x7f, 0x00, 0xff};
        GradientLut::Color blue   = {0x00, 0x00, 0xff};
        GradientLut::Color green  = {0x00, 0xff, 0x00};
        GradientLut::Color yellow = {0xff, 0xff, 0x00};
        GradientLut::Color orange = {0xff, 0x7f, 0x00};
        GradientLut::Color red    = {0xff, 0x00, 0x00};
        gradient.fillRange( N2P(-1.0), violet, N2P(-0.6), blue   );
        gradient.fillRange( N2P(-0.6), blue,   N2P(-0.2), green   );
        gradient.fillRange( N2P(-0.2), green,   N2P( 0.2), yellow  );
        gradient.fillRange( N2P( 0.2), yellow,  N2P( 0.6), orange );
        gradient.fillRange( N2P( 0.6), orange, N2P( 1.0), red    );
    } else { // grayscale
        GradientLut::Color black = {0x00, 0x00, 0x00};
        GradientLut::Color white = {0xff, 0xff, 0xff};
        gradient.fillRange( N2P(-1.0), black, N2P( 1.0), white );
    }
}

inline double Ndvi::getComponent(uint8_t *sample, unsigned int chan, double offset, double scale)
{
    double c =  sample[chan];
    c = (c + offset) * scale;
    c = CLAMP(c, 0.0, 255.0);
    return c;
}

inline void Ndvi::setColor(uint8_t *sample, double index)
{
    double pos = N2P(index);
    const GradientLut::Color& falseColor = gradient[pos];
    sample[0] = falseColor.r;
    sample[1] = falseColor.g;
    sample[2] = falseColor.b;
    sample[3] = 0xff;
}

void Ndvi::drawLegend(uint32_t* out)
{
    unsigned int legendHeight = height / 20;

    // Black border above legend
    unsigned int borderHeight = legendHeight / 15;
    unsigned int borderY = height - legendHeight;
    drawRect( out, 0, 0, 0, 0, borderY, width, borderHeight );

    // Gradient
    unsigned int gradientHeight = legendHeight - borderHeight;
    unsigned int gradientY = height - gradientHeight;
    drawGradient( out, 0, gradientY, width, gradientHeight );

    // Text
    unsigned int textHeight = gradientHeight * 8 / 10;
    unsigned int textY = height - ( gradientHeight - textHeight ) / 2;
    unsigned int textX = width / 25;
    if (paramIndex == "vi") {
        drawText( out, "0", textX, textY , textHeight );
        drawText( out, "VI", width / 2, textY , textHeight );
        drawText( out, "1", width - textX, textY , textHeight );
    } else { // ndvi
        drawText( out, "-1", textX, textY , textHeight );
        drawText( out, "NDVI", width / 2, textY , textHeight );
        drawText( out, "1", width - textX, textY , textHeight );
    }
}

void Ndvi::drawRect( uint32_t* out, uint8_t r, uint8_t g, uint8_t b, unsigned int x, unsigned int y, unsigned int w, unsigned int h )
{
    for (unsigned int i = 0; i < h; i++) {
        uint8_t *sample = (uint8_t*)(out + ((i + y) * width) + x);
        for (unsigned int j = 0; j < w; j++) {
            sample[0] = r;
            sample[1] = g;
            sample[2] = b;
            sample += 4;
        }
    }
}

void Ndvi::drawGradient( uint32_t* out, unsigned int x, unsigned int y, unsigned int w, unsigned int h )
{
    for (unsigned int i = 0; i < w; i++) {
        double pos = (double)i / (double)w;
        const GradientLut::Color& falseColor = gradient[pos];
        uint8_t *sample = (uint8_t*)(out + (y * width) + x + i);
        for (unsigned int j = 0; j < h; j++) {
            sample[0] = falseColor.r;
            sample[1] = falseColor.g;
            sample[2] = falseColor.b;
            sample += width * 4;
        }
    }
}

void Ndvi::drawText( uint32_t* out, std::string text, unsigned int x, unsigned int y, unsigned int textHeight )
{
#ifdef HAVE_CAIRO
    int stride = cairo_format_stride_for_width (CAIRO_FORMAT_ARGB32, width);
    cairo_surface_t* surface = cairo_image_surface_create_for_data((unsigned char*)out,
                                                                   CAIRO_FORMAT_ARGB32,
                                                                   width,
                                                                   height,
                                                                   stride);
    cairo_t *cr = cairo_create(surface);
    cairo_text_extents_t te;
    cairo_font_extents_t fe;

    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, textHeight);
    cairo_text_extents(cr, text.c_str(), &te);
    cairo_font_extents(cr, &fe);

    // Center text on x
    x -= te.width / 2;
    // Align bottom of text on y (assume no characters go below baseline)
    y -= (fe.height - fe.ascent + 1) / 2;

    cairo_move_to(cr, x, y);
    cairo_text_path(cr, text.c_str());
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_fill_preserve(cr);
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_set_line_width(cr, textHeight / 20);
    cairo_stroke (cr);
    cairo_surface_destroy (surface);
    cairo_destroy(cr);
#endif
}

frei0r::construct<Ndvi> plugin("NDVI filter",
            "This filter creates a false image from a visible + infrared source.",
            "Brian Matherly",
            0,2,
            F0R_COLOR_MODEL_RGBA8888);
