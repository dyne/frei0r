/* Water filter
 *
 * (c) Copyright 2000-2007 Denis Rojo <jaromil@dyne.org>
 * 
 * from an original idea of water algorithm by Federico 'Pix' Feroldi
 *
 * this code contains optimizations by Jason Hood and Scott Scriven
 *
 * animated background, 32bit colorspace and interactivity by jaromil
 * ported to C++ and frei0r plugin API in 2007
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
 * "$Id: water.c 193 2004-06-01 11:00:25Z jaromil $"
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <frei0r.hpp>


#define CLIP_EDGES \
  if(x - radius < 1) left -= (x-radius-1); \
  if(y - radius < 1) top  -= (y-radius-1); \
  if(x + radius > geo->w-1) right -= (x+radius-geo->w+1); \
  if(y + radius > geo->h-1) bottom-= (y+radius-geo->h+1);

/* water physics */
#define WATER 1
#define JELLY 2
#define SLUDGE 3
#define SUPER_SLUDGE 4

/* precalculated sinusoidal tables */
#include <math.h>
#define FSINMAX 2047
#define SINFIX 16
#define FSINBITS 16
#ifndef PI
#define PI 3.14159265358979323846
#endif

typedef struct {
  int16_t w;
  int16_t h;
  uint8_t bpp;
  uint32_t size;
} ScreenGeometry;

class Water: public frei0r::filter {
public:

  f0r_param_position splash;
  f0r_param_double physics;
  bool rain;
  bool distort;
  bool smooth;
  bool surfer;
  bool swirl;
  bool randomize_swirl;

  Water(unsigned int width, unsigned int height) {
    register_param(splash, "splash", "make a big splash in the center");
    register_param(physics, "physics", "water density: from 1 to 4");
    register_param(rain, "rain", "rain drops all over");
    register_param(distort, "distort", "distort all surface like dropping a bucket to the floor");
    register_param(smooth, "smooth", "smooth up all perturbations on the surface");
    register_param(surfer, "surfer", "surf the surface with a wandering finger");
    register_param(swirl, "swirl", "swirling whirpool in the center");
    register_param(randomize_swirl, "randomize_swirl", "randomize the swirling angle");

    Hpage = 0;
    ox = 80;
    oy = 80;
    done = 0;
    mode = 0x4000;

    BkGdImagePre = BkGdImage = BkGdImagePost = 0;
    Height[0] = Height[1] = 0;
    
    /* default physics */
    density = 4;
    pheight = 600;
    radius = 30;
    
    raincount = 0;
    blend = 0;
    
    fastsrand(::time(NULL));
    
    FCreateSines();

    geo = new ScreenGeometry();
    geo->w = width;
    geo->h = height;
    geo->size =  width*height*sizeof(uint32_t);

    water_surfacesize = geo->size;
    calc_optimization = (height-1)*width;
    
    xang = fastrand()%2048;
    yang = fastrand()%2048;
    swirlangle = fastrand()%2048;
    
    /* buffer allocation tango */
    if ( width*height > 0 ) {
        Height[0] = (uint32_t*)calloc(width*height, sizeof(uint32_t));
        Height[1] = (uint32_t*)calloc(width*height, sizeof(uint32_t));
    }
    //    buffer =    (uint32_t*)    malloc(geo->size);
    if ( geo->size > 0 ) {
        BkGdImagePre = (uint32_t*) malloc(geo->size);
        BkGdImage =    (uint32_t*) malloc(geo->size);
        BkGdImagePost = (uint32_t*)malloc(geo->size);
    }


    swirl = 1;

  }

  ~Water() {
    delete geo;
    free(Height[0]);
    free(Height[1]);
    free(BkGdImagePre);
    free(BkGdImage);
    free(BkGdImagePost);
    //    free(buffer);
  }

  virtual void update() {

    memcpy(BkGdImage, in, width*height*sizeof(uint32_t));
    
    water_update();

  }
  
private:
  ScreenGeometry *geo;

