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

#include <cstdio>
#include <climits>
#include <algorithm>

#define GETA(abgr) (((abgr) >> (3*CHAR_BIT)) & 0xFF)
#define GETB(abgr) (((abgr) >> (2*CHAR_BIT)) & 0xFF)
#define GETG(abgr) (((abgr) >> (1*CHAR_BIT)) & 0xFF)
#define GETR(abgr) (((abgr) >> (0*CHAR_BIT)) & 0xFF)
#define RGBA(r,g,b,a) ( ((r) << (0*CHAR_BIT)) | ((g) << (1*CHAR_BIT)) | ((b) << (2*CHAR_BIT)) | ((a) << (3*CHAR_BIT)) )
#define MOVING_AVG(mean, in, alpha) (((uint32_t) (((mean) & 0xFF) * (1 - (alpha)) + ((in) & 0xFF ) * (alpha))) \
        | (((uint32_t) ((((mean) >> CHAR_BIT) & 0xFF) * (1 - (alpha)) + (((in) >> CHAR_BIT) & 0xFF ) * (alpha))) << CHAR_BIT) \
        | (((uint32_t) ((((mean) >> 2*CHAR_BIT) & 0xFF) * (1 - (alpha)) + (((in) >> 2*CHAR_BIT) & 0xFF ) * (alpha))) << 2*CHAR_BIT) \
        | (((uint32_t) ((((mean) >> 3*CHAR_BIT) & 0xFF) * (1 - (alpha)) + (((in) >> 3*CHAR_BIT) & 0xFF ) * (alpha))) << 3*CHAR_BIT))
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

#define SCREEN1(mask,img) ((uint8_t) (255-(255.0-(mask))*(255.0-(img))/255.0))

class LightGraffiti : public frei0r::filter
{

public:

    f0r_param_double m_diffToMean;
    f0r_param_double m_pLongAlpha;

    LightGraffiti(unsigned int width, unsigned int height) :
            m_lightMask(width*height, 0),
            m_alphaMap(4*width*height, 1.0),
            m_alpha(1/8.0),
            m_longAlpha(1/128.0),
            m_meanInitialized(false)

    {
        m_mode = Graffiti_LongAvgAlpha2;

        register_param(m_pLongAlpha, "longAlpha", "Alpha value for moving average");
        m_pLongAlpha = 1/128.0;
        updateParams();
    }

    ~LightGraffiti()
    {
    }

    enum GraffitiMode { Graffiti_max, Graffiti_max_sum, Graffiti_Y, Graffiti_Avg, Graffiti_Avg2,
                        Graffiti_Avg_Stat, Graffiti_AvgTresh_Stat, Graffiti_Max_Stat, Graffiti_Y_Stat, Graffiti_S_Stat,
                        Graffiti_STresh_Stat, Graffiti_SDiff_Stat, Graffiti_SDiffTresh_Stat,
                        Graffiti_SSqrt_Stat,
                        Graffiti_LongAvg, Graffiti_LongAvg_Stat, Graffiti_LongAvgAlpha, Graffiti_LongAvgAlpha2, Graffiti_LongAvgAlpha_Stat };

    void updateParams()
    {
        m_longAlpha = m_pLongAlpha;
//        std::cout << "Alpha: " << m_pLongAlpha << "\n";
    }

