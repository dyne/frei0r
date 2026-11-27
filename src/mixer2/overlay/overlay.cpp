/* overlay.cpp
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

class overlay : public frei0r::mixer2
{
public:
  overlay(unsigned int width, unsigned int height)
  {
    this->width = width;
    this->height = height;
    this->size = width * height;
  }

  /**
   *
   * Perform an RGB[A] overlay operation between the pixel sources
   * in1 and in2, using the generalised algorithm:
   *
   * D =  A * (B + (2 * B) * (255 - A))
   *
   **/
  void update(double time,
              uint32_t* out,
              const uint32_t* in1,
              const uint32_t* in2)
  {
    // Validate inputs
    if (!out || !in1 || !in2) {
      return;
    }

    const uint8_t *__restrict__ src1 = reinterpret_cast<const uint8_t*>(in1);
    const uint8_t *__restrict__ src2 = reinterpret_cast<const uint8_t*>(in2);
    uint8_t *__restrict__ dst = reinterpret_cast<uint8_t*>(out);
    uint32_t n = size;
    if (n == 0) {
      return;
    }

    uint32_t i = 0;
    for (; i + 16 <= n; i += 16)
      {
        const uint8_t *__restrict__ s1 = src1 + i * 4;
        const uint8_t *__restrict__ s2 = src2 + i * 4;
        uint8_t *__restrict__ d = dst + i * 4;
        for (int k = 0; k < 64; ++k)
          {
            uint32_t a = s1[k];
            uint32_t b = s2[k];
            uint32_t tm = (2u * b) * (255u - a) + 0x80u;
            uint32_t m = ((tm >> 8) + tm) >> 8;
            uint32_t t = a * (a + m) + 0x80u;
            d[k] = (uint8_t)(((t >> 8) + t) >> 8);
          }
        for (int p = 0; p < 16; ++p)
          d[p * 4 + 3] = s1[p * 4 + 3] < s2[p * 4 + 3] ? s1[p * 4 + 3] : s2[p * 4 + 3];
      }
    for (; i < n; ++i)
      {
        const uint8_t *s1 = src1 + i * 4;
        const uint8_t *s2 = src2 + i * 4;
        uint8_t *d = dst + i * 4;
        uint32_t tmp, tmpM;
        for (uint32_t b = 0; b < ALPHA; b++)
          d[b] = INT_MULT(s1[b], s1[b] + INT_MULT(2 * s2[b], 255 - s1[b], tmpM), tmp);
        d[ALPHA] = MIN(s1[ALPHA], s2[ALPHA]);
      }
  }
  
  
    
};


static frei0r::construct<overlay> plugin("overlay",
                                  "Perform an RGB[A] overlay operation between the pixel sources, using the generalised algorithm: D =  A * (B + (2 * B) * (255 - A))",
                                  "Jean-Sebastien Senecal",
                                  0,2,
                                  F0R_COLOR_MODEL_RGBA8888);
