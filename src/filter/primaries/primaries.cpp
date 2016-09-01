/* 
 * primaries
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

union px_t {
	uint32_t u;
	unsigned char c[4]; // 0=B, 1=G,2=R,3=A ? i think :P
};

class primaries : public frei0r::filter {
private:
	f0r_param_double factor;
	
public:
	primaries(unsigned int width, unsigned int height) {
		factor = 1;
		register_param(factor, "Factor", "influence of mean px value. > 32 = 0");
	}
	~primaries() {
	}

	virtual void update(double time,
	                    uint32_t* out,
                        const uint32_t* in) {
		unsigned char mean = 0;
		
		int f = factor+1; // f = [2,inf)
		int factor127 = (f*f-3)*127;
		int factorTot = f*f;
		if (factor127 < 0) {
			factor127 = 0;
			factorTot = 3;
		}
		
		for (int i = 0; i < size; i++) {
			px_t pi;
			pi.u = in[i];
			
			if (f > 32) // influence of mean color value does hardly change after this value
				mean = 127;
			else 
				mean = (pi.c[0] + pi.c[1] + pi.c[2] + factor127)/factorTot;
			pi.c[0] = (pi.c[0] > mean ? 255 : 0);
			pi.c[1] = (pi.c[1] > mean ? 255 : 0);
			pi.c[2] = (pi.c[2] > mean ? 255 : 0);
			
			out[i] = pi.u;
		}
	}
};


frei0r::construct<primaries> plugin("primaries",
									"Reduce image to primary colors",
									"Hedde Bosman",
									0,2);

