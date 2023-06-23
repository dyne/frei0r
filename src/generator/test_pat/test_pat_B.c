/*
 * test_pat_B
 * This frei0r plugin generates TV test card approximations
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
 * Test patterns: Broadcast test cards
 *
 * This plugin draws a set of test patterns, similar to popular
 * TV test cards
 *
 * The patterns are drawn into a temporary float array, for two reasons:
 * 1. drawing routines are color model independent,
 * 2. drawing is done only when a parameter changes.
 *
 * only the function floatrgba2color()
 * needs to care about color models, endianness, DV legality etc.
 *
 *************************************************************/

//compile:	gcc -Wall -c -fPIC test_pat_B.c -o test_pat_B.o

//link: gcc -lm -shared -o test_pat_B.so test_pat_B.o

#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include "frei0r.h"



double PI = 3.14159265358979;

typedef struct
{
  float r;
  float g;
  float b;
  float a;
} float_rgba;


//------------------------------------------------------------------
void draw_rectangle(float_rgba *s, int w, int h, float x, float y, float wr, float hr, float_rgba c)
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
      s[w * i + j] = c;
    }
  }
}

//-----------------------------------------------------------
//pocasna za velike kroge.....
void draw_circle(float_rgba *sl, int w, int h, float ar, int x, int y, int rn, int rz, float_rgba c)
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
  kx = x + rz / ar + 1;  if (kx > w)
  {
    kx = w;
  }
  ky = y + rz + 1;  if (ky > h)
  {
    ky = h;
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
        sl[w * i + j] = c;
      }
    }
  }
}

//------------------------------------------------------------------
//ar = pixel aspect ratio
//x,y = center
//xb,yb = upper left corner of box
//vx,vy = size of box
void draw_boxed_circle(float_rgba *s, int w, int h, float x, float y, float r, float xb, float yb, float vx, float vy, float ar, float_rgba c)
{
  int   xz, yz, xk, yk;
  int   i, j;
  float rr;

  xz = x - r / ar - 1; if (xz < 0)
  {
    xz = 0;
  }
  if (xz < xb)
  {
    xz = xb;
  }
  xk = x + r / ar + 1; if (xk > w)
  {
    xk = w;
  }
  if (xk > (xb + vx))
  {
    xk = xb + vx;
  }
  yz = y - r - 1; if (yz < 0)
  {
    yz = 0;
  }
  if (yz < yb)
  {
    yz = yb;
  }
  yk = y + r + 1; if (yk > h)
  {
    yk = h;
  }
  if (yk > (yb + vy))
  {
    yk = yb + vy;
  }

  for (i = yz; i < yk; i++)
  {
    for (j = xz; j < xk; j++)
    {
      rr = sqrtf((i - y) * (i - y) + (j - x) * (j - x) * ar * ar);
      if (rr < r)
      {
        s[i * w + j] = c;
      }
    }
  }
}

