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
        const float o1 = opacity;
        const float o2= 1-opacity;
        ABGR ret;
        ret.r = o1*r + o2*other.r;
        ret.g = o1*g + o2*other.g;
        ret.b = o1*b + o2*other.b;
        ret.a =          other.a;
        return ret;
    }
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

    virtual void update(double time,
	                    uint32_t* out,
                        const uint32_t* in)
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


        for (int y = y0; y > int(yt); y--) {

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
    f0r_param_double m_time = 0.0;
    f0r_param_color m_color = {0.0, 0.0, 0.0};
    f0r_param_double m_transparency = 0.0;

    unsigned int x0, y0;
    unsigned int W , H ;

};



frei0r::construct<Timeout> plugin("Timeout indicator",
                "Timeout indicators e.g. for slides.",
                "Simon A. Eugster",
                0,2,
                F0R_COLOR_MODEL_RGBA8888);
