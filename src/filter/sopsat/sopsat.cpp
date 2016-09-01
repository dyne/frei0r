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
#include <stdlib.h>
#include "frei0r.hpp"
#include "frei0r_math.h"

/**
  This filter implements a standard way of color correction proposed by
  the American Society of Cinematographers: The Color Decision List, also
  known as the

     ASC CDL

  More information about the ASC CDL can be found on Wikipedia[1], and
  the current revision of the specification, including example code, can
  be obtained by sending a mail to asc-cdl at theasc dot com. (Really works.)

  The ASC CDL is a standard format for basic primary color correction (primary
  meaning affecting the whole image and not only selected parts). Even big
  editing systems use it :) This filter only obtains the values; Importing and
  exporting to one of the possible ASC CDL exchange files must be done elsewhere.

  Basically there are two stages in the correction:
  1. SOP correction for each channel separately
  2. Overall saturation correction
  All corrections work on [0,1], so the RGB(A) values need to be transposed
  from {0,...,255} to [0,1].
  1. SOP correction
     * Slope:   out = in * slope;   0 <= slope < \infty
     * Offset:  out = in + offset;  -\infty < offset < \infty
     * Power:   out = in^power;     0 < power < \infty
  2. Saturation
     * Luma:    Y = 0.2126 R + 0.7152 G + 0.0722 B (according to Rec. 709)
     * Forall channels:
                out = luma + sat * (in-luma)
  As the values may exceed 1 (or 0), they need to be clamped where necessary.


  [1] http://en.wikipedia.org/wiki/Color_Decision_List
 */

class SOPSat : public frei0r::filter
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
    f0r_param_double saturation;

    SOPSat(unsigned int, unsigned int)
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
        register_param(saturation, "saturation", "Overall saturation");
        rSlope = 1 / 20.;
        gSlope = 1 / 20.;
        bSlope = 1 / 20.;
        aSlope = 1 / 20.;
        rOffset = (0 - (-4)) / (double)(4 - (-4));
        gOffset = (0 - (-4)) / (double)(4 - (-4));
        bOffset = (0 - (-4)) / (double)(4 - (-4));
        aOffset = (0 - (-4)) / (double)(4 - (-4));
        rPower = 1 / 20.;
        gPower = 1 / 20.;
        bPower = 1 / 20.;
        aPower = 1 / 20.;
        saturation = 1 / 10.;

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
    
    ~SOPSat()
    {
        free(m_lutR);
        free(m_lutG);
        free(m_lutB);
        free(m_lutA);
    }

    virtual void update(double time,
	                    uint32_t* out,
                        const uint32_t* in)
    {
        // Rebuild the lookup table in case the prarameters have changed.
        updateLUT();

        unsigned char *pixel = (unsigned char *) in;
        unsigned char *dest = (unsigned char *) out;

        if (fabs(m_sat-1) < 0.001) {
            // Calculating the saturation is expensive. So first check whether
            // we really need to do it.
            // Keeping the if/else outside of the loop gives a little speed gain.
            // Worth the duplicate code, as only 4 lines so far :)

            for (unsigned int i = 0; i < size; i++) {
                *dest++ = m_lutR[*pixel++];
                *dest++ = m_lutG[*pixel++];
                *dest++ = m_lutB[*pixel++];
                *dest++ = m_lutA[*pixel++];
            }
        } else {
            double luma;
            for (unsigned int i = 0; i < size; i++) {
                luma =   0.2126 * m_lutR[*(pixel+0)]
                       + 0.7152 * m_lutG[*(pixel+1)]
                       + 0.0722 * m_lutB[*(pixel+2)];
                *dest++ = CLAMP0255(luma + m_sat*(m_lutR[*pixel++]-luma));
                *dest++ = CLAMP0255(luma + m_sat*(m_lutG[*pixel++]-luma));
                *dest++ = CLAMP0255(luma + m_sat*(m_lutB[*pixel++]-luma));
                *dest++ = m_lutA[*pixel++];
            }
        }
    }

private:
    unsigned char *m_lutR;
    unsigned char *m_lutG;
    unsigned char *m_lutB;
    unsigned char *m_lutA;

    double m_sat;

    void updateLUT() {
        double rS = rSlope * 20;
        double gS = gSlope * 20;
        double bS = bSlope * 20;
        double aS = aSlope * 20;

        double rO = -4 + rOffset * (4 - (-4));
        double gO = -4 + gOffset * (4 - (-4));
        double bO = -4 + bOffset * (4 - (-4));
        double aO = -4 + aOffset * (4 - (-4));

        double rP = rPower * 20;
        double gP = gPower * 20;
        double bP = bPower * 20;
        double aP = aPower * 20;

        m_sat = saturation * 10;

        for (int i = 0; i < 256; i++) {
            // above0 avoids overflows for negative numbers.
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



frei0r::construct<SOPSat> plugin("SOP/Sat",
                "Slope/Offset/Power and Saturation color corrections according to the ASC CDL (Color Decision List)",
                "Simon A. Eugster (Granjow)",
                0,3,
                F0R_COLOR_MODEL_RGBA8888);