//---------------------------------------------------------
//draw an approximation of the Philips PM5544 test pattern
//ar = pixel aspect ratio
void draw_pm(float_rgba *s, int w, int h, float ar)
{
  float_rgba c;
  int        vky, vkx, nkx, x0, y0, x0d, y0d;
  int        i, j;
  float      rk, v1, v2;
  float      f, df; //faza in korak faze za rastre
  float      a;

//doloci velikost kocke po y tako, da jih bo po visini 13.x
//z malo rezerve...
  vky = (h - 25) / 13;
//velikost koceke po x prilagodimo glede na frame in pix aspect
  vkx = (int)rintf((float)vky / ar);
  nkx = w / vkx - 1;         //stevilo kock po x  (naredi da bo vedno liho???)
  x0  = (w - vkx * nkx) / 2; //zacetni offset za centriranje
  y0  = (h - vky * 13) / 2;
  x0d = x0; y0d = y0;        //ostanek na desni in dol
  if ((w - vkx * nkx) % 2 == 1)
  {
    x0d++;
  }
  if ((h - vky * 13) % 2 == 1)
  {
    y0d++;
  }
  rk  = 6.0 * vky;
  c.a = 1.0;

//ozadje
  c.r = 0.45; c.g = 0.45; c.b = 0.45;
  for (i = 0; i < w * h; i++)
  {
    s[i] = c;
  }

//narisi mrezo
  c.r = 0.92; c.g = 0.92; c.b = 0.92;
  for (i = x0; i < w; i = i + vkx)
  {
    draw_rectangle(s, w, h, i - 1, 0, 3, h, c);
  }
  for (i = y0; i < h; i = i + vky)
  {
    draw_rectangle(s, w, h, 0, i - 1, w, 3, c);
  }

//robovi
  c.r = 0.0; c.g = 0.0; c.b = 0.0;
  for (i = x0; i < w; i = i + 2 * vkx)
  {
    draw_rectangle(s, w, h, i + 2, 0, vkx - 3, y0 - 1, c);            //zgoraj
    draw_rectangle(s, w, h, i + 2, h - y0d + 2, vkx - 3, y0d - 2, c); //spodaj
  }
  for (i = y0; i < h; i = i + 2 * vky)
  {
    draw_rectangle(s, w, h, 0, i + 2, x0 - 1, vky - 3, c); //levo
  }
  for (i = y0; i < h; i = i + 2 * vky)
  {
    if ((nkx % 2) == 0)
    {
      c.r = 0.98; c.g = 0.98; c.b = 0.98;
    }
    draw_rectangle(s, w, h, w - x0d + 2, i + 2, x0d - 2, vky - 3, c);//desno
  }
  c.r = 0.98; c.g = 0.98; c.b = 0.98;
  for (i = x0 + vkx; i < w - vkx; i = i + 2 * vkx)
  {
    draw_rectangle(s, w, h, i + 2, 0, vkx - 3, y0 - 1, c);            //zgoraj
    draw_rectangle(s, w, h, i + 2, h - y0d + 2, vkx - 3, y0d - 2, c); //spodaj
  }
  for (i = y0 + vky; i < h - vky; i = i + 2 * vky)
  {
    draw_rectangle(s, w, h - 1, 0, i + 2, x0 - 1, vky - 3, c); //levo
  }
  for (i = y0 + vky; i < h - vky; i = i + 2 * vky)
  {
    if ((nkx % 2) == 0)
    {
      c.r = 0.0; c.g = 0.0; c.b = 0.0;
    }
    draw_rectangle(s, w, h, w - x0d + 2, i + 2, x0d - 2, vky - 3, c);//desno
  }
  c.r = 0.98; c.g = 0.98; c.b = 0.98;
  draw_rectangle(s, w, h, 0, 0, x0 - 1, y0 - 1, c);            //zg lev
  draw_rectangle(s, w, h, 0, h - y0d + 2, x0 - 1, y0d - 1, c); //sp lev
  if ((nkx % 2) == 0)
  {
    c.r = 0.0; c.g = 0.0; c.b = 0.0;
  }
  draw_rectangle(s, w, h, w - x0d + 2, 0, x0d - 1, y0 - 1, c);            //zg
                                                                          // des
  draw_rectangle(s, w, h, w - x0d + 2, h - y0d + 2, x0d - 1, y0d - 1, c); //sp
                                                                          // des

// "slusalke"
  c.r = 0.158; c.g = 0.602; c.b = 0.441;
  draw_rectangle(s, w, h, x0 + vkx + 2, y0 + vky + 2, vkx - 3, 11 * vky / 2 - 2, c);
  draw_rectangle(s, w, h, x0 + 2 * vkx - 1, y0 + vky + 2, 2, 2 * vky - 3, c);
  c.r = 0.286; c.g = 0.455; c.b = 0.861;
  draw_rectangle(s, w, h, x0 + 2 * vkx + 2, y0 + vky + 2, vkx - 3, 2 * vky - 4, c);
  draw_rectangle(s, w, h, x0 + 2 * vkx + 1, y0 + vky + 2, 2, 2 * vky - 3, c);
  c.r = 0.736; c.g = 0.307; c.b = 0.441;
  draw_rectangle(s, w, h, x0 + vkx + 2, y0 + 13 * vky / 2 + 2, vkx - 3, 11 * vky / 2 - 3, c);
  draw_rectangle(s, w, h, x0 + 2 * vkx - 1, y0 + 10 * vky + 2, 2, 2 * vky - 3, c);
  c.r = 0.609; c.g = 0.455; c.b = 0.020;
  draw_rectangle(s, w, h, x0 + 2 * vkx + 2, y0 + 10 * vky + 2, vkx - 3, 2 * vky - 4, c);
  draw_rectangle(s, w, h, x0 + 2 * vkx + 1, y0 + 10 * vky + 2, 2, 2 * vky - 3, c);
  c.r = 0.286; c.g = 0.455; c.b = 0.861;
  draw_rectangle(s, w, h, x0 + (nkx - 3) * vkx + 2, y0 + vky + 2, vkx - 3, 2 * vky - 4, c);
  draw_rectangle(s, w, h, x0 + (nkx - 2) * vkx - 1, y0 + vky + 2, 2, 2 * vky - 3, c);
  c.r = 0.451; c.g = 0.555; c.b = 0.000;
  draw_rectangle(s, w, h, x0 + (nkx - 2) * vkx + 2, y0 + vky + 2, vkx - 3, 11 * vky / 2 - 2, c);
  draw_rectangle(s, w, h, x0 + (nkx - 2) * vkx + 1, y0 + vky + 2, 2, 2 * vky - 3, c);
  c.r = 0.609; c.g = 0.455; c.b = 0.020;
  draw_rectangle(s, w, h, x0 + (nkx - 3) * vkx + 2, y0 + 10 * vky + 2, vkx - 3, 2 * vky - 4, c);
  draw_rectangle(s, w, h, x0 + (nkx - 2) * vkx - 1, y0 + 10 * vky + 2, 2, 2 * vky - 3, c);
  c.r = 0.455; c.g = 0.355; c.b = 0.957;
  draw_rectangle(s, w, h, x0 + (nkx - 2) * vkx + 2, y0 + 13 * vky / 2 + 2, vkx - 3, 11 * vky / 2 - 3, c);
  draw_rectangle(s, w, h, x0 + (nkx - 2) * vkx + 1, y0 + 10 * vky + 2, 2, 2 * vky - 3, c);

  c.r = 0.98; c.g = 0.98; c.b = 0.98;
  draw_boxed_circle(s, w, h, w / 2.0, h / 2.0, rk, 0.0, 0.0, w, y0 + 2 * vky, ar, c);
  c.r = 0.0; c.g = 0.0; c.b = 0.0;                                                                           //beli
                                                                                                             // na
                                                                                                             // vrhu
  draw_boxed_circle(s, w, h, w / 2.0, h / 2.0, rk, w / 2 - 6 * vkx, y0 + 2 * vky, 12 * vkx, 2 * vky, ar, c); //crni
                                                                                                             // visine
                                                                                                             // 2

  c.r = 0.98; c.g = 0.98; c.b = 0.98;
  draw_rectangle(s, w, h, w / 2 - 3 * vkx, y0 + 2 * vky, 6 * vkx, vky, c); //beli
                                                                           // za
                                                                           // crtico

  c.r = 0.0; c.g = 0.0; c.b = 0.0;
  draw_rectangle(s, w, h, w / 2 - 2.5 * vkx, y0 + 2 * vky, 3, vky, c); //crtica
  draw_rectangle(s, w, h, w / 2 - 2 * vkx, y0 + vky, 4 * vkx, vky, c); //title
                                                                       // zg

//izmenicni crno sivi
  c.r = 0.723; c.g = 0.723; c.b = 0.723;
  v1  = vkx * 11.09 / 16.0;
  for (i = w / 2 - 5.5 * vkx + v1; i < w / 2 + 4 * vkx; i = i + 2.0 * v1)
  {
    draw_rectangle(s, w, h, i, y0 + 3 * vky, v1, vky, c);
  }
  draw_boxed_circle(s, w, h, w / 2.0, h / 2.0, rk, w / 2 - 5.5 * vkx + 15 * v1, y0 + 3 * vky, vkx, vky, ar, c);

//barve
  v2  = rk / 3.0 / ar;
  c.r = 0.730; c.g = 0.73; c.b = 0.0;
  draw_boxed_circle(s, w, h, w / 2.0, h / 2.0, rk, w / 2 - 3 * v2, y0 + 4 * vky, v2, 2 * vky, ar, c);
  c.r = 0.0; c.g = 0.74; c.b = 0.74;
  draw_rectangle(s, w, h, w / 2 - 2 * v2, y0 + 4 * vky, v2, 2 * vky, c);
  c.r = 0.0; c.g = 0.75; c.b = 0.0;
  draw_rectangle(s, w, h, w / 2 - v2, y0 + 4 * vky, v2 + 1, 2 * vky, c);
  c.r = 0.75; c.g = 0.0; c.b = 0.75;
  draw_rectangle(s, w, h, w / 2, y0 + 4 * vky, v2, 2 * vky, c);
  c.r = 0.765; c.g = 0.0; c.b = 0.0;
  draw_rectangle(s, w, h, w / 2 + v2, y0 + 4 * vky, v2, 2 * vky, c);
  c.r = 0.0; c.g = 0.0; c.b = 0.765;
  draw_boxed_circle(s, w, h, w / 2.0, h / 2.0, rk, w / 2 + 2 * v2, y0 + 4 * vky, v2, 2 * vky, ar, c);

  c.r = 0.0; c.g = 0.0; c.b = 0.0;
  draw_boxed_circle(s, w, h, w / 2.0, h / 2.0, rk, w / 2 - 6.1 * vkx, y0 + 6 * vky, 13.2 * vkx, 4 * vky, ar, c); //crno
                                                                                                                 // spodaj,
                                                                                                                 // podloga
                                                                                                                 // za
                                                                                                                 // center,
                                                                                                                 // raster,
                                                                                                                 // sivine

//rastri
  f  = 3.14;
  df = (float)w / 42.66 / ar; //perioda v pix
  df = 6.28 / df;             //fazni korak
  for (i = (w / 2 - 2.5 * v2); i < (w / 2 - 1.5 * v2); i++)
  {
    a   = 0.47 + 0.47 * cosf(f);
    f   = f + df;
    c.r = a; c.g = a; c.b = a;
    for (j = (y0 + 7 * vky); j < (y0 + 9 * vky); j++)
    {
      s[j * w + i] = c;
    }
  }
  df = (float)w / 96.0 / ar; df = 6.28 / df; //fazni korak
  for (i = (w / 2 - 1.5 * v2); i < (w / 2 - 0.5 * v2); i++)
  {
    a   = 0.47 + 0.47 * cosf(f);
    f   = f + df;
    c.r = a; c.g = a; c.b = a;
    for (j = (y0 + 7 * vky); j < (y0 + 9 * vky); j++)
    {
      s[j * w + i] = c;
    }
  }
  df = (float)w / 149.0 / ar; df = 6.28 / df; //fazni korak
  for (i = (w / 2 - 0.5 * v2); i < (w / 2 + 0.5 * v2); i++)
  {
    a   = 0.47 + 0.47 * cosf(f);
    f   = f + df;
    c.r = a; c.g = a; c.b = a;
    for (j = (y0 + 7 * vky); j < (y0 + 9 * vky); j++)
    {
      s[j * w + i] = c;
    }
  }
  df = (float)w / 202.0 / ar; df = 6.28 / df; //fazni korak
  for (i = (w / 2 + 0.5 * v2); i < (w / 2 + 1.5 * v2); i++)
  {
    a   = 0.47 + 0.47 * cosf(f);
    f   = f + df;
    c.r = a; c.g = a; c.b = a;
    for (j = (y0 + 7 * vky); j < (y0 + 9 * vky); j++)
    {
      s[j * w + i] = c;
    }
  }
  df = (float)w / 256.0 / ar; df = 6.28 / df; //fazni korak
  for (i = (w / 2 + 1.5 * v2); i < (w / 2 + 2.5 * v2); i++)
  {
    a   = 0.47 + 0.47 * cosf(f);
    f   = f + df;
    c.r = a; c.g = a; c.b = a;
    for (j = (y0 + 7 * vky); j < (y0 + 9 * vky); j++)
    {
      s[j * w + i] = c;
    }
  }

//centralni del
  c.r = 0.0; c.g = 0.0; c.b = 0.0;
  draw_rectangle(s, w, h, w / 2 - vkx / 2, y0 + 5 * vky, vkx, 3 * vky, c);         //pokonc
                                                                                   // crni
  c.r = 0.98; c.g = 0.98; c.b = 0.98;
  draw_rectangle(s, w, h, w / 2 - rk / ar + 1, y0 + 6.5 * vky, 2 * rk / ar, 2, c); //bela
                                                                                   // crta
  for (i = (w / 2 - 5.5 * vkx - 1); i < (w / 2 + 5.5 * vkx); i = i + vkx)
  {
    draw_rectangle(s, w, h, i, y0 + 6 * vky, 2, vky, c);           //bele crtice
  }
  draw_rectangle(s, w, h, w / 2 - 1, y0 + 5 * vky, 2, 3 * vky, c); //pokonci
                                                                   // bela

//sivine
  c.r = 0.172; c.g = 0.172; c.b = 0.172;
  draw_rectangle(s, w, h, w / 2 - 2 * v2, y0 + 9 * vky, v2, vky, c);
  c.r = 0.37; c.g = 0.37; c.b = 0.37;
  draw_rectangle(s, w, h, w / 2 - v2, y0 + 9 * vky, v2 + 1, vky, c);
  c.r = 0.566; c.g = 0.566; c.b = 0.566;
  draw_rectangle(s, w, h, w / 2, y0 + 9 * vky, v2, vky, c);
  c.r = 0.77; c.g = 0.77; c.b = 0.77;
  draw_rectangle(s, w, h, w / 2 + v2, y0 + 9 * vky, v2, vky, c);
  c.r = 0.97; c.g = 0.97; c.b = 0.97;
  draw_boxed_circle(s, w, h, w / 2.0, h / 2.0, rk, w / 2 + 2 * v2, y0 + 9 * vky, v2, vky, ar, c);


  draw_boxed_circle(s, w, h, w / 2.0, h / 2.0, rk, w / 2 - rk, y0 + 10 * vky, 2 * rk, vky, ar, c); //bela
                                                                                                   // podloga
  c.r = 0.0; c.g = 0.0; c.b = 0.0;
  draw_rectangle(s, w, h, w / 2 - 3 * vkx, y0 + 10 * vky, 6 * vkx, vky, c);                        //title
                                                                                                   // sp
  c.r = 0.97; c.g = 0.97; c.b = 0.97;
  draw_rectangle(s, w, h, w / 2 - 2.5 * vkx, y0 + 10 * vky, 3, vky, c);                            //crtica

//spodnji del
  c.r = 0.73; c.g = 0.73; c.b = 0.0;
  draw_boxed_circle(s, w, h, w / 2.0, h / 2.0, rk, w / 2 - 4 * vkx, y0 + 11 * vky, 4 * vkx, 2 * vky, ar, c);
  draw_boxed_circle(s, w, h, w / 2.0, h / 2.0, rk, w / 2 + 0.5 * vkx, y0 + 11 * vky, 4 * vkx, 2 * vky, ar, c);
  c.r = 0.758; c.g = 0.0; c.b = 0.0;
  draw_boxed_circle(s, w, h, w / 2.0, h / 2.0, rk, w / 2 - 0.5 * vkx, y0 + 11 * vky, vkx, 2 * vky, ar, c);
}

