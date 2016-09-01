/*
 * nervousTV - The name says it all...
 * Copyright (C) 2002 TANNENBAUM Edo
 *
 * 2002/2/9 
 *   Original code copied same frame twice, and did not use memcpy().
 *   I modifed those point.
 *   -Kentarou Fukuchi
 *
 * 2009/8/26
 *   Ported to frei0r from the old FreeJ filter API
 *   -Jaromil
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


#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <string.h>

#include <frei0r.hpp>


#define PLANES 32

// freej compat facilitator
typedef struct {
  int16_t w;
  int16_t h;
  uint8_t bpp;
  uint32_t size;
} ScreenGeometry;



class Nervous: public frei0r::filter {

public:
  
  Nervous(int width, int height);

  ~Nervous();

  virtual void update(double time,
                      uint32_t* out,
                      const uint32_t* in);

private:

  ScreenGeometry geo;

  void _init(int wdt, int hgt);
  int32_t *buffer;
  int32_t *planetable[PLANES];
  int mode;
  int plane, stock, timer, stride, readplane;

  /* cheap & fast randomizer (by Fukuchi Kentarou) */
  uint32_t randval;
  uint32_t fastrand() { return (randval=randval*1103515245+12345); };
  void fastsrand(uint32_t seed) { randval = seed; };

};

Nervous::Nervous(int wdt, int hgt) {
    int c;
    _init(wdt, hgt);
    
    buffer = (int32_t*) malloc(geo.size*PLANES);
    if(!buffer) {
      fprintf(stderr,"ERROR: nervous plugin can't allocate needed memory: %u bytes\n",
	      geo.size*PLANES);
      return;
    }
    memset(buffer,0,geo.size*PLANES);
    for(c=0;c<PLANES;c++)
      planetable[c] = &buffer[geo.w*geo.h*c];
    
    plane = 0;
    stock = 0;
    timer = 0;
    readplane = 0;
    mode = 1;
}

Nervous::~Nervous() {
  if(buffer) free(buffer);
}

void Nervous::_init(int wdt, int hgt) {
  geo.w = wdt;
  geo.h = hgt;
  geo.bpp = 32;
  geo.size = geo.w*geo.h*(geo.bpp/8);
}




void Nervous::update(double time,
                     uint32_t* out,
                     const uint32_t* in) {
  memcpy(planetable[plane],in,geo.size);

  if(stock<PLANES) stock++;

  if(mode) {
    if(timer) {
      readplane = readplane + stride;
      while(readplane < 0) readplane += stock;
      while(readplane >= stock) readplane -= stock;
      timer--;
    } else {
      readplane = fastrand() % stock;
      stride = fastrand() % 5 - 2;
      if(stride >= 0) stride++;
      timer = fastrand() % 6 + 2;
    }
  } else
    if(stock > 0)
      readplane = fastrand() % stock;
  
  plane++;
  if(plane==PLANES) plane=0;

  memcpy(out,planetable[readplane],geo.size);

}



frei0r::construct<Nervous> plugin("Nervous",
				"flushes frames in time in a nervous way",
				"Tannenbaum, Kentaro, Jaromil",
				3,1);
