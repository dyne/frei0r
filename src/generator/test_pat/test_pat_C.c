/*
 * test_pat_C
 * This frei0r plugin generates cross sections of color spaces
 * Version 0.1	aug 2010
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
 * Test patterns: cross sections of color spaces
 *************************************************************/

//compile:	gcc -Wall -c -fPIC test_pat_C.c -o test_pat_C.o

//link: gcc -lm -shared -o test_pat_C.so test_pat_C.o

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

//--------------------------------------------------------------
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

//-------------------------------------------------------
int inside(float_rgba c)
{
  if (c.r < 0.0)
  {
    return (0);
  }
  if (c.r > 1.0)
  {
    return (0);
  }
  if (c.g < 0.0)
  {
    return (0);
  }
  if (c.g > 1.0)
  {
    return (0);
  }
  if (c.b < 0.0)
  {
    return (0);
  }
  if (c.b > 1.0)
  {
    return (0);
  }
  return (1);
}

//-----------------------------------------------------------
//os:   0=RG(B)   1=GB(R)   2=BR(G)
//a:	value on third axis
void risi_presek_rgb(float_rgba *s, int w, int h, float x, float y, float wr, float hr, int os, float a)
{
  int        i, j;
  int        zx, kx, zy, ky;
  float_rgba c;
  float      d1, d2;

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
  switch (os)
  {
  case 0:
    c.b = a;
    d1  = 1.0 / hr;
    d2  = 1.0 / wr;
    c.r = 0.0;
    for (i = zy; i < ky; i++)
    {
      c.r = c.r + d1;
      c.g = 0.0;
      for (j = zx; j < kx; j++)
      {
        c.g          = c.g + d2;
        c.a          = 1.0;
        s[w * i + j] = c;
      }
    }
    break;

  case 1:
    c.r = a;
    d1  = 1.0 / hr;
    d2  = 1.0 / wr;
    c.g = 0.0;
    for (i = zy; i < ky; i++)
    {
      c.g = c.g + d1;
      c.b = 0.0;
      for (j = zx; j < kx; j++)
      {
        c.b          = c.b + d2;
        c.a          = 1.0;
        s[w * i + j] = c;
      }
    }
    break;

  case 2:
    c.g = a;
    d1  = 1.0 / hr;
    d2  = 1.0 / wr;
    c.b = 0.0;
    for (i = zy; i < ky; i++)
    {
      c.b = c.b + d1;
      c.r = 0.0;
      for (j = zx; j < kx; j++)
      {
        c.r          = c.r + d2;
        c.a          = 1.0;
        s[w * i + j] = c;
      }
    }
    break;

  default:
    break;
  }
}

//-----------------------------------------------------------
//PAZI!!!!  ali so meje -0.5 do 0.5 za Pr in Pb prave?
//os:   0=Y'Pr(Pb)   1=PrPb(Y)   2=PbY'(Pr)
//a:	value on third axis
void risi_presek_yprpb601(float_rgba *s, int w, int h, float x, float y, float wr, float hr, int os, float a)
{
  int        i, j;
  int        zx, kx, zy, ky;
  float_rgba c;
  float      d1, d2, yy, pr, pb;

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
  switch (os)
  {
  case 0:
    pb = a - 0.5;
    d1 = 1.0 / hr;
    d2 = 1.0 / wr;
    yy = 0.0;
    for (i = zy; i < ky; i++)
    {
      yy = yy + d1;
      pr = -0.5;
      for (j = zx; j < kx; j++)
      {
        pr  = pr + d2;
        c.r = pr + yy;
        c.b = pb + yy;
        c.g = (yy - 0.3 * c.r - 0.1 * c.b) / 0.6;
        c.a = 1.0;
        if (inside(c) == 1)
        {
          s[w * i + j] = c;
        }
      }
    }
    break;

  case 1:
    yy = a;
    d1 = 1.0 / hr;
    d2 = 1.0 / wr;
    pr = -0.5;
    for (i = zy; i < ky; i++)
    {
      pr = pr + d1;
      pb = -0.5;
      for (j = zx; j < kx; j++)
      {
        pb  = pb + d2;
        c.r = pr + yy;
        c.b = pb + yy;
        c.g = (yy - 0.3 * c.r - 0.1 * c.b) / 0.6;
        c.a = 1.0;
        if (inside(c) == 1)
        {
          s[w * i + j] = c;
        }
      }
    }
    break;

  case 2:
    pr = a - 0.5;
    d1 = 1.0 / hr;
    d2 = 1.0 / wr;
    pb = -0.5;
    for (i = zy; i < ky; i++)
    {
      pb = pb + d1;
      yy = 0.0;
      for (j = zx; j < kx; j++)
      {
        yy  = yy + d2;
        c.r = pr + yy;
        c.b = pb + yy;
        c.g = (yy - 0.3 * c.r - 0.1 * c.b) / 0.6;
        c.a = 1.0;
        if (inside(c) == 1)
        {
          s[w * i + j] = c;
        }
      }
    }
    break;

  default:
    break;
  }
}

