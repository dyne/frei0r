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

#include <cstring>

/* Clamps a int32-range int between 0 and 255 inclusive. */
unsigned char CLAMP0255(int32_t a)
{
  return (unsigned char)
    ( (((-a) >> 31) & a)  // 0 if the number was negative
      | (255 - a) >> 31); // -1 if the number was greater than 255
}

class equaliz0r : public frei0r::filter
{
  // Look-up tables for equaliz0r values.
  unsigned char rlut[256];
  unsigned char glut[256];
  unsigned char blut[256];
  
  // Intensity histograms.
  unsigned int rhist[256];
  unsigned int ghist[256];
  unsigned int bhist[256];

  void updateLookUpTables()
  {
    unsigned int size = width*height;
    
    // First pass : build histograms.
    
    // Reset histograms.
    memset(rhist, 0, 256*sizeof(unsigned int));
    memset(ghist, 0, 256*sizeof(unsigned int));
    memset(bhist, 0, 256*sizeof(unsigned int));

    // Update histograms.
    const unsigned char *in_ptr = (const unsigned char*) in;
    for (unsigned int i=0; i<size; ++i)
    {
      // update 'em
      rhist[*in_ptr++]++;
      ghist[*in_ptr++]++;
      bhist[*in_ptr++]++;
      in_ptr++; // skip alpha
    }

    // Second pass : update look-up tables.
    in_ptr = (const unsigned char*) in;

    // Cumulative intensities of histograms.
    unsigned int
      rcum = 0,
      gcum = 0,
      bcum = 0;
      
    for (int i=0; i<256; ++i)
    {
      // update cumulatives
      rcum += rhist[i];
      gcum += ghist[i];
      bcum += bhist[i];
      
      // update 'em
      rlut[i] = CLAMP0255( (rcum << 8) / size ); // = 256 * rcum / size
      glut[i] = CLAMP0255( (gcum << 8) / size ); // = 256 * gcum / size
      blut[i] = CLAMP0255( (bcum << 8) / size ); // = 256 * bcum / size
      
      in_ptr++; // skip alpha
    }

  }
  
public:
  equaliz0r(unsigned int width, unsigned int height)
  {
  }
  
  virtual void update()
  {
    std::copy(in, in + width*height, out);
    updateLookUpTables();
    unsigned int size = width*height;
    const unsigned char *in_ptr = (const unsigned char*) in;
    unsigned char *out_ptr = (unsigned char*) out;
    for (unsigned int i=0; i<size; ++i)
    {
      *out_ptr++ = rlut[*in_ptr++];
      *out_ptr++ = glut[*in_ptr++];
      *out_ptr++ = blut[*in_ptr++];
      *out_ptr++ = *in_ptr++; // copy alpha
    }
  }
};


frei0r::construct<equaliz0r> plugin("Equaliz0r",
                                    "Equalizes the intensity histograms",
                                    "Jean-Sebastien Senecal (Drone)",
                                    0,1,
                                    F0R_COLOR_MODEL_RGBA8888);

