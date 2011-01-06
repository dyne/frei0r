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

/**
  This effect is intended to simulate what happens when you use a shutter speed of
  e.g. 10 seconds for your camera and paint with light, like lamps, in the air
  -- just with video. It tries to remember bright spots and keeps them in a mask.
  Areas that are not very bright (i.e. background) will not sum up.
  */
#include "frei0r.hpp"

#include <cmath>
#include <cstdio>
#include <climits>
#include <algorithm>

// Macros to extract color components
#define GETA(abgr) (((abgr) >> (3*CHAR_BIT)) & 0xFF)
#define GETB(abgr) (((abgr) >> (2*CHAR_BIT)) & 0xFF)
#define GETG(abgr) (((abgr) >> (1*CHAR_BIT)) & 0xFF)
#define GETR(abgr) (((abgr) >> (0*CHAR_BIT)) & 0xFF)

// Macro to assemble a color in RGBA8888 format
#define RGBA(r,g,b,a) ( ((r) << (0*CHAR_BIT)) | ((g) << (1*CHAR_BIT)) | ((b) << (2*CHAR_BIT)) | ((a) << (3*CHAR_BIT)) )
// Component-wise maximum
#define MAX(a,b)  ( (((((a) >> (0*CHAR_BIT)) & 0xFF) > (((b) >> (0*CHAR_BIT)) & 0xFF)) ? ((a) & (0xFF << (0*CHAR_BIT))) : ((b) & (0xFF << (0*CHAR_BIT)))) \
                  | (((((a) >> (1*CHAR_BIT)) & 0xFF) > (((b) >> (1*CHAR_BIT)) & 0xFF)) ? ((a) & (0xFF << (1*CHAR_BIT))) : ((b) & (0xFF << (1*CHAR_BIT)))) \
                  | (((((a) >> (2*CHAR_BIT)) & 0xFF) > (((b) >> (2*CHAR_BIT)) & 0xFF)) ? ((a) & (0xFF << (2*CHAR_BIT))) : ((b) & (0xFF << (2*CHAR_BIT)))) \
                  | (((((a) >> (3*CHAR_BIT)) & 0xFF) > (((b) >> (3*CHAR_BIT)) & 0xFF)) ? ((a) & (0xFF << (3*CHAR_BIT))) : ((b) & (0xFF << (3*CHAR_BIT)))) )

#define CLAMP(a) (((a) < 0) ? 0 : (((a) > 255) ? 255 : (a)))

#define ALPHA(mask,img) \
  (   ( ((uint32_t) ( ((((mask) >> (0*CHAR_BIT)) & 0xFF)/255.0) * ( ((mask) >> (0*CHAR_BIT)) & 0xFF) \
                      + (1 - (( ((mask) >> (0*CHAR_BIT)) & 0xFF)/255.0)) * ( ((img) >> (0*CHAR_BIT)) & 0xFF) )) << (0*CHAR_BIT)) \
      | ( ((uint32_t) ( ((((mask) >> (1*CHAR_BIT)) & 0xFF)/255.0) * ( ((mask) >> (1*CHAR_BIT)) & 0xFF) \
                        + (1 - (( ((mask) >> (1*CHAR_BIT)) & 0xFF)/255.0)) * ( ((img) >> (1*CHAR_BIT)) & 0xFF) )) << (1*CHAR_BIT)) \
      | ( ((uint32_t) ( ((((mask) >> (2*CHAR_BIT)) & 0xFF)/255.0) * ( ((mask) >> (2*CHAR_BIT)) & 0xFF) \
                        + (1 - (( ((mask) >> (2*CHAR_BIT)) & 0xFF)/255.0)) * ( ((img) >> (2*CHAR_BIT)) & 0xFF) )) << (2*CHAR_BIT)) \
      | ( ((uint32_t) ( ((((mask) >> (3*CHAR_BIT)) & 0xFF)/255.0) * ( ((mask) >> (3*CHAR_BIT)) & 0xFF) \
                        + (1 - (( ((mask) >> (3*CHAR_BIT)) & 0xFF)/255.0)) * ( ((img) >> (3*CHAR_BIT)) & 0xFF) )) << (3*CHAR_BIT))   )

// Screen layer mode
#define SCREEN1(mask,img) ((uint8_t) (255-(255.0-(mask))*(255.0-(img))/255.0))