//---------------------------------------------------------
//draw an approximation of the Telefunken FuBK test pattern
//ar = pixel aspect ratio
//simpl=1:  does not draw the circle and central vertical
void draw_fu(float_rgba *s, int w, int h, float ar, int simpl)
{
  float_rgba c;
  int        vky, vkx, nkx, x0, y0;
  int        i, j, z, k;
  float      rk, v1;
  float      f, df; //faza in korak faze za rastre
  float      a;

//doloci velikost kocke po y tako, da jih bo po visini 14.x
//z malo rezerve...
  vky = (h - 10) / 14;
//velikost koceke po x prilagodimo glede na frame in pix aspect
  vkx = (int)rintf((float)vky / ar);
  nkx = w / vkx - 1; //stevilo kock po x
  if (nkx % 2 == 1)
  {
    nkx--;
  }
  x0 = (w - vkx * nkx) / 2; //zacetni offset za centriranje
  if (x0 > vkx)
  {
    x0 = x0 - vkx;
  }
  y0  = (h - vky * 14) / 2;
  rk  = 6.7 * vky;
  c.a = 1.0;

//ozadje
  c.r = 0.25; c.g = 0.25; c.b = 0.25;
  for (i = 0; i < w * h; i++)
  {
    s[i] = c;
  }

//narisi mrezo
  c.r = 1.00; c.g = 1.00; c.b = 1.00;
  for (i = x0; i < w; i = i + vkx)
  {
    draw_rectangle(s, w, h, i - 1, 0, 3, h, c);
  }
  for (i = y0; i < h; i = i + vky)
  {
    draw_rectangle(s, w, h, 0, i - 1, w, 3, c);
  }

//barve zgoraj
  c.r = 0.75; c.g = 0.75; c.b = 0.75;
  draw_rectangle(s, w, h, w / 2 - 6 * vkx + 1, y0 + 2 * vky + 1, 1.5 * vkx, 3 * vky, c);
  c.r = 0.75; c.g = 0.75; c.b = 0.0;
  draw_rectangle(s, w, h, w / 2 - 4.5 * vkx, y0 + 2 * vky + 1, 1.5 * vkx, 3 * vky, c);
  c.r = 0.0; c.g = 0.75; c.b = 0.75;
  draw_rectangle(s, w, h, w / 2 - 3 * vkx, y0 + 2 * vky + 1, 1.5 * vkx, 3 * vky, c);
  c.r = 0.0; c.g = 0.75; c.b = 0.0;
  draw_rectangle(s, w, h, w / 2 - 1.5 * vkx, y0 + 2 * vky + 1, 1.5 * vkx, 3 * vky, c);
  c.r = 0.75; c.g = 0.0; c.b = 0.75;
  draw_rectangle(s, w, h, w / 2, y0 + 2 * vky + 1, 1.5 * vkx, 3 * vky, c);
  c.r = 0.75; c.g = 0.0; c.b = 0.0;
  draw_rectangle(s, w, h, w / 2 + 1.5 * vkx, y0 + 2 * vky + 1, 1.5 * vkx, 3 * vky, c);
  c.r = 0.0; c.g = 0.0; c.b = 0.75;
  draw_rectangle(s, w, h, w / 2 + 3 * vkx, y0 + 2 * vky + 1, 1.5 * vkx, 3 * vky, c);
  c.r = 0.0; c.g = 0.0; c.b = 0.0;
  draw_rectangle(s, w, h, w / 2 + 4.5 * vkx, y0 + 2 * vky + 1, 1.5 * vkx - 1, 3 * vky, c);

//sivine
  v1  = 12 * vkx / 5;
  c.r = 0.0; c.g = 0.0; c.b = 0.0;
  draw_rectangle(s, w, h, w / 2 - 6 * vkx + 1, y0 + 5 * vky, v1 - 1, 2 * vky - 1, c);
  c.r = 0.3; c.g = 0.3; c.b = 0.3;
  draw_rectangle(s, w, h, w / 2 - 6 * vkx + v1, y0 + 5 * vky, v1, 2 * vky - 1, c);
  c.r = 0.5; c.g = 0.5; c.b = 0.5;
  draw_rectangle(s, w, h, w / 2 - 6 * vkx + 2 * v1, y0 + 5 * vky, v1, 2 * vky - 1, c);
  c.r = 0.75; c.g = 0.75; c.b = 0.75;
  draw_rectangle(s, w, h, w / 2 - 6 * vkx + 3 * v1, y0 + 5 * vky, v1, 2 * vky - 1, c);
  c.r = 1.00; c.g = 1.00; c.b = 1.00;
  draw_rectangle(s, w, h, w / 2 - 6 * vkx + 4 * v1, y0 + 5 * vky, v1, 2 * vky - 1, c);

//bela podloga za title, rasteje in spico
  draw_rectangle(s, w, h, w / 2 - 6 * vkx, y0 + 7 * vky, 12 * vkx, 3 * vky, c);

//title bar
  c.r = 0.0; c.g = 0.0; c.b = 0.0;
  draw_rectangle(s, w, h, w / 2 - 1.5 * v1, y0 + 7 * vky + 1, 3 * v1, vky, c);
//sivina pod rastri
  c.r = 0.54; c.g = 0.54; c.b = 0.54;
  draw_rectangle(s, w, h, w / 2 - 4.5 * vkx, y0 + 8 * vky, 10.5 * vkx - 1, vky, c);

//rastri
  f  = -3.14 / 2.0;
  df = (float)w / 52.0 / ar; df = 6.28 / df; //fazni korak
  for (i = (w / 2 - 0.646 * rk / ar); i < (w / 2 - 0.373 * rk / ar); i++)
  {
    a   = 0.5 + 0.49 * cosf(f);
    f   = f + df;
    c.r = a; c.g = a; c.b = a;
    for (j = (y0 + 8 * vky); j < (y0 + 9 * vky); j++)
    {
      s[j * w + i] = c;
    }
  }
  f  = -3.14 / 2.0;
  df = (float)w / 103.0 / ar; df = 6.28 / df; //fazni korak
  for (i = (w / 2 - 0.332 * rk / ar); i < (w / 2 - 0.060 * rk / ar); i++)
  {
    a   = 0.5 + 0.49 * cosf(f);
    f   = f + df;
    c.r = a; c.g = a; c.b = a;
    for (j = (y0 + 8 * vky); j < (y0 + 9 * vky); j++)
    {
      s[j * w + i] = c;
    }
  }
  f  = -3.14 / 2.0;
  df = (float)w / 156.0 / ar; df = 6.28 / df; //fazni korak
  for (i = (w / 2 + 0.056 * rk / ar); i < (w / 2 + 0.299 * rk / ar); i++)
  {
    a   = 0.5 + 0.49 * cosf(f);
    f   = f + df;
    c.r = a; c.g = a; c.b = a;
    for (j = (y0 + 8 * vky); j < (y0 + 9 * vky); j++)
    {
      s[j * w + i] = c;
    }
  }

//rjava
  c.r = 0.69; c.g = 0.5; c.b = 0.0;
  draw_rectangle(s, w, h, w / 2 + 0.369 * rk / ar, y0 + 8 * vky, 0.437 * rk / ar, vky, c);


//spica
  c.r = 0.0; c.g = 0.0; c.b = 0.0;
  v1  = vkx / 2.857;
  z   = w / 2 - v1 / 2 + 2;
  for (i = (y0 + 9 * vky); i < (y0 + 10 * vky); i++)
  {
    k = z + v1 * ((y0 + 10 * vky) - i) / vky;
    for (j = z; j < k; j++)
    {
      s[i * w + j] = c;
    }
  }

//rdeci gradient
  c.r = 1.00; c.g = 0.055; c.b = 0.375;
  draw_rectangle(s, w, h, w / 2 - 6 * vkx + 1, y0 + 10 * vky + 1, 2 * vkx, vky - 1, c);
  for (i = (w / 2 - 4 * vkx); i < (w / 2 + 2 * vkx); i++)
  {
    a   = (((float)w / 2.0 + 2.0 * vkx) - (float)i) / (6.0 * (float)vkx);
    c.r = .999 * a; c.g = 0.055 * a; c.b = 0.375 * a;
    for (j = (y0 + 10 * vky + 1); j < (y0 + 11 * vky); j++)
    {
      s[j * w + i] = c;
    }
  }
//plavi gradient
  c.r = 0.375; c.g = 0.254; c.b = 1.00;
  draw_rectangle(s, w, h, w / 2 - 6 * vkx + 1, y0 + 11 * vky, 2 * vkx, vky - 1, c);
  for (i = (w / 2 - 4 * vkx); i < (w / 2 + 2 * vkx); i++)
  {
    a   = (((float)w / 2.0 + 2.0 * vkx) - (float)i) / (6.0 * (float)vkx);
    c.r = .375 * a; c.g = 0.254 * a; c.b = 1.00 * a;
    for (j = (y0 + 11 * vky); j < (y0 + 12 * vky - 1); j++)
    {
      s[j * w + i] = c;
    }
  }

//sivo spodaj desno
  c.r = 0.375; c.g = 0.375; c.b = 0.375;
  draw_rectangle(s, w, h, w / 2 + 2 * vkx, y0 + 10 * vky + 1, 4 * vkx - 1, 2 * vky - 2, c);

  if (simpl == 0)
  {
    //crta na sredi
    c.r = 1.00; c.g = 1.00; c.b = 1.00;
    draw_rectangle(s, w, h, w / 2 - 1, y0 + 5 * vky, 3, 4 * vky, c);

    //krog
    draw_circle(s, w, h, ar, w / 2, h / 2, rk, rk + 3, c);
  }
}

