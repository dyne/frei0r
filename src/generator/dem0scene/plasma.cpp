/*
  Plasma 8bit demos-scene effect

  Copyright (C) 2002 W.P. van Paassen - peter@paassen.tmfweb.nl
            (C) 2009 Denis Roio - jaromil@dyne.org

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free  Software Foundation; either  version 3 of the  License, or
  (at your option) any later version.
  
  This program is distributed in the  hope that it will be useful, but
  WITHOUT  ANY   WARRANTY;  without  even  the   implied  warranty  of
  MERCHANTABILITY or  FITNESS FOR A  PARTICULAR PURPOSE.  See  the GNU
  General Public License for more details.
  
  You should  have received a copy  of the GNU  General Public License
  along  with  this  program;  if  not, write  to  the  Free  Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
    
*/


#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include <inttypes.h>

#include <frei0r.hpp>


typedef struct {
  int16_t x; ///< x axis position coordinate
  int16_t y; ///< y axis position coordinate
  uint16_t w; ///< width of frame in pixels
  uint16_t h; ///< height of frame in pixels
  uint8_t bpp; ///< bits per pixel
  uint16_t pitch; ///< width of frame in bytes
  uint32_t size; ///< size of the whole frame in bytes
} ScreenGeometry;

typedef struct {
  uint8_t r;
  uint8_t g;
  uint8_t b;
} Palette;

class Plasma: public frei0r::source {

public:
  Plasma(int wdt, int hgt);
  ~Plasma();

  virtual void update();

private:

  ScreenGeometry geo;

  void _init(int wdt, int hgt);


  uint16_t pos1, pos2, pos3, pos4;
  uint16_t tpos1, tpos2, tpos3, tpos4;

  int aSin[512];

  Palette colors[256];

  uint32_t palette2rgb(uint8_t idx);

  // vectors (exposed parameters from 0 to 1)
  double speed1; // 5
  double speed2; // 3
  double speed3; // 3
  double speed4; // 1
  double move1; // 9
  double move2; // 8

  // scalars
  double _speed1; // 5
  double _speed2; // 3
  double _speed3; // 3
  double _speed4; // 1
  double _move1; // 9
  double _move2; // 8

};

void Plasma::_init(int wdt, int hgt) {
  geo.w = wdt;
  geo.h = hgt;
  geo.bpp = 32;
  geo.size = geo.w*geo.h*(geo.bpp/8);
  geo.pitch = geo.w*(geo.bpp/8);
}

Plasma::Plasma(int wdt, int hgt) {

  register_param(speed1, "1_speed", " ");
  register_param(speed2, "2_speed", " ");
  register_param(speed3, "3_speed", " ");
  register_param(speed4, "4_speed", " ");

  register_param(move1, "1_move", " ");
  register_param(move2, "2_move", " ");
  
  
  int i;
  float rad;

  _init(wdt, hgt);

 
  /*create sin lookup table */
  for (i = 0; i < 512; i++)
    {
      /* 360 / 512 * degree to rad,
	 360 degrees spread over 512 values
	 to be able to use AND 512-1 instead
	 of using modulo 360*/
      rad =  ((float)i * 0.703125) * 0.0174532;

      /*using fixed point math with 1024 as base*/
      aSin[i] = sin(rad) * 1024;
      
    }
  
  /* create palette */
  for (i = 0; i < 64; ++i)
    {
      colors[i].r = i << 2;
      colors[i].g = 255 - ((i << 2) + 1); 
      colors[i+64].r = 255;
      colors[i+64].g = (i << 2) + 1;
      colors[i+128].r = 255 - ((i << 2) + 1);
      colors[i+128].g = 255 - ((i << 2) + 1);
      colors[i+192].g = (i << 2) + 1; 
    } 

  speed1 = 1.;
  speed2 = 1.;
  speed3 = 1.;
  speed4 = 1.;
  move1  = 1.;
  move2  = 1.;
  
  _speed1 = 5;
  _speed2 = 3;
  _speed3 = 3;
  _speed4 = 1;
  _move1  = 9;
  _move2  = 8;

}

Plasma::~Plasma() {

}

void Plasma::update() {
  uint16_t i, j;
  uint8_t index;
  int x;

  uint32_t *image = (uint32_t*)out;

  // number parameters are not good in frei0r
  // we need types defining multipliers and min/max values
  // so for now we leave this generator like this
  // but TODO ASAP new parameter types: fader and minmax
  // then fix also parameters of the partik0l which is suffering
  // of the same problem - jaromil

  // update parameters
  _speed1 = _speed1 * speed1;
  _speed2 = _speed2 * speed2;
  _speed3 = _speed3 * speed3;
  _speed4 = _speed4 * speed4;
  
  _move1 = _move1 * move1;
  _move2 = _move2 * move2;

  tpos4 = pos4;
  tpos3 = pos3;
  
  for (i = 0; i < geo.h; ++i) {
    tpos1 = pos1 + (int)_speed1;
    tpos2 = pos2 + (int)_speed2;
    
    tpos3 &= 511;
    tpos4 &= 511;
    
    for (j = 0; j < geo.w; ++j) {
      tpos1 &= 511;
      tpos2 &= 511;
      
      x = aSin[tpos1] + aSin[tpos2] + aSin[tpos3] + aSin[tpos4];
      /*actual plasma calculation*/
      
      index = 128 + (x >> 4);
      /* fixed point multiplication but optimized so basically
	 it  says (x  * (64  * 1024)  / (1024  * 1024)),  x is
	 already multiplied by 1024*/
      
      *image++ = palette2rgb(index);
      
      tpos1 += (int)_speed1; 
      tpos2 += (int)_speed2; 
    }
    
    tpos4 += (int)_speed3;
    tpos3 += (int)_speed4;
  }
  
  /* move plasma */
  
  pos1 += (int)_move1;
  pos3 += (int)_move2;
}

uint32_t Plasma::palette2rgb(uint8_t idx) {
  uint32_t rgba;
  rgba = 0xffffffff;
  // just for little endian
  // TODO: big endian
  rgba = (colors[idx].r << 16) 
    | (colors[idx].g << 8)
    | (colors[idx].b );
  
  return rgba;
}

frei0r::construct<Plasma> plugin("Plasma",
				   "Demo scene 8bit plasma",
				   "Jaromil",
				   0,2);