//-----------------------------------------------------------
//PAZI!!!!  ali so meje -1.0 do 1.0 za aa in bb prave?
//os:   0=AB(I)   1=BI(A)   2=IA(B)
//a:	value on third axis
void risi_presek_abi(float_rgba *s, int w, int h, float x, float y, float wr, float hr, int os, float a)
{
  int        i, j;
  int        zx, kx, zy, ky;
  float_rgba c;
  float      d1, d2, aa, bb, ii, k3, ik3;

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
  k3  = sqrtf(3.0) / 2.0;
  ik3 = 0.5 / k3;

  switch (os)
  {
  case 0:
    ii = a;
    d1 = 2.0 / hr;
    d2 = 2.0 / wr;
    aa = -1.0;
    for (i = zy; i < ky; i++)
    {
      aa = aa + d1;
      bb = -1.0;
      for (j = zx; j < kx; j++)
      {
        bb  = bb + d2;
        c.r = (aa + 1.5 * ii) * 0.666666666;
        c.b = ii - 0.333333 * aa - ik3 * bb;
        c.g = bb + k3 * c.b;
        c.a = 1.0;
        if (inside(c) == 1)
        {
          s[w * i + j] = c;
        }
      }
    }
    break;

  case 1:
    aa = 2.0 * a - 1.0;
    d1 = 2.0 / hr;
    d2 = 1.0 / wr;
    bb = -1.0;
    for (i = zy; i < ky; i++)
    {
      bb = bb + d1;
      ii = 0.0;
      for (j = zx; j < kx; j++)
      {
        ii  = ii + d2;
        c.r = (aa + 1.5 * ii) * 0.666666666;
        c.b = ii - 0.333333 * aa - ik3 * bb;
        c.g = bb + k3 * c.b;
        c.a = 1.0;
        if (inside(c) == 1)
        {
          s[w * i + j] = c;
        }
      }
    }
    break;

  case 2:
    bb = 2.0 * a - 1.0;
    d1 = 1.0 / hr;
    d2 = 2.0 / wr;
    ii = 0.0;
    for (i = zy; i < ky; i++)
    {
      ii = ii + d1;
      aa = -1.0;
      for (j = zx; j < kx; j++)
      {
        aa  = aa + d2;
        c.r = (aa + 1.5 * ii) * 0.666666666;
        c.b = ii - 0.333333 * aa - ik3 * bb;
        c.g = bb + k3 * c.b;
        c.a = 1.0;
        if (inside(c) == 1)
        {
          s[w * i + j] = c;
        }
      }
    }
    break;

  default:
    break;
  }
}