  /* 2 pages of Height field */
  uint32_t *Height[2];
  /* 3 copies of the background */
  uint32_t *BkGdImagePre;
  uint32_t *BkGdImage;
  uint32_t *BkGdImagePost;
  
  //  uint32_t *buffer;
  
  void *surface;
  
  /* water effect variables */
  int Hpage;
  int xang, yang;
  int swirlangle;
  int x, y, ox, oy;
  int done;
  int mode;
  
  /* precalculated to optimize a bit */
  int water_surfacesize;
  int calc_optimization;
  
  /* density: water density (step 1)
     pheight: splash height (step 40)
     radius: waterdrop radius (step 1) */
  int density, pheight, radius;
  int offset;  
  
  int raincount;
  int blend;

  void water_clear();
  void water_distort();
  void water_setphysics(double physics);
  void water_update();
  void water_drop(int x, int y);
  void water_bigsplash(int x, int y);
  void water_surfer();
  void water_swirl();
  void water_3swirls();
  
  void DrawWater(int page);
  void CalcWater(int npage, int density);
  void CalcWaterBigFilter(int npage, int density);
  
  void SmoothWater(int npage);
  
  void HeightBlob(int x, int y, int radius, int height, int page);
  void HeightBox (int x, int y, int radius, int height, int page);
  
  void WarpBlob(int x, int y, int radius, int height, int page);
  void SineBlob(int x, int y, int radius, int height, int page);

  /* precalculated sinusoidal tables */
  int FSinTab[2048], FCosTab[2048];
  int FSin(int angle) { return FSinTab[angle&FSINMAX]; }
  int FCos(int angle) { return FCosTab[angle&FSINMAX]; }
  void FCreateSines() {
    int i; double angle;  
    for(i=0; i<2048; i++) {
      angle = (float)i * (PI/1024.0);
      FSinTab[i] = (int)(sin(angle) * (float)0x10000);
      FCosTab[i] = (int)(cos(angle) * (float)0x10000);
    }
  }

  /* cheap & fast randomizer (by Fukuchi Kentarou) */
  uint32_t randval;
  uint32_t fastrand() { return (randval=randval*1103515245+12345); };
  void fastsrand(uint32_t seed) { randval = seed; };
  
  /* integer optimized square root by jaromil */
  int isqrt(unsigned int x) {
    unsigned int m, y, b; m = 0x40000000;
    y = 0; while(m != 0) { b = y | m; y = y>>1;
      if(x>=b) { x=x-b; y=y|m; }
      m=m>>2; } return y;
  }
  
};





/* TODO: port as parameters:

int kbd_input(char key) {
  int res = 1;
  switch(key) {
  case 'e': // bigsplash in center
    water_bigsplash(geo->w>>1,geo->y>>1);
    break;
  case 'r': // random splash 
    water_bigsplash(fastrand()%geo->w,fastrand()%geo->h);
    break;
  case 't': // rain
    rain = (rain)?0:1;
    break;
  case 'd': // distort surface
    if(!rain) water_distort();
    break;
  case 'f': // smooth surface
    SmoothWater(Hpage);
    break;
  case 'y': // swirl
    swirl = (swirl)?0:1;
    break;
  case 'u': // surfer
    surfer = (surfer)?0:1;
    break;
  case 'g': // randomize swirl angles
    swirlangle = fastrand()%2048;
    xang = fastrand()%2048;
    yang = fastrand()%2048;
    break;
    
  case 'q':
    if(physics>1) physics--;
    water_setphysics(physics);
    break;
  case 'w':
    if(physics<4) physics++;
    water_setphysics(physics);

  default:
    res = 0;
    break;
  }
  return(res);
}
*/

void Water::water_clear() {
  memset(Height[0], 0, water_surfacesize);
  memset(Height[1], 0, water_surfacesize);
}

void Water::water_distort() {
  memset(Height[Hpage], 0, water_surfacesize);
}

