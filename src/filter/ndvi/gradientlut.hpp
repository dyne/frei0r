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

#include <vector>
//#define GRADIENTLUT_PRINT
#ifdef GRADIENTLUT_PRINT
#include <cstdio>
#endif

class GradientLut
{
public:
    struct Color {
        uint8_t r;
        uint8_t g;
        uint8_t b;
    };

    void setDepth(size_t depth);
    void fillRange(double startPos, const Color& startColor,
                   double endPos, const Color& endColor);
    const Color& operator[](double pos) const;
    void print() const;

private:
    std::vector<Color> lut;
};

/**
 * Set the size of the look-up table.
 */
void GradientLut::setDepth(size_t depth) {
    lut.resize(depth);
}

/**
 * Fill a part of the look-up table.
 * The indexes in the table and their corresponding colors will be calculated
 * proportionally to the depth of the LUT.
 */
void GradientLut::fillRange( double startPos, const GradientLut::Color& startColor,
                             double endPos,   const GradientLut::Color& endColor ) {
    unsigned int startIndex = (double)(lut.size() - 1) * startPos + 0.5;
    unsigned int endIndex = (double)(lut.size() - 1) * endPos + 0.5;
    unsigned int span = endIndex - startIndex;

    if(span < 1) span = 1;

    for(unsigned int i = 0; i <= span; i++) {
        Color color;
        double ratio = (double)i / (double)span;

        color.r = startColor.r + ratio * ((double)endColor.r - (double)startColor.r);
        color.g = startColor.g + ratio * ((double)endColor.g - (double)startColor.g);
        color.b = startColor.b + ratio * ((double)endColor.b - (double)startColor.b);
        lut[i + startIndex] = color;
    }
}

/**
 * Get a color value for a given position in the table.
 * The index in the table will be calculated proportionally to the depth of the
 * LUT.
 */
const GradientLut::Color& GradientLut::operator[](double pos) const {
    unsigned int size = lut.size();
    unsigned int index = (double)size * pos;
    if(index >= size) {
        index = size - 1;
    }
    return lut[index];
};

/**
 * Debug print function.
 */
void GradientLut::print() const {
#ifdef GRADIENTLUT_PRINT
    printf("LUT:\tIndex\tRed\tGreen\tBlue\n");
    for(int i = 0; i < lut.size(); i++) {
        const Color& color = lut[i];
        printf("\t%3d\t%02x\t%02x\t%02x\n", i, color.r, color.g, color.b);
    }
#endif
}
