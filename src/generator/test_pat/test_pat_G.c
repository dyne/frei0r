/*
 * test_pat_G
 * This frei0r plugin generates geometry test pattern images
 * Version 0.1	may 2010
 *
 * Copyright (C) 2010  Marko Cebokli    http://lea.hamradio.si/~s57uuu
 *
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
 *
 */

/***********************************************************
 * Test patterns: Geometry
 *
 * This plugin draws a set of test patterns, which are useful for image geometry
 * checking.
 *
 * The patterns are drawn into a temporary char array, for two reasons:
 * 1. drawing routines are color model independent,
 * 2. drawing is done only when a parameter changes.
 *
 * only the functions make_char2color_table(), quadrants() and
 * f0r_update() need to care about color models, endianness,
 * DV legality etc.
 *
 *************************************************************/

//compile:	gcc -Wall -c -fPIC test_pat_G.c -o test_pat_G.o

//link: gcc -lm -shared -o test_pat_G.so test_pat_G.o

#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include "frei0r.h"


//----------------------------------------------------------
void draw_rectangle(unsigned char *sl, int w, int h, int x, int y, int wr, int hr, unsigned char gray)
{
  int i, j;
  int zx, kx, zy, ky;

  zx = x;  if (zx < 0)
  {
    zx = 0;
  }
  zy = y;  if (zy < 0)
  {
    zy = 0;
  }
  kx = x + wr;  if (kx > w)
  {
    kx = w;
  }
  ky = y + hr;  if (ky > h)
  {
    ky = h;
  }
  for (i = zy; i < ky; i++)
  {
    for (j = zx; j < kx; j++)
    {
      sl[w * i + j] = gray;
    }
  }
}

//-----------------------------------------------------------
//pocasna za velike kroge.....
void draw_circle(unsigned char *sl, int w, int h, float ar, int x, int y, int rn, int rz, unsigned char gray)
{
  int   i, j;
  int   zx, kx, zy, ky;
  float rr, rmin, rmax;

  zx = x - rz / ar - 1;  if (zx < 0)
  {
    zx = 0;
  }
  zy = y - rz - 1;  if (zy < 0)
  {
    zy = 0;
  }
  kx = x + rz / ar + 1;  if (kx >= w)
  {
    kx = w - 1;
  }
  ky = y + rz + 1;  if (ky >= h)
  {
    ky = h - 1;
  }
  rmin = (float)rn;
  rmax = (float)rz;
  for (i = zy; i < ky; i++)
  {
    for (j = zx; j < kx; j++)
    {
      rr = sqrtf((i - y) * (i - y) + (j - x) * (j - x) * ar * ar);
      if ((rr >= rmin) && (rr <= rmax))
      {
        sl[w * i + j] = gray;
      }
    }
  }
}

//-----------------------------------------------------------
//dir:   1=up   2=right   3=down   4=left
void draw_wedge(unsigned char *sl, int w, int h, int x, int y, int size, int dir, unsigned char gray)
{
  int i, j, ii, jj;

  switch (dir)
  {
  case 1:               //up
    for (i = 0; i < size; i++)
    {
      for (j = 0; j <= i; j++)
      {
        ii = y + i; if (ii >= h)
        {
          ii = h - 1;
        }
        jj = x + j; if (jj >= w)
        {
          jj = w - 1;
        }
        sl[w * ii + jj] = gray;
        jj = x - j; if (jj < 0)
        {
          jj = 0;
        }
        sl[w * ii + jj] = gray;
      }
    }
    break;

  case 2:               //right
    for (i = 0; i < size; i++)
    {
      for (j = 0; j <= i; j++)
      {
        ii = x - i; if (ii < 0)
        {
          ii = 0;
        }
        jj = y + j; if (jj >= h)
        {
          jj = h - 1;
        }
        sl[w * jj + ii] = gray;
        jj = y - j; if (jj < 0)
        {
          jj = 0;
        }
        sl[w * jj + ii] = gray;
      }
    }
    break;

  case 3:               //down
    for (i = 0; i < size; i++)
    {
      for (j = 0; j <= i; j++)
      {
        ii = y - i; if (ii < 0)
        {
          ii = 0;
        }
        jj = x + j; if (jj >= w)
        {
          jj = w - 1;
        }
        sl[w * ii + jj] = gray;
        jj = x - j; if (jj < 0)
        {
          jj = 0;
        }
        sl[w * ii + jj] = gray;
      }
    }
    break;

  case 4:               //left
    for (i = 0; i < size; i++)
    {
      for (j = 0; j <= i; j++)
      {
        ii = x + i; if (ii >= w)
        {
          ii = w - 1;
        }
        jj = y + j; if (jj >= h)
        {
          jj = h - 1;
        }
        sl[w * jj + ii] = gray;
        jj = y - j; if (jj < 0)
        {
          jj = 0;
        }
        sl[w * jj + ii] = gray;
      }
    }
    break;

  default:
    break;
  }
}

