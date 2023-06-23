/*
 * test_pat_L
 * This frei0r plugin generates test patterns for levels and
 * linearity checking
 *
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
 * Test patterns: Levels and Linearity
 *
 * This plugin draws a set of test patterns, used for checking of
 * linearity, gamma, contrast, etc.
 *
 * The patterns are drawn into a temporary float array, for two reasons:
 * 1. drawing routines are color model independent,
 * 2. drawing is done only when a parameter changes.
 *
 * only the function float2color()
 * needs to care about color models, endianness, DV legality etc.
 *
 *************************************************************/

//compile:	gcc -Wall -c -fPIC test_pat_L.c -o test_pat_L.o

//link: gcc -lm -shared -o test_pat_L.so test_pat_L.o

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include "frei0r.h"


double PI = 3.14159265358979;


//----------------------------------------------------------
void draw_rectangle(float *sl, int w, int h, int x, int y, int wr, int hr, float gray)
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

//----------------------------------------------------------
//rectangle with gray gradient
//dir:  0=left to right, 1=top to bottom, 2=r to l, 3=b to t
void draw_gradient(float *sl, int w, int h, int x, int y, int wr, int hr, float gray1, float gray2, int dir)
{
  int   i, j;
  int   zx, kx, zy, ky;
  float g, dg;

  if (wr <= 1)
  {
    return;
  }
  if (hr <= 1)
  {
    return;
  }
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
  switch (dir)
  {
  case 0:
    dg = (gray2 - gray1) / (wr - 1);
    g  = gray1;
    for (j = zx; j < kx; j++)
    {
      for (i = zy; i < ky; i++)
      {
        sl[w * i + j] = g;
      }
      g = g + dg;
    }
    break;

  case 1:
    dg = (gray2 - gray1) / (hr - 1);
    g  = gray1;
    for (i = zy; i < ky; i++)
    {
      for (j = zx; j < kx; j++)
      {
        sl[w * i + j] = g;
      }
      g = g + dg;
    }
    break;

  case 2:
    dg = (gray1 - gray2) / (wr - 1);
    g  = gray2;
    for (j = zx; j < kx; j++)
    {
      for (i = zy; i < ky; i++)
      {
        sl[w * i + j] = g;
      }
      g = g + dg;
    }
    break;

  case 3:
    dg = (gray1 - gray2) / (hr - 1);
    g  = gray2;
    for (i = zy; i < ky; i++)
    {
      for (j = zx; j < kx; j++)
      {
        sl[w * i + j] = g;
      }
      g = g + dg;
    }

  default:
    break;
  }
}

//-----------------------------------------------------------
//pocasna za velike kroge.....
void draw_circle(float *sl, int w, int h, float ar, int x, int y, int rn, int rz, float gray)
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
        sl[w * i + j] = gray;
      }
    }
  }
}

//-------------------------------------------------------
//draw one numerical digit, 7-segment style
//v=size in x direction (in y it is 2*v)
//d= number [0...9]
void disp7s(float *sl, int w, int h, int x, int y, int v, int d, float gray)
{
  char seg[10] = { 0xEE, 0x24, 0xBA, 0xB6, 0x74, 0xD6, 0xDE, 0xA4, 0xFE, 0xF6 };

  if ((d < 0) || (d > 9))
  {
    return;
  }

  if ((seg[d] & 128) != 0)
  {
    draw_rectangle(sl, w, h, x, y - 2 * v, v, 1, gray);
  }
  if ((seg[d] & 64) != 0)
  {
    draw_rectangle(sl, w, h, x, y - 2 * v, 1, v, gray);
  }
  if ((seg[d] & 32) != 0)
  {
    draw_rectangle(sl, w, h, x + v, y - 2 * v, 1, v, gray);
  }
  if ((seg[d] & 16) != 0)
  {
    draw_rectangle(sl, w, h, x, y - v, v, 1, gray);
  }
  if ((seg[d] & 8) != 0)
  {
    draw_rectangle(sl, w, h, x, y - v, 1, v, gray);
  }
  if ((seg[d] & 4) != 0)
  {
    draw_rectangle(sl, w, h, x + v, y - v, 1, v, gray);
  }
  if ((seg[d] & 2) != 0)
  {
    draw_rectangle(sl, w, h, x, y, v, 1, gray);
  }
}

