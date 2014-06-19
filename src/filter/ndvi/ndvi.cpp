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

#include "frei0r.hpp"
#include "frei0r_math.h"
#include "gradientlut.hpp"
#include <string>
#include <stdlib.h>

/**
This filter calculates NDVI values from near-infrared + blue (infrablue) video.
The NDVI values are mapped to a false color image.
*/

static inline double N2P(double ndvi) {
    // Convert NDVi value (-1 to 1) to position (0 to 1).
    return (ndvi + 1.0) / 2.0;
}

class Ndvi : public frei0r::filter
{
public:
    Ndvi(unsigned int width, unsigned int height);
    virtual void update();

private:
    void initLut();

    double paramLutLevels;
    std::string paramColorMap;
    double paramVisScale;
    double paramVisOffset;
    double paramNirScale;
    double paramNirOffset;
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
}

void Ndvi::update() {
    uint8_t *inP = (uint8_t*)in;
    uint8_t *outP = (uint8_t*)out;
    double visScale = paramVisScale * 10.0;
    double visOffset =  (paramVisOffset * 510) - 255;
    double nirScale = paramNirScale * 10.0;
    double nirOffset = (paramNirOffset * 510) - 255;

    initLut();

    for (int i = 0; i < size; i++) {
        double vis =  inP[2]; // blue channel
        double nir =  inP[0]; // red channel
        vis = (vis + visOffset) * visScale;
        vis = CLAMP(vis, 0.0, 255.0);
        nir = (nir + nirOffset) * nirScale;
        nir = CLAMP(nir, 0.0, 255.0);
        double ndvi = (nir -vis) / (nir + vis);
        double pos = N2P(ndvi);
        const GradientLut::Color& falseColor = gradient[pos];
        outP[0] = falseColor.r;
        outP[1] = falseColor.g;
        outP[2] = falseColor.b;
        outP[3] = 0xff;
        inP += 4;
        outP += 4;
    }
}

void Ndvi::initLut() {
    // Only update the LUT if a parameter has changed.
    unsigned int paramLutLevelsInt = paramLutLevels * 1000.0 + 0.5;
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

frei0r::construct<Ndvi> plugin("NDVI filter",
            "This filter creates a NDVI false image from an infrablue source.",
            "Brian Matherly",
            0,1,
            F0R_COLOR_MODEL_RGBA8888);