//----------------------------------------------------------
//draws a checkerboard pattern
//size = size of squares (pixels)
//ar = pixel aspect ratio
//rim : 0=uniform	1=gray rim
void sah1(unsigned char *sl, int w, int h, int size, float ar, int rim)
{
  int           i, j, kx, ky, z, pv, ps, zv, zs;
  unsigned char black, gray1, gray2, white;
  int           ox, oy;

  if (size < 1)
  {
    size = 1;
  }
  kx = size; ky = size;
  kx = kx / ar;         //kao aspect!=1  (anamorph)

  black = 0;
  white = 255;
  gray1 = black + (white - black) * 0.3;
  gray2 = black + (white - black) * 0.7;

  ox = kx * 2 - (w / 2) % (kx * 2); //centering offset
  oy = ky * 2 - (h / 2) % (ky * 2);
  ps = (w / 2) % kx; if (ps == 0)
  {
    ps = kx;
  }
  pv = (h / 2) % ky; if (pv == 0)
  {
    pv = ky;
  }
  zv = h - pv;
  zs = w - ps;

  if (rim == 0)
  {
    for (i = 0; i < h; i++)
    {
      for (j = 0; j < w; j++)
      {
        if ((((i + oy) / ky) % 2) ^ (((j + ox) / kx) % 2))
        {
          sl[i * w + j] = white;
        }
        else
        {
          sl[i * w + j] = black;
        }
      }
    }
  }
  else
  {
    for (i = 0; i < h; i++)
    {
      for (j = 0; j < w; j++)
      {
        z = 0;
        if ((j < ps) || (j >= zs) || (i < pv) || (i >= zv))
        {
          z = 1;
        }
        if ((((i + oy) / ky) % 2) ^ (((j + ox) / kx) % 2))
        {
          sl[i * w + j] = (z == 0) ? white : gray2;
        }
        else
        {
          sl[i * w + j] = (z == 0) ? black : gray1;
        }
      }
    }
  }
}

//-------------------------------------------------
//draws horizontal lines
//clr=clear background
void hlines(unsigned char *sl, int w, int h, int size1, int size2, float ar, int clr)
{
  int           i, iz;
  unsigned char black, white;

  black = 0;
  white = 255;

  if (clr != 0)
  {
    for (i = 0; i < (w * h); i++)
    {
      sl[i] = black;                            //black background
    }
  }
  if (size1 < 1)
  {
    size1 = 1;
  }
  if (size2 < 1)
  {
    size2 = 1;
  }
  iz = h / 2 - size1 * ((h / 2) / size1);
  for (i = iz; i < h; i = i + size1) //hor. lines
  {
    draw_rectangle(sl, w, h, 0, i - size2 / 2, w, size2, white);
  }
}

//-------------------------------------------------
//draws vertical lines
//clr=clear background
void vlines(unsigned char *sl, int w, int h, int size1, int size2, float ar, int clr)
{
  int i, iz;
  unsigned char black, white;

  black = 0;
  white = 255;

  if (clr != 0)
  {
    for (i = 0; i < (w * h); i++)
    {
      sl[i] = black;                            //black background
    }
  }
  if (size1 < 1)
  {
    size1 = 1;
  }
  if (size2 < 1)
  {
    size2 = 1;
  }
  if (ar == 0)
  {
    ar = 1.0;
  }
  size1 = size1 / ar;
  iz    = w / 2 - size1 * ((w / 2) / size1);
  for (i = iz; i < w; i = i + size1) //vert. lines
  {
    draw_rectangle(sl, w, h, i - size2 / 2, 0, size2, h, white);
  }
}

//-------------------------------------------------
//draws a rectangular grid pattern
void mreza(unsigned char *sl, int w, int h, int size1, int size2, float ar)
{
  if (ar == 0)
  {
    ar = 1.0;
  }
  hlines(sl, w, h, size1, size2, ar, 1);
  vlines(sl, w, h, size1 / ar, size2, ar, 0);
}

