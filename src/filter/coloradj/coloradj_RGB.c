/*
coloradj_RGB.c

This frei0r plugin   is for simple RGB color adjustment
Version 0.1	jul 2010

Copyright (C) 2010  Marko Cebokli    http://lea.hamradio.si/~s57uuu


 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/


//compile: gcc -c -fPIC -Wall coloradj_RGB.c -o coloradj_RGB.o
//link: gcc -shared -lm -o coloradj_RGB.so coloradj_RGB.o

//#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include <frei0r.h>
#ifdef __SSE2__
#include <emmintrin.h>
#endif

//------------------------------------------------------
//computes x to the power p
//only for positive x
static float pwr(float x, float p)
{
if (x<=0) return 0;
return expf(p*logf(x));
}

//-----------------------------------------------------
//stretch [0...1] to parameter range [min...max] logarithmic
//min and max must be positive!
static float map_value_forward_log(double v, float min, float max)
{
float sr,k;

sr=sqrtf(min*max);
k=2.0*log(max/sr);
return sr*expf(k*(v-0.5));
}

//---------------------------------------
//8 bit RGB lookup table
typedef struct
  {
  unsigned char r[256];
  unsigned char g[256];
  unsigned char b[256];
  } lut_s;

//-------------------------------------------------------
//  "Add constant"
//norm=0 don't normalize    norm=1 do normalize
static void make_lut1(float r, float g, float b, lut_s *lut, int norm, int cm)
{
int i;
float rr,gg,bb,l;

for (i=0;i<256;i++)
	{
	rr=(float)i+(r-0.5)*150.0;  if (rr<0.0) rr=0.0;
	gg=(float)i+(g-0.5)*150.0;  if (gg<0.0) gg=0.0;
	bb=(float)i+(b-0.5)*150.0;  if (bb<0.0) bb=0.0;

	if (norm==1)
		{
		switch (cm)
			{
			case 0:  //rec 601
				{
				l = 0.299*rr + 0.587*gg + 0.114*bb;
				break;
				}
			case 1:  //rec 709
				{
				l = 0.2126*rr + 0.7152*gg + 0.0722*bb;
				break;
				}
			default:
				{
				l=(float)i;
				break;
				}
			}
		if (l>0.0)
			{
			rr=rr*(float)i/l;
			gg=gg*(float)i/l;
			bb=bb*(float)i/l;
			}
		else
			{
			rr=0.0; gg=0.0; bb=0.0;
			}
		}

	if (rr>255.0) rr=255.0;
	if (gg>255.0) gg=255.0;
	if (bb>255.0) bb=255.0;
	lut->r[i]=rintf(rr);
	lut->g[i]=rintf(gg);
	lut->b[i]=rintf(bb);
	}
}

//-------------------------------------------------------
//  "Change gamma"
//norm=0 don't normalize    norm=1 do normalize
static void make_lut2(float r, float g, float b, lut_s *lut, int norm, int cm)
{
int i;
float rr,gg,bb,gama,l;

float gama_r=map_value_forward_log(r,3.0,0.3333);
float gama_g=map_value_forward_log(g,3.0,0.3333);
float gama_b=map_value_forward_log(b,3.0,0.3333);
(void)gama;

for (i=0;i<256;i++)
	{
	rr=255.0*pwr((float)i/255.0,gama_r);
	gg=255.0*pwr((float)i/255.0,gama_g);
	bb=255.0*pwr((float)i/255.0,gama_b);

	if (norm==1)
		{
		switch (cm)
			{
			case 0:  //rec 601
				{
				l = 0.299*rr + 0.587*gg + 0.114*bb;
				break;
				}
			case 1:  //rec 709
				{
				l = 0.2126*rr + 0.7152*gg + 0.0722*bb;
				break;
				}
			default:
				{
				l=(float)i;
				break;
				}
			}
		if (l>0.0)
			{
			rr=rr*(float)i/l;
			gg=gg*(float)i/l;
			bb=bb*(float)i/l;
			}
		else
			{
			rr=0.0; gg=0.0; bb=0.0;
			}
		}

	if (rr>255.0) rr=255.0;  if (rr<0.0) rr=0.0;
	if (gg>255.0) gg=255.0;  if (gg<0.0) gg=0.0;
	if (bb>255.0) bb=255.0;  if (bb<0.0) bb=0.0;
	lut->r[i]=rintf(rr);
	lut->g[i]=rintf(gg);
	lut->b[i]=rintf(bb);
	}
}

//-------------------------------------------------------
//  "Multiply"
//norm=0 don't normalize    norm=1 do normalize
static void make_lut3(float r, float g, float b, lut_s *lut, int norm, int cm)
{
int i;
float rr,gg,bb,l;

float mult_r=map_value_forward_log(r,0.3333,3.0);
float mult_g=map_value_forward_log(g,0.3333,3.0);
float mult_b=map_value_forward_log(b,0.3333,3.0);

for (i=0;i<256;i++)
	{
	rr=(float)i*mult_r;
	gg=(float)i*mult_g;
	bb=(float)i*mult_b;

	if (norm==1)
		{
		switch (cm)
			{
			case 0:  //rec 601
				{
				l = 0.299*rr + 0.587*gg + 0.114*bb;
				break;
				}
			case 1:  //rec 709
				{
				l = 0.2126*rr + 0.7152*gg + 0.0722*bb;
				break;
				}
			default:
				{
				l=(float)i;
				break;
				}
			}
		if (l>0.0)
			{
			rr=rr*(float)i/l;
			gg=gg*(float)i/l;
			bb=bb*(float)i/l;
			}
		else
			{
			rr=0.0; gg=0.0; bb=0.0;
			}
		}

	if (rr>255.0) rr=255.0;  if (rr<0.0) rr=0.0;
	if (gg>255.0) gg=255.0;  if (gg<0.0) gg=0.0;
	if (bb>255.0) bb=255.0;  if (bb<0.0) bb=0.0;
	lut->r[i]=rintf(rr);
	lut->g[i]=rintf(gg);
	lut->b[i]=rintf(bb);
	}
}

//----------------------------------------------------
//F0R_COLOR_MODEL_RGBA8888  little endian
static inline uint32_t div255_exact(uint32_t x)
{
	return (x + 1u + (x >> 8)) >> 8;
}

static void apply_lut(const uint32_t* inframe, uint32_t* outframe, int size, lut_s *lut, int ac)
{
const unsigned char *lr = lut->r;
const unsigned char *lg = lut->g;
const unsigned char *lb = lut->b;
int i;

if (ac==0)
	{
	for (i=0;i<size;i++)
		{
		uint32_t px = inframe[i];
		outframe[i] = (uint32_t)lr[px & 255u]
		            | ((uint32_t)lg[(px >> 8) & 255u] << 8)
		            | ((uint32_t)lb[(px >> 16) & 255u] << 16)
		            | (px & 0xFF000000u);
		}
	}
else		//alpha controlled
	{
	i = 0;
#ifdef __SSE2__
	__m128i vZero = _mm_setzero_si128();
	__m128i v255 = _mm_set1_epi16(255);
	__m128i vOne = _mm_set1_epi16(1);
	__m128i vAlphaMask = _mm_set1_epi32((int)0xFF000000u);
	__m128i vRgbMask = _mm_set1_epi32(0x00FFFFFF);
	for (; i + 1 < size; i += 2)
		{
		uint32_t p0 = inframe[i];
		uint32_t p1 = inframe[i + 1];
		uint32_t l0 = (uint32_t)lr[p0 & 255u]
		            | ((uint32_t)lg[(p0 >> 8) & 255u] << 8)
		            | ((uint32_t)lb[(p0 >> 16) & 255u] << 16);
		uint32_t l1 = (uint32_t)lr[p1 & 255u]
		            | ((uint32_t)lg[(p1 >> 8) & 255u] << 8)
		            | ((uint32_t)lb[(p1 >> 16) & 255u] << 16);
		__m128i vSrc8 = _mm_loadl_epi64((const __m128i *)(inframe + i));
		__m128i vLut8 = _mm_set_epi32(0, 0, (int)l1, (int)l0);
		__m128i vSrc16 = _mm_unpacklo_epi8(vSrc8, vZero);
		__m128i vLut16 = _mm_unpacklo_epi8(vLut8, vZero);
		/* Broadcast alpha of each pixel across its 4 16-bit lanes */
		__m128i vA16 = _mm_shufflelo_epi16(vSrc16, 0xFF);
		vA16 = _mm_shufflehi_epi16(vA16, 0xFF);
		__m128i vIA16 = _mm_sub_epi16(v255, vA16);
		__m128i vSum = _mm_add_epi16(_mm_mullo_epi16(vIA16, vSrc16),
		                             _mm_mullo_epi16(vA16, vLut16));
		__m128i vDiv = _mm_srli_epi16(_mm_add_epi16(_mm_add_epi16(vSum, vOne),
		                                            _mm_srli_epi16(vSum, 8)), 8);
		__m128i vOut8 = _mm_packus_epi16(vDiv, vZero);
		vOut8 = _mm_or_si128(_mm_and_si128(vOut8, vRgbMask),
		                     _mm_and_si128(vSrc8, vAlphaMask));
		_mm_storel_epi64((__m128i *)(outframe + i), vOut8);
		}
