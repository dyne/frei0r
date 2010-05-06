/*
 *
 * BaltanTV - like StreakTV, but following for a long time
 * Copyright (C) 2001 FUKUCHI Kentarou
 * ported to FreeJ by jaromil
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
 */

#include <stdlib.h>
#include <string.h>

#include <frei0r.hpp>

#define PLANES 32

#define STRIDE 8
#define STRIDE2 16 /* (STRIDE*2) */
#define STRIDE3 24 /* (STRIDE*3) */

// freej compat facilitator
typedef struct {
  int16_t w;
  int16_t h;
  uint8_t bpp;
  uint32_t size;
} ScreenGeometry;


class Baltan: public frei0r::filter {

public:
  Baltan(int wdt, int hgt);
  ~Baltan();

  virtual void update();

private:
  ScreenGeometry geo;

  void _init(int wdt, int hgt);
  
  uint32_t *planebuf;
  uint32_t *planetable[PLANES];
  void *procbuf;
  int plane;
  int pixels;
};

Baltan::Baltan(int wdt, int hgt) {
  int i;
    
  _init(wdt, hgt);
  pixels = geo.w*geo.h;
  
  planebuf = (uint32_t*)malloc(geo.size*PLANES);
  bzero(planebuf, geo.size*PLANES);
    
  for(i=0;i<PLANES;i++)
    planetable[i] = &planebuf[pixels*i];

  procbuf = malloc(geo.size);
    
  plane = 0;
}

Baltan::~Baltan() {
  free(procbuf);
}

void Baltan::update() {
  int i, cf;

  uint32_t *buf = (uint32_t*)in;
  uint32_t *dst = (uint32_t*)out;

  
  for(i=0; i<pixels; i++)
    planetable[plane][i] = (buf[i] & 0xfcfcfc)>>2;
  

  cf = plane & (STRIDE-1);
  
  for(i=0; i<pixels; i++) {
    dst[i] = (src[i]&0xFF000000)
      |(planetable[cf][i]
	+ planetable[cf+STRIDE][i]
	+ planetable[cf+STRIDE2][i]
	+ planetable[cf+STRIDE3][i]);
    planetable[plane][i] = (dst[i]&0xfcfcfc)>>2;
  }


  plane++;
  plane = plane & (PLANES-1);
  
}

void Baltan::_init(int wdt, int hgt) {
  geo.w = wdt;
  geo.h = hgt;
  geo.bpp = 32;
  geo.size = geo.w*geo.h*(geo.bpp/8);
}

frei0r::construct<Baltan> plugin("Baltan",
				  "delayed alpha smoothed blit of time",
				  "Kentaro, Jaromil",
				  3,0);