//-------------------------------------------------
//draws points (small squares really)
void pike(unsigned char *sl, int w, int h, int size1, int size2, float ar)
{
  int i, j, iz, jz;
  int black, white;
  int size1a, size2a;

  black = 0;
  white = 255;

  for (i = 0; i < (w * h); i++)
  {
    sl[i] = black;                      //black background
  }
  if (size1 < 1)
  {
    size1 = 1;
  }
  if (size2 < 1)
  {
    size2 = 1;
  }
  if (ar == 0)
  {
    ar = 1.0;
  }
  size1a = size1 / ar; size2a = size2 / ar;
  iz     = h / 2 - size1 * ((h / 2) / size1);
  jz     = w / 2 - size1a * ((w / 2) / size1a);
  for (i = iz; i < h; i = i + size1)
  {
    for (j = jz; j < w; j = j + size1a)
    {
      draw_rectangle(sl, w, h, j - size2 / 2, i - size2 / 2, size2a, size2, white);
    }
  }
}

//-----------------------------------------------------------
//draws a bullseye pattern
void tarca(unsigned char *sl, int w, int h, int size1, int size2, float ar)
{
  unsigned char black, white;
  int i;

  black = 0;
  white = 255;

  for (i = 0; i < (w * h); i++)
  {
    sl[i] = black;                      //black background
  }
  if (size1 < 20)
  {
    size1 = 20;                 //otherwise too slooooow...
  }
  draw_circle(sl, w, h, ar, w / 2, h / 2, 0, size2 / 2, white);
  for (i = size1; i < (h / 2); i = i + size1)
  {
    draw_circle(sl, w, h, ar, w / 2, h / 2, i - size2 / 2, i + size2 / 2, white);
  }
}

//---------------------------------------------------------
//draws image edge markers
void robovi(unsigned char *sl, int w, int h)
{
  int i, j, l;
  unsigned char black, white;

  black = 0;
  white = 255;

  for (i = 0; i < (w * h); i++)
  {
    sl[i] = black;                      //black background
  }
  draw_wedge(sl, w, h, w / 4, 0, 50, 1, white);
  draw_wedge(sl, w, h, 3 * w / 4, 0, 50, 1, white);

  draw_wedge(sl, w, h, w - 1, h / 4, 50, 2, white);
  draw_wedge(sl, w, h, w - 1, 3 * h / 4, 50, 2, white);

  draw_wedge(sl, w, h, w / 4, h - 1, 50, 3, white);
  draw_wedge(sl, w, h, 3 * w / 4, h - 1, 50, 3, white);

  draw_wedge(sl, w, h, 0, h / 4, 50, 4, white);
  draw_wedge(sl, w, h, 0, 3 * h / 4, 50, 4, white);

  for (i = 0; i < 50; i++)
  {
    l = (i % 10) * 10 + 10;
    for (j = w / 2 - 50; j < w / 2 - 50 + l; j++)
    {
      sl[i * w + w - j - 1]   = white;
      sl[(h - i - 1) * w + j] = white;
    }
    for (j = h / 2 - 50; j < h / 2 - 50 + l; j++)
    {
      sl[j * w + i] = white;
      sl[(h - j - 1) * w + w - i - 1] = white;
    }
  }
}