#endif
	for (;i<size;i++)
		{
		uint32_t px = inframe[i];
		uint32_t a = px >> 24;
		uint32_t ia = 255u - a;
		uint32_t sr = px & 255u;
		uint32_t sg = (px >> 8) & 255u;
		uint32_t sb = (px >> 16) & 255u;
		uint32_t r = div255_exact(ia * sr + a * lr[sr]);
		uint32_t g = div255_exact(ia * sg + a * lg[sg]);
		uint32_t b = div255_exact(ia * sb + a * lb[sb]);
		outframe[i] = r | (g << 8) | (b << 16) | (px & 0xFF000000u);
		}
	}
}

//-----------------------------------------------------
//stretch [0...1] to parameter range [min...max] linear
static float map_value_forward(double v, float min, float max)
{
return min+(max-min)*v;
}

//-----------------------------------------------------
//collapse from parameter range [min...max] to [0...1] linear
static double map_value_backward(float v, float min, float max)
{
return (v-min)/(max-min);
}

//----------------------------------------
//struktura za instanco efekta
typedef struct
{
int h;
int w;
float r,g,b;
int act;
int norm;
int ac;
int cm;
lut_s *lut;
} inst;

//***********************************************
// OBVEZNE FREI0R FUNKCIJE

//-----------------------------------------------
int f0r_init()
{
return 1;
}

