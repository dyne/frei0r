/*
 *  Particle generator
 *  (c) Copyright 2004-2007 Denis Roio aka jaromil <jaromil@dyne.org>
 *
 *  blossom original algo is (c) 2003 by ragnar (waves 1.2)
 *  http://home.uninet.ee/~ragnar/waves
 *  further optimizations and changes followed
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
 * "$Id: gen_layer.cpp 845 2007-04-03 07:04:47Z jaromil $"
 *
 */

#if defined(_MSC_VER)
#define _USE_MATH_DEFINES
#endif /* _MSC_VER */
#include <cmath>

#include "frei0r.hpp"

#include <stdlib.h>
#include <string.h>

#include <algorithm>

#include <time.h>
#include <inttypes.h>
#include <limits.h>

/* defines for blob size and roundness */
#define LIM 8 // 25
#define NB_BLOB 16 // 25

#define PRIMES 11

class Partik0l: public frei0r::source {
public:

  Partik0l(unsigned int width, unsigned int height);
  ~Partik0l();
  
  void update(double time,
              uint32_t* out);

  int w, h;

  double up;
  double down;

private:

  uint32_t size;

  /* blossom vars */
  double blossom_count;
  double blossom_m;
  double blossom_n;
  double blossom_i;
  double blossom_j;
  double blossom_k;
  double blossom_l;
  float blossom_r;
  float blossom_a;

  /* primes */
  int prime[11];

  float pi2;
  double wd, hd;


  void blob(uint32_t* out, int x, int y);
  void blossom(uint32_t* out);
  void blob_init(int ray);
  void blossom_recal(bool r);

  /* surface buffer */
  //  uint32_t *pixels;
  uint32_t *blob_buf;
  int blob_size;

  void fastsrand(uint32_t seed);
  uint32_t fastrand();

  uint32_t randval;
};

Partik0l::Partik0l(unsigned int width, unsigned int height) {

  register_param(up, "up", "blossom on a higher prime number");
  register_param(down, "down", "blossom on a lower prime number");

  /* initialize prime numbers */
  prime[0] = 2;
  prime[1] = 3;
  prime[2] = 5;
  prime[3] = 7;
  prime[4] = 11;
  prime[5] = 13;
  prime[6] = 17;
  prime[7] = 19;
  prime[8] = 23;
  prime[9] = 29;
  prime[10] = 31;
  
  /* blossom vars */
  blossom_count = 0; 
  blossom_m = 0;
  blossom_n = 0;
  blossom_i = 0;
  blossom_j = 0;
  blossom_k = 0;
  blossom_l = 0;
  blossom_r = 1;
  blossom_a = 0;

  up = 0;
  down = 0;

  pi2 = 2.0*M_PI;
  
  fastsrand( ::time(NULL) );
  
  w = width;
  h = height;
  size = w * h * 4; // 32bit pixels
  //  pixels = (uint32_t*)malloc(size);
  
  
  blob_buf = NULL;
  
  blossom_recal(true);
  
  /* blob initialization */
  blob_init(8);
  
  
}

Partik0l::~Partik0l() {
  //  if(pixels) free(pixels);
  if(blob_buf) free(blob_buf);
}



void Partik0l::update(double time,
                      uint32_t* out) {
  /* automatic random recalculation:
     if( !blossom_count ) {    
     recalculate();
     blossom_count = 100+(50.0)*rand()/RAND_MAX;
     } else {
     blossom_count--;
  */

  if(up) {
    blossom_recal(false);
    up = false;
  } else if(down) {
    blossom_recal(true);
    down = false;
  }

  blossom_a += 0.01;
  if( blossom_a > pi2 )
    blossom_a -= pi2;


  memset(out,0,size);

  blossom(out);

}

void Partik0l::blossom_recal(bool r) {

  float z = ((PRIMES-2)*fastrand()/INT_MAX)+1;
  blossom_m = 1.0+(30.0)*fastrand()/INT_MAX;
  blossom_n = 1.0+(30.0)*fastrand()/INT_MAX;
  blossom_i = prime[ (int) (z*fastrand()/INT_MAX) ];
  blossom_j = prime[ (int) (z*fastrand()/INT_MAX) ];
  blossom_k = prime[ (int) (z*fastrand()/INT_MAX) ];
  blossom_l = prime[ (int) (z*fastrand()/INT_MAX) ];
  wd = (double)w;
  hd = (double)h;
  if(r)
    blossom_r = (blossom_r>=1.0)?1.0:blossom_r+0.1;
  else
    blossom_r = (blossom_r<=0.1)?0.1:blossom_r-0.1;
}  

void Partik0l::blossom(uint32_t* out) {
  
  float	a;
  int x, y;
  double zx, zy;

  /* here place a formula that draws on the screen
     the surface being drawed at this point is always blank */
  for( a=0.0 ; a<pi2; a+=0.005 ) {
    zx = blossom_m*a;
    zy = blossom_n*a;
    x = (int)(wd*(0.47+ (blossom_r*sin(blossom_i*zx+blossom_a)+
			 (1.0-blossom_r)*sin(blossom_k*zy+blossom_a)) /2.2 ));
    
    y = (int)(hd*(0.47+ (blossom_r*cos(blossom_j*zx+blossom_a)+
			 (1.0-blossom_r)*cos(blossom_l*zy+blossom_a)) /2.2 ));
    
    blob(out, x,y);
    
  } 

}

