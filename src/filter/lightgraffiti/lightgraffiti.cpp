/*
 * Copyright (C) 2010-2011 Simon Andreas Eugster (simon.eu@gmail.com)
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
            LIGHT GRAFFITI / LIGHT PAINTING / LUMASOL


  This effect is intended to simulate what happens when you use a shutter speed of
  e.g. 10 seconds for your camera and paint with light, like lamps, in the air
  -- just with video. It tries to remember bright spots and keeps them in a mask.
  Areas that are not very bright (i.e. background) will not sum up.

  Originally I saw this effect in some Ford Kuga commercials on YouTube when a friend
  shew me those. One of them was [1]. No information about how this effect works is given
  -- only that a guy from the Dutch PIPS:LAB[2] was involved. The technique seems to be
  slightly different though; whileas this frei0r effect works in post, the original Lumasol
  effect is said to work directly in-camera.

  Since the technique is fascinating I started writing this Open-Source effect.[3] The general
  concept is:
  1. Extract the light by using thresholding (absolute brightness and brightness change relative to
     the background image fetched from the first frame)
  2. Store the color in a light mask, and the estimated density in an alpha map (increased every time
     that a light source hits a pixel to simulate overexposure)
  3. Paint the light mask over the video image
  4. Dim the alpha map, and update the background image (moving average), if desired.
  5. Repeat for the next frame.

  The second approach (LG_ADV) is based on the observation that colour mixing does not work well
  with the above one that stores colour values and changes the brightness via an alpha map. Therefore
  the new approach directly sums up colour values detected in the light source and does not use an
  alpha map.
  * Transitions are not very smooth out-of-the-box. This is solved by multiplying the light source's
    RGB value by (r+g+b)/3 (after normalizing them to [0,1]); Darker lights will then be even darker
    and the transition to the background looks smoother.
  * Lights may look a little faint regarding color. Therefore the saturation can be increased by a custom
    factor. Saturation depends on the brightness of the light map; the darker the light is, the more
    the saturation is increased. This will, within a sensible range, make the lights look more vital.
  Dimming works by scaling each color values individually.

  If you write your own Light Graffiti effect (e.g. for After Effects) I'd very much appreciate
  to hear about it!

  -- Simon (Granjow)

  [1] http://www.youtube.com/watch?v=WVaxuIKPKvU
  [2] http://www.pipslab.org/bio/keez-duyves/
  [3] http://kdenlive.org/users/granjow/writing-light-graffiti-effect

  */
#include "frei0r.hpp"

#include <cmath>
#include <cstdio>
#include <climits>
#include <algorithm>

#define LG_ADV
//#define LG_NO_OVERLAY // Not really working yet
//#define LG_DEBUG

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

  struct RGBFloat {
                      float r;
                      float g;
                      float b;
                  };

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

#ifdef LG_ADV
        RGBFloat rgb0;
        rgb0.r = 0;
        rgb0.g = 0;
        rgb0.b = 0;
        m_rgbLightMask = std::vector<RGBFloat>(width*height, rgb0);

#ifdef LG_DEBUG
        for (int i = 0; i < width*height; i++) {
            if (m_rgbLightMask[i].r != 0 || m_rgbLightMask[i].g != 0 || m_rgbLightMask[i].b != 0) {
                std::cout << "ERROR: " << m_rgbLightMask[i].r;
            }
        }
#endif
#endif

#ifdef LG_NO_OVERLAY
        m_prevMask = std::vector<RGBFloat>(width*height, rgb0);