//----------------------------------------------------------
//draws centered pixel rulers
void rulers(unsigned char *sl, int w, int h, unsigned char *a)
{
  int i;
  unsigned char black, white, tr;

  black = 0;
  white = 255;
  tr    = 200;  //how opaque are the marks

  for (i = 0; i < (w * h); i++)
  {
    sl[i] = black;                      //black background
  }
  for (i = 0; i < (w * h); i++)
  {
    a[i] = 0;                           //transparent
  }
  for (i = w / 2; i < w; i = i + 2)
  {
    draw_rectangle(sl, w, h, i, h / 2, 1, 1, white);
    draw_rectangle(sl, w, h, w - i, h / 2 - 1, 1, 1, white);
    draw_rectangle(a, w, h, i, h / 2, 1, 1, tr);
    draw_rectangle(a, w, h, w - i, h / 2 - 1, 1, 1, tr);
  }

  for (i = w / 2 + 10; i < w; i = i + 10)
  {
    draw_rectangle(sl, w, h, i, h / 2, 1, 3, white);
    draw_rectangle(sl, w, h, w - i, h / 2 - 3, 1, 3, white);
    draw_rectangle(a, w, h, i, h / 2, 1, 3, tr);
    draw_rectangle(a, w, h, w - i, h / 2 - 3, 1, 3, tr);
  }

  for (i = w / 2 + 50; i < w; i = i + 50)
  {
    draw_rectangle(sl, w, h, i, h / 2, 1, 5, white);
    draw_rectangle(sl, w, h, w - i, h / 2 - 5, 1, 5, white);
    draw_rectangle(a, w, h, i, h / 2, 1, 5, tr);
    draw_rectangle(a, w, h, w - i, h / 2 - 5, 1, 5, tr);
  }

  for (i = w / 2 + 100; i < w; i = i + 100)
  {
    draw_rectangle(sl, w, h, i, h / 2, 1, 10, white);
    draw_rectangle(sl, w, h, w - i, h / 2 - 10, 1, 10, white);
    draw_rectangle(a, w, h, i, h / 2, 1, 10, tr);
    draw_rectangle(a, w, h, w - i, h / 2 - 10, 1, 10, tr);
  }

  for (i = h / 2; i < h; i = i + 2)
  {
    draw_rectangle(sl, w, h, w / 2 - 1, i, 1, 1, white);
    draw_rectangle(sl, w, h, w / 2, h - i, 1, 1, white);
    draw_rectangle(a, w, h, w / 2 - 1, i, 1, 1, tr);
    draw_rectangle(a, w, h, w / 2, h - i, 1, 1, tr);
  }

  for (i = h / 2 + 10; i < h; i = i + 10)
  {
    draw_rectangle(sl, w, h, w / 2 - 3, i, 3, 1, white);
    draw_rectangle(sl, w, h, w / 2, h - i, 3, 1, white);
    draw_rectangle(a, w, h, w / 2 - 3, i, 3, 1, tr);
    draw_rectangle(a, w, h, w / 2, h - i, 3, 1, tr);
  }

  for (i = h / 2 + 50; i < h; i = i + 50)
  {
    draw_rectangle(sl, w, h, w / 2 - 5, i, 5, 1, white);
    draw_rectangle(sl, w, h, w / 2, h - i, 5, 1, white);
    draw_rectangle(a, w, h, w / 2 - 5, i, 5, 1, tr);
    draw_rectangle(a, w, h, w / 2, h - i, 5, 1, tr);
  }

  for (i = h / 2 + 100; i < h; i = i + 100)
  {
    draw_rectangle(sl, w, h, w / 2 - 10, i, 10, 1, white);
    draw_rectangle(sl, w, h, w / 2, h - i, 10, 1, white);
    draw_rectangle(a, w, h, w / 2 - 10, i, 10, 1, tr);
    draw_rectangle(a, w, h, w / 2, h - i, 10, 1, tr);
  }
}

//----------------------------------------------------------
//draws a transparent mesurement grid
//*a = alpha channel
void grid(unsigned char *sl, int w, int h, unsigned char *a)
{
  int i, j;
  unsigned char black, white, tr;

  black = 0;
  white = 255;
  tr    = 200;  //how opaque are the marks

  for (i = 0; i < (w * h); i++)
  {
    sl[i] = black;                      //black background
  }
  for (i = 0; i < (w * h); i++)
  {
    a[i] = 0;                           //transparent
  }
  for (i = 0; i < h; i = i + 2)
  {
    for (j = 0; j < w; j = j + 10)
    {
      sl[i * w + j] = white;
      a[i * w + j]  = tr;
    }
  }
  for (i = 0; i < h; i = i + 10)
  {
    for (j = 0; j < w; j = j + 2)
    {
      sl[i * w + j] = white;
      a[i * w + j]  = tr;
    }
  }

  for (i = 0; i < h; i = i + 50)
  {
    for (j = 0; j < w; j = j + 50)
    {
      draw_rectangle(sl, w, h, i, j - 1, 1, 3, white);
      draw_rectangle(sl, w, h, i - 1, j, 3, 1, white);
      draw_rectangle(a, w, h, i, j - 1, 1, 3, tr);
      draw_rectangle(a, w, h, i - 1, j, 3, 1, tr);
    }
  }

  for (i = 0; i < h; i = i + 100)
  {
    for (j = 0; j < w; j = j + 100)
    {
      draw_rectangle(sl, w, h, i, j - 4, 1, 9, white);
      draw_rectangle(sl, w, h, i - 4, j, 9, 1, white);
      draw_rectangle(sl, w, h, i - 1, j - 1, 3, 3, white);
      draw_rectangle(a, w, h, i, j - 4, 1, 9, tr);
      draw_rectangle(a, w, h, i - 4, j, 9, 1, tr);
      draw_rectangle(a, w, h, i - 1, j - 1, 3, 3, tr);
    }
  }
}

