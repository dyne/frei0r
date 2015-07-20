/* value.cpp
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
#include "frei0r_colorspace.h"

#define NBYTES 4

class value : public frei0r::mixer2
{
public:
  value(unsigned int width, unsigned int height)
  {
  }

  /**
   *
   * Perform a conversion to value only of the source in1 using
   * the value of in2.
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
    int r1, g1, b1;
    int r2, g2, b2;
  
      /*  assumes inputs are only 4 byte RGBA pixels  */
      /*  assumes inputs are only 4 byte RGBA pixels  */
      while (sizeCounter--)
        {
          r1 = src1[0];
          g1 = src1[1];
          b1 = src1[2];
          r2 = src2[0];
          g2 = src2[1];
          b2 = src2[2];
          rgb_to_hsv_int(&r1, &g1, &b1);
          rgb_to_hsv_int(&r2, &g2, &b2);
  
          b1 = b2;
  
          /*  set the dstination  */
          hsv_to_rgb_int(&r1, &g1, &b1);
  
          dst[0] = r1;
          dst[1] = g1;
          dst[2] = b1;
  
          dst[3] = MIN(src1[3], src2[3]);
  
          src1 += NBYTES;
          src2 += NBYTES;
          dst += NBYTES;
        }
  }  
  
    
};


frei0r::construct<value> plugin("value",
                                "Perform a conversion to value only of the source input1 using the value of input2.",
                                "Jean-Sebastien Senecal",
                                0,2,
                                F0R_COLOR_MODEL_RGBA8888);