//----------------------------------------------------------------
//draw a floating point number
//v=size
//n=number
//f=format (as in printf)
void dispF(float *sl, int w, int h, int x, int y, int v, float n, char *f, float gray)
{
  char str[64];
  int  i;

  sprintf(str, f, n);
  i = 0;
  while (str[i] != 0)
  {
    if (str[i] == '-')
    {
      draw_rectangle(sl, w, h, x + i * (v + v / 3 + 1), y - v, v, 1, gray);
    }
    else
    {
      disp7s(sl, w, h, x + i * (v + v / 3 + 1), y, v, str[i] - 48, gray);
    }
    i++;
  }
}

//----------------------------------------------------------
//gray staircase
void stopnice(float *sl, int w, int h)
{
  int   j, n;
  float s;

  n = 8;
  for (j = 0; j < n; j++)
  {
    s = (float)j / (float)(n - 1);
    draw_rectangle(sl, w, h, j * w / n, 0, w / n, h, s);
  }
}

//----------------------------------------------------------
//gray staircase with contrast check
void stopnice_k(float *sl, int w, int h)
{
  int   j, n, w1, h1;
  float s, s1, s2;

  n  = 8;
  w1 = w / n / 3;
  h1 = w1; if (h1 > h / 20)
  {
    h1 = h / 20;
  }
  for (j = 0; j < n; j++)
  {
    s = ((float)j + 0.5) / (float)n;
    draw_rectangle(sl, w, h, j * w / n, 0, w / n, h, s);

    s1 = s - 0.01; if (s1 < 0.0)
    {
      s1 = 0.0;
    }
    s2 = s + 0.01; if (s2 > 1.0)
    {
      s2 = 1.0;
    }
    draw_rectangle(sl, w, h, j * w / n + w1, 1 * h / 16, w1, h1, s1);
    draw_rectangle(sl, w, h, j * w / n + w1, 2 * h / 16, w1, h1, s2);

    s1 = s - 0.02; if (s1 < 0.0)
    {
      s1 = 0.0;
    }
    s2 = s + 0.02; if (s2 > 1.0)
    {
      s2 = 1.0;
    }
    draw_rectangle(sl, w, h, j * w / n + w1, 4 * h / 16, w1, h1, s1);
    draw_rectangle(sl, w, h, j * w / n + w1, 5 * h / 16, w1, h1, s2);

    s1 = s - 0.05; if (s1 < 0.0)
    {
      s1 = 0.0;
    }
    s2 = s + 0.05; if (s2 > 1.0)
    {
      s2 = 1.0;
    }
    draw_rectangle(sl, w, h, j * w / n + w1, 7 * h / 16, w1, h1, s1);
    draw_rectangle(sl, w, h, j * w / n + w1, 8 * h / 16, w1, h1, s2);

    s1 = s - 0.1; if (s1 < 0.0)
    {
      s1 = 0.0;
    }
    s2 = s + 0.1; if (s2 > 1.0)
    {
      s2 = 1.0;
    }
    draw_rectangle(sl, w, h, j * w / n + w1, 10 * h / 16, w1, h1, s1);
    draw_rectangle(sl, w, h, j * w / n + w1, 11 * h / 16, w1, h1, s2);

    s1 = s - 0.2; if (s1 < 0.0)
    {
      s1 = 0.0;
    }
    s2 = s + 0.2; if (s2 > 1.0)
    {
      s2 = 1.0;
    }
    draw_rectangle(sl, w, h, j * w / n + w1, 13 * h / 16, w1, w1, s1);
    draw_rectangle(sl, w, h, j * w / n + w1, 14 * h / 16, w1, w1, s2);
  }
}

//-----------------------------------------------------
//gray gradient
void sivi_klin(float *sl, int w, int h)
{
  draw_rectangle(sl, w, h, 0, 0, w / 7, h, 0.5);
  draw_rectangle(sl, w, h, 6 * w / 7, 0, w / 7, h, 0.5);
  draw_gradient(sl, w, h, w / 8, 0, 3 * w / 4, h, 0.0, 1.0, 0);
}