    virtual void update()
    {
        updateParams();

        std::copy(in, in + width*height, out);

        if (!m_meanInitialized) {
            m_meanImage = std::vector<uint32_t>(&in[0], &in[width*height-1]);
            m_longMeanImage = std::vector<float>(width*height*3);
            for (int pixel = 0; pixel < width*height; pixel++) {
                m_longMeanImage[3*pixel+0] = GETR(in[pixel]);
                m_longMeanImage[3*pixel+1] = GETG(in[pixel]);
                m_longMeanImage[3*pixel+2] = GETB(in[pixel]);
            }
            m_meanInitialized = true;
        } else {
            for (int pixel = 0; pixel < width*height; pixel++) {
                m_meanImage[pixel] = MOVING_AVG(m_meanImage[pixel], in[pixel], m_alpha);
                m_longMeanImage[3*pixel+0] = (1-m_longAlpha) * m_longMeanImage[3*pixel+0] + m_longAlpha * GETR(in[pixel]);
                m_longMeanImage[3*pixel+1] = (1-m_longAlpha) * m_longMeanImage[3*pixel+1] + m_longAlpha * GETG(in[pixel]);
                m_longMeanImage[3*pixel+2] = (1-m_longAlpha) * m_longMeanImage[3*pixel+2] + m_longAlpha * GETB(in[pixel]);
            }
        }


        int r, g, b;
        int maxDiff, temp;
        int min;
        int max;
        float f;


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

        case Graffiti_Avg:
            for (int pixel = 0; pixel < width*height; pixel++) {
                if ((GETR(out[pixel]) > 0xCC
                     || GETG(out[pixel]) > 0xCC
                     || GETB(out[pixel]) > 0xCC)
                    &&
                    GETR(out[pixel])
                    + GETG(out[pixel])
                    + GETB(out[pixel])
                    > 3*0xa0) {
                    if ((int)GETR(out[pixel]-GETR(m_meanImage[pixel]))
                        + GETG(out[pixel]-GETG(m_meanImage[pixel]))
                        + GETB(out[pixel]-GETB(m_meanImage[pixel])) > 0xa0) {
                        m_lightMask[pixel] = MAX(m_lightMask[pixel], out[pixel]);

                        m_alphaMap[4*pixel+0] = GETR(m_lightMask[pixel])/255.0;
                        m_alphaMap[4*pixel+1] = GETG(m_lightMask[pixel])/255.0;
                        m_alphaMap[4*pixel+2] = GETB(m_lightMask[pixel])/255.0;
                        m_alphaMap[4*pixel+3] = GETA(m_lightMask[pixel])/255.0;

                        m_alphaMap[4*pixel+0] *= m_alphaMap[4*pixel+0];
                        m_alphaMap[4*pixel+1] *= m_alphaMap[4*pixel+1];
                        m_alphaMap[4*pixel+2] *= m_alphaMap[4*pixel+2];
                        m_alphaMap[4*pixel+3] *= m_alphaMap[4*pixel+3];

                    }
                }

                if (m_lightMask[pixel] != 0) {
                    out[pixel] = ((uint32_t) (m_alphaMap[4*pixel+0]*GETR(m_lightMask[pixel]) + (1-m_alphaMap[4*pixel+0])*GETR(out[pixel]))) << (0*CHAR_BIT)
                                 | ((uint32_t) (m_alphaMap[4*pixel+1]*GETR(m_lightMask[pixel]) + (1-m_alphaMap[4*pixel+1])*GETR(out[pixel]))) << (1*CHAR_BIT)
                                 | ((uint32_t) (m_alphaMap[4*pixel+2]*GETR(m_lightMask[pixel]) + (1-m_alphaMap[4*pixel+2])*GETR(out[pixel]))) << (2*CHAR_BIT)
                                 | ((uint32_t) (m_alphaMap[4*pixel+3]*GETR(m_lightMask[pixel]) + (1-m_alphaMap[4*pixel+3])*GETR(out[pixel]))) << (3*CHAR_BIT);
                }
            }
            break;

        case Graffiti_Avg2:
            maxDiff = 0;
            temp = 0;
            for (int pixel = 0; pixel < width*height; pixel++) {
                temp = GETR(out[pixel]) - GETR(m_meanImage[pixel]);
                if (temp > maxDiff) maxDiff = temp;
                temp = GETG(out[pixel]) - GETG(m_meanImage[pixel]);
                if (temp > maxDiff) maxDiff = temp;
                temp = GETB(out[pixel]) - GETB(m_meanImage[pixel]);
                if (temp > maxDiff) maxDiff = temp;

                if (maxDiff > 0x50) {
                    m_lightMask[pixel] = MAX(m_lightMask[pixel], out[pixel]);

                    m_alphaMap[4*pixel+0] = GETR(m_lightMask[pixel])/255.0;
                    m_alphaMap[4*pixel+1] = GETG(m_lightMask[pixel])/255.0;
                    m_alphaMap[4*pixel+2] = GETB(m_lightMask[pixel])/255.0;
                    m_alphaMap[4*pixel+3] = GETA(m_lightMask[pixel])/255.0;

                    m_alphaMap[4*pixel+0] *= m_alphaMap[4*pixel+0];
                    m_alphaMap[4*pixel+1] *= m_alphaMap[4*pixel+1];
                    m_alphaMap[4*pixel+2] *= m_alphaMap[4*pixel+2];
                    m_alphaMap[4*pixel+3] *= m_alphaMap[4*pixel+3];
                }

                if (m_lightMask[pixel] != 0) {
                    out[pixel] = ((uint32_t) (m_alphaMap[4*pixel+0]*GETR(m_lightMask[pixel]) + (1-m_alphaMap[4*pixel+0])*GETR(out[pixel]))) << (0*CHAR_BIT)
                                 | ((uint32_t) (m_alphaMap[4*pixel+1]*GETR(m_lightMask[pixel]) + (1-m_alphaMap[4*pixel+1])*GETR(out[pixel]))) << (1*CHAR_BIT)
                                 | ((uint32_t) (m_alphaMap[4*pixel+2]*GETR(m_lightMask[pixel]) + (1-m_alphaMap[4*pixel+2])*GETR(out[pixel]))) << (2*CHAR_BIT)
                                 | ((uint32_t) (m_alphaMap[4*pixel+3]*GETR(m_lightMask[pixel]) + (1-m_alphaMap[4*pixel+3])*GETR(out[pixel]))) << (3*CHAR_BIT);
                }
            }
            break;

        case Graffiti_Avg_Stat:
            maxDiff = 0;
            temp = 0;
            for (int pixel = 0; pixel < width*height; pixel++) {
                r = 0x7f + (GETR(out[pixel]) - GETR(m_meanImage[pixel]));
                r = CLAMP(r);
                g = 0x7f + (GETG(out[pixel]) - GETG(m_meanImage[pixel]));
                g = CLAMP(g);
                b = 0x7f + (GETB(out[pixel]) - GETB(m_meanImage[pixel]));
                b = CLAMP(b);

                out[pixel] = RGBA(r,g,b,0xFF);
            }
            break;

        case Graffiti_AvgTresh_Stat:
            maxDiff = 0;
            temp = 0;
            for (int pixel = 0; pixel < width*height; pixel++) {
                r = 0x7f + (GETR(out[pixel]) - GETR(m_meanImage[pixel]));
                max = GETR(out[pixel]);
                r = CLAMP(r);
                g = 0x7f + (GETG(out[pixel]) - GETG(m_meanImage[pixel]));
                if (max < GETG(out[pixel])) max = GETG(out[pixel]);
                g = CLAMP(g);
                b = 0x7f + (GETB(out[pixel]) - GETB(m_meanImage[pixel]));
                if (max < GETB(out[pixel])) max = GETB(out[pixel]);
                b = CLAMP(b);

                if (max < 0x80) {
                    r /= 2;
                    g /= 2;
                    b /= 2;
                }
                out[pixel] = RGBA(r,g,b,0xFF);
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
                min = GETR(m_meanImage[pixel]);
                max = GETR(m_meanImage[pixel]);
                if (GETG(m_meanImage[pixel]) < min) min = GETG(m_meanImage[pixel]);
                if (GETG(m_meanImage[pixel]) > max) max = GETG(m_meanImage[pixel]);
                if (GETB(m_meanImage[pixel]) < min) min = GETB(m_meanImage[pixel]);
                if (GETB(m_meanImage[pixel]) > max) max = GETB(m_meanImage[pixel]);
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
                    min = GETR(m_meanImage[pixel]);
                    max = GETR(m_meanImage[pixel]);
                    if (GETG(m_meanImage[pixel]) < min) min = GETG(m_meanImage[pixel]);
                    if (GETG(m_meanImage[pixel]) > max) max = GETG(m_meanImage[pixel]);
                    if (GETB(m_meanImage[pixel]) < min) min = GETB(m_meanImage[pixel]);
                    if (GETB(m_meanImage[pixel]) > max) max = GETB(m_meanImage[pixel]);
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
                r = 0x7f + (GETR(out[pixel]) - m_longMeanImage[3*pixel+0]);
                r = CLAMP(r);
                g = 0x7f + (GETG(out[pixel]) - m_longMeanImage[3*pixel+1]);
                g = CLAMP(g);
                b = 0x7f + (GETB(out[pixel]) - m_longMeanImage[3*pixel+2]);
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
//                    out[pixel] = ((uint32_t) (m_alphaMap[4*pixel+0]*GETR(m_lightMask[pixel]) + (1-m_alphaMap[4*pixel+0])*GETR(out[pixel]))) << (0*CHAR_BIT)
//                                 | ((uint32_t) (m_alphaMap[4*pixel+1]*GETR(m_lightMask[pixel]) + (1-m_alphaMap[4*pixel+1])*GETR(out[pixel]))) << (1*CHAR_BIT)
//                                 | ((uint32_t) (m_alphaMap[4*pixel+2]*GETR(m_lightMask[pixel]) + (1-m_alphaMap[4*pixel+2])*GETR(out[pixel]))) << (2*CHAR_BIT)
//                                 | ((uint32_t) (m_alphaMap[4*pixel+3]*GETR(m_lightMask[pixel]) + (1-m_alphaMap[4*pixel+3])*GETR(out[pixel]))) << (3*CHAR_BIT);
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
        case Graffiti_LongAvgAlpha2:
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

                    f = GETR(out[pixel]) + GETG(out[pixel]) + GETB(out[pixel]);
                    f = 16*(f/255.0/3 - (1-1/16.0));
                    if (f < 0) f = 0;
                    f *= f;
                    if (f > m_alphaMap[4*pixel+0]) {
                        m_alphaMap[4*pixel+0] = f;
                        m_alphaMap[4*pixel+1] = f;
                        m_alphaMap[4*pixel+2] = f;
                    }
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
        }
    }

private:
    std::vector<uint32_t> m_lightMask;
    std::vector<uint32_t> m_meanImage;
    std::vector<float> m_longMeanImage;
    std::vector<float> m_alphaMap;
    bool m_meanInitialized;
    float m_alpha;
    float m_longAlpha;
    GraffitiMode m_mode;

};



frei0r::construct<LightGraffiti> plugin("Light Graffiti",
                "Creates light graffitis from a video by keeping the brightest spots.",
                "Simon A. Eugster (Granjow)",
                0,1,
                F0R_COLOR_MODEL_RGBA8888);
