/* edgeglow.cpp
 * Copyright (C) 2008 Salsaman (salsaman@gmail.com)
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

class edgeglow : public frei0r::filter
{
public:

  f0r_param_double lthresh;
  f0r_param_double lupscale;
  f0r_param_double lredscale;


  edgeglow(unsigned int width, unsigned int height)
  {
    lthresh = 0.0;
    lupscale = 0.0;
    lredscale = 0.0;
    register_param(lthresh, "lthresh", "threshold for edge lightening");
    register_param(lupscale, "lupscale", "multiplier for upscaling edge brightness");
    register_param(lredscale, "lredscale", "multiplier for downscaling non-edge brightness");
  }
  
  virtual void update()
  {
    std::copy(in, in + width*height, out);
    for (unsigned int y=1; y<height-1; ++y)
    {
      for (unsigned int x=1; x<width-1; ++x)
      {
        // XXX faut eviter le * width en y allant par rangee
        unsigned char *p1 = (unsigned char *)&in[(y-1)*width+(x-1)];
        unsigned char *p2 = (unsigned char *)&in[(y-1)*width+x];
        unsigned char *p3 = (unsigned char *)&in[(y-1)*width+(x+1)];
        unsigned char *p4 = (unsigned char *)&in[y*width+(x-1)];
        unsigned char *p6 = (unsigned char *)&in[y*width+(x+1)];
        unsigned char *p7 = (unsigned char *)&in[(y+1)*width+(x-1)];
        unsigned char *p8 = (unsigned char *)&in[(y+1)*width+x];
        unsigned char *p9 = (unsigned char *)&in[(y+1)*width+(x+1)];
        
        unsigned char *g = (unsigned char *)&out[y*width+x];
        
        for (int i=0; i<3; ++i)
          g[i] = CLAMP0255(
                           abs(p1[i] + p2[i]*2 + p3[i] - p7[i] - p8[i]*2 - p9[i]) +
                           abs(p3[i] + p6[i]*2 + p9[i] - p1[i] - p4[i]*2 - p7[i]) );


	unsigned char *p5 = (unsigned char *)&in[y*width+x];

        g[3] = p5[3]; // copy alpha

	float lt;

	// calculate "lightness"
	unsigned char R=g[0];
	unsigned char G=g[1];
	unsigned char B=g[2];

	unsigned char max=R;
	if (G>max) max=G;
	if (B>max) max=B;

	unsigned char min=R;
	if (G<min) min=G;
	if (B<min) min=B;

	unsigned char l=(unsigned char) (((float)max+(float)min)/2.);



	// convert in and out to HSL, add L value of out
	
	unsigned int h;
	float s;
	
	R=p5[0];
	G=p5[1];
	B=p5[2];
	
	max=R;
	if (G>max) max=G;
	if (B>max) max=B;
	
	min=R;
	if (G<min) min=G;
	if (B<min) min=B;
	

	if (l>(lt=lthresh*255.)) {
	  // if lightness > threshold, we add it to the lightness of the original
	  l=CLAMP0255((int32_t)((float)l*lupscale+((float)max+(float)min)/2.));
	}
	// otherwise reduce
	else if (lredscale>0.) {
	  l=((float)max+(float)min)/2.*(1.-lredscale);
	}

	if (lredscale>0.||l>lt) {
	  if (max==min) {
	    h=0;
	    s=0.;
	  }
	  else {
	    if (max==R) {
	      h=(unsigned int)(60.*((float)G-(float)B)/((float)max-(float)min));
	      if (G<B) h+=360;
	    }
	    else {
	      if (max==G) h=(unsigned int)(60.*((float)B-(float)R)/((float)max-(float)min)+120.);
	      else h=(unsigned int)(60.*(float)(R-G)/float(max-min)+240.);
	    }
	    
	    if (l<=0.5) s=((float)max-(float)min)/((float)max+(float)min);
	    else s=((float)max-(float)min)/(2.-((float)max+(float)min));
	  }
	  
	  
	  // convert back to RGB
	  
	  float q;
	  
	  if (l<0.5) q=l*(1.+s);
	  else q=l+s-(l*s);
	  
	  float p=2.*l-q;
	  
	  float hk=(float)h/360.;
	  
	  float tr=hk+1./3.;
	  float tg=hk;
	  float tb=hk-1./3.;
	  
	  if (tr>1.) tr-=1.;
	  if (tb<0.) tb+=1.;
	  
	  if (tr<1./6.) g[0]=CLAMP0255(p+((q-p)*6.*tr));
	  else if (tr<0.5) g[0]=CLAMP0255((int32_t)q);
	  else if (tr<1./6.) g[0]=CLAMP0255(p+((q-p)*6.*(2./3.-tr)));
	  else g[0]=CLAMP0255((int32_t)p);
	  
	  if (tg<1./6.) g[1]=CLAMP0255(p+((q-p)*6.*tg));
	  else if (tg<0.5) g[1]=CLAMP0255((int32_t)q);
	  else if (tg<1./6.) g[1]=CLAMP0255(p+((q-p)*6.*(2./3.-tg)));
	  else g[1]=CLAMP0255((int32_t)p);
	  
	  if (tb<1./6.) g[2]=CLAMP0255(p+((q-p)*6.*tb));
	  else if (tb<0.5) g[2]=CLAMP0255((int32_t)q);
	  else if (tb<1./6.) g[2]=CLAMP0255(p+((q-p)*6.*(2./3.-tb)));
	  else g[2]=CLAMP0255((int32_t)p);
	}
	else {
	  g[0]=p5[0];
	  g[1]=p5[1];
	  g[2]=p5[2];
	}
      }
    }
  }
};


frei0r::construct<edgeglow> plugin("Edgeglow",
                                "Edgeglow filter",
                                "Salsaman",
                                0,1,
                                F0R_COLOR_MODEL_RGBA8888);