void Water::water_setphysics(double physics) {
  if(physics<0.25) {
    // case WATER:
    mode |= 0x4000;
    density=4;
    pheight=600;
  } else if(physics<0.50) {
    // case JELLY:
    mode &= 0xBFFF;
    density=3;
    pheight=400;
  } else if(physics<0.75) {
    // case SLUDGE:
    mode &= 0xBFFF;
    density=6;
    pheight=400;
  } else {
    //  case SUPER_SLUDGE:
    mode &=0xBFFF;
    density=8;
    pheight=400;
  }
}

void Water::water_update() {

  if(rain) {
    raincount++;
    if(raincount>3) {
      water_drop( (fastrand()%geo->w-40)+20 , (fastrand()%geo->h-40)+20 );
      raincount=0;
    }
  }

  if(swirl) water_swirl();
  if(surfer) water_surfer();
  DrawWater(Hpage);

  CalcWater(Hpage^1, density);
  Hpage ^=1 ;
}

void Water::water_drop(int x, int y) {
  if(mode & 0x4000)
    HeightBlob(x,y, radius>>2, pheight, Hpage);
  else
    WarpBlob(x, y, radius, pheight, Hpage);
}

void Water::water_bigsplash(int x, int y) {
  if(mode & 0x4000)
    HeightBlob(x, y, (radius>>1), pheight, Hpage);
  else
    SineBlob(x, y, radius, -pheight*6, Hpage);
}

void Water::water_surfer() {
  x = (geo->w>>1)
    + ((
	(
	 (FSin( (xang* 65) >>8) >>8) *
	 (FSin( (xang*349) >>8) >>8)
	 ) * ((geo->w-8)>>1)
	) >> 16);
  y = (geo->h>>1)
    + ((
	(
	 (FSin( (yang*377) >>8) >>8) *
	 (FSin( (yang* 84) >>8) >>8)
	 ) * ((geo->h-8)>>1)
	) >> 16);
  xang += 13;
  yang += 12;
  
  if(mode & 0x4000)
    {
      offset = (oy+y)/2*geo->w + ((ox+x)>>1); // QUAAA
      Height[Hpage][offset] = pheight;
      Height[Hpage][offset + 1] =
	Height[Hpage][offset - 1] =
	Height[Hpage][offset + geo->w] =
	Height[Hpage][offset - geo->w] = pheight >> 1;
      
      offset = y*geo->w + x;
      Height[Hpage][offset] = pheight<<1;
      Height[Hpage][offset + 1] =
	Height[Hpage][offset - 1] =
	Height[Hpage][offset + geo->w] =
	Height[Hpage][offset - geo->w] = pheight;
    }
  else
    {
      SineBlob(((ox+x)>>1), ((oy+y)>>1), 3, -1200, Hpage);
      SineBlob(x, y, 4, -2000, Hpage);
    }
  
  ox = x;
  oy = y; 
}

void Water::water_swirl() {
  x = (geo->w>>1)
    + ((
	(FCos(swirlangle)) * (25)
	) >> 16);
  y = (geo->h>>1)
    + ((
	(FSin(swirlangle)) * (25)
	) >> 16);
  swirlangle += 50;
  if(mode & 0x4000)
    HeightBlob(x,y, radius>>2, pheight, Hpage);
  else
    WarpBlob(x, y, radius, pheight, Hpage);
}


void Water::water_3swirls() {
#define ANGLE 15
  x = (95)
    + ((
	(FCos(swirlangle)) * (ANGLE)
	) >> 16);
  y = (45)
    + ((
	(FSin(swirlangle)) * (ANGLE)
	) >> 16);

  if(mode & 0x4000) HeightBlob(x,y, radius>>2, pheight, Hpage);
  else WarpBlob(x, y, radius, pheight, Hpage);
  
  x = (95)
    + ((
	(FCos(swirlangle)) * (ANGLE)
	) >> 16);
  y = (255)
    + ((
	(FSin(swirlangle)) * (ANGLE)
	) >> 16);
  
  if(mode & 0x4000) HeightBlob(x,y, radius>>2, pheight, Hpage);
  else WarpBlob(x, y, radius, pheight, Hpage);
  
  x = (345)
    + ((
	(FCos(swirlangle)) * (ANGLE)
	) >> 16);
  y = (150)
    + ((
	(FSin(swirlangle)) * (ANGLE)
	) >> 16);
 
 if(mode & 0x4000) HeightBlob(x,y, radius>>2, pheight, Hpage);
  else WarpBlob(x, y, radius, pheight, Hpage);


  swirlangle += 50;
}