//----------------------------------------------------------
//simple color bars
//m=modulation	0: 100%	  1: 95%   2: 75%
//r=0	bars only	r=1 read area below ("PAL" color bars)
void bars_simple(float_rgba *s, int w, int h, int m, int r)
{
  float_rgba white, yellow, cyan, green, magenta, red, blue, black;
  int        h1;

  switch (m)
  {
  case 0:               //100% (PAL) color bars      100.0.100.0
    white.r   = 1.00;   white.g = 1.00;   white.b = 1.00;
    yellow.r  = 1.00;  yellow.g = 1.00;  yellow.b = 0.0;
    cyan.r    = 0.0;     cyan.g = 1.00;    cyan.b = 1.00;
    green.r   = 0.0;    green.g = 1.00;   green.b = 0.0;
    magenta.r = 1.00; magenta.g = 0.0;  magenta.b = 1.00;
    red.r     = 1.00;     red.g = 0.0;      red.b = 0.0;
    blue.r    = 0.0;     blue.g = 0.0;     blue.b = 1.00;
    black.r   = 0.0;    black.g = 0.0;    black.b = 0.0;
    break;

  case 1:               //95% (BBC) color bars     100.0.100.25
    white.r   = 1.00;   white.g = 1.00;   white.b = 1.00;
    yellow.r  = 1.00;  yellow.g = 1.00;  yellow.b = 0.25;
    cyan.r    = 0.25;    cyan.g = 1.00;    cyan.b = 1.00;
    green.r   = 0.25;   green.g = 1.00;   green.b = 0.25;
    magenta.r = 1.00; magenta.g = 0.25; magenta.b = 1.00;
    red.r     = 1.00;     red.g = 0.25;     red.b = 0.25;
    blue.r    = 0.25;    blue.g = 0.25;    blue.b = 1.00;
    black.r   = 0.0;    black.g = 0.0;    black.b = 0.0;
    break;

  case 2:               //75% (IBA/EBU) color bars   100.0.75.0
    white.r   = 1.00;   white.g = 1.00;   white.b = 1.00;
    yellow.r  = 0.75;  yellow.g = 0.75;  yellow.b = 0.0;
    cyan.r    = 0.0;     cyan.g = 0.75;    cyan.b = 0.75;
    green.r   = 0.0;    green.g = 0.75;   green.b = 0.0;
    magenta.r = 0.75; magenta.g = 0.0;  magenta.b = 0.75;
    red.r     = 0.75;     red.g = 0.0;      red.b = 0.0;
    blue.r    = 0.0;     blue.g = 0.0;     blue.b = 0.75;
    black.r   = 0.0;    black.g = 0.0;    black.b = 0.0;
    break;

  default:
    break;
  }
  white.a   = 1.0; yellow.a = 1.0; cyan.a = 1.0; green.a = 1.0;
  magenta.a = 1.0; red.a = 1.0; blue.a = 1.0; black.a = 1.0;

  if (r == 0)
  {
    h1 = h;
  }
  else
  {
    h1 = 0.55 * h;
  }

  draw_rectangle(s, w, h, 0 * w / 8, 0, w / 8 + 1, h, white);
  draw_rectangle(s, w, h, 1 * w / 8, 0, w / 8 + 1, h, yellow);
  draw_rectangle(s, w, h, 2 * w / 8, 0, w / 8 + 1, h, cyan);
  draw_rectangle(s, w, h, 3 * w / 8, 0, w / 8 + 1, h, green);
  draw_rectangle(s, w, h, 4 * w / 8, 0, w / 8 + 1, h, magenta);
  draw_rectangle(s, w, h, 5 * w / 8, 0, w / 8 + 1, h, red);
  draw_rectangle(s, w, h, 6 * w / 8, 0, w / 8 + 1, h, blue);
  draw_rectangle(s, w, h, 7 * w / 8, 0, w / 8 + 1, h, black);

  if (r == 1)
  {
    draw_rectangle(s, w, h, 0, h1, w, h - h1, red);
  }
}

