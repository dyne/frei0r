/* equaliz0r.cpp
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
#include <string.h>

#include <cstring>

class equaliz0r : public frei0r::filter
{
  // Pre-shifted look-up tables for equaliz0r values.
  uint32_t rlut[256];
  uint32_t glut[256];
  uint32_t blut[256];
  
  // 4-way banked intensity histograms to eliminate store-to-load forwarding stalls.
  unsigned int rhist[4][256];
  unsigned int ghist[4][256];
  unsigned int bhist[4][256];

  void updateLookUpTables(const uint32_t* in)
  {
    const unsigned int total = width * height;
    
    // Reset histograms.
    memset(rhist, 0, sizeof(rhist));
    memset(ghist, 0, sizeof(ghist));
    memset(bhist, 0, sizeof(bhist));

    // First pass : build histograms (4-way banked, 4 pixels per iteration).
    unsigned int i = 0;
    for (; i + 3 < total; i += 4)
    {
      uint32_t p0 = in[i];
      uint32_t p1 = in[i + 1];
      uint32_t p2 = in[i + 2];
      uint32_t p3 = in[i + 3];

      rhist[0][p0 & 0xffu]++;
      ghist[0][(p0 >> 8) & 0xffu]++;
      bhist[0][(p0 >> 16) & 0xffu]++;

      rhist[1][p1 & 0xffu]++;
      ghist[1][(p1 >> 8) & 0xffu]++;
      bhist[1][(p1 >> 16) & 0xffu]++;

      rhist[2][p2 & 0xffu]++;
      ghist[2][(p2 >> 8) & 0xffu]++;
      bhist[2][(p2 >> 16) & 0xffu]++;

      rhist[3][p3 & 0xffu]++;
      ghist[3][(p3 >> 8) & 0xffu]++;
      bhist[3][(p3 >> 16) & 0xffu]++;
    }
    for (; i < total; ++i)
    {
      uint32_t p = in[i];
      rhist[0][p & 0xffu]++;
      ghist[0][(p >> 8) & 0xffu]++;
      bhist[0][(p >> 16) & 0xffu]++;
    }

    // Second pass : merge banks and build pre-shifted look-up tables.
    unsigned int rcum = 0, gcum = 0, bcum = 0;
    for (int k = 0; k < 256; ++k)
    {
      rcum += rhist[0][k] + rhist[1][k] + rhist[2][k] + rhist[3][k];
      gcum += ghist[0][k] + ghist[1][k] + ghist[2][k] + ghist[3][k];
      bcum += bhist[0][k] + bhist[1][k] + bhist[2][k] + bhist[3][k];

      rlut[k] = (uint32_t)CLAMP0255((rcum << 8) / total);
      glut[k] = (uint32_t)CLAMP0255((gcum << 8) / total) << 8;
      blut[k] = (uint32_t)CLAMP0255((bcum << 8) / total) << 16;
    }
  }
  
public:
  equaliz0r(unsigned int width, unsigned int height)
  {
    (void)width;
    (void)height;
  }
  
  virtual void update(double time,
                      uint32_t* out,
                      const uint32_t* in)
  {
    (void)time;
    updateLookUpTables(in);
    const unsigned int total = width * height;
    const uint32_t* lr = rlut;
    const uint32_t* lg = glut;
    const uint32_t* lb = blut;

    unsigned int i = 0;
    for (; i + 3 < total; i += 4)
    {
      uint32_t p0 = in[i];
      uint32_t p1 = in[i + 1];
      uint32_t p2 = in[i + 2];
      uint32_t p3 = in[i + 3];

      out[i]     = lr[p0 & 0xffu] | lg[(p0 >> 8) & 0xffu] | lb[(p0 >> 16) & 0xffu] | (p0 & 0xff000000u);
      out[i + 1] = lr[p1 & 0xffu] | lg[(p1 >> 8) & 0xffu] | lb[(p1 >> 16) & 0xffu] | (p1 & 0xff000000u);
      out[i + 2] = lr[p2 & 0xffu] | lg[(p2 >> 8) & 0xffu] | lb[(p2 >> 16) & 0xffu] | (p2 & 0xff000000u);
      out[i + 3] = lr[p3 & 0xffu] | lg[(p3 >> 8) & 0xffu] | lb[(p3 >> 16) & 0xffu] | (p3 & 0xff000000u);
    }
    for (; i < total; ++i)
    {
      uint32_t p = in[i];
      out[i] = lr[p & 0xffu] | lg[(p >> 8) & 0xffu] | lb[(p >> 16) & 0xffu] | (p & 0xff000000u);
    }
  }
};


static frei0r::construct<equaliz0r> plugin("Equaliz0r",
                                    "Equalizes the intensity histograms",
                                    "Jean-Sebastien Senecal (Drone)",
                                    0,2,
                                    F0R_COLOR_MODEL_RGBA8888);
