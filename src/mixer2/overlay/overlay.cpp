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
#include "frei0r_math.h"

#define NBYTES 4
#define ALPHA 3

class overlay : public frei0r::mixer2
{
public:
  overlay(unsigned int width, unsigned int height)
  {
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
              const uint32_t* in2,
              const uint32_t* in3)
  {
    const uint8_t *src1 = reinterpret_cast<const uint8_t*>(in1);
    const uint8_t *src2 = reinterpret_cast<const uint8_t*>(in2);
    uint8_t *dst = reinterpret_cast<uint8_t*>(out);
    uint32_t sizeCounter = size;
            
    uint32_t b, tmp, tmpM;
  
    while (sizeCounter--)
      {
        for (b = 0; b < ALPHA; b++)
          {
            dst[b] = INT_MULT(src1[b], src1[b] + INT_MULT(2 * src2[b], 255 - src1[b], tmpM), tmp);
          }
  
        dst[ALPHA] = MIN(src1[ALPHA], src2[ALPHA]);
  
        src1 += NBYTES;
        src2 += NBYTES;
        dst += NBYTES;
      }
  }
  
  
    
};


frei0r::construct<overlay> plugin("overlay",
                                  "Perform an RGB[A] overlay operation between the pixel sources, using the generalised algorithm:\n"
								  "D =  A * (B + (2 * B) * (255 - A))",
                                  "Jean-Sebastien Senecal",
                                  0,2,
                                  F0R_COLOR_MODEL_RGBA8888);

