/* Cartoon filter
 * main algorithm: (c) Copyright 2003 Dries Pruimboom <dries@irssystems.nl>
 * further optimizations and frei0r port by Denis Rojo <jaromil@dyne.org>
 * 
 * This source code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Public License as published 
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * This source code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * Please refer to the GNU Public License for more details.
 *
 * You should have received a copy of the GNU Public License along with
 * this source code; if not, write to:
 * Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * "$Id: cartoon.c 193 2004-06-01 11:00:25Z jaromil $"
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include <frei0r.hpp>

#define RED(n)  ((n>>16) & 0x000000FF)
#define GREEN(n) ((n>>8) & 0x000000FF)
#define BLUE(n)  (n & 0x000000FF)
#define RGB(r,g,b) ((r<<16) + (g <<8) + (b))

#define BOOST(n) { \
if((*p = *p<<4)<0)>>n; \
*(p+1) = (*(p+1)<<4)>>n; \
*(p+2) = (*(p+2)<<4)>>n; }

/* setup some data to identify the plugin */

typedef struct {
  int16_t w;
  int16_t h;
  uint8_t bpp;
  uint32_t size;
} ScreenGeometry;

#define PIXELAT(x1,y1,s) ((s)+(x1)+ yprecal[y1])// (y1)*(geo->w)))
#define GMERROR(cc1,cc2) ((((RED(cc1)-RED(cc2))*(RED(cc1)-RED(cc2))) +	\
   			((GREEN(cc1)-GREEN(cc2)) *(GREEN(cc1)-GREEN(cc2))) + \
			((BLUE(cc1)-BLUE(cc2))*(BLUE(cc1)-BLUE(cc2)))))

class Cartoon: public frei0r::filter {
public:

  f0r_param_double triplevel;
  f0r_param_double diffspace;

  Cartoon(unsigned int width, unsigned int height) {
    int c;
    register_param(triplevel, "triplevel", "level of trip: use high numbers, incremented by 100");
    register_param(diffspace, "diffspace", "difference space: a value from 0 to 256");

    geo = new ScreenGeometry();
    geo->w = width;
    geo->h = height;
    geo->size =  width*height*sizeof(uint32_t);

    prePixBuffer = (int32_t*)malloc(geo->size);
    conBuffer = (int32_t*)malloc(geo->size);

    yprecal = (int*)malloc(geo->h*2*sizeof(int));
    for(c=0;c<geo->h*2;c++)
      yprecal[c] = geo->w*c;
    for(c=0;c<256;c++) 
      powprecal[c] = c*c;
    black = 0x00000000;

    triplevel = 1000;
    diffspace = 1;

  }
  
  ~Cartoon() {
    free(prePixBuffer);
    free(conBuffer);
    free(yprecal);
  }

  virtual void update() {
    // Cartoonify picture, do a form of edge detect 
    int x, y, t;


    for (x=(int)diffspace;x<geo->w-(1+(int)diffspace);x++) {

      for (y=(int)diffspace;y<geo->h-(1+(int)diffspace);y++) {
	
	t = GetMaxContrast((int32_t*)in,x,y);
	if (t > triplevel) {
	  
	  //  Make a border pixel 
	  *(out+x+yprecal[y]) = 0x0;
	  
	} else {
	  
	  //   Copy original color 
	  *(out+x+yprecal[y]) = *(in+x+yprecal[y]);
	  FlattenColor((int32_t*)out+x+yprecal[y]);
	  
	}
      }
    }
  }

private:
  ScreenGeometry *geo;
  /* buffer where to copy the screen
     a pointer to it is being given back by process() */
  
  int32_t *prePixBuffer;
  int32_t *conBuffer;
  int *yprecal;
  uint16_t powprecal[256];
  int32_t black;
  
  void FlattenColor(int32_t *c);
  long GetMaxContrast(int32_t *src,int x,int y);
  
  inline uint16_t gmerror(int32_t a, int32_t b);
};

/* the following should be faster on older CPUs
   but runs slower than the GMERROR define on SSE units*/
inline uint16_t Cartoon::gmerror(int32_t a, int32_t b) {
  register int dr, dg, db;
  if((dr = RED(a) - RED(b)) < 0) dr = -dr;
  if((dg = GREEN(a) - GREEN(b)) < 0) dg = -dg;
  if((db = BLUE(a) - BLUE(b)) < 0) db = -db;
  return(powprecal[dr]+powprecal[dg]+powprecal[db]);
}


void Cartoon::FlattenColor(int32_t *c) {
  // (*c) = RGB(40*(RED(*c)/40),40*(GREEN(*c)/40),40*(BLUE(*c)/40)); */
  uint8_t *p;
  p = (uint8_t*)c;
  (*p) = ((*p)>>5)<<5; p++;
  (*p) = ((*p)>>5)<<5; p++;
  (*p) = ((*p)>>5)<<5;
}



long Cartoon::GetMaxContrast(int32_t *src,int x,int y) {
  int32_t c1,c2;
  long error,max=0;
  
  /* Assumes PrePixelModify has been run */
  c1 = *PIXELAT(x-(int)diffspace,y,src);
  c2 = *PIXELAT(x+(int)diffspace,y,src);	
  error = GMERROR(c1,c2);
  if (error>max) max = error;
  
  c1 = *PIXELAT(x,y-(int)diffspace,src);
  c2 = *PIXELAT(x,y+(int)diffspace,src);
  error = GMERROR(c1,c2);
  if (error>max) max = error;
  
  c1 = *PIXELAT(x-(int)diffspace,y-(int)diffspace,src);
  c2 = *PIXELAT(x+(int)diffspace,y+(int)diffspace,src);
  error = GMERROR(c1,c2);
  if (error>max) max = error;
  
  c1 = *PIXELAT(x+(int)diffspace,y-(int)diffspace,src);
  c2 = *PIXELAT(x-(int)diffspace,y+(int)diffspace,src);
  error = GMERROR(c1,c2);
  if (error>max) max = error;
  
  return(max);
}

frei0r::construct<Cartoon> plugin("Cartoon",
				  "Cartoonify video, do a form of edge detect",
				  "Dries Pruimboom, Jaromil",
				  2,0);


