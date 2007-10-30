/*
This frei0r plugin generates solid color images

Copyright (C) 2004, 2005 Martin Bayer <martin@gephex.org>

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

class onecol0r : public frei0r::source
{
public:
  onecol0r(unsigned int width, unsigned int height)
  {
    register_param(color,"Color","the color of the image");
  }
  
  virtual void update()
  {
    unsigned int col;
    unsigned char* c = reinterpret_cast<unsigned char*>(&col);

    c[0]=static_cast<unsigned char>(color.b*255);
    c[1]=static_cast<unsigned char>(color.g*255);
    c[2]=static_cast<unsigned char>(color.r*255);
    c[3]=255;
    
    std::fill(out, out+width*height, col);
  }
  
private:
  f0r_param_color color;
};


frei0r::construct<onecol0r> plugin("onecol0r",
				   "image with just one color",
				   "Martin Bayer",
				   0,1);