/* internal physics routines */
void Water::DrawWater(int page) {
  int dx, dy;
  int x, y;
  int c;
  int offset=geo->w + 1;
  int *ptr = (int*)&Height[page][0];
  
  for (y = calc_optimization; offset < y; offset += 2) {
    for (x = offset+geo->w-2; offset < x; offset++) {
      dx = ptr[offset] - ptr[offset+1];
      dy = ptr[offset] - ptr[offset+geo->w];
      c = BkGdImage[offset + geo->w*(dy>>3) + (dx>>3)];
      
      out[offset] = c;

      offset++;
      dx = ptr[offset] - ptr[offset+1];
      dy = ptr[offset] - ptr[offset+geo->w];
      c = BkGdImage[offset + geo->w*(dy>>3) + (dx>>3)];

      out[offset] = c;      
    }
  }
}

void Water::CalcWater(int npage, int density) {
  int newh;
  int count = geo->w + 1;
  int *newptr = (int*) &Height[npage][0];
  int *oldptr = (int*) &Height[npage^1][0];
  int x, y;

  for (y = calc_optimization; count < y; count += 2) {
    for (x = count+geo->w-2; count < x; count++) {
      /* eight pixels */
      newh = ((oldptr[count + geo->w]
	       + oldptr[count - geo->w]
	       + oldptr[count + 1]
	       + oldptr[count - 1]
	       + oldptr[count - geo->w - 1]
	       + oldptr[count - geo->w + 1]
	       + oldptr[count + geo->w - 1]
	       + oldptr[count + geo->w + 1]
	       ) >> 2 )
	- newptr[count];
      newptr[count] =  newh - (newh >> density);
    }
  }
}

void Water::SmoothWater(int npage) {
  int newh;
  int count = geo->w + 1;
  int *newptr = (int*) &Height[npage][0];
  int *oldptr = (int*) &Height[npage^1][0];
  int x, y;

  for(y=1; y<geo->h-1; y++) {
    for(x=1; x<geo->w-1; x++) {
      /* eight pixel */
      newh          = ((oldptr[count + geo->w]
			+ oldptr[count - geo->w]
			+ oldptr[count + 1]
			+ oldptr[count - 1]
			+ oldptr[count - geo->w - 1]
			+ oldptr[count - geo->w + 1]
			+ oldptr[count + geo->w - 1]
			+ oldptr[count + geo->w + 1]
			) >> 3 )
	+ newptr[count];
      
      
      newptr[count] =  newh>>1;
      count++;
    }
    count += 2;
  }
}

void Water::CalcWaterBigFilter(int npage, int density) {
  int newh;
  int count = (geo->w<<1) + 2;
  int *newptr = (int*) &Height[npage][0];
  int *oldptr = (int*) &Height[npage^1][0];
  int x, y;
  
  for(y=2; y<geo->h-2; y++) {
    for(x=2; x<geo->w-2; x++) {
      /* 25 pixels */
      newh = (
	      (
	       (
		(oldptr[count + geo->w]
		 + oldptr[count - geo->w]
		 + oldptr[count + 1]
		 + oldptr[count - 1]
		 )<<1)
	       + ((oldptr[count - geo->w - 1]
		   + oldptr[count - geo->w + 1]
		   + oldptr[count + geo->w - 1]
		   + oldptr[count + geo->w + 1]))
	       + ( (
		    oldptr[count - (geo->w<<1)]
		    + oldptr[count + (geo->w<<1)]
		    + oldptr[count - 2]
		    + oldptr[count + 2]
		    ) >> 1 )
	       + ( (
		    oldptr[count - (geo->w<<1) - 1]
		    + oldptr[count - (geo->w<<1) + 1]
		    + oldptr[count + (geo->w<<1) - 1]
		    + oldptr[count + (geo->w<<1) + 1]
		    + oldptr[count - 2 - geo->w]
		    + oldptr[count - 2 + geo->w]
		    + oldptr[count + 2 - geo->w]
		    + oldptr[count + 2 + geo->w]
		    ) >> 2 )
	       )
	      >> 3)
	- (newptr[count]);
      newptr[count] =  newh - (newh >> density);
      count++;
    }
    count += 4;
  }
}

