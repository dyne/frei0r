/* hardlight.cpp
 * Copyright (C) 2006 Jean-Sebastien Senecal (js@drone.ws)
 * This file is a Frei0r plugin.
 * The code is a modified version of code from the Gimp.
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
#include "frei0r/math.h"

#define NBYTES 4
#define ALPHA 3

class hardlight : public frei0r::mixer2
{
public:
  hardlight(unsigned int width, unsigned int height)
  {
  }

  /**
   *
   * Perform an RGB[A] hardlight operation between the pixel sources
   * in1 and in2.
   *
   **/
  void update(double time,
              uint32_t* out,
              const uint32_t* in1,
              const uint32_t* in2)
  {
    const uint8_t *__restrict__ src1 = reinterpret_cast<const uint8_t*>(in1);
    const uint8_t *__restrict__ src2 = reinterpret_cast<const uint8_t*>(in2);
    uint8_t *__restrict__ dst = reinterpret_cast<uint8_t*>(out);
    uint32_t i = 0;
    for (; i + 16 <= size; i += 16)
      {
        const uint8_t *__restrict__ s1 = src1 + i * 4;
        const uint8_t *__restrict__ s2 = src2 + i * 4;
        uint8_t *__restrict__ d = dst + i * 4;
        for (int k = 0; k < 64; ++k)
          {
            uint16_t a = s1[k];
            uint16_t b = s2[k];
            /* For b > 128: (255 - a) * (255 - 2*(b-128)) <= 255*253 = 64515 (fits uint16_t)
               For b <= 128: if b == 128 result is a; if b < 128, a * (2*b) <= 255*254 = 64770 (fits uint16_t) */
            uint16_t b_hi = (b > 128u) ? (uint16_t)(255u - ((b - 128u) << 1)) : 0u;
            uint16_t hi = (uint16_t)(255u - (((uint16_t)(255u - a) * b_hi) >> 8));
            uint16_t b_lo = (b < 128u) ? (uint16_t)(b << 1) : 0u;
            uint16_t lo = (b == 128u) ? a : (uint16_t)((a * b_lo) >> 8);
            d[k] = (uint8_t)((b > 128u) ? hi : lo);
          }
        for (int p = 0; p < 16; ++p)
          d[p * 4 + 3] = s1[p * 4 + 3] < s2[p * 4 + 3] ? s1[p * 4 + 3] : s2[p * 4 + 3];
      }
    for (; i < size; ++i)
      {
        const uint8_t *s1 = src1 + i * 4;
        const uint8_t *s2 = src2 + i * 4;
        uint8_t *d = dst + i * 4;
        for (uint32_t b = 0; b < ALPHA; b++)
          {
            uint32_t tmp;
            if (s2[b] > 128)
              {
                tmp = ((int) 255 - s1[b]) * ((int) 255 - ((s2[b] - 128) << 1));
                d[b] = (uint8_t) MAX255 (255 - (tmp >> 8));
              }
            else
              {
                tmp = (int) s1[b] * ((int) s2[b] << 1);
                d[b] = (uint8_t) MAX255 (tmp >> 8);
              }
          }
        d[ALPHA] = MIN(s1[ALPHA], s2[ALPHA]);
      }
  }
  
  
    
};


static frei0r::construct<hardlight> plugin("hardlight",
                                    "Perform an RGB[A] hardlight operation between the pixel sources",
                                    "Jean-Sebastien Senecal",
                                    0,2,
                                    F0R_COLOR_MODEL_RGBA8888);