//----------------------------------------------------
//marks the four quadrants with different colors
//COLOR MODEL DEPENDENT!
void kvadranti(uint32_t *sl, int w, int h, int neg)
{
  uint32_t c1, c2, c3, c4;
  int i, j;

//RGBA8888 little endian, opaque
  if (neg == 0)
  {
    c1 = 0xFF10F010;    //green
    c2 = 0xFF10F0F0;    //yellow
    c3 = 0xFFF01010;    //blue
    c4 = 0xFF1010F0;    //red
  }
  else
  {
    c1 = 0xFFF010F0;    //magenta
    c2 = 0xFFF01010;    //blue
    c3 = 0xFF10F0F0;    //yellow
    c4 = 0xFFF0F010;    //cyan
  }

  for (i = 0; i < h / 2; i++)
  {
    for (j = 0; j < w / 2; j++)
    {
      sl[i * w + j] = c1;
    }
    for (j = w / 2; j < w; j++)
    {
      sl[i * w + j] = c2;
    }
  }
  for (i = h / 2; i < h; i++)
  {
    for (j = 0; j < w / 2; j++)
    {
      sl[i * w + j] = c3;
    }
    for (j = w / 2; j < w; j++)
    {
      sl[i * w + j] = c4;
    }
  }
}

//-----------------------------------------------------
//makes a table used for conversion of the char*
//intermediate image into the required color model
//alpha = 0  (transparent!)
//COLOR MODEL DEPENDENT!
void make_char2color_table(uint32_t *c2c, int neg)
{
  unsigned int i;

  if (neg == 0)
  {
    for (i = 0; i < 256; i++)
    {
      c2c[i] = ((i & 0xFF) << 0) + ((i & 0xFF) << 8) + ((i & 0xFF) << 16);
    }
  }
  else
  {
    for (i = 0; i < 256; i++)
    {
      c2c[255 - i] = ((i & 0xFF) << 0) + ((i & 0xFF) << 8) + ((i & 0xFF) << 16);
    }
  }
}

//-----------------------------------------------------
//stretch [0...1] to parameter range [min...max] linear
float map_value_forward(double v, float min, float max)
{
  return (min + (max - min) * v);
}

//-----------------------------------------------------
//collapse from parameter range [min...max] to [0...1] linear
double map_value_backward(float v, float min, float max)
{
  return ((v - min) / (max - min));
}

//-----------------------------------------------------
//stretch [0...1] to parameter range [min...max] logarithmic
//min and max must be positive!
float map_value_forward_log(double v, float min, float max)
{
  float sr, k;

  sr = sqrtf(min * max);
  k  = 2.0 * log(max / sr);
  return (sr * expf(k * (v - 0.5)));
}

//-----------------------------------------------------
//collapse from parameter range [min...max] to [0...1] logarithmic
//min and max must be positive!
double map_value_backward_log(float v, float min, float max)
{
  float sr, k;

  sr = sqrtf(min * max);
  k  = 2.0 * log(max / sr);
  return (logf(v / sr) / k + 0.5);
}

//**************************************************
//obligatory frei0r stuff follows

//------------------------------------------------
//this structure holds an instance of the test_pat_G plugin
typedef struct
{
  unsigned int w;
  unsigned int h;

  int type;
  int size1;
  int size2;
  int aspt;
  float mpar;
  int neg;

  float par;
  unsigned char *sl;
  unsigned char *alpha;
  uint32_t *c2c;
} tp_inst_t;

//----------------------------------------------------
int f0r_init()
{
  return (1);
}

//--------------------------------------------------
void f0r_deinit()
{ /* no initialization required */
}

