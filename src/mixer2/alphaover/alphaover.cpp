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
#include "frei0r/math.h"

#include <algorithm>

class alphaover : public frei0r::mixer2
{
public:
  uint32_t m_inv[256];

  alphaover(unsigned int width, unsigned int height)
  {
    m_inv[0] = 0;
    for (uint32_t d = 1; d < 256; ++d)
      m_inv[d] = ((1u << 26) + d - 1u) / d;
  }

  void update(double time,
              uint32_t* out,
              const uint32_t* in1,
              const uint32_t* in2)
  {
    uint8_t *__restrict__ dst = reinterpret_cast<uint8_t*>(out);
    const uint8_t *__restrict__ src1 = reinterpret_cast<const uint8_t*>(in1);
    const uint8_t *__restrict__ src2 = reinterpret_cast<const uint8_t*>(in2);
    const uint32_t *__restrict__ inv = m_inv;

    for (unsigned int i = 0; i < size; ++i)
    {
      uint8_t alpha_src1 = src1[3];
      if (alpha_src1 == 255)
      {
        out[i] = in1[i];
        src1 += 4;
        src2 += 4;
        dst += 4;
        continue;
      }

      uint32_t tmp1, tmp2;
      uint8_t alpha_src2 = src2[3];
      uint8_t w2 = 0xffu ^ alpha_src1; // w2 = 255 - alpha_1

      // compute destination alpha
      uint8_t alpha_dst = dst[3] = INT_MULT(alpha_src1, alpha_src1, tmp1) + INT_MULT3(alpha_src2, alpha_src2, w2, tmp2);

      // compute destination values
      if (alpha_dst == 0)
      {
        dst[0] = 0;
        dst[1] = 0;
        dst[2] = 0;
      }
      else
      {
        uint64_t inv_d = inv[alpha_dst];
        uint32_t n0 = (uint32_t)src1[0] * alpha_src1 + INT_MULT(src2[0], alpha_src2, tmp1) * w2;
        uint32_t n1 = (uint32_t)src1[1] * alpha_src1 + INT_MULT(src2[1], alpha_src2, tmp1) * w2;
        uint32_t n2 = (uint32_t)src1[2] * alpha_src1 + INT_MULT(src2[2], alpha_src2, tmp1) * w2;
        dst[0] = CLAMP0255((int32_t)((n0 * inv_d) >> 26));
        dst[1] = CLAMP0255((int32_t)((n1 * inv_d) >> 26));
        dst[2] = CLAMP0255((int32_t)((n2 * inv_d) >> 26));
      }

      src1 += 4;
      src2 += 4;
      dst += 4;
    }
  }
  
};


static frei0r::construct<alphaover> plugin("alphaover",
                                    "the alpha OVER operation",
                                    "Jean-Sebastien Senecal",
                                    0,2,
                                    F0R_COLOR_MODEL_RGBA8888);
