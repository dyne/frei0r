/* divide.cpp
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

class divide : public frei0r::mixer2
{
public:
  uint32_t m_inv[256];

  divide(unsigned int width, unsigned int height)
  {
    for (uint32_t s2 = 0; s2 < 256; ++s2)
      m_inv[s2] = ((1u << 24) + s2) / (1u + s2);
  }

  /**
   *
   * Perform an RGB[A] divide operation between the pixel sources in1
   * and in2.  in1 is the numerator, in2 the denominator.
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
    const uint32_t *__restrict__ inv = m_inv;

    for (uint32_t i = 0; i < size; ++i)
      {
        uint32_t r0 = ((uint32_t)src1[0] * inv[src2[0]]) >> 16;
        uint32_t r1 = ((uint32_t)src1[1] * inv[src2[1]]) >> 16;
        uint32_t r2 = ((uint32_t)src1[2] * inv[src2[2]]) >> 16;
        dst[0] = (uint8_t)(r0 > 255u ? 255u : r0);
        dst[1] = (uint8_t)(r1 > 255u ? 255u : r1);
        dst[2] = (uint8_t)(r2 > 255u ? 255u : r2);
        dst[3] = src1[3] < src2[3] ? src1[3] : src2[3];
        src1 += NBYTES;
        src2 += NBYTES;
        dst += NBYTES;
      }
  }
  
  
    
};


static frei0r::construct<divide> plugin("divide",
                                 "Perform an RGB[A] divide operation between the pixel sources: input1 is the numerator, input2 the denominator",
                                 "Jean-Sebastien Senecal",
                                 0,2,
                                 F0R_COLOR_MODEL_RGBA8888);
