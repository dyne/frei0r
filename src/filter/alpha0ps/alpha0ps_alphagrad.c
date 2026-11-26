/*
alphagrad.c

This frei0r plugin fills alpha channel with a gradient
Version 0.1	aug 2010

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

#include <stdio.h>
#include <frei0r.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#if defined(__SSE2__)
#include <emmintrin.h>
#endif

typedef struct
{
int h;
int w;

float poz,wdt,tilt,min,max;
uint8_t *gr8;
int op;

} inst;

static void fill_grad(inst *in)
{
int i,j;
float st,ct,po,wd,d,a;

st=sinf(in->tilt);
ct=cosf(in->tilt);
po=(-in->w/2.0+in->poz*in->w)*1.5;
wd=in->wdt*in->w;

if (in->min==in->max)
	{
	uint8_t v = (uint8_t)(uint32_t)(in->min*255.0);
	memset(in->gr8, v, (size_t)in->h*in->w);
	return;
	}

float half_wd = wd / 2.0f;
float range = in->max - in->min;
for (i=0;i<in->h;i++)
	{
	float row_base = (i-in->h/2)*ct - po;
	uint8_t *row = in->gr8 + (size_t)i * in->w;
	for (j=0;j<in->w;j++)
		{
		d=row_base+(j-in->w/2)*st;
		if (fabsf(d)>half_wd)
			{
			if (d>0.0f)
				a=in->min;
			else
				a=in->max;
			}
		else
			{
			if (d>half_wd) d=half_wd;
			a = in->min+(wd/2.0-d) / wd*range;
			}
		a=255.0*a;
		row[j] = (uint8_t)(uint32_t)a;
		}
	}
}

static float map_value_forward(double v, float min, float max)
{
return min+(max-min)*v;
}

static double map_value_backward(float v, float min, float max)
{
return (v-min)/(max-min);
}

int f0r_init()
{
return 1;
}

void f0r_deinit()
{
}

void f0r_get_plugin_info(f0r_plugin_info_t* info)
{
info->name="alphagrad";
info->author="Marko Cebokli";
info->plugin_type=F0R_PLUGIN_TYPE_FILTER;
info->color_model=F0R_COLOR_MODEL_RGBA8888;
info->frei0r_version=FREI0R_MAJOR_VERSION;
info->major_version=0;
info->minor_version=2;
info->num_params=6;
info->explanation="Fills alpha channel with a gradient";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
switch(param_index)
	{
	case 0:
		info->name = "Position";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "";
		break;
	case 1:
		info->name = "Transition width";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "";
		break;
	case 2:
		info->name = "Tilt";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "";
		break;
	case 3:
		info->name = "Min";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "";
		break;
	case 4:
		info->name = "Max";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "";
		break;
	case 5:
		info->name = "Operation";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "";
		break;
	}
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
inst *in;

in=calloc(1,sizeof(inst));
in->w=width;
in->h=height;

in->poz=0.5f;
in->wdt=0.5f;
in->tilt=0.0f;
in->min=0.0f;
in->max=1.0f;
in->op=0;

in->gr8 = (uint8_t*)calloc((size_t)in->w*in->h, sizeof(uint8_t));

fill_grad(in);

return (f0r_instance_t)in;
}

void f0r_destruct(f0r_instance_t instance)
{
inst *in = (inst*)instance;

free(in->gr8);
free(instance);
}

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
		tmpf=*((double*)parm);
		if (tmpf!=p->poz) chg=1;
		p->poz=tmpf;
		break;
	case 1:
		tmpf=*((double*)parm);
		if (tmpf!=p->wdt) chg=1;
		p->wdt=tmpf;
		break;
	case 2:
		tmpf=map_value_forward(*((double*)parm), -3.15f, 3.15f);
		if (tmpf!=p->tilt) chg=1;
		p->tilt=tmpf;
		break;
	case 3:
		tmpf=*((double*)parm);
		if (tmpf!=p->min) chg=1;
		p->min=tmpf;
		break;
	case 4:
		tmpf=*((double*)parm);
		if (tmpf!=p->max) chg=1;
		p->max=tmpf;
		break;
	case 5:
		tmpi=map_value_forward(*((double*)parm), 0.0f, 4.9999f);
		p->op=tmpi;
		break;
	}

if (chg==0) return;

fill_grad(p);
}

void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index)
{
inst *p;

p=(inst*)instance;

switch(param_index)
	{
	case 0:
		*((double*)param)=p->poz;
		break;
	case 1:
		*((double*)param)=p->wdt;
		break;
	case 2:
		*((double*)param)=map_value_backward(p->tilt, -3.15f, 3.15f);
		break;
	case 3:
		*((double*)param)=p->min;
		break;
	case 4:
		*((double*)param)=p->max;
		break;
	case 5:
		*((double*)param)=map_value_backward(p->op, 0.0f, 4.9999f);
		break;
	}
}

#if defined(__SSE2__)
static inline __m128i load_4_alpha_shifted(const uint8_t *gr)
{
	uint32_t g4;
	memcpy(&g4, gr, sizeof(g4));
	__m128i v = _mm_cvtsi32_si128((int)g4);
	v = _mm_unpacklo_epi8(v, _mm_setzero_si128());
	v = _mm_unpacklo_epi16(v, _mm_setzero_si128());
	return _mm_slli_epi32(v, 24);
}
#endif

//RGBA8888 little endian
void f0r_update(f0r_instance_t instance, double time, const uint32_t* inframe, uint32_t* outframe)
{
inst *in;
int i = 0;

assert(instance);
in=(inst*)instance;
const int n = in->h * in->w;
const uint8_t *gr = in->gr8;

switch (in->op)
	{
	case 0:		//write on clear
#if defined(__SSE2__)
		{
		const __m128i rgb_mask = _mm_set1_epi32(0x00FFFFFF);
		for (; i + 3 < n; i += 4) {
			__m128i px = _mm_loadu_si128((const __m128i *)(inframe + i));
			__m128i a = load_4_alpha_shifted(gr + i);
			_mm_storeu_si128((__m128i *)(outframe + i), _mm_or_si128(_mm_and_si128(px, rgb_mask), a));
		}
		}
#endif
		for (; i < n; i++)
			outframe[i] = (inframe[i] & 0x00FFFFFFu) | ((uint32_t)gr[i] << 24);
		break;
	case 1:		//max
#if defined(__SSE2__)
		{
		const __m128i rgb_mask = _mm_set1_epi32(0x00FFFFFF);
		for (; i + 3 < n; i += 4) {
			__m128i px = _mm_loadu_si128((const __m128i *)(inframe + i));
			__m128i a = load_4_alpha_shifted(gr + i);
			__m128i mx = _mm_max_epu8(px, a);
			_mm_storeu_si128((__m128i *)(outframe + i), _mm_or_si128(_mm_and_si128(px, rgb_mask), _mm_andnot_si128(rgb_mask, mx)));
		}
		}
#endif
		for (; i < n; i++) {
			uint32_t a1 = inframe[i] >> 24;
			uint32_t a2 = gr[i];
			outframe[i] = (inframe[i] & 0x00FFFFFFu) | (((a1 > a2) ? a1 : a2) << 24);
		}
		break;
	case 2:		//min
#if defined(__SSE2__)
		{
		const __m128i rgb_mask = _mm_set1_epi32(0x00FFFFFF);
		for (; i + 3 < n; i += 4) {
			__m128i px = _mm_loadu_si128((const __m128i *)(inframe + i));
			__m128i a = load_4_alpha_shifted(gr + i);
			__m128i mn = _mm_min_epu8(px, a);
			_mm_storeu_si128((__m128i *)(outframe + i), _mm_or_si128(_mm_and_si128(px, rgb_mask), _mm_andnot_si128(rgb_mask, mn)));
		}
		}
#endif
		for (; i < n; i++) {
			uint32_t a1 = inframe[i] >> 24;
			uint32_t a2 = gr[i];
			outframe[i] = (inframe[i] & 0x00FFFFFFu) | (((a1 < a2) ? a1 : a2) << 24);
		}
		break;
	case 3:		//add
#if defined(__SSE2__)
		{
		const __m128i rgb_mask = _mm_set1_epi32(0x00FFFFFF);
		for (; i + 3 < n; i += 4) {
			__m128i px = _mm_loadu_si128((const __m128i *)(inframe + i));
			__m128i a = load_4_alpha_shifted(gr + i);
			__m128i ad = _mm_adds_epu8(px, a);
			_mm_storeu_si128((__m128i *)(outframe + i), _mm_or_si128(_mm_and_si128(px, rgb_mask), _mm_andnot_si128(rgb_mask, ad)));
		}
		}
#endif
		for (; i < n; i++) {
			uint32_t t = (inframe[i] >> 24) + gr[i];
			if (t > 255u) t = 255u;
			outframe[i] = (inframe[i] & 0x00FFFFFFu) | (t << 24);
		}
		break;
	case 4:		//subtract
#if defined(__SSE2__)
		{
		const __m128i rgb_mask = _mm_set1_epi32(0x00FFFFFF);
		for (; i + 3 < n; i += 4) {
			__m128i px = _mm_loadu_si128((const __m128i *)(inframe + i));
			__m128i a = load_4_alpha_shifted(gr + i);
			__m128i sb = _mm_subs_epu8(px, a);
			_mm_storeu_si128((__m128i *)(outframe + i), _mm_or_si128(_mm_and_si128(px, rgb_mask), _mm_andnot_si128(rgb_mask, sb)));
		}
		}
#endif
		for (; i < n; i++) {
			uint32_t a1 = inframe[i] >> 24;
			uint32_t a2 = gr[i];
			uint32_t t = (a1 > a2) ? (a1 - a2) : 0u;
			outframe[i] = (inframe[i] & 0x00FFFFFFu) | (t << 24);
		}
		break;
	default:
		break;
	}
}
