/* 
 * Threelay0r
 * 2009 Hedde Bosman
 *
 * This source code  is free software; you can  redistribute it and/or
 * modify it under the terms of the GNU Public License as published by
 * the Free Software  Foundation; either version 3 of  the License, or
 * (at your option) any later version.
 *
 * This source code is distributed in the hope that it will be useful,
 * but  WITHOUT ANY  WARRANTY; without  even the  implied  warranty of
 * MERCHANTABILITY or FITNESS FOR  A PARTICULAR PURPOSE.  Please refer
 * to the GNU Public License for more details.
 *
 * You should  have received  a copy of  the GNU Public  License along
 * with this source code; if  not, write to: Free Software Foundation,
 * Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
#include "frei0r.hpp"

#include <algorithm>
#include <vector>
#include <utility>
#include <cassert>

#include <iostream>

// based upon twolay0r with some tweaks.
class threelay0r : public frei0r::filter
{
private:
	static unsigned char grey(unsigned int value) {
		unsigned char* rgba = reinterpret_cast<unsigned char*>(&value);
		unsigned char gw= (rgba[0] + rgba[1] + 2*rgba[2])/4;
		return gw;
	}
	
	struct histogram {
		histogram()	: hist(256)	{
			std::fill(hist.begin(),hist.end(),0);
		}
		
		void operator()(uint32_t value)	{
			++hist[grey(value)];
		}
		
		std::vector<unsigned int> hist;
	};

public:
	threelay0r(unsigned int width, unsigned int height) {}

	virtual void update() {
		histogram h;
		
		// create histogramm
		for (const unsigned int* i=in; i != in + (width*height);++i)
			h(*i);

		// calc th
		int th1 = 1;
		int th2 = 255;
		
		unsigned num = 0;
		unsigned int num1div3 = 4*size/10; // number of pixels in the lower level
		unsigned int num2div3 = 8*size/10; // number of pixels in the lower two levels
		for (int i = 0; i < 256; i++) { // wee bit faster than a double loop
			num += h.hist[i];
			if (num < num1div3) th1 = i;
			if (num < num2div3) th2 = i;
		}
		
		// create the 3 level image
		uint32_t* outpixel= out;
		const uint32_t* pixel=in;
		while(pixel != in+size) // size = defined in frei0r.hpp
		{
			if ( grey(*pixel) < th1 )
				*outpixel=0xFF000000;
			else if ( grey(*pixel) < th2)
				*outpixel=0xFF808080;
			else
				*outpixel=0xFFFFFFFF;
			++outpixel;
			++pixel;
		}
	}
};


frei0r::construct<threelay0r> plugin("threelay0r",
									"dynamic 3 level thresholding",
									"Hedde Bosman",
									0,1);