//--------------------------------------------------
void f0r_get_plugin_info(f0r_plugin_info_t *tp_info)
{
  tp_info->name        = "test_pat_G";
  tp_info->author      = "Marko Cebokli";
  tp_info->plugin_type = F0R_PLUGIN_TYPE_SOURCE;
//  tp_info->plugin_type    = F0R_PLUGIN_TYPE_FILTER;
  tp_info->color_model    = F0R_COLOR_MODEL_RGBA8888;
  tp_info->frei0r_version = FREI0R_MAJOR_VERSION;
  tp_info->major_version  = 0;
  tp_info->minor_version  = 2;
  tp_info->num_params     = 6;
  tp_info->explanation    = "Generates geometry test pattern images";
}

//--------------------------------------------------
void f0r_get_param_info(f0r_param_info_t *info, int param_index)
{
  switch (param_index)
  {
  case 0:
    info->name        = "Type";
    info->type        = F0R_PARAM_DOUBLE;
    info->explanation = "Type of test pattern"; break;

  case 1:
    info->name        = "Size 1";
    info->type        = F0R_PARAM_DOUBLE;
    info->explanation = "Size of major features"; break;

  case 2:
    info->name        = "Size 2";
    info->type        = F0R_PARAM_DOUBLE;
    info->explanation = "Size of minor features"; break;

  case 3:
    info->name        = "Negative";
    info->type        = F0R_PARAM_BOOL;
    info->explanation = "Polarity of image"; break;

  case 4:
    info->name        = "Aspect type";
    info->type        = F0R_PARAM_DOUBLE;
    info->explanation = "Pixel aspect ratio presets";
    break;

  case 5:
    info->name        = "Manual Aspect";
    info->type        = F0R_PARAM_DOUBLE;
    info->explanation = "Manual pixel aspect ratio";
    break;
  }
}

//--------------------------------------------------
f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
  tp_inst_t *inst = calloc(1, sizeof(*inst));
  inst->w = width;
  inst->h = height;

  inst->type  = 0;
  inst->size1 = 72;
  inst->size2 = 4;
  inst->aspt  = 0;
  inst->mpar  = 1.0;
  inst->neg   = 0;

  inst->par   = 1.0;
  inst->sl    = (unsigned char *)calloc(width * height, 1);
  inst->alpha = (unsigned char *)calloc(width * height, 1);
  inst->c2c   = (uint32_t *)calloc(256, sizeof(uint32_t));

  make_char2color_table(inst->c2c, inst->neg);
  sah1(inst->sl, inst->w, inst->h, inst->size1, inst->par, 0);

  return ((f0r_instance_t)inst);
}

//--------------------------------------------------
void f0r_destruct(f0r_instance_t instance)
{
  tp_inst_t *inst = (tp_inst_t *)instance;

  free(inst->sl);
  free(inst->alpha);
  free(inst->c2c);
  free(inst);
}

