/*
This frei0r plugin generates white noise images

Copyright (C) 2004, 2005 Martin Bayer <martin@gephex.org>
Copyright (C) 2005       Georg Seidel <georg@gephex.org>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "frei0r.hpp"

#include <algorithm>
#include <cmath>


#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class lissajous0r: public frei0r::source
{
public:
  lissajous0r(unsigned int width, unsigned int height)
  {
    r_x = r_y = 0.0;
    register_param(r_x,"ratiox","x-ratio");
    register_param(r_y,"ratioy","y-ratio");
  }

  
  virtual void update(double time,
                      uint32_t* out,
		              const uint32_t* in,
		              const uint32_t* in2,
		              const uint32_t* in3)
  {
    std::fill(out, out+width*height, 0x00000000);

    double rx=1.0/(0.999999-r_x);
    double ry=1.0/(0.999999-r_y);
    
    double w = 0.5*(width-1);
    double h = 0.5*(height-1);
    
    const unsigned int samples = 15*(width+height);

    double deltax = (rx*2*M_PI) / (double) samples;
    double deltay = (ry*2*M_PI) / (double) samples;
    double tx = 0;
    double ty = 0;
    for (unsigned int i=samples; i != 0; --i, tx+=deltax, ty+=deltay)
      {
	unsigned int x = static_cast<unsigned int>(w*(1.0+sin(tx)));
 	unsigned int y = static_cast<unsigned int>(h*(1.0+cos(ty)));

	out[width*y + x]=0xffffffff;	
      }
  }
private:
  double r_x;
  double r_y;
};


frei0r::construct<lissajous0r> plugin("Lissajous0r",
				   "Generates Lissajous0r images",
				   "Martin Bayer",
				   0,3);