//----------------------------------------------------------
//draws an approximation to the SMPTE color bars
void bars_smpte(float_rgba *s, int w, int h)
{
  float_rgba c;

  c.a = 1.0;

//top bars
  c.r = 0.75; c.g = 0.75; c.b = 0.75;
  draw_rectangle(s, w, h, 0 * w / 7, 0, w / 7 + 1, 2 * h / 3 + 1, c);
  c.r = 0.75; c.g = 0.75; c.b = 0.0;
  draw_rectangle(s, w, h, 1 * w / 7, 0, w / 7 + 1, 2 * h / 3 + 1, c);
  c.r = 0.0; c.g = 0.75; c.b = 0.75;
  draw_rectangle(s, w, h, 2 * w / 7, 0, w / 7 + 1, 2 * h / 3 + 1, c);
  c.r = 0.0; c.g = 0.75; c.b = 0.0;
  draw_rectangle(s, w, h, 3 * w / 7, 0, w / 7 + 1, 2 * h / 3 + 1, c);
  c.r = 0.75; c.g = 0.0; c.b = 0.75;
  draw_rectangle(s, w, h, 4 * w / 7, 0, w / 7 + 1, 2 * h / 3 + 1, c);
  c.r = 0.75; c.g = 0.0; c.b = 0.0;
  draw_rectangle(s, w, h, 5 * w / 7, 0, w / 7 + 1, 2 * h / 3 + 1, c);
  c.r = 0.0; c.g = 0.0; c.b = 0.75;
  draw_rectangle(s, w, h, 6 * w / 7, 0, w / 7 + 1, 2 * h / 3 + 1, c);

//middle bars
  c.r = 0.0; c.g = 0.0; c.b = 0.75;
  draw_rectangle(s, w, h, 0 * w / 7, 2 * h / 3, w / 7 + 1, h / 12 + 1, c);
  c.r = 0.074; c.g = 0.074; c.b = 0.074;
  draw_rectangle(s, w, h, 1 * w / 7, 2 * h / 3, w / 7 + 1, h / 12 + 1, c);
  c.r = 0.75; c.g = 0.0; c.b = 0.75;
  draw_rectangle(s, w, h, 2 * w / 7, 2 * h / 3, w / 7 + 1, h / 12 + 1, c);
  c.r = 0.074; c.g = 0.074; c.b = 0.074;
  draw_rectangle(s, w, h, 3 * w / 7, 2 * h / 3, w / 7 + 1, h / 12 + 1, c);
  c.r = 0.0; c.g = 0.75; c.b = 0.75;
  draw_rectangle(s, w, h, 4 * w / 7, 2 * h / 3, w / 7 + 1, h / 12 + 1, c);
  c.r = 0.074; c.g = 0.074; c.b = 0.074;
  draw_rectangle(s, w, h, 5 * w / 7, 2 * h / 3, w / 7 + 1, h / 12 + 1, c);
  c.r = 0.75; c.g = 0.75; c.b = 0.75;
  draw_rectangle(s, w, h, 6 * w / 7, 2 * h / 3, w / 7 + 1, h / 12 + 1, c);

//bottom bars
  c.r = 0.0; c.g = 0.129; c.b = 0.297;
  draw_rectangle(s, w, h, 0 * w / 28, 3 * h / 4, 5 * w / 28 + 1, h / 4 + 1, c);
  c.r = 1.00; c.g = 1.00; c.b = 1.00;
  draw_rectangle(s, w, h, 5 * w / 28, 3 * h / 4, 5 * w / 28 + 1, h / 4 + 1, c);
  c.r = 0.195; c.g = 0.0; c.b = 0.414;
  draw_rectangle(s, w, h, 10 * w / 28, 3 * h / 4, 5 * w / 28 + 1, h / 4 + 1, c);
  c.r = 0.074; c.g = 0.074; c.b = 0.074;
  draw_rectangle(s, w, h, 15 * w / 28, 3 * h / 4, 7 * w / 28 + 1, h / 4 + 1, c);

//pluge bars
  c.r = 0.035; c.g = 0.035; c.b = 0.035;
  draw_rectangle(s, w, h, 15 * w / 21, 3 * h / 4, w / 21 + 1, h / 4 + 1, c);
  c.r = 0.074; c.g = 0.074; c.b = 0.074;
  draw_rectangle(s, w, h, 16 * w / 21, 3 * h / 4, w / 21 + 1, h / 4 + 1, c);
  c.r = 0.113; c.g = 0.113; c.b = 0.113;
  draw_rectangle(s, w, h, 17 * w / 21, 3 * h / 4, w / 21 + 1, h / 4 + 1, c);

  c.r = 0.074; c.g = 0.074; c.b = 0.074;
  draw_rectangle(s, w, h, 6 * w / 7, 3 * h / 4, w / 7 + 1, h / 4 + 1, c);
}

