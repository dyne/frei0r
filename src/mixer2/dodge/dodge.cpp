/* dodge.cpp
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

namespace {

struct DodgeLut {
  uint8_t table[256][256];
  DodgeLut() {
    for (uint32_t a = 0; a < 256; ++a) {
      const uint32_t num = a << 8;
      for (uint32_t b = 0; b < 256; ++b) {
        const uint32_t tmp = num / (256u - b);
        table[a][b] = static_cast<uint8_t>(MIN(tmp, 255u));
      }
    }
  }
};

static const DodgeLut kDodgeLut;

} // namespace

class dodge : public frei0r::mixer2
{
public:
  dodge(unsigned int width, unsigned int height)
  {
  }

  /**
   *
   * Perform an RGB[A] dodge operation between the pixel sources
   * in1 and in2, using the generalised algorithm:
   *
   * D = saturation of 255 or (A * 256) / (256 - B)
   *
   **/
  void update(double time,
              uint32_t* out,
              const uint32_t* in1,
              const uint32_t* in2)
  {
    const uint8_t* __restrict__ src1 = reinterpret_cast<const uint8_t*>(in1);
    const uint8_t* __restrict__ src2 = reinterpret_cast<const uint8_t*>(in2);
    uint8_t* __restrict__ dst = reinterpret_cast<uint8_t*>(out);
    const size_t total_bytes = static_cast<size_t>(size) * NBYTES;

    for (size_t i = 0; i < total_bytes; i += NBYTES)
      {
        dst[i + 0] = kDodgeLut.table[src1[i + 0]][src2[i + 0]];
        dst[i + 1] = kDodgeLut.table[src1[i + 1]][src2[i + 1]];
        dst[i + 2] = kDodgeLut.table[src1[i + 2]][src2[i + 2]];
        dst[i + ALPHA] = MIN(src1[i + ALPHA], src2[i + ALPHA]);
      }
  }
};

static frei0r::construct<dodge> plugin("dodge",
                                "Perform an RGB[A] dodge operation between the pixel sources, using the generalised algorithm: D = saturation of 255 or (A * 256) / (256 - B)",
                                "Jean-Sebastien Senecal",
                                0,3,
                                F0R_COLOR_MODEL_RGBA8888);