//----------------------------------------------------
//256 grays
void sivine256(float *sl, int w, int h)
{
  int   i, j, w1, h1;
  float s;

  draw_rectangle(sl, w, h, 0, 0, w, h, 0.5);
  if (w > h)
  {
    w1 = h / 20;
  }
  else
  {
    w1 = w / 20;
  }
  h1 = w1 - 2;
  for (i = 0; i < 16; i++)
  {
    for (j = 0; j < 16; j++)
    {
      s = (float)(16 * i + j) / 255.0;
      draw_rectangle(sl, w, h, (w - h) / 2 + (j + 2) * w1, (i + 2) * w1, h1, h1, s);
    }
  }
}

//------------------------------------------------------
//contrast bands
void trakovi(float *sl, int w, int h)
{
  int i, h1;

  draw_rectangle(sl, w, h, 0, 0, w, h, 0.5);
  h1 = h / 64;
  for (i = 0; i < 4; i++)
  {
    draw_gradient(sl, w, h, w / 8, (7 + 2 * i) * h1, 3 * w / 4, h1, 0.0, 0.99, 0);
    draw_gradient(sl, w, h, w / 8, (8 + 2 * i) * h1, 3 * w / 4, h1, 0.01, 1.0, 0);
  }
  for (i = 0; i < 4; i++)
  {
    draw_gradient(sl, w, h, w / 8, (21 + 2 * i) * h1, 3 * w / 4, h1, 0.0, 0.98, 0);
    draw_gradient(sl, w, h, w / 8, (22 + 2 * i) * h1, 3 * w / 4, h1, 0.02, 1.0, 0);
  }
  for (i = 0; i < 4; i++)
  {
    draw_gradient(sl, w, h, w / 8, (35 + 2 * i) * h1, 3 * w / 4, h1, 0.0, 0.95, 0);
    draw_gradient(sl, w, h, w / 8, (36 + 2 * i) * h1, 3 * w / 4, h1, 0.05, 1.0, 0);
  }
  for (i = 0; i < 4; i++)
  {
    draw_gradient(sl, w, h, w / 8, (49 + 2 * i) * h1, 3 * w / 4, h1, 0.0, 0.90, 0);
    draw_gradient(sl, w, h, w / 8, (50 + 2 * i) * h1, 3 * w / 4, h1, 0.1, 1.0, 0);
  }
}

//----------------------------------------------------------
void gamatest(float *sl, int w, int h)
{
  int   i, s, x, y;
  float g;

  for (i = 0; i < w * h; i++)
  {
    sl[i] = 0.5;                //gray background
  }
//gray patches
  for (i = 0; i < 30; i++)
  {
    s = 66 + 5 * i;
    g = 1.0 / (logf((float)s / 255.0) / logf(0.5));
    x = w / 4 + 3 * w / 16 * (i / 10);
    y = (i % 10 + 1) * h / 12;
    draw_rectangle(sl, w, h, x, y, w / 8, h / 13, (float)s / 255.0);
    s = (s < 140)?240:20;
    dispF(sl, w, h, x + w / 16 - 18, y + h / 24 + 4, 6, g, "%4.2f", s / 255.0);
  }
//zebra bars
  for (i = h / 16; i < 15 * h / 16; i++)
  {
    g = (i % 2 == 0)?1.0:0.0;
    draw_rectangle(sl, w, h, 3 * w / 16, i, w / 16, 1, g);
    draw_rectangle(sl, w, h, 6 * w / 16, i, w / 16, 1, g);
    draw_rectangle(sl, w, h, 9 * w / 16, i, w / 16, 1, g);
    draw_rectangle(sl, w, h, 12 * w / 16, i, w / 16, 1, g);
  }
//black and white level sidebars
  draw_rectangle(sl, w, h, w / 16, h / 12, w / 16, 10 * h / 12, 0.0);
  draw_rectangle(sl, w, h, 14 * w / 16, h / 12, w / 16, 10 * h / 12, 1.0);
  for (i = 1; i < 11; i++)
  {
    g = (float)i * 0.01;
    draw_rectangle(sl, w, h, w / 16 + w / 48, i * h / 12 + h / 36, w / 48, h / 36, g);
    g = (float)(100 - i) * 0.01;
    draw_rectangle(sl, w, h, 14 * w / 16 + w / 48, i * h / 12 + h / 36, w / 48, h / 36, g);
  }
}

