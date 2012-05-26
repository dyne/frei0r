/*
 * Copyright (C) 2010-2011 Simon Andreas Eugster (simon.eu@gmail.com)
 * This file is not a Frei0r plugin but a collection of ideas.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "frei0r.hpp"
#include <iostream>

//#define DEBUG

typedef unsigned char uchar;
struct ABGR {
    uchar r;
    uchar g;
    uchar b;
    uchar a;

    ABGR blend(const ABGR &other, const float opacity)
    {
#ifdef DEBUG
        if (opacity < 0 || opacity > 1) {
            std::cerr << "Timeout indicator: Opacity must be within 0 and 1!" << std::endl;
        }
#endif
        const float l = opacity;
        const float r = 1-opacity;
        ABGR ret;
        ret.r = l*r + r*other.r;
        ret.g = l*g + r*other.g;
        ret.b = l*b + r*other.b;
        ret.a =         other.a;
        return ret;
    }
};
union ABGRPixel {
    ABGR color;
    uint32_t value;
};

class Timeout : public frei0r::filter
{

public:

    Timeout(unsigned int width, unsigned int height)

    {
        register_param(m_time, "time", "Current time");
        register_param(m_color, "color", "Indicator colour");
        register_param(m_transparency, "transparency", "Indicator transparency");

        W = std::min(width, height) / 20;
        H = W;

        x0 = width-2*W;
        y0 = height-H;
    }

    ~Timeout()
    {
        // Delete member variables if necessary.
    }

    virtual void update()
    {
        std::copy(in, in + width*height, out);


        ABGR col;
        col.r = 255*m_color.r;
        col.g = 255*m_color.g;
        col.b = 255*m_color.b;
        col.a = 255;

        float yt = y0 - (1-m_time)*H;
        ABGR *outAsABGR = (ABGR*) out;

#ifdef DEBUG
#define printcol(x) std::cout << #x ": [ " << (int) x.r << " | " << (int) x.g << " | " << (int) x.b << " ]" << std::endl;

        std::cout << "r = " << m_color.r << ", g = " << m_color.g << std::endl;
        ABGR blended = col.blend(outAsABGR[0], .5);
        printcol(col);
        printcol(outAsABGR[0]);
        printcol(blended);
#undef printcol
#endif


        for (int y = y0; y >= int(yt); y--) {

            float factor = 1-m_transparency;
            if (y == int(yt)) {
                factor *= 1 - (yt-int(yt));
            }

            for (int x = x0; x < x0+W; x++) {

                outAsABGR[width*y + x] = col.blend(outAsABGR[width*y + x], factor);
            }
        }

    }

private:
    // The various f0r_params are adjustable parameters.
    // This one determines the size of the black bar in this example.
    f0r_param_double m_time;
    f0r_param_color m_color;
    f0r_param_double m_transparency;

    unsigned int x0, y0;
    unsigned int W , H ;

};



frei0r::construct<Timeout> plugin("Timeout indicator",
                "Timeout indicators e.g. for slides.",
                "Simon A. Eugster",
                0,1,
                F0R_COLOR_MODEL_RGBA8888);
