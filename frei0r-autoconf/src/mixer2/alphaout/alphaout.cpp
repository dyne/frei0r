/* alphaout.cpp
 * Copyright (C) 2005 Jean-Sebastien Senecal (js@drone.ws)
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

#include "frei0r.hpp"
#include "frei0r_math.h"

#include <algorithm>

class alphaout : public frei0r::mixer2
{
public:
  alphaout(unsigned int width, unsigned int height)
  {
  }

  void update()
  {
    uint8_t *dst = reinterpret_cast<uint8_t*>(out);
    const uint8_t *src1 = reinterpret_cast<const uint8_t*>(in1);
    const uint8_t *src2 = reinterpret_cast<const uint8_t*>(in2);
    
    for (int i=0; i<size; ++i)
    {
      uint32_t tmp;
      uint8_t alpha_src1 = src1[3];
      uint8_t alpha_dst;
      uint8_t w1 = 0xff ^ src2[3];  // w2 = 255 - alpha_2

      // compute destination alpha
      alpha_dst = dst[3] = INT_MULT(alpha_src1, w1, tmp);

       // compute destination values
      if (alpha_dst == 0)
        for (int b=0; b<3; ++b)
          dst[b] = 0;
      else
        for (int b=0; b<3; ++b)
          dst[b] = CLAMP0255( (uint32_t)( (uint32_t) (INT_MULT(src1[b], alpha_src1, tmp) * w1) / alpha_dst) );
      
      src1 += 4;
      src2 += 4;
      dst += 4;
    }
  }
  
};


frei0r::construct<alphaout> plugin("alphaout",
                                    "the alpha OUT operation",
                                    "Jean-Sebastien Senecal",
                                    0,1,
                                    F0R_COLOR_MODEL_RGBA8888);