//--------------------------------------------------
void ortikon(float *sl, int w, int h)
{
  int   i;
  float s1, s2;

  draw_rectangle(sl, w, h, 0, 0, w, h, 0.6);
//mali krogec
  draw_circle(sl, w, h, 1.0, 0.3 * w, h / 8, 0, 10, 0.95);
//crni in beli krog
  draw_circle(sl, w, h, 1.0, 0.6 * w, h / 8, 0, 20, 0.95);
  draw_circle(sl, w, h, 1.0, 0.6 * w + 40, h / 8, 0, 20, 0.05);
//gradient levo
  draw_gradient(sl, w, h, 0, h / 4, 0.3 * w, 3 * h / 4, 0.84, 0.094, 1);
//svetel trak
  draw_rectangle(sl, w, h, 0.13 * w, h / 4, w / 20, 3 * h / 4, 0.97);
//svetel trak z gradientom
  draw_gradient(sl, w, h, 17 * w / 40, h / 4, w / 20, 3 * h / 4, 0.97, 0.6, 1);
  s1 = 0.9; s2 = 0.1;
  for (i = h / 4; i < h; i = i + h / 4.5)
  {
    draw_rectangle(sl, w, h, 0.6 * w, i, h / 9, h / 9, s2);
    draw_rectangle(sl, w, h, 0.6 * w + h / 9, i, h / 9, h / 9, s1);
    draw_rectangle(sl, w, h, 0.6 * w + 2 * h / 9, i, h / 9, h / 9, s2);
    draw_rectangle(sl, w, h, 0.6 * w, i + h / 9, h / 9, h / 9, s1);
    draw_rectangle(sl, w, h, 0.6 * w + h / 9, i + h / 9, h / 9, h / 9, s2);
    draw_rectangle(sl, w, h, 0.6 * w + 2 * h / 9, i + h / 9, h / 9, h / 9, s1);
  }
}