//------------------------------------------------
void f0r_deinit()
{
}

//-----------------------------------------------
void f0r_get_plugin_info(f0r_plugin_info_t* info)
{

info->name="coloradj_RGB";
info->author="Marko Cebokli";
info->plugin_type=F0R_PLUGIN_TYPE_FILTER;
info->color_model=F0R_COLOR_MODEL_RGBA8888;
info->frei0r_version=FREI0R_MAJOR_VERSION;
info->major_version=0;
info->minor_version=2;
info->num_params=7;
info->explanation="Simple color adjustment";
}

//--------------------------------------------------
void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
switch(param_index)
	{
	case 0:
		info->name = "R";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "Amount of red";
		break;
	case 1:
		info->name = "G";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "Amount of green";
		break;
	case 2:
		info->name = "B";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "Amount of blue";
		break;
	case 3:
		info->name = "Action";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "Type of color adjustment";
		break;
	case 4:
		info->name = "Keep luma";
		info->type = F0R_PARAM_BOOL;
		info->explanation = "Don't change brightness";
		break;
	case 5:
		info->name = "Alpha controlled";
		info->type = F0R_PARAM_BOOL;
		info->explanation = "Adjust only areas with nonzero alpha";
		break;
	case 6:
		info->name = "Luma formula";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "";
		break;
	}
}