// Luma calculation. Refer to the SOP/Sat filter.
#define REC709Y(r,g,b) (.2126*(r) + .7152*(g) + .0722*(b))

class LightGraffiti : public frei0r::filter
{

public:

    LightGraffiti(unsigned int width, unsigned int height) :
            m_lightMask(width*height, 0),
            m_alphaMap(4*width*height, 0),
            m_meanInitialized(false)

    {
        m_mode = Graffiti_LongAvgAlphaCumC;
        m_dimMode = Dim_Mult;

        // TODO move down, less important
        register_param(m_pSensitivity, "sensitivity", "Basic opacity for a light source. Will be summed up.");
        register_param(m_pBackgroundWeight, "backgroundWeight", "Describes how strong the (accumulated) background should be");
        register_param(m_pThresholdBrightness, "thresholdBrightness", "Brightness threshold to distinguish between foreground and background");
        register_param(m_pThresholdDifference, "thresholdDifference", "Threshold: Difference to background to distinguis between fore- and background");
        register_param(m_pThresholdDiffSum, "thresholdDiffSum", "Threshold for sum of differences");
        register_param(m_pDim, "dim", "Dimming of the light mask");
        register_param(m_pStatsBrightness, "statsBrightness", "Display the brightness and threshold");
        register_param(m_pStatsDiff, "statsDifference", "Display the background difference and threshold");
        register_param(m_pStatsDiffSum, "statsDiffSum", "Display the sum of the background difference and the threshold");
        register_param(m_pReset, "reset", "Reset filter masks");
        register_param(m_pTransparentBackground, "transparentBackground", "Make the background transparent");
        register_param(m_pLongAlpha, "longAlpha", "Alpha value for moving average");
        register_param(m_pNonlinearDim, "nonlinearDim", "Nonlinear dimming (may look more natural)");
        m_pLongAlpha = 1/128.0;
        m_pSensitivity = 1;
        m_pBackgroundWeight = 0;
        m_pThresholdBrightness = 450;
        m_pThresholdDiffSum = 0;
        m_pDim = 0;

    }

    ~LightGraffiti()
    {
    }

    enum GraffitiMode { Graffiti_max, Graffiti_max_sum, Graffiti_Y, Graffiti_Avg, Graffiti_Avg2,
                        Graffiti_Avg_Stat, Graffiti_AvgTresh_Stat, Graffiti_Max_Stat, Graffiti_Y_Stat, Graffiti_S_Stat,
                        Graffiti_STresh_Stat, Graffiti_SDiff_Stat, Graffiti_SDiffTresh_Stat,
                        Graffiti_SSqrt_Stat,
                        Graffiti_LongAvg, Graffiti_LongAvg_Stat, Graffiti_LongAvgAlpha, Graffiti_LongAvgAlpha_Stat,
                        Graffiti_LongAvgAlphaCumC };
    enum DimMode { Dim_Mult, Dim_Sin };





