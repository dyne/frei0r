/* blend.cpp
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

#define NBYTES    4
#define ALPHA     3

class blend : public frei0r::mixer2
{
public:
  blend(unsigned int width, unsigned int height)
  {
    blend_factor = 0.5;
    register_param(blend_factor, "blend", "blend factor");
  }

  /**
   *
   * Perform a blend operation between sources in1 and in2, using
   * the generalised algorithm: D = A * (255 - &beta;) + B * &beta;
   *
   * The result is left in out
   **/
  void update(double time,
              uint32_t *out,
              const uint32_t *in1,
              const uint32_t *in2)
  {
    const uint8_t *src1         = reinterpret_cast <const uint8_t *>(in1);
    const uint8_t *src2         = reinterpret_cast <const uint8_t *>(in2);
    uint8_t *      dst          = reinterpret_cast <uint8_t *>(out);
    const uint8_t  bf           = (const uint8_t)(255 * blend_factor);
    const uint8_t  one_minus_bf = (255 - bf);
    uint32_t       w            = size;
    uint32_t       b;

    while (w--)
    {
      for (b = 0; b < NBYTES; b++)
      {
        dst[b] = (src1[b] * one_minus_bf + src2[b] * bf) / 255;
      }

      src1 += NBYTES;
      src2 += NBYTES;
      dst  += NBYTES;
    }
  }

private:
  double blend_factor;
};


frei0r::construct <blend> plugin("blend",
                                 "Perform a blend operation between two sources",
                                 "Jean-Sebastien Senecal",
                                 0, 2,
                                 F0R_COLOR_MODEL_RGBA8888);