//-----------------------------------------------------
//converts the internal RGB float image into
//Frei0r rgba8888 color
//sets alpha to opaque
void floatrgba2color(float_rgba *sl, uint32_t *outframe, int w, int h)
{
  int      i;
  uint32_t p;

  for (i = 0; i < w * h; i++)
  {
    p           = (uint32_t)(255.0 * sl[i].b) & 0xFF;
    p           = (p << 8) + ((uint32_t)(255.0 * sl[i].g) & 0xFF);
    p           = (p << 8) + ((uint32_t)(255.0 * sl[i].r) & 0xFF);
    outframe[i] = p + 0xFF000000;       //no transparency
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
//this structure holds an instance of the test_pat_B plugin
typedef struct
{
  unsigned int w;
  unsigned int h;

  int          type;
  int          aspt;
  float        mpar;

  float        par;
  float_rgba * sl;
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
  tp_info->name        = "test_pat_B";
  tp_info->author      = "Marko Cebokli";
  tp_info->plugin_type = F0R_PLUGIN_TYPE_SOURCE;
//  tp_info->plugin_type    = F0R_PLUGIN_TYPE_FILTER;
  tp_info->color_model    = F0R_COLOR_MODEL_RGBA8888;
  tp_info->frei0r_version = FREI0R_MAJOR_VERSION;
  tp_info->major_version  = 0;
  tp_info->minor_version  = 1;
  tp_info->num_params     = 3;
  tp_info->explanation    = "Generates test card lookalikes";
}

//--------------------------------------------------
void f0r_get_param_info(f0r_param_info_t *info, int param_index)
{
  switch (param_index)
  {
  case 0:
    info->name        = "Type";
    info->type        = F0R_PARAM_DOUBLE;
    info->explanation = "8 choices, select test pattern"; break;

  case 1:
    info->name        = "Aspect type";
    info->type        = F0R_PARAM_DOUBLE;
    info->explanation = "7 choices, pixel aspect ratio";
    break;

  case 2:
    info->name        = "Manual Aspect";
    info->type        = F0R_PARAM_DOUBLE;
    info->explanation = "Manual pixel aspect ratio (Aspect type 6)";
    break;
  }
}

//--------------------------------------------------
f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
  tp_inst_t *inst = calloc(1, sizeof(*inst));

  inst->w = width;
  inst->h = height;

  inst->type = 0;
  inst->aspt = 0;
  inst->mpar = 1.0;

  inst->par = 1.0;
  inst->sl  = (float_rgba *)calloc(width * height, sizeof(float_rgba));

  bars_simple(inst->sl, inst->w, inst->h, 0, 0);

  return ((f0r_instance_t)inst);
}

//--------------------------------------------------
void f0r_destruct(f0r_instance_t instance)
{
  tp_inst_t *inst = (tp_inst_t *)instance;

  free(inst->sl);
  free(inst);
}

//--------------------------------------------------
void f0r_set_param_value(f0r_instance_t instance, f0r_param_t param, int param_index)
{
  tp_inst_t *inst = (tp_inst_t *)instance;

  f0r_param_double *p = (f0r_param_double *)param;

  int   chg, tmpi;
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
      tmpi = map_value_forward(tmpf, 0.0, 7.9999);
    }
    if ((tmpi < 0) || (tmpi > 7.0))
    {
      break;
    }
    if (inst->type != tmpi)
    {
      chg = 1;
    }
    inst->type = tmpi;
    break;

  case 1:       //aspect type
    tmpf = *((double *)p);
    if (tmpf >= 1.0)
    {
      tmpi = (int)tmpf;
    }
    else
    {
      tmpi = map_value_forward(tmpf, 0.0, 6.9999);
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

  case 2:       //manual aspect
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
  case 0:               //100% PAL color bars
    bars_simple(inst->sl, inst->w, inst->h, 0, 0);
    break;

  case 1:               //PAL color bars with red
    bars_simple(inst->sl, inst->w, inst->h, 0, 1);
    break;

  case 2:               //95% BBC color bars
    bars_simple(inst->sl, inst->w, inst->h, 1, 0);
    break;

  case 3:               //75% EBU color bars
    bars_simple(inst->sl, inst->w, inst->h, 2, 0);
    break;

  case 4:               //SMPTE color bars
    bars_smpte(inst->sl, inst->w, inst->h);
    break;

  case 5:               //philips PM5544
    draw_pm(inst->sl, inst->w, inst->h, inst->par);
    break;

  case 6:               //FuBK
    draw_fu(inst->sl, inst->w, inst->h, inst->par, 0);
    break;

  case 7:               //simplified FuBK
    draw_fu(inst->sl, inst->w, inst->h, inst->par, 1);
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
    *p = map_value_backward(inst->type, 0.0, 7.9999);
    break;

  case 1:       //aspect type
    *p = map_value_backward(inst->aspt, 0.0, 6.9999);
    break;

  case 2:       //manual aspect
    *p = map_value_backward_log(inst->mpar, 0.5, 2.0);
    break;
  }
}

//---------------------------------------------------
void f0r_update(f0r_instance_t instance, double time, const uint32_t *inframe, uint32_t *outframe)
{
  assert(instance);
  tp_inst_t *inst = (tp_inst_t *)instance;

  floatrgba2color(inst->sl, outframe, inst->w, inst->h);
}