//----------------------------------------------
f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
inst *in;

in=calloc(1,sizeof(inst));
in->w=width;
in->h=height;
in->r = 0.5;
in->g = 0.5;
in->b = 0.5;
in->act=1;	//change gamma
in->norm=1;	//keep luma
in->ac=0;	//alpha controlled OFF
in->cm=1;	//rec 709

in->lut=(lut_s*)calloc(1,sizeof(lut_s));
make_lut1(0.5,0.5,0.5,in->lut,0,1);

return (f0r_instance_t)in;
}

//---------------------------------------------------
void f0r_destruct(f0r_instance_t instance)
{
inst *in;

in=(inst*)instance;

free(in->lut);
free(instance);
}

//-----------------------------------------------------
void f0r_set_param_value(f0r_instance_t instance, f0r_param_t parm, int param_index)
{
inst *p;
double tmpf;
int tmpi,chg;

p=(inst*)instance;

chg=0;
switch(param_index)
	{
	case 0:
		tmpf=*(double*)parm;
		if (tmpf!=p->r) chg=1;
		p->r=tmpf;
		break;
	case 1:
		tmpf=*(double*)parm;
		if (tmpf!=p->g) chg=1;
		p->g=tmpf;
		break;
	case 2:
		tmpf=*(double*)parm;
		if (tmpf!=p->b) chg=1;
		p->b=tmpf;
		break;
	case 3:
                tmpi=map_value_forward(*((double*)parm), 0.0, 2.9999);
		if (tmpi != p->act) chg=1;
		p->act=tmpi;
		break;
	case 4:
                tmpi=map_value_forward(*((double*)parm), 0.0, 1.0); //BOOL!!
                if (p->norm != tmpi) chg=1;
                p->norm=tmpi;
                break;
	case 5:
                tmpi=map_value_forward(*((double*)parm), 0.0, 1.0); //BOOL!!
                if (p->ac != tmpi) chg=1;
                p->ac=tmpi;
                break;
	case 6:
                tmpi=map_value_forward(*((double*)parm), 0.0, 1.9999);
                if (p->cm != tmpi) chg=1;
                p->cm=tmpi;
                break;
	}

if (chg==0) return;

switch(p->act)
	{
	case 0:
		make_lut1(p->r,p->g,p->b,p->lut,p->norm,p->cm);
		break;
	case 1:
		make_lut2(p->r,p->g,p->b,p->lut,p->norm,p->cm);
		break;
	case 2:
		make_lut3(p->r,p->g,p->b,p->lut,p->norm,p->cm);
		break;
	}

}

//--------------------------------------------------
void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index)
{
inst *p;

p=(inst*)instance;

switch(param_index)
	{
	case 0:
		*((double*)param)=p->r;
		break;
	case 1:
		*((double*)param)=p->g;
		break;
	case 2:
		*((double*)param)=p->b;
		break;
	case 3:
		*((double*)param)=map_value_backward(p->act, 0.0, 2.9999);
		break;
	case 4:
                *((double*)param)=map_value_backward(p->norm, 0.0, 1.0);//BOOL!!
		break;
	case 5:
                *((double*)param)=map_value_backward(p->ac, 0.0, 1.0);//BOOL!!
		break;
	case 6:
                *((double*)param)=map_value_backward(p->cm, 0.0, 1.9999);
		break;
	}
}

//-------------------------------------------------
void f0r_update(f0r_instance_t instance, double time, const uint32_t* inframe, uint32_t* outframe)
{
inst *in;

assert(instance);
in=(inst*)instance;

apply_lut(inframe,outframe,in->w*in->h, in->lut, in->ac);

}

//**********************************************************
