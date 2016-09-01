/* lighten.cpp
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
#include "frei0r_math.h"

#define NBYTES 4
#define ALPHA 3

class lighten : public frei0r::mixer2
{
public:
  lighten(unsigned int width, unsigned int height)
  {
  }

  /**
   *
   * Perform a lighten operation between sources in1 and in2, using the
   * generalised algorithm:
   * D_r = max(A_r, B_r);
   * D_g = max(A_g, B_g);
   * D_b = max(A_b, B_b);
   * D_a = min(A_a, B_a);
   *
   **/
  void update(double time,
              uint32_t* out,
              const uint32_t* in1,
              const uint32_t* in2)
  {
    const uint8_t *src1 = reinterpret_cast<const uint8_t*>(in1);
    const uint8_t *src2 = reinterpret_cast<const uint8_t*>(in2);
    uint8_t *dst = reinterpret_cast<uint8_t*>(out);
    uint32_t sizeCounter = size;
            
    uint32_t b;
    uint8_t s1, s2;
  
    while (sizeCounter--)
      {
        for (b = 0; b < ALPHA; b++)
          {
            s1 = src1[b];
            s2 = src2[b];
            dst[b] = MAX(s1,s2);
          }
  
        dst[ALPHA] = MIN(src1[ALPHA], src2[ALPHA]);
  
        src1 += NBYTES;
        src2 += NBYTES;
        dst += NBYTES;
      }
  }
  
  
    
};


frei0r::construct<lighten> plugin("lighten",
                                  "Perform a lighten operation between two sources (maximum value of both sources).",
                                  "Jean-Sebastien Senecal",
                                  0,2,
                                  F0R_COLOR_MODEL_RGBA8888);