void Water::HeightBlob(int x, int y, int radius, int height, int page) {
  int rquad;
  int cx, cy, cyq;
  int left, top, right, bottom;

  rquad = radius * radius;

  /* Make a randomly-placed blob... */
  if(x<0) x = 1+radius+ fastrand()%(geo->w-2*radius-1);
  if(y<0) y = 1+radius+ fastrand()%(geo->h-2*radius-1);

  left=-radius; right = radius;
  top=-radius; bottom = radius;

  CLIP_EDGES

  for(cy = top; cy < bottom; cy++) {
    cyq = cy*cy;
    for(cx = left; cx < right; cx++) {
      if(cx*cx + cyq < rquad)
        Height[page][geo->w*(cy+y) + (cx+x)] += height;
    }
  }
}


void Water::HeightBox (int x, int y, int radius, int height, int page) {
  int cx, cy;
  int left, top, right, bottom;

  if(x<0) x = 1+radius+ fastrand()%(geo->w-2*radius-1);
  if(y<0) y = 1+radius+ fastrand()%(geo->h-2*radius-1);
  
  left=-radius; right = radius;
  top=-radius; bottom = radius;
  
  CLIP_EDGES
  
  for(cy = top; cy < bottom; cy++) {
    for(cx = left; cx < right; cx++) {
      Height[page][geo->w*(cy+y) + (cx+x)] = height;
    }
  } 
}

void Water::WarpBlob(int x, int y, int radius, int height, int page) {
  int cx, cy;
  int left,top,right,bottom;
  int square;
  int radsquare = radius * radius;
  
  radsquare = (radius*radius);
  
  height = height>>5;
  
  left=-radius; right = radius;
  top=-radius; bottom = radius;

  CLIP_EDGES
  
  for(cy = top; cy < bottom; cy++) {
    for(cx = left; cx < right; cx++) {
      square = cy*cy + cx*cx;
      if(square < radsquare) {
	Height[page][geo->w*(cy+y) + cx+x]
          += (int)((radius-isqrt(square))*(float)(height));
      }
    }
  }
}

void Water::SineBlob(int x, int y, int radius, int height, int page) {
  int cx, cy;
  int left,top,right,bottom;
  int square, dist;
  int radsquare = radius * radius;
  float length = (1024.0/(float)radius)*(1024.0/(float)radius);
  
  if(x<0) x = 1+radius+ fastrand()%(geo->w-2*radius-1);
  if(y<0) y = 1+radius+ fastrand()%(geo->h-2*radius-1);

  radsquare = (radius*radius);
  left=-radius; right = radius;
  top=-radius; bottom = radius;

  CLIP_EDGES

  for(cy = top; cy < bottom; cy++) {
    for(cx = left; cx < right; cx++) {
      square = cy*cy + cx*cx;
      if(square < radsquare) {
        dist = (int)(isqrt(square*length));
        Height[page][geo->w*(cy+y) + cx+x]
          += (int)((FCos(dist)+0xffff)*(height)) >> 19;
      }
    }
  }
}

frei0r::construct<Water> plugin("Water",
				"water drops on a video surface",
				"Jaromil",
				3,0);

