/* 
 * Copyright (C) 2010 Simon Andreas Eugster (simon.eu@gmail.com)
 * This file is a Frei0r plugin.
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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "frei0r.hpp"
#include "frei0r_math.h"

//struct SOP {
//    float slope;
//    float offset;
//    float power;
//} sopR, sopG, sopB, sopA;

class ColorDecisionList : public frei0r::filter
{

public:

    f0r_param_double rSlope;
    f0r_param_double gSlope;
    f0r_param_double bSlope;
    f0r_param_double aSlope;
    f0r_param_double rOffset;
    f0r_param_double gOffset;
    f0r_param_double bOffset;
    f0r_param_double aOffset;
    f0r_param_double rPower;
    f0r_param_double gPower;
    f0r_param_double bPower;
    f0r_param_double aPower;

    ColorDecisionList(unsigned int, unsigned int)
    {

        register_param(rSlope, "rSlope", "Slope of the red color component");
        register_param(gSlope, "gSlope", "Slope of the green color component");
        register_param(bSlope, "bSlope", "Slope of the blue color component");
        register_param(aSlope, "aSlope", "Slope of the alpha component");
        register_param(rOffset, "rOffset", "Offset of the red color component");
        register_param(gOffset, "gOffset", "Offset of the green color component");
        register_param(bOffset, "bOffset", "Offset of the blue color component");
        register_param(aOffset, "aOffset", "Offset of the alpha component");
        register_param(rPower, "rPower", "Power (Gamma) of the red color component");
        register_param(gPower, "gPower", "Power (Gamma) of the green color component");
        register_param(bPower, "bPower", "Power (Gamma) of the blue color component");
        register_param(aPower, "aPower", "Power (Gamma) of the alpha component");
        rSlope = 1;
        gSlope = 1;
        bSlope = 1;
        aSlope = 1;
        rOffset = 0;
        gOffset = 0;
        bOffset = 0;
        aOffset = 0;
        rPower = 1;
        gPower = 1;
        bPower = 1;
        aPower = 1;


//        sopR.slope = .7;
//        sopR.offset = 0.3;
//        sopR.power = 1.5;
//        sopG.slope = 1;
//        sopG.offset = 0;
//        sopG.power = 1;
//        sopB = sopG;
//        sopA = sopG;

        // Pre-build the lookup table.
        // For 1080p, rendering a 5-second video took
        // * 37 s without the LUT
        // * 7  s with the LUT
        // * 5  s without any effect applied (plain rendering).
        // So the LUT brings about 15x speedup.
        m_lutR = (unsigned char *) malloc(256*sizeof(char));
        m_lutG = (unsigned char *) malloc(256*sizeof(char));
        m_lutB = (unsigned char *) malloc(256*sizeof(char));
        m_lutA = (unsigned char *) malloc(256*sizeof(char));

        updateLUT();
    }
    
    ~ColorDecisionList()
    {
        delete m_lutR;
        delete m_lutG;
        delete m_lutB;
        delete m_lutA;
    }

    virtual void update()
    {
        // Rebuild the lookup table in case the prarameters have changed.
        updateLUT();

        unsigned char *pixel = (unsigned char *) in;
        unsigned char *dest = (unsigned char *) out;

        for (unsigned int i = 0; i < size; i++) {
            *dest++ = m_lutR[*pixel++];
            *dest++ = m_lutG[*pixel++];
            *dest++ = m_lutB[*pixel++];
            *dest++ = m_lutA[*pixel++];
        }
    }

private:
    unsigned char *m_lutR;
    unsigned char *m_lutG;
    unsigned char *m_lutB;
    unsigned char *m_lutA;

    void updateLUT() {
        double rS = rSlope*10;
        double gS = gSlope*10;
        double bS = bSlope*10;
        double aS = aSlope*10;

        double rO = (rOffset-0.5);
        double gO = (gOffset-0.5);
        double bO = (bOffset-0.5);
        double aO = (aOffset-0.5);

        double rP = rPower*10;
        double gP = gPower*10;
        double bP = bPower*10;
        double aP = aPower*10;

        printf("rS: %f, %f\n", rSlope, rS);
        printf("rO: %f, %f\n", rOffset, rO);
        printf("rP: %f, %f\n", rPower, rP);

        for (int i = 0; i < 256; i++) {
            m_lutR[i] = CLAMP0255((int) (pow(above0((float)i/255 * rS + rO), rP)*255));
            m_lutG[i] = CLAMP0255((int) (pow(above0((float)i/255 * gS + gO), gP)*255));
            m_lutB[i] = CLAMP0255((int) (pow(above0((float)i/255 * bS + bO), bP)*255));
            m_lutA[i] = CLAMP0255((int) (pow(above0((float)i/255 * aS + aO), aP)*255));
        }
    }

    double above0(double f) {
        if (f < 0) {
            return 0;
        } else {
            return f;
        }
    }
    
};



frei0r::construct<ColorDecisionList> plugin("Color Decision List",
                "ASC CDL test",
                "Simon A. Eugster (Granjow)",
                0,1,
                F0R_COLOR_MODEL_RGBA8888);