//-----------------------------------------------------------
//os:   0=HC(I)   1=CI(H)   2=IH(C)
//a:	value on third axis
void risi_presek_hci(float_rgba *s, int w, int h, float x, float y, float wr, float hr, int os, float a)
{
  int        i, j;
  int        zx, kx, zy, ky;
  float_rgba c;
  float      d1, d2, aa, bb, ii, k3, ik3, hh, cc;

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
  k3  = sqrtf(3.0) / 2.0;
  ik3 = 0.5 / k3;

  switch (os)
  {
  case 0:
    ii = a;
    d1 = 2.0 * PI / hr;
    d2 = 1.0 / wr;
    hh = 0.0;
    for (i = zy; i < ky; i++)
    {
      hh = hh + d1;
      cc = 0.0;
      for (j = zx; j < kx; j++)
      {
        cc  = cc + d2;
        aa  = cc * cos(hh);
        bb  = cc * sin(hh);
        c.r = (aa + 1.5 * ii) * 0.666666666;
        c.b = ii - 0.333333 * aa - ik3 * bb;
        c.g = bb + k3 * c.b;
        c.a = 1.0;
        if (inside(c) == 1)
        {
          s[w * i + j] = c;
        }
      }
    }
    break;

  case 1:
    hh = a * 2.0 * PI;
    d1 = 1.0 / hr;
    d2 = 1.0 / wr;
    cc = 0.0;
    for (i = zy; i < ky; i++)
    {
      cc = cc + d1;
      ii = 0.0;
      for (j = zx; j < kx; j++)
      {
        ii  = ii + d2;
        aa  = cc * cos(hh);
        bb  = cc * sin(hh);
        c.r = (aa + 1.5 * ii) * 0.666666666;
        c.b = ii - 0.333333 * aa - ik3 * bb;
        c.g = bb + k3 * c.b;
        c.a = 1.0;
        if (inside(c) == 1)
        {
          s[w * i + j] = c;
        }
      }
    }
    break;

  case 2:
    cc = a;
    d1 = 1.0 / hr;
    d2 = 2.0 * PI / wr;
    ii = 0.0;
    for (i = zy; i < ky; i++)
    {
      ii = ii + d1;
      hh = 0.0;
      for (j = zx; j < kx; j++)
      {
        hh  = hh + d2;
        aa  = cc * cos(hh);
        bb  = cc * sin(hh);
        c.r = (aa + 1.5 * ii) * 0.666666666;
        c.b = ii - 0.333333 * aa - ik3 * bb;
        c.g = bb + k3 * c.b;
        c.a = 1.0;
        if (inside(c) == 1)
        {
          s[w * i + j] = c;
        }
      }
    }
    break;

  default:
    break;
  }
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
//this structure holds an instance of the test_pat_C plugin
typedef struct
{
  unsigned int w;
  unsigned int h;

  int          spc;
  int          cs;
  float        thav;
  int          fs;

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
  tp_info->name        = "test_pat_C";
  tp_info->author      = "Marko Cebokli";
  tp_info->plugin_type = F0R_PLUGIN_TYPE_SOURCE;
//  tp_info->plugin_type    = F0R_PLUGIN_TYPE_FILTER;
  tp_info->color_model    = F0R_COLOR_MODEL_RGBA8888;
  tp_info->frei0r_version = FREI0R_MAJOR_VERSION;
  tp_info->major_version  = 0;
  tp_info->minor_version  = 1;
  tp_info->num_params     = 4;
  tp_info->explanation    = "Generates cross sections of color spaces";
}

//--------------------------------------------------
void f0r_get_param_info(f0r_param_info_t *info, int param_index)
{
  switch (param_index)
  {
  case 0:
    info->name        = "Color space";
    info->type        = F0R_PARAM_DOUBLE;
    info->explanation = "";
    break;

  case 1:
    info->name        = "Cross section";
    info->type        = F0R_PARAM_DOUBLE;
    info->explanation = "";
    break;

  case 2:
    info->name        = "Third axis value";
    info->type        = F0R_PARAM_DOUBLE;
    info->explanation = "";
    break;

  case 3:
    info->name        = "Fullscreen";
    info->type        = F0R_PARAM_BOOL;
    info->explanation = "";
    break;
  }
}

//--------------------------------------------------
f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
  int        x0, y0, velx, vely;
  float_rgba c;
  tp_inst_t *inst = calloc(1, sizeof(*inst));

  inst->w = width;
  inst->h = height;

  inst->spc  = 0;
  inst->cs   = 0;
  inst->thav = 0.5;
  inst->fs   = 0;

  inst->sl = (float_rgba *)calloc(width * height, sizeof(float_rgba));

  x0   = (inst->w - 3 * inst->h / 4) / 2;
  y0   = inst->h / 8;
  velx = 3 * inst->h / 4;
  vely = 3 * inst->h / 4;
  c.r  = 0.5; c.g = 0.5; c.b = 0.5; c.a = 1.0; //gray background
  draw_rectangle(inst->sl, inst->w, inst->h, 0.0, 0.0, (float)inst->w, (float)inst->h, c);
  c.r = 0.4; c.g = 0.4; c.b = 0.4; c.a = 1.0;  //darker gray background
  draw_rectangle(inst->sl, inst->w, inst->h, x0, y0, velx, vely, c);
  risi_presek_rgb(inst->sl, inst->w, inst->h, x0, y0, velx, vely, inst->cs, inst->thav);

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
  tp_inst_t *       inst = (tp_inst_t *)instance;
  f0r_param_double *p    = (f0r_param_double *)param;
  int        chg, tmpi, x0, y0, velx, vely;
  float      tmpf;
  float_rgba c;

  chg = 0;
  switch (param_index)
  {
  case 0:       //color space
    tmpf = *((double *)p);
    if (tmpf >= 1.0)
    {
      tmpi = (int)tmpf;
    }
    else
    {
      tmpi = map_value_forward(tmpf, 0.0, 3.9999);
    }
    if ((tmpi < 0) || (tmpi > 3.0))
    {
      break;
    }
    if (inst->spc != tmpi)
    {
      chg = 1;
    }
    inst->spc = tmpi;
    break;

  case 1:       //cross section
    tmpf = *((double *)p);
    if (tmpf >= 1.0)
    {
      tmpi = (int)tmpf;
    }
    else
    {
      tmpi = map_value_forward(tmpf, 0.0, 2.9999);
    }
    if ((tmpi < 0) || (tmpi > 2.0))
    {
      break;
    }
    if (inst->cs != tmpi)
    {
      chg = 1;
    }
    inst->cs = tmpi;
    break;

  case 2:       //third axis value
    tmpf = map_value_forward(*((double *)p), 0.0, 1.0);
    if (inst->thav != tmpf)
    {
      chg = 1;
    }
    inst->thav = tmpf;
    break;

  case 3:       //fullscreen   (BOOL)
    tmpi = map_value_forward(*((double *)p), 0.0, 1.0);
    if (inst->fs != tmpi)
    {
      chg = 1;
    }
    inst->fs = tmpi;
    break;
  }

  if (chg == 0)
  {
    return;
  }

  if (inst->fs == 0)
  {
    x0   = (inst->w - 3 * inst->h / 4) / 2;
    y0   = inst->h / 8;
    velx = 3 * inst->h / 4;
    vely = 3 * inst->h / 4;
  }
  else
  {
    x0   = 0;
    y0   = 0;
    velx = inst->w;
    vely = inst->h;
  }

  c.r = 0.5; c.g = 0.5; c.b = 0.5; c.a = 1.0; //gray background
  draw_rectangle(inst->sl, inst->w, inst->h, 0.0, 0.0, (float)inst->w, (float)inst->h, c);
  c.r = 0.4; c.g = 0.4; c.b = 0.4; c.a = 1.0; //darker gray background
  draw_rectangle(inst->sl, inst->w, inst->h, x0, y0, velx, vely, c);

  switch (inst->spc)
  {
  case 0:
    risi_presek_rgb(inst->sl, inst->w, inst->h, x0, y0, velx, vely, inst->cs, inst->thav);
    break;

  case 1:
    risi_presek_yprpb601(inst->sl, inst->w, inst->h, x0, y0, velx, vely, inst->cs, inst->thav);
    break;

  case 2:
    risi_presek_abi(inst->sl, inst->w, inst->h, x0, y0, velx, vely, inst->cs, inst->thav);
    break;

  case 3:
    risi_presek_hci(inst->sl, inst->w, inst->h, x0, y0, velx, vely, inst->cs, inst->thav);
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
  case 0:       //color space
    *p = map_value_backward(inst->spc, 0.0, 3.9999);
    break;

  case 1:       //cross section
    *p = map_value_backward(inst->cs, 0.0, 2.9999);
    break;

  case 2:       //third axis value
    *p = map_value_backward(inst->thav, 0.0, 1.0);
    break;

  case 3:       //fullscreen (BOOL)
    *p = map_value_backward_log(inst->fs, 0.0, 1.0);
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