//--------------------------------------------------
void f0r_set_param_value(f0r_instance_t instance, f0r_param_t param, int param_index)
{
  tp_inst_t *inst = (tp_inst_t *)instance;

  f0r_param_double *p = (f0r_param_double *)param;

  int chg, tmpi;
  float tmpf;

  chg = 0;
  switch (param_index)
  {
  case 0:       //type
    tmpf = *((double *)p);
    if (tmpf >= 1.0)
    {
      tmpi = (int)tmpf;
    }
    else
    {
      tmpi = map_value_forward(tmpf, 0.0, 12.9999);
    }
    if ((tmpi < 0) || (tmpi > 12.0))
    {
      break;
    }
    if (inst->type != tmpi)
    {
      chg = 1;
    }
    inst->type = tmpi;
    break;

  case 1:       //size 1
    tmpi = map_value_forward(*((double *)p), 0.0, 256.0);
    if (inst->size1 != tmpi)
    {
      chg = 1;
    }
    inst->size1 = tmpi;
    break;

  case 2:       //size 2
    tmpi = map_value_forward(*((double *)p), 0.0, 64.0);
    if (inst->size2 != tmpi)
    {
      chg = 1;
    }
    inst->size2 = tmpi;
    break;

  case 3:       //negative
    tmpi = map_value_forward(*((double *)p), 0.0, 1.0);
    if (inst->neg != tmpi)
    {
      chg = 1;
    }
    inst->neg = tmpi;
    make_char2color_table(inst->c2c, inst->neg);
    break;

  case 4:       //aspect type
    tmpf = *((double *)p);
    if (tmpf >= 1.0)
    {
      tmpi = (int)tmpf;
    }
    else
    {
      tmpi = map_value_forward(*((double *)p), 0.0, 6.9999);
    }
    if ((tmpi < 0) || (tmpi > 6.0))
    {
      break;
    }
    if (inst->aspt != tmpi)
    {
      chg = 1;
    }
    inst->aspt = tmpi;
    switch (inst->aspt)                    //pixel aspect ratio
    {
    case 0: inst->par = 1.000; break;      //square pixels

    case 1: inst->par = 1.067; break;      //PAL DV

    case 2: inst->par = 1.455; break;      //PAL wide

    case 3: inst->par = 0.889; break;      //NTSC DV

    case 4: inst->par = 1.212; break;      //NTSC wide

    case 5: inst->par = 1.333; break;      //HDV

    case 6: inst->par = inst->mpar; break; //manual
    }
    break;

  case 5:       //manual aspect
    tmpf = map_value_forward_log(*((double *)p), 0.5, 2.0);
    if (inst->mpar != tmpf)
    {
      chg = 1;
    }
    inst->mpar = tmpf;
    if (inst->aspt == 4)
    {
      inst->par = inst->mpar;
    }
    break;
  }

  if (chg == 0)
  {
    return;
  }

  switch (inst->type)
  {
  case 0:               //checkerboard
    sah1(inst->sl, inst->w, inst->h, inst->size1, inst->par, 0);
    break;

  case 1:               //checkerboard with border
    sah1(inst->sl, inst->w, inst->h, inst->size1, inst->par, 1);
    break;

  case 2:               //horizontal lines
    hlines(inst->sl, inst->w, inst->h, inst->size1, inst->size2, inst->par, 1);
    break;

  case 3:               //vertical lines
    vlines(inst->sl, inst->w, inst->h, inst->size1, inst->size2, inst->par, 1);
    break;

  case 4:               //grid
    mreza(inst->sl, inst->w, inst->h, inst->size1, inst->size2, inst->par);
    break;

  case 5:               //points
    pike(inst->sl, inst->w, inst->h, inst->size1, inst->size2, inst->par);
    break;

  case 6:               //bullseye
    tarca(inst->sl, inst->w, inst->h, inst->size1, inst->size2 + 1, inst->par);
    break;

  case 7:               //edge marks
    robovi(inst->sl, inst->w, inst->h);
    break;

  case 8:               //color quadrants  are drawn in update()
    break;

  case 9:               //pixel rulers
  case 11:
    rulers(inst->sl, inst->w, inst->h, inst->alpha);
    break;

  case 10:              //measurement grid
  case 12:
    grid(inst->sl, inst->w, inst->h, inst->alpha);
    break;

  default:
    break;
  }
}

//-------------------------------------------------
void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index)
{
  tp_inst_t *inst = (tp_inst_t *)instance;

  f0r_param_double *p = (f0r_param_double *)param;

  switch (param_index)
  {
  case 0:       //type
    *p = map_value_backward(inst->type, 0.0, 12.9999);
    break;

  case 1:       //size 1
    *p = map_value_backward(inst->size1, 0.0, 256.0);
    break;

  case 2:       //size 2
    *p = map_value_backward(inst->size2, 0.0, 64.0);
    break;

  case 3:       //negative
    *p = map_value_backward(inst->neg, 0.0, 1.0);
    break;

  case 4:       //aspect type
    *p = map_value_backward(inst->aspt, 0.0, 6.9999);
    break;

  case 5:       //manual aspect
    *p = map_value_backward_log(inst->mpar, 0.5, 2.0);
    break;
  }
}

//---------------------------------------------------
//COLOR MODEL DEPENDENT
void f0r_update(f0r_instance_t instance, double time, const uint32_t *inframe, uint32_t *outframe)
{
  int i;

  assert(instance);
  tp_inst_t *inst = (tp_inst_t *)instance;

  switch (inst->type)
  {
  case 0:
  case 1:
  case 2:
  case 3:
  case 4:
  case 5:
  case 6:
  case 7:
  case 9:
  case 10:
    for (i = 0; i < (inst->h * inst->w); i++)
    {
      outframe[i] = 0xFF000000 | inst->c2c[inst->sl[i]];
    }
    break;

  case 8:
    kvadranti(outframe, inst->w, inst->h, inst->neg);
    break;

  case 11:
  case 12:
    for (i = 0; i < (inst->h * inst->w); i++)
    {
      outframe[i] = ((uint32_t)inst->alpha[i]) << 24 | inst->c2c[inst->sl[i]];
    }
    break;

  default:
    break;
  }
}
