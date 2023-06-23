/* sobel.cpp
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
#include <stdlib.h>

class sobel : public frei0r::filter
{
public:
  sobel(unsigned int width, unsigned int height)
  {
  }

  virtual void update(double time,
                      uint32_t *out,
                      const uint32_t *in)
  {
    std::copy(in, in + width * height, out);
    for (unsigned int y = 1; y < height - 1; ++y)
    {
      for (unsigned int x = 1; x < width - 1; ++x)
      {
        // XXX faut eviter le * width en y allant par rangee
        unsigned char *p1 = (unsigned char *)&in[(y - 1) * width + (x - 1)];
        unsigned char *p2 = (unsigned char *)&in[(y - 1) * width + x];
        unsigned char *p3 = (unsigned char *)&in[(y - 1) * width + (x + 1)];
        unsigned char *p4 = (unsigned char *)&in[y * width + (x - 1)];
        unsigned char *p6 = (unsigned char *)&in[y * width + (x + 1)];
        unsigned char *p7 = (unsigned char *)&in[(y + 1) * width + (x - 1)];
        unsigned char *p8 = (unsigned char *)&in[(y + 1) * width + x];
        unsigned char *p9 = (unsigned char *)&in[(y + 1) * width + (x + 1)];

        unsigned char *g = (unsigned char *)&out[y * width + x];

        for (int i = 0; i < 3; ++i)
        {
          g[i] = CLAMP0255(
            abs(p1[i] + p2[i] * 2 + p3[i] - p7[i] - p8[i] * 2 - p9[i]) +
            abs(p3[i] + p6[i] * 2 + p9[i] - p1[i] - p4[i] * 2 - p7[i]));
        }

        g[3] = ((unsigned char *)&in[y * width + x])[3]; // copy alpha
      }
    }
  }
};


frei0r::construct <sobel> plugin("Sobel",
                                 "Sobel filter",
                                 "Jean-Sebastien Senecal (Drone)",
                                 0, 2,
                                 F0R_COLOR_MODEL_RGBA8888);