#endif

        register_param(m_pSensitivity, "sensitivity", "Sensitivity of the effect for light (higher sensitivity will lead to brighter lights)");
        register_param(m_pBackgroundWeight, "backgroundWeight", "Describes how strong the (accumulated) background should shine through");
        register_param(m_pThresholdBrightness, "thresholdBrightness", "Brightness threshold to distinguish between foreground and background");
        register_param(m_pThresholdDifference, "thresholdDifference", "Threshold: Difference to background to distinguish between fore- and background");
        register_param(m_pThresholdDiffSum, "thresholdDiffSum", "Threshold for sum of differences. Can in most cases be ignored (set to 0).");
        register_param(m_pDim, "dim", "Dimming of the light mask");
        register_param(m_pSaturation, "saturation", "Saturation of lights");
        register_param(m_pLowerOverexposure, "lowerOverexposure", "Prevents some overexposure if the light source stays steady too long (varying speed)");
        register_param(m_pStatsBrightness, "statsBrightness", "Display the brightness and threshold, for adjusting the brightness threshold parameter");
        register_param(m_pStatsDiff, "statsDifference", "Display the background difference and threshold");
        register_param(m_pStatsDiffSum, "statsDiffSum", "Display the sum of the background difference and the threshold");
        register_param(m_pReset, "reset", "Reset filter masks");
        register_param(m_pTransparentBackground, "transparentBackground", "Make the background transparent");
        register_param(m_pBlackReference, "blackReference", "Uses black as background image instead of the first frame.");
        register_param(m_pLongAlpha, "longAlpha", "Alpha value for moving average");
        register_param(m_pNonlinearDim, "nonlinearDim", "Nonlinear dimming (may look more natural)");
        m_pLongAlpha = 1/128.0;
        m_pSensitivity = 1 / 5.;
        m_pBackgroundWeight = 0;
        m_pThresholdBrightness = 450 / 765.;
        m_pThresholdDifference = 0;
        m_pThresholdDiffSum = 0;
        m_pDim = 0;
        m_pSaturation = 1 / 4.;
        m_pLowerOverexposure = 0;
        m_pStatsBrightness = false;
        m_pStatsDiff = false;
        m_pStatsDiffSum = false;
        m_pReset = false;
        m_pTransparentBackground = false;
        m_pBlackReference = false;
        m_pLongAlpha = 0;
        m_pNonlinearDim = 0;

    }

    ~LightGraffiti()
    {
        // I was told that a std::vector does not need to be deleted --
        // therefore nothing to do here!
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
        double sensitivity = m_pSensitivity * 5;
        double thresholdBrightness = m_pThresholdBrightness * 765;
        double thresholdDifference = m_pThresholdDifference * 255;
        double thresholdDiffSum = m_pThresholdDiffSum * 765;
        double saturation = m_pSaturation * 4;
        double lowerOverexposure = m_pLowerOverexposure * 10;

        // Copy everything to the output image.
        // Most of the image will very likely not change at all.
        std::copy(in, in + width*height, out);

#ifdef LG_ADV
        RGBFloat rgb0;
        rgb0.r = 0;
        rgb0.g = 0;
        rgb0.b = 0;
#endif

        if (m_pNonlinearDim) {
            m_dimMode = Dim_Sin;
        } else {
            m_dimMode = Dim_Mult;
        }


        /*
         Refresh the background image
         */
        if (!m_meanInitialized || m_pReset) {
            if (m_pBlackReference) {
                // Do not use the first frame from the movie as background image but plain black
                // to calculate the added light. Useful e.g. when dealing with still images.
                m_longMeanImage = std::vector<float>(width*height*3, 0);
            } else {
                m_longMeanImage = std::vector<float>(width*height*3);
                for (int pixel = 0; pixel < width*height; pixel++) {
                    m_longMeanImage[3*pixel+0] = GETR(in[pixel]);
                    m_longMeanImage[3*pixel+1] = GETG(in[pixel]);
                    m_longMeanImage[3*pixel+2] = GETB(in[pixel]);
                }
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
#ifdef LG_ADV
                    for (int i = 0; i < m_rgbLightMask.size(); i++) {
                        m_rgbLightMask[i].r *= factor;
                        m_rgbLightMask[i].g *= factor;
                        m_rgbLightMask[i].b *= factor;
                    }
#else
                    for (int i = 0; i < width*height; i++) {
                        m_alphaMap[4*i + 0] *= factor;
                        m_alphaMap[4*i + 1] *= factor;
                        m_alphaMap[4*i + 2] *= factor;
                        m_alphaMap[4*i + 3] *= factor;
                    }
#endif
                    break;


                case Dim_Sin:
#ifdef LG_ADV
                    for (int i = 0; i < m_rgbLightMask.size(); i++) {
                        // Red
                        if (m_rgbLightMask[i].r < 1) {
                            m_rgbLightMask[i].r *= pow(sin(m_rgbLightMask[i].r * M_PI/2), m_pDim) - .01;
                        } else {
                            m_rgbLightMask[i].r *= factor;
                        }
                        if (m_rgbLightMask[i].r < 0) { m_rgbLightMask[i].r = 0; }

                        // Green
                        if (m_rgbLightMask[i].g < 1) {
                            m_rgbLightMask[i].g *= pow(sin(m_rgbLightMask[i].g * M_PI/2), m_pDim) - .01;
                        } else {
                            m_rgbLightMask[i].g *= factor;
                        }
                        if (m_rgbLightMask[i].g < 0) { m_rgbLightMask[i].g = 0; }

                        // Blue
                        if (m_rgbLightMask[i].b < 1) {
                            m_rgbLightMask[i].b *= pow(sin(m_rgbLightMask[i].b * M_PI/2), m_pDim) - .01;
                        } else {
                            m_rgbLightMask[i].b *= factor;
                        }
                        if (m_rgbLightMask[i].b < 0) { m_rgbLightMask[i].b = 0; }
                    }
#else
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
#endif
                    break;
            }

        }



        /*
         Reset all masks if desired
         (mainly for parameter adjustments when working in the NLE)
         */
        if (m_pReset) {
#ifdef LG_ADV
            m_rgbLightMask = std::vector<RGBFloat>(width*height, rgb0);
#else
            std::fill(&m_lightMask[0], &m_lightMask[width*height - 1], 0);
            std::fill(&m_alphaMap[0], &m_alphaMap[width*height*4 - 1], 0);
#endif
            // m_longMeanImage has been handled above already (set to the current image).
        }


        int r, g, b;
        int maxDiff, temp, sum;
        int min;
        int max;
        float f, y;
        uint32_t color;
        float fr, fg, fb, sr, sg, sb, fy, fsat;

#ifdef LG_DEBUG
        int deCount = 0;
#endif


        switch (m_mode) {
            /*
             Lots of testing modes here!
             */
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
                  Ideas (partially considered) to get a realistic look:
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
                            maxDiff > thresholdDifference
                            && temp > thresholdDiffSum
                            && sum > thresholdBrightness
                            // If all requirements are met, then this should be a light source.
                        )
                    {
#ifdef LG_ADV
                        // Just add values as float. Overflows are highly unlikely (3.4E38+ frames ...).
                        fr = CLAMP(r)/255.0;
                        fg = CLAMP(g)/255.0;
                        fb = CLAMP(b)/255.0;

                        f = (fr + fg + fb) / 3 * sensitivity;
                        fr *= f;
                        fg *= f;
                        fb *= f;

#ifdef LG_NO_OVERLAY
//                        std::cout << "fr: " << fr << "; fg: " << fg << "; fb: " << fb << "\n";
                        fr -= m_prevMask[pixel].r;
                        fg -= m_prevMask[pixel].g;
                        fb -= m_prevMask[pixel].b;
                        m_prevMask[pixel].r += fr;
                        m_prevMask[pixel].g += fg;
                        m_prevMask[pixel].b += fb;
//                        std::cout << "fr2: " << fr << "; fg2: " << fg << "; fb2: " << fb << "\n";
                        if (fr < 0) { fr = 0; }
                        if (fg < 0) { fg = 0; }
                        if (fb < 0) { fb = 0; }
#endif

                        m_rgbLightMask[pixel].r += fr;
                        m_rgbLightMask[pixel].g += fg;
                        m_rgbLightMask[pixel].b += fb;

#else
                        // Store the «additional» light delivered by the light source in the light mask.
                        color = RGBA(CLAMP(r), CLAMP(g), CLAMP(b),0xFF);
                        m_lightMask[pixel] = MAX(m_lightMask[pixel], color);

                        // Add the brightness of the light source to the brightness map (alpha map)
                        y = REC709Y(CLAMP(r), CLAMP(g), CLAMP(b)) / 255.0;
                        y = y * sensitivity;
                        m_alphaMap[4*pixel] += y;
#endif
                    } else {
#ifdef LG_NO_OVERLAY
                        m_prevMask[pixel] = rgb0;
#endif
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
#ifdef LG_ADV
                    if (
                            (m_rgbLightMask[pixel].r != 0 || m_rgbLightMask[pixel].g != 0 || m_rgbLightMask[pixel].b != 0)
                            && !m_pStatsBrightness && !m_pStatsDiff && !m_pStatsDiffSum
                       )
                    {

                        fr = m_rgbLightMask[pixel].r;
                        fg = m_rgbLightMask[pixel].g;
                        fb = m_rgbLightMask[pixel].b;

                        if (lowerOverexposure > 0) {
                            // Comparisation of plots with octave:
                            // clf;hold on;plot([0 1],[0 1],'k');plot(range,ones(length(range),1),'k');plot(range,sqrt(range));plot(range,log(1+range),'k');plot(range,log(1+range),'g');plot(range,(log(1+range)/3).^.5,'r');axis equal
                            fr = pow( log(1+fr)/lowerOverexposure, .5 );
                            fg = pow( log(1+fg)/lowerOverexposure, .5 );
                            fb = pow( log(1+fb)/lowerOverexposure, .5 );
                        }


                        // Calculate overflow between different colours:
                        // A very bright red light source will eventually overflow into other channels.
                        sr = 0;
                        sg = 0;
                        sb = 0;
                        if (fr > 1) {
                            sr += fr - 1;
                        }
                        if (fg > 1) {
                            sg += fg - 1;
                        }
                        if (fb > 1) {
                            sb += fb - 1;
                        }
                        fr += (sg + sb)/2;
                        fg += (sr + sb)/2;
                        fb += (sg + sb)/2;
                        if (fr > 1) {
                            fr = 1;
                        }
                        if (fg > 1) {
                            fg = 1;
                        }
                        if (fb > 1) {
                            fb = 1;
                        }

                        // Increase the saturation if the average brightness is below a certain level
                        // Do not use Rec709 Luma since we want to consider all colours to equal parts.
                        fy = (fr + fg + fb) / 3;
                        if (fy < 1 && saturation > 0) {
                            fsat = 1 + saturation*(1-fy);

                            fr = fy + fsat * (fr-fy);
                            fg = fy + fsat * (fg-fy);
                            fb = fy + fsat * (fb-fy);
                        }

                        // Paint the light on top of the image using addition
                        // Since brightness is equidistant in sRGB, this works fine.
                        r = 255*fr + GETR(out[pixel]);
                        g = 255*fg + GETG(out[pixel]);
                        b = 255*fb + GETB(out[pixel]);
                        r = CLAMP(r);
                        g = CLAMP(g);
                        b = CLAMP(b);
                        out[pixel] = RGBA(r,g,b,0xFF);

#ifdef LG_DEBUG
                        deCount++;
                        if (deCount < 10) {
                            std::cout << "r: " << m_rgbLightMask[pixel].r << ", fy: " << fy << ", fr: " << fr << ", sr: " << sr << ", R: " << r << ", inR: " << GETR(in[pixel]) << "\n";
                        }
#endif

                    } else if (m_pTransparentBackground) {
                        // Transparent background
                        out[pixel] &= RGBA(0xFF, 0xFF, 0xFF, 0);
                    }
#else
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
#endif


                    /*
                     In-video statistics for easier parameter adjustment (thresholds)
                     */
                    if (m_pStatsBrightness) {
                        // Show the image's brightness and highlight the threshold set by the user

                        // Limit maximum brightness to 80% for still being able to distinguish
                        // between «bright spot» (light grey) and «over the threshold» (blue)
                        r = .8*sum/3;
                        g = .8*sum/3;
                        b = .8*sum/3;
                        if (sum > thresholdBrightness) {
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

                        if (maxDiff > thresholdDifference) {
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
                        if (temp > thresholdDiffSum) {
                            r = 255;
                        }
                        out[pixel] = RGBA(r,g,b,0xFF);
                    }
                }
                break;
            default:
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

#ifdef LG_ADV
    std::vector<RGBFloat> m_rgbLightMask;
#endif
#ifdef LG_NO_OVERLAY
    std::vector<RGBFloat> m_prevMask;
#endif

    f0r_param_double m_pLongAlpha;
    f0r_param_double m_pSensitivity;
    f0r_param_double m_pBackgroundWeight;
    f0r_param_double m_pThresholdBrightness;
    f0r_param_double m_pThresholdDifference;
    f0r_param_double m_pThresholdDiffSum;
    f0r_param_double m_pDim;
    f0r_param_double m_pSaturation;
    f0r_param_double m_pLowerOverexposure;
    f0r_param_bool m_pStatsBrightness;
    f0r_param_bool m_pStatsDiff;
    f0r_param_bool m_pStatsDiffSum;
    f0r_param_bool m_pTransparentBackground;
    f0r_param_bool m_pBlackReference;
    f0r_param_bool m_pNonlinearDim;
    f0r_param_bool m_pReset;

};



frei0r::construct<LightGraffiti> plugin("Light Graffiti",
                "Creates light graffitis from a video by keeping the brightest spots.",
                "Simon A. Eugster (Granjow)",
                0,2,
                F0R_COLOR_MODEL_RGBA8888);
