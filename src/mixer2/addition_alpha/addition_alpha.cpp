/* addition_alpha.cpp
 * Copyright (C) 2006 Jean-Sebastien Senecal (js@drone.ws)
 *               2011 Simon A. Eugster (simon.eu@gmail.com)
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

class addition_alpha : public frei0r::mixer2
{
public:
  addition_alpha(unsigned int width, unsigned int height)
  {
    // initialize look-up table
    for (int i = 0; i < 256; i++)
      add_lut[i] = i;
  	for (int i = 256; i <= 510; i++)
      add_lut[i] = 255;
  }

  /**
   *
   * Perform an RGB[A] addition_alpha operation of the pixel sources in1
   * and in2.
   *
   * This is a modification of the original addition filter, adding the color of the
   * second channel only if its alpha channel is not 0.
   *
   **/
  void update(double time,
              uint32_t* out,
              const uint32_t* in1,
              const uint32_t* in2)
  {
    const uint8_t *A = reinterpret_cast<const uint8_t*>(in1);
    const uint8_t *B = reinterpret_cast<const uint8_t*>(in2);
    uint8_t *D = reinterpret_cast<uint8_t*>(out);
    uint32_t sizeCounter = size;
            
    uint32_t b;
  
    while (sizeCounter--)
      {
        for (b = 0; b < ALPHA; b++)
          D[b] = add_lut[A[b] + ((B[b]*B[ALPHA])>>8)];
        
        D[ALPHA] = 255;
        A += NBYTES;
        B += NBYTES;
        D += NBYTES;
      }
  }
  
private:
  static uint8_t add_lut[511]; // look-up table storing values to do a quick MAX of two values when you know you add two unsigned chars
};

uint8_t addition_alpha::add_lut[511];

frei0r::construct<addition_alpha> plugin("addition_alpha",
                                  "Perform an RGB[A] addition_alpha operation of the pixel sources.",
                                  "Jean-Sebastien Senecal",
                                  0,2,
                                  F0R_COLOR_MODEL_RGBA8888);