    virtual void update()
    {
        std::copy(in, in + width*height, out);


        if (m_pNonlinearDim) {
            m_dimMode = Dim_Sin;
        } else {
            m_dimMode = Dim_Mult;
        }


        /*
         Refresh background image
         */
        if (!m_meanInitialized || m_pReset) {
            m_longMeanImage = std::vector<float>(width*height*3);
            for (int pixel = 0; pixel < width*height; pixel++) {
                m_longMeanImage[3*pixel+0] = GETR(in[pixel]);
                m_longMeanImage[3*pixel+1] = GETG(in[pixel]);
                m_longMeanImage[3*pixel+2] = GETB(in[pixel]);
            }
            m_meanInitialized = true;
        } else {
            // Calculate the mean image to estimate the background. If alpha is set > 0, bright light sources
            // moving into the image and standing still will eventually be treated as background.
            if (m_pLongAlpha > 0) {
                for (int pixel = 0; pixel < width*height; pixel++) {
                    m_longMeanImage[3*pixel+0] = (1-m_pLongAlpha) * m_longMeanImage[3*pixel+0] + m_pLongAlpha * GETR(in[pixel]);
                    m_longMeanImage[3*pixel+1] = (1-m_pLongAlpha) * m_longMeanImage[3*pixel+1] + m_pLongAlpha * GETG(in[pixel]);
                    m_longMeanImage[3*pixel+2] = (1-m_pLongAlpha) * m_longMeanImage[3*pixel+2] + m_pLongAlpha * GETB(in[pixel]);
                }
            }
        }


        /*
         Light mask dimming
         */
        if (m_pDim > 0) {
            // Dims the light mask. Lights will leave fainting trails.

            float factor = 1-m_pDim;

            /* Gnu Octave:
               range=linspace(0,1,100);
               % Sin
               plot(range,sin(range*pi/2).^.5)
               plot(range,sin(range*pi/2).^.25)
            */

            switch (m_dimMode) {
                case Dim_Mult:
                    for (int i = 0; i < width*height; i++) {
                        m_alphaMap[4*i + 0] *= factor;
                        m_alphaMap[4*i + 1] *= factor;
                        m_alphaMap[4*i + 2] *= factor;
                        m_alphaMap[4*i + 3] *= factor;
                    }
                    break;
                case Dim_Sin:
                    // Attention: Since Graffiti_LongAvgAlphaCumC only makes use of the first alpha channel
                    // the other channels are not calculated here due to efficiency reasons.
                    // May have to be adjusted if required.
                    for (int i = 0; i < width*height; i++) {
                        if (m_alphaMap[4*i + 0] < 1) {
                            m_alphaMap[4*i + 0] *= pow(sin(m_alphaMap[4*i + 0] * M_PI/2), m_pDim) - .01;
                        } else {
                            m_alphaMap[4*i + 0] *= factor;
                        }
                        if (m_alphaMap[4*i + 0] < 0) { m_alphaMap[4*i + 0] = 0; }
                    }
                    break;
            }

        }



        /*
         Reset all masks if desired
         */
        if (m_pReset) {
            std::fill(&m_lightMask[0], &m_lightMask[width*height - 1], 0);
            std::fill(&m_alphaMap[0], &m_alphaMap[width*height*4 - 1], 0);
            // m_longMeanImage has been handled above already (set to the current image).
        }


        int r, g, b;
        int maxDiff, temp, sum;
        int min;
        int max;
        float f, y;
        uint32_t color;


        switch (m_mode) {
        case Graffiti_max:
            for (int pixel = 0; pixel < width*height; pixel++) {

                if (
                        (GETR(out[pixel]) == 0xFF
                         || GETG(out[pixel]) == 0xFF
                         || GETB(out[pixel]) == 0xFF)
                    ){
                    m_lightMask[pixel] |= out[pixel];
                }
                if (m_lightMask[pixel] != 0) {
                    out[pixel] = m_lightMask[pixel];
                }
            }
            break;
        case Graffiti_max_sum:
            for (int pixel = 0; pixel < width*height; pixel++) {

                if (
                        (GETR(out[pixel]) == 0xFF
                         || GETG(out[pixel]) == 0xFF
                         || GETB(out[pixel]) == 0xFF)
                        &&
                        (GETR(out[pixel])
                         + GETG(out[pixel])
                         + GETB(out[pixel])
                         > 0xFF + 0xCC + 0xCC)
                    ){
                    m_lightMask[pixel] |= out[pixel];
                }
                if (m_lightMask[pixel] != 0) {
                    out[pixel] = m_lightMask[pixel];
                }
            }
            break;

        case Graffiti_Y:
            for (int pixel = 0; pixel < width*height; pixel++) {
                if (
                           .299*GETR(out[pixel])/255.0
                         + .587 * GETG(out[pixel])/255.0
                         + .114 * GETB(out[pixel])/255.0
                         >= .85
                    ){
                    m_lightMask[pixel] |= out[pixel];
                }
                if (m_lightMask[pixel] != 0) {
                    out[pixel] = m_lightMask[pixel];
                }
            }
            break;

        case Graffiti_Max_Stat:
            for (int pixel = 0; pixel < width*height; pixel++) {
                if (GETR(out[pixel]) == 0xFF || GETG(out[pixel]) == 0xFF || GETB(out[pixel]) == 0xFF) {
                    out[pixel] = 0xFFFFFFFF;
                } else {
                    out[pixel] = 0;
                }
            }
            break;
        case Graffiti_Y_Stat:
            for (int pixel = 0; pixel < width*height; pixel++) {
                temp = .299*GETR(out[pixel]) + .587 * GETG(out[pixel]) + .114 * GETB(out[pixel]);
                temp = CLAMP(temp);
                out[pixel] = RGBA(temp, temp, temp, 0xFF);
            }
            break;
        case Graffiti_S_Stat:
            for (int pixel = 0; pixel < width*height; pixel++) {
                min = GETR(out[pixel]);
                max = GETR(out[pixel]);
                if (GETG(out[pixel]) < min) min = GETG(out[pixel]);
                if (GETG(out[pixel]) > max) max = GETG(out[pixel]);
                if (GETB(out[pixel]) < min) min = GETB(out[pixel]);
                if (GETB(out[pixel]) > max) max = GETB(out[pixel]);
                if (min == 0) { out[pixel] = 0; }
                else {
                    temp = 255.0*(max-min)/(float)max;
                    temp = CLAMP(temp);
                    out[pixel] = RGBA(temp, temp, temp, 0xFF);
                }
            }
            break;
        case Graffiti_STresh_Stat:
            for (int pixel = 0; pixel < width*height; pixel++) {
                min = GETR(out[pixel]);
                max = GETR(out[pixel]);
                if (GETG(out[pixel]) < min) min = GETG(out[pixel]);
                if (GETG(out[pixel]) > max) max = GETG(out[pixel]);
                if (GETB(out[pixel]) < min) min = GETB(out[pixel]);
                if (GETB(out[pixel]) > max) max = GETB(out[pixel]);
                if (min == 0 || max < 0x80) { out[pixel] = 0; }
                else {
                    temp = 255.0*((float)max-min)/max;
                    temp = CLAMP(temp);
                    out[pixel] = RGBA(temp, temp, temp, 0xFF);
                }
            }
            break;
        case Graffiti_SDiff_Stat:
            for (int pixel = 0; pixel < width*height; pixel++) {
                min = GETR(out[pixel]);
                max = GETR(out[pixel]);
                if (GETG(out[pixel]) < min) min = GETG(out[pixel]);
                if (GETG(out[pixel]) > max) max = GETG(out[pixel]);
                if (GETB(out[pixel]) < min) min = GETB(out[pixel]);
                if (GETB(out[pixel]) > max) max = GETB(out[pixel]);
                int sat;
                if (min == 0) { sat = 0; }
                else {
                    temp = 255.0*(max-min)/(float)max;
                    temp = CLAMP(temp);
                    sat = RGBA(temp, temp, temp, 0xFF);
                }
                min = m_longMeanImage[3*pixel+0];
                max = m_longMeanImage[3*pixel+0];
                if (m_longMeanImage[3*pixel+1] < min) min = m_longMeanImage[3*pixel+1];
                if (m_longMeanImage[3*pixel+1] > max) max = m_longMeanImage[3*pixel+1];
                if (m_longMeanImage[3*pixel+2] < min) min = m_longMeanImage[3*pixel+2];
                if (m_longMeanImage[3*pixel+2] > max) max = m_longMeanImage[3*pixel+2];
                if (min == 0) { out[pixel] = 0; }
                else {
                    temp = 255.0*(max-min)/(float)max;
                    temp = CLAMP(temp);
                    out[pixel] = RGBA(temp, temp, temp, 0xFF);
                }
                temp = 0x7f + GETR(out[pixel]) - GETR(sat);
                temp = CLAMP(temp);
                out[pixel] = RGBA(temp, temp, temp, 0xFF);
            }
            break;
        case Graffiti_SDiffTresh_Stat:
            for (int pixel = 0; pixel < width*height; pixel++) {
                min = GETR(out[pixel]);
                max = GETR(out[pixel]);
                if (GETG(out[pixel]) < min) min = GETG(out[pixel]);
                if (GETG(out[pixel]) > max) max = GETG(out[pixel]);
                if (GETB(out[pixel]) < min) min = GETB(out[pixel]);
                if (GETB(out[pixel]) > max) max = GETB(out[pixel]);
                int sat;
                if (min == 0) { sat = 0; }
                else {
                    temp = 255.0*(max-min)/(float)max;
                    temp = CLAMP(temp);
                    sat = RGBA(temp, temp, temp, 0xFF);
                }
                if (max < 0x80) {
                    out[pixel] = RGBA(0,0,0,0xFF);
                } else {
                    min = m_longMeanImage[3*pixel+0];
                    max = m_longMeanImage[3*pixel+0];
                    if (m_longMeanImage[3*pixel+1] < min) min = m_longMeanImage[3*pixel+1];
                    if (m_longMeanImage[3*pixel+1] > max) max = m_longMeanImage[3*pixel+1];
                    if (m_longMeanImage[3*pixel+2] < min) min = m_longMeanImage[3*pixel+2];
                    if (m_longMeanImage[3*pixel+2] > max) max = m_longMeanImage[3*pixel+2];
                    if (min == 0) { out[pixel] = 0; }
                    else {
                        temp = 255.0*(max-min)/(float)max;
                        temp = CLAMP(temp);
                        out[pixel] = RGBA(temp, temp, temp, 0xFF);
                    }
                    temp = 0x7f + GETR(out[pixel]) - GETR(sat);
                    temp = CLAMP(temp);
                    out[pixel] = RGBA(temp, temp, temp, 0xFF);
                }
            }
            break;
        case Graffiti_SSqrt_Stat:
            for (int pixel = 0; pixel < width*height; pixel++) {
                min = GETR(out[pixel]);
                max = GETR(out[pixel]);
                if (GETG(out[pixel]) < min) min = GETG(out[pixel]);
                if (GETG(out[pixel]) > max) max = GETG(out[pixel]);
                if (GETB(out[pixel]) < min) min = GETB(out[pixel]);
                if (GETB(out[pixel]) > max) max = GETB(out[pixel]);
                if (min == 0) { out[pixel] = 0; }
                else {
                    temp = 255.0*(max-min)/(float)max/(256.0-max);
                    temp = CLAMP(temp);
                    out[pixel] = RGBA(temp, temp, temp, 0xFF);
                }
            }
            break;
        case Graffiti_LongAvg_Stat:
            maxDiff = 0;
            temp = 0;
            for (int pixel = 0; pixel < width*height; pixel++) {
                r = 0x7f + (GETR(out[pixel]) - m_longMeanImage[3*pixel+0])/2;
                r = CLAMP(r);
                g = 0x7f + (GETG(out[pixel]) - m_longMeanImage[3*pixel+1])/2;
                g = CLAMP(g);
                b = 0x7f + (GETB(out[pixel]) - m_longMeanImage[3*pixel+2])/2;
                b = CLAMP(b);

                out[pixel] = RGBA(r,g,b,0xFF);
            }
            break;
        case Graffiti_LongAvg:
            for (int pixel = 0; pixel < width*height; pixel++) {

                r = 0x7f + (GETR(out[pixel]) - m_longMeanImage[3*pixel+0]);
                r = CLAMP(r);
                max = GETR(out[pixel]);
                maxDiff = r;
                temp = r;

                g = 0x7f + (GETG(out[pixel]) - m_longMeanImage[3*pixel+1]);
                g = CLAMP(g);
                if (maxDiff < g) maxDiff = g;
                if (max < GETG(out[pixel])) max = GETG(out[pixel]);
                temp += g;

                b = 0x7f + (GETB(out[pixel]) - m_longMeanImage[3*pixel+2]);
                b = CLAMP(b);
                if (maxDiff < b) maxDiff = b;
                if (max < GETB(out[pixel])) max = GETB(out[pixel]);
                temp += b;

                if (maxDiff > 0xe0 && temp > 0xe0 + 0xd0 + 0x80) {
                    m_lightMask[pixel] = MAX(m_lightMask[pixel], out[pixel]);

                    m_alphaMap[4*pixel+0] = 2*(GETR(out[pixel])-m_longMeanImage[3*pixel+0]);
                    m_alphaMap[4*pixel+0] = CLAMP(m_alphaMap[4*pixel+0])/255.0;

                    m_alphaMap[4*pixel+1] = 2*(GETG(out[pixel])-m_longMeanImage[3*pixel+1]);
                    m_alphaMap[4*pixel+1] = CLAMP(m_alphaMap[4*pixel+1])/255.0;

                    m_alphaMap[4*pixel+2] = 2*(GETB(out[pixel])-m_longMeanImage[3*pixel+2]);
                    m_alphaMap[4*pixel+2] = CLAMP(m_alphaMap[4*pixel+2])/255.0;

                    m_alphaMap[4*pixel+3] = 1;
                }

                if (m_lightMask[pixel] != 0) {
                    r = SCREEN1(GETR(out[pixel]), GETR(m_lightMask[pixel]));
                    g = SCREEN1(GETG(out[pixel]), GETG(m_lightMask[pixel]));
                    b = SCREEN1(GETB(out[pixel]), GETB(m_lightMask[pixel]));
                    r = CLAMP(r);
                    g = CLAMP(g);
                    b = CLAMP(b);
                    out[pixel] = RGBA(r,g,b,0xFF);
                }
            }
            break;
        case Graffiti_LongAvgAlpha_Stat:
            for (int pixel = 0; pixel < width*height; pixel++) {

                r = 0x7f + (GETR(out[pixel]) - m_longMeanImage[3*pixel+0]);
                r = CLAMP(r);
                max = GETR(out[pixel]);
                maxDiff = r;
                temp = r;

                g = 0x7f + (GETG(out[pixel]) - m_longMeanImage[3*pixel+1]);
                g = CLAMP(g);
                if (maxDiff < g) maxDiff = g;
                if (max < GETG(out[pixel])) max = GETG(out[pixel]);
                temp += g;

                b = 0x7f + (GETB(out[pixel]) - m_longMeanImage[3*pixel+2]);
                b = CLAMP(b);
                if (maxDiff < b) maxDiff = b;
                if (max < GETB(out[pixel])) max = GETB(out[pixel]);
                temp += b;

                if (maxDiff > 0xe0 && temp > 0xe0 + 0xd0 + 0x80) {
                    m_lightMask[pixel] = MAX(m_lightMask[pixel], out[pixel]);

                    f = 2*(GETR(out[pixel])-m_longMeanImage[3*pixel+0]);
                    f = CLAMP(f)/255.0;
                    if (f > m_alphaMap[4*pixel+0]) m_alphaMap[4*pixel+0] = f;

                    f = 2*(GETG(out[pixel])-m_longMeanImage[3*pixel+1]);
                    f = CLAMP(f)/255.0;
                    if (f > m_alphaMap[4*pixel+1]) m_alphaMap[4*pixel+1] = f;

                    f = 2*(GETB(out[pixel])-m_longMeanImage[3*pixel+2]);
                    f = CLAMP(f)/255.0;
                    if (f > m_alphaMap[4*pixel+2]) m_alphaMap[4*pixel+2] = f;

                    m_alphaMap[4*pixel+3] = 1;
                }
                r = 255.0*m_alphaMap[4*pixel+0];
                g = 255*m_alphaMap[4*pixel+1];
                b = 255*m_alphaMap[4*pixel+2];
                out[pixel] = RGBA(r,g,b,0xFF);
            }
            break;
        case Graffiti_LongAvgAlpha:
            for (int pixel = 0; pixel < width*height; pixel++) {

                r = 0x7f + (GETR(out[pixel]) - m_longMeanImage[3*pixel+0]);
                r = CLAMP(r);
                max = GETR(out[pixel]);
                maxDiff = r;
                temp = r;

                g = 0x7f + (GETG(out[pixel]) - m_longMeanImage[3*pixel+1]);
                g = CLAMP(g);
                if (maxDiff < g) maxDiff = g;
                if (max < GETG(out[pixel])) max = GETG(out[pixel]);
                temp += g;

                b = 0x7f + (GETB(out[pixel]) - m_longMeanImage[3*pixel+2]);
                b = CLAMP(b);
                if (maxDiff < b) maxDiff = b;
                if (max < GETB(out[pixel])) max = GETB(out[pixel]);
                temp += b;

                if (maxDiff > 0xe0 && temp > 0xe0 + 0xd0 + 0x80) {
                    m_lightMask[pixel] = MAX(m_lightMask[pixel], out[pixel]);

                    f = 2*(GETR(out[pixel])-m_longMeanImage[3*pixel+0]);
                    f = CLAMP(f)/255.0;
                    f *= f;
                    if (f > m_alphaMap[4*pixel+0]) m_alphaMap[4*pixel+0] = f;

                    f = 2*(GETG(out[pixel])-m_longMeanImage[3*pixel+1]);
                    f = CLAMP(f)/255.0;
                    f *= f;
                    if (f > m_alphaMap[4*pixel+1]) m_alphaMap[4*pixel+1] = f;

                    f = 2*(GETB(out[pixel])-m_longMeanImage[3*pixel+2]);
                    f = CLAMP(f)/255.0;
                    f *= f;
                    if (f > m_alphaMap[4*pixel+2]) m_alphaMap[4*pixel+2] = f;
                }
                if (m_lightMask[pixel] != 0) {
                    r = SCREEN1(GETR(out[pixel]), m_alphaMap[4*pixel+0]*GETR(m_lightMask[pixel]));
                    g = SCREEN1(GETG(out[pixel]), m_alphaMap[4*pixel+1]*GETG(m_lightMask[pixel]));
                    b = SCREEN1(GETB(out[pixel]), m_alphaMap[4*pixel+2]*GETB(m_lightMask[pixel]));
                    r = CLAMP(r);
                    g = CLAMP(g);
                    b = CLAMP(b);
                    out[pixel] = RGBA(r,g,b,0xFF);
                }
            }
            break;
        case Graffiti_LongAvgAlphaCumC:

            /**
              Ideas:
              * Remember Hue if Saturation > 0.1 (below: Close to white, so Hue might be wrong → remember Saturation as well)
              * Maximize Saturation for low alpha (opacity)
              * Make alpha depend on the light source's brightness
              * If alpha > 1: Simulate overexposure by going towards white
              * If pixel is bright in another frame: Sum up alpha values (longer exposure)
                Maybe: Logarithmic scale? → Overexposure becomes harder
                log(alpha/factor + 1) or sqrt(alpha/factor)
              */
            for (int pixel = 0; pixel < width*height; pixel++) {

                /*
                 Light detection
                 */

                // maxDiff: Maximum difference to the mean image
                //          {-255,...,255}
                // max:     Maximum pixel value
                //          {0,...,255}
                // temp:    Sum of all differences
                //          {-3*255,...,3*255}
                // sum:     Sum of all pixel values
                //          {0,...,3*255}

                r = GETR(out[pixel]) - m_longMeanImage[3*pixel+0];
                maxDiff = r;
                max = GETR(out[pixel]);
                temp = r;

                g = GETG(out[pixel]) - m_longMeanImage[3*pixel+1];
                if (max < GETG(out[pixel])) {
                    max = GETG(out[pixel]);
                }
                if (maxDiff < g) {
                    maxDiff = g;
                }
                temp += g;

                b = GETB(out[pixel]) - m_longMeanImage[3*pixel+2];
                if (max < GETB(out[pixel])) {
                    max = GETB(out[pixel]);
                }
                if (maxDiff < b) {
                    maxDiff = b;
                }
                temp += b;

                sum = GETR(out[pixel]) + GETG(out[pixel]) + GETB(out[pixel]);

                if (
                        maxDiff > m_pThresholdDifference    // I regard 0x50 as a meaningful value.
                        && temp > m_pThresholdDiffSum       // 3*0x45 sometimes okay for filtering reflections on bright patches
                        && sum > m_pThresholdBrightness     // A value of 450 seems to make sense here.
                    )
                {
                    // Store the «additional» light delivered by the light source in the light mask.
                    color = RGBA(CLAMP(r), CLAMP(g), CLAMP(b),0xFF);
                    m_lightMask[pixel] = MAX(m_lightMask[pixel], color);

                    // Add the brightness of the light source to the brightness map (alpha map)
                    y = REC709Y(CLAMP(r), CLAMP(g), CLAMP(b)) / 255.0;
                    y = y * m_pSensitivity;
                    m_alphaMap[4*pixel] += y;
                }


                /*
                 Background weight
                 */
                if (m_pBackgroundWeight > 0) {
                    // Use part of the background mean. This allows to have only lights appearing in the video
                    // if people or other objects walk into the video after the first frame (darker, therefore not in the light mask).
                    out[pixel] = RGBA((int) (m_pBackgroundWeight*m_longMeanImage[3*pixel+0] + (1-m_pBackgroundWeight)*GETR(out[pixel])),
                                      (int) (m_pBackgroundWeight*m_longMeanImage[3*pixel+1] + (1-m_pBackgroundWeight)*GETG(out[pixel])),
                                      (int) (m_pBackgroundWeight*m_longMeanImage[3*pixel+2] + (1-m_pBackgroundWeight)*GETB(out[pixel])),
                                      0xFF);
                }


                /*
                 Adding light mask
                 */
                if (
                        m_lightMask[pixel] != 0  && m_alphaMap[4*pixel + 0] != 0
                        && !m_pStatsBrightness && !m_pStatsDiff && !m_pStatsDiffSum
                    )
                {

                    f = sqrt(m_alphaMap[4*pixel]);

                    r = f * GETR(m_lightMask[pixel]);
                    g = f * GETG(m_lightMask[pixel]);
                    b = f * GETB(m_lightMask[pixel]);

                    if (f > 1) {
                        // Simulate overexposure
                        sum = 0;
                        if (r > 255) {
                            sum += r-255;
                        }
                        if (g > 255) {
                            sum += g-255;
                        }
                        if (b > 255) {
                            sum += g-255;
                        }

                        if (sum > 0) {
                            sum = sum/10.0;
                            r += sum;
                            g += sum;
                            b += sum;
                        }
                    } else if (f < 1) {
                        // Lower exposure: Stronger colors
                        y = REC709Y(r,g,b);
                        float sat = 2.0;

                        r = y + sat * (r-y);
                        g = y + sat * (g-y);
                        b = y + sat * (b-y);
                    }


                    // Add the light map as additional light to the image
                    r += GETR(out[pixel]);
                    g += GETG(out[pixel]);
                    b += GETB(out[pixel]);
                    r = CLAMP(r);
                    g = CLAMP(g);
                    b = CLAMP(b);
                    out[pixel] = RGBA(r,g,b,0xFF);
                } else if (m_pTransparentBackground) {
                    // Transparent background
                    out[pixel] &= RGBA(0xFF, 0xFF, 0xFF, 0);
                }


                /*
                 Statistics
                 */
                if (m_pStatsBrightness) {
                    // Show the image's brightness and highlight the threshold
                    // set by the user for detecting the right threshold easier

                    // Multiply with 0.8 for still being able to distinguish between white and threshold
                    r = .8*sum/3;
                    g = .8*sum/3;
                    b = .8*sum/3;
                    if (sum > m_pThresholdBrightness) {
                        b = 255;
                    }
                    out[pixel] = RGBA(r,g,b,0xFF);
                }

                if (m_pStatsDiff) {
                    // As above, but for the brightness difference relative to the background.
                    r = .8*CLAMP(maxDiff);
                    g = r;
                    if (!m_pStatsBrightness) {
                        b = r;
                    }

                    if (maxDiff > m_pThresholdDifference) {
                        g = 255;
                    }
                    out[pixel] = RGBA(r,g,b,0xFF);
                }

                if (m_pStatsDiffSum) {
                    // As above, for the sum of the differences in each color channel.
                    r = .8*CLAMP(temp/3.0);
                    if (!m_pStatsDiff) {
                        g = r;
                    }
                    if (!m_pStatsBrightness) {
                        b = r;
                    }
                    if (temp > m_pThresholdDiffSum) {
                        r = 255;
                    }
                    out[pixel] = RGBA(r,g,b,0xFF);
                }
            }
            break;
        }
    }

private:
    std::vector<uint32_t> m_lightMask;
    std::vector<float> m_longMeanImage;
    std::vector<float> m_alphaMap;
    bool m_meanInitialized;
    GraffitiMode m_mode;
    DimMode m_dimMode;

    f0r_param_double m_pLongAlpha;
    f0r_param_double m_pSensitivity;
    f0r_param_double m_pBackgroundWeight;
    f0r_param_double m_pThresholdBrightness;
    f0r_param_double m_pThresholdDifference;
    f0r_param_double m_pThresholdDiffSum;
    f0r_param_double m_pDim;
    f0r_param_bool m_pStatsBrightness;
    f0r_param_bool m_pStatsDiff;
    f0r_param_bool m_pStatsDiffSum;
    f0r_param_bool m_pTransparentBackground;
    f0r_param_bool m_pNonlinearDim;
    f0r_param_bool m_pReset;

};



frei0r::construct<LightGraffiti> plugin("Light Graffiti",
                "Creates light graffitis from a video by keeping the brightest spots.",
                "Simon A. Eugster (Granjow)",
                0,1,
                F0R_COLOR_MODEL_RGBA8888);
