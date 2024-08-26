// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 Erik H. Beck, bacon@tahomasoft.com
// https://www.gnu.org/licenses/old-licenses/gpl-2.0-standalone.html

/* This program is free software; you can redistribute it and/or modify
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA
 *
 * Also see:
 *
 * https://www.gnu.org/licenses/old-licenses/gpl-2.0-standalone.html
 *
 */

/** This is the source file for the euclid eraser two-input mixer for
    frei0r. It uses euclidean distance to remove a static background
    from a video. See file ./euclid_eraser.md for more info
*/

#include <math.h>
#include "frei0r.hpp"
#include "frei0r/math.h"

#define NBYTES 4 
#define CHANNELS 3 // Actually 4; 0-3

double euclidDistance(uint8_t x_r, uint8_t x_g, uint8_t x_b,
		      uint8_t y_r, uint8_t y_g, uint8_t y_b)
   {
   //calculating color channel differences for next steps
   double red_d = x_r - y_r;
   double green_d = x_g - y_g;
   double blue_d = x_b - y_b; 
  
   double sq_sum, dist;

   //calculating Euclidean distance
   sq_sum = pow(red_d, 2) + pow(green_d, 2) + pow (blue_d, 2);
   dist = sqrt(sq_sum);                  
  
   return dist;
   }


class euclid_eraser : public frei0r::mixer2
{
  
public:
  euclid_eraser(unsigned int width, unsigned int height)
  {
    threshold = 5.6;      // Default distance threshold value
    register_param(threshold, "threshold", "Matching Threshold");
  }
  
  void update(double time,
              uint32_t* out,
              const uint32_t* in1,
              const uint32_t* in2)
  {
       
    const uint8_t *src1 = reinterpret_cast<const uint8_t*>(in1); //frst track (0)
    const uint8_t *src2 = reinterpret_cast<const uint8_t*>(in2); //second trk (1)
    uint8_t *dst = reinterpret_cast<uint8_t*>(out);
        
    double e_dist;

    uint32_t sizeCounter = size;
    uint32_t b;
 
    while (sizeCounter--)
      {

	// Loop over rgb
	// Copy pixels from src2 to destination
	  
	for (b=0; b<3; b++)
	  {
	    dst[b]=src2[b];
	  }
	e_dist=euclidDistance(src1[0],src1[1],src1[2],
			      src2[0],src2[1],src2[2]);
	
	if (e_dist <=  euclid_eraser::threshold) {
	    // Make alpha channel for pixel fully transparent
	    dst[3]=0;
	  }
	else {
	  // Make alpha channel for the pixel fully opaque
	  dst[3]=255;
	}
	
	src1 += NBYTES;
	src2 += NBYTES;
	dst  += NBYTES;
      }
    
  }
private:
  double threshold;

};

frei0r::construct<euclid_eraser> plugin("euclid_eraser",
	"Erasing backgrounds with euclidian distance",
        "Erik H. Beck",
        0,1,
        F0R_COLOR_MODEL_RGBA8888);