//-----------------------------------------------------
//converts the internal monochrome float image into
//Frei0r rgba8888 color
//ch selects the channel   0=all  1=R  2=G  3=B
//sets alpha to opaque
void float2color(float *sl, uint32_t *outframe, int w, int h, int ch)
{
  int      i, ri, gi, bi;
  uint32_t p;
  float    r, g, b;

  switch (ch)
  {
  case 0:               //all (gray)
    for (i = 0; i < w * h; i++)
    {
      p           = (uint32_t)(255.0 * sl[i]) & 0xFF;
      outframe[i] = (p << 16) + (p << 8) + p + 0xFF000000;
    }
    break;

  case 1:               //R
    for (i = 0; i < w * h; i++)
    {
      p           = (uint32_t)(255.0 * sl[i]) & 0xFF;
      outframe[i] = p + 0xFF000000;
    }
    break;

  case 2:               //G
    for (i = 0; i < w * h; i++)
    {
      p           = (uint32_t)(255.0 * sl[i]) & 0xFF;
      outframe[i] = (p << 8) + 0xFF000000;
    }
    break;

  case 3:               //B
    for (i = 0; i < w * h; i++)
    {
      p           = (uint32_t)(255.0 * sl[i]) & 0xFF;
      outframe[i] = (p << 16) + 0xFF000000;
    }
    break;

  case 4:               //ccir rec 601  R-Y   on 50 gray
    for (i = 0; i < w * h; i++)
    {
      r           = sl[i];
      b           = 0.5;
      g           = (0.5 - 0.299 * r - 0.114 * b) / 0.587;
      ri          = (int)(255.0 * r);
      gi          = (int)(255.0 * g);
      bi          = (int)(255.0 * b);
      outframe[i] = (bi << 16) + (gi << 8) + ri + 0xFF000000;
    }
    break;

  case 5:               //ccir rec 601  B-Y   on 50% gray
    for (i = 0; i < w * h; i++)
    {
      b           = sl[i];
      r           = 0.5;
      g           = (0.5 - 0.299 * r - 0.114 * b) / 0.587;
      ri          = (int)(255.0 * r);
      gi          = (int)(255.0 * g);
      bi          = (int)(255.0 * b);
      outframe[i] = (bi << 16) + (gi << 8) + ri + 0xFF000000;
    }
    break;

  case 6:               //ccir rec 709  R-Y   on 50 gray
    for (i = 0; i < w * h; i++)
    {
      r           = sl[i];
      b           = 0.5;
      g           = (0.5 - 0.2126 * r - 0.0722 * b) / 0.7152;
      ri          = (int)(255.0 * r);
      gi          = (int)(255.0 * g);
      bi          = (int)(255.0 * b);
      outframe[i] = (bi << 16) + (gi << 8) + ri + 0xFF000000;
    }
    break;

  case 7:               //ccir rec 709  B-Y   on 50% gray
    for (i = 0; i < w * h; i++)
    {
      b           = sl[i];
      r           = 0.5;
      g           = (0.5 - 0.2126 * r - 0.0722 * b) / 0.7152;
      ri          = (int)(255.0 * r);
      gi          = (int)(255.0 * g);
      bi          = (int)(255.0 * b);
      outframe[i] = (bi << 16) + (gi << 8) + ri + 0xFF000000;
    }
    break;

  default:
    break;
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
//this structure holds an instance of the test_pat_L plugin
typedef struct
{
  unsigned int w;
  unsigned int h;

  int          type;
  int          chan;

  float *      sl;
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
  tp_info->name        = "test_pat_L";
  tp_info->author      = "Marko Cebokli";
  tp_info->plugin_type = F0R_PLUGIN_TYPE_SOURCE;
//  tp_info->plugin_type    = F0R_PLUGIN_TYPE_FILTER;
  tp_info->color_model    = F0R_COLOR_MODEL_RGBA8888;
  tp_info->frei0r_version = FREI0R_MAJOR_VERSION;
  tp_info->major_version  = 0;
  tp_info->minor_version  = 1;
  tp_info->num_params     = 2;
  tp_info->explanation    = "Generates linearity checking patterns";
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
    info->name        = "Channel";
    info->type        = F0R_PARAM_DOUBLE;
    info->explanation = "Into which color channel to draw";
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
  inst->chan = 0;

  inst->sl = (float *)calloc(width * height, sizeof(float));

  stopnice(inst->sl, inst->w, inst->h);

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
      tmpi = map_value_forward(tmpf, 0.0, 6.9999);
    }
    if ((tmpi < 0) || (tmpi > 6.0))
    {
      break;
    }
    if (inst->type != tmpi)
    {
      chg = 1;
    }
    inst->type = tmpi;
    break;

  case 1:       //channel
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
    if (inst->chan != tmpi)
    {
      chg = 1;
    }
    inst->chan = tmpi;
  }

  if (chg == 0)
  {
    return;
  }

  switch (inst->type)
  {
  case 0:               //gray steps
    stopnice(inst->sl, inst->w, inst->h);
    break;

  case 1:               //gray steps with contrast squares
    stopnice_k(inst->sl, inst->w, inst->h);
    break;

  case 2:               //gray gradient
    sivi_klin(inst->sl, inst->w, inst->h);
    break;

  case 3:               //256 gray squares in a 16x16 matrix
    sivine256(inst->sl, inst->w, inst->h);
    break;

  case 4:               //contrast bands
    trakovi(inst->sl, inst->w, inst->h);
    break;

  case 5:               //gama ckecking chart
    gamatest(inst->sl, inst->w, inst->h);
    break;

  case 6:               //for testing orthicon simulator
    ortikon(inst->sl, inst->w, inst->h);
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
    *p = map_value_backward(inst->type, 0.0, 6.9999);
    break;

  case 1:       //channel
    *p = map_value_backward(inst->chan, 0.0, 7.9999);
    break;
  }
}

//---------------------------------------------------
void f0r_update(f0r_instance_t instance, double time, const uint32_t *inframe, uint32_t *outframe)
{
  assert(instance);
  tp_inst_t *inst = (tp_inst_t *)instance;

  float2color(inst->sl, outframe, inst->w, inst->h, inst->chan);
}