void Partik0l::blob_init(int ray) {
  uint8_t col;

  blob_size = ray*2;

  /* there is the blob gradient sphere here
     Niels helps me with this: calculating a circle
     while(theta <= 360) {
     double radians = (theta / 180.0) * PI;
     double dx = ( origin[0] + cos( radians ) * radius );
     double dy = ( origin[1] + sin( radians ) * radius );
     
     (there are always some basics you learn at school and then forget)
  */
  uint32_t dx,dy;
  double rad, th;
  int c;
  srand(::time(NULL));
  if(blob_buf) free(blob_buf);
  
  blob_buf = (uint32_t*) calloc(ray*2*ray*2*2,sizeof(uint32_t));
  //  memset(blob_buf,0,ray*2*ray*2*sizeof(uint32_t));

  for(th=1;th<=360;th++) {
    rad = (th / 180.0) * M_PI;
    for(c=ray;c>0;c--) {
      dx = ( (ray) + cos( rad ) * c );
      dy = ( (ray) + sin( rad ) * c );
      //      col = (int)(10.0*rand()/(RAND_MAX+1.0))/c;
      //      col += 0x99/c * 0.8;
      col = 0x99/c * 0.8;
      blob_buf[ (dx+((ray*2)*dy)) ] =
	col|(col<<8)|(col<<16)|(col<<24);
    }
  }
  
}
  

void Partik0l::blob(uint32_t* out, int x, int y) {
  //  if(y>h-blob_size) return;
  //  if(x>w-blob_size) return;

  int i, j;
  int stride = (w-blob_size)>>1;

  uint64_t *tmp_scr = (uint64_t*)out + ((x + y*w)>>1);
  uint64_t *tmp_blob = (uint64_t*)blob_buf;

#ifdef HAVE_MMX
  /* using mmx packed unsaturated addition on bytes
     for cleaner and shiny result */
  for(j=blob_size; j>0; j--) {
    for(i=blob_size>>4; i>0; i--) {
      
      asm volatile("movq (%1),%%mm0;"
		   "movq 8(%1),%%mm1;"
		   "movq 16(%1),%%mm2;"
		   "movq 24(%1),%%mm3;"
		   "movq 32(%1),%%mm4;"
		   "movq 40(%1),%%mm5;"
		   "movq 48(%1),%%mm6;"
		   "movq 56(%1),%%mm7;"

		   "paddusb (%0),%%mm0;" // packed add unsaturated on bytes
		   "paddusb 8(%0),%%mm1;" // addizione clippata
		   "paddusb 16(%0),%%mm2;"
		   "paddusb 24(%0),%%mm3;"
		   "paddusb 32(%0),%%mm4;"
		   "paddusb 40(%0),%%mm5;"
		   "paddusb 48(%0),%%mm6;"
		   "paddusb 56(%0),%%mm7;"

		   "movq %%mm0,(%0);"	  
		   "movq %%mm1,8(%0);"	  
		   "movq %%mm2,16(%0);"	  
		   "movq %%mm3,24(%0);"	  
		   "movq %%mm4,32(%0);"	  
		   "movq %%mm5,40(%0);"	  
		   "movq %%mm6,48(%0);"	 
		   "movq %%mm7,56(%0);"
		   //	  "paddsw %0, %%mm0;"// halo violetto?
		   :
		   :"r"(tmp_scr),"r"(tmp_blob)
		   :"memory");
      tmp_scr+=8;
      tmp_blob+=8;
    }
    tmp_scr += stride;
  }
  asm("emms;");


#else // ! HAVE_MMX
  for(j=blob_size; j>0; j--) {
    for(i=blob_size>>1; i>0; i--) {
      *(tmp_scr++) += *(tmp_blob++);
    }
    tmp_scr += stride;
  }
#endif

}

/*
 * fastrand - fast fake random number generator
 * by Fukuchi Kentarou
 * Warning: The low-order bits of numbers generated by fastrand()
 *          are bad as random numbers. For example, fastrand()%4
 *          generates 1,2,3,0,1,2,3,0...
 *          You should use high-order bits.
 *
 */



uint32_t Partik0l::fastrand()
{
  //    kentaro's original one:
  //	return (randval=randval*1103515245+12345);
	//15:55  <salsaman2> mine uses two prime numbers and the cycling is much reduced
	//15:55  <salsaman2>   return (randval=randval*1073741789+32749);
  return(randval = randval * 1073741789 + 32749 );
}

void Partik0l::fastsrand(uint32_t seed)
{
	randval = seed;
}

/*
bool Partik0l::keypress(int key) {
  if(key=='p')
    blossom_recal(true);
  else if(key=='o')
    blossom_recal(false);
  else return(false);

  return(true);
}
*/  
frei0r::construct<Partik0l> plugin("Partik0l",
				 "Particles generated on prime number sinusoidal blossoming",
				 "Jaromil",
				 0,3);
