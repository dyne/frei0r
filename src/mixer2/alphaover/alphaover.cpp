/* alphaover.cpp
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

class alphaover : public frei0r::mixer2
{
public:
  alphaover(unsigned int width, unsigned int height)
  {
  }

  void update(double time,
              uint32_t* out,
              const uint32_t* in1,
              const uint32_t* in2)
  {
    uint8_t *dst = reinterpret_cast<uint8_t*>(out);
    const uint8_t *src1 = reinterpret_cast<const uint8_t*>(in1);
    const uint8_t *src2 = reinterpret_cast<const uint8_t*>(in2);
    
    for (unsigned int i=0; i<size; ++i)
    {
      uint32_t tmp1, tmp2;
      uint8_t alpha_src1 = src1[3];
      uint8_t alpha_src2 = src2[3];
      uint8_t alpha_dst;
      uint8_t w2 = 0xff ^ alpha_src1; // w2 = 255 - alpha_1

      // compute destination alpha
      alpha_dst = dst[3] = INT_MULT(alpha_src1, alpha_src1, tmp1) + INT_MULT3(alpha_src2, alpha_src2, w2, tmp2);

       // compute destination values
      if (alpha_dst == 0)
        for (int b=0; b<3; ++b)
          dst[b] = 0;
      else
        for (int b=0; b<3; ++b)
          dst[b] = CLAMP0255( (uint32_t)( (uint32_t) (src1[b] * alpha_src1 + INT_MULT(src2[b], alpha_src2, tmp1) * w2) / alpha_dst) );
      
      src1 += 4;
      src2 += 4;
      dst += 4;
    }
  }
  
};


frei0r::construct<alphaover> plugin("alphaover",
                                    "the alpha OVER operation",
                                    "Jean-Sebastien Senecal",
                                    0,2,
                                    F0R_COLOR_MODEL_RGBA8888);

