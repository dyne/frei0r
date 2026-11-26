/*
alpha0ps.c

This frei0r plugin is for display & manipulation of alpha channel
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
 
 
 
  28 aug 2012	ver 0.2		endian proofing
  
  03 sep 2012	ver 0.3		add alpha blur
  
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

#include "fibe_f.h"

typedef struct
{
	int h;
	int w;
	
	//parameters
	int disp;
	int din;
	int op;
	float thr;
	float sga;
	int inv;
	
	//auxiliary variables for fibe2o
	float f,q,a0,a1,a2,b0,b1,b2,rd1,rd2,rs1,rs2,rc1,rc2;

	//cached scratch buffers to avoid per-frame calloc/free
	float *falpha;
	float *ab;
} inst;

static void zero_border(float *ab, int w, int h)
{
	memset(ab, 0, (size_t)w * sizeof(float));
	if (h > 1) {
		memset(ab + (size_t)(h - 1) * w, 0, (size_t)w * sizeof(float));
	}
	for (int i = 1; i < h - 1; i++) {
		ab[(size_t)i * w] = 0.0f;
		ab[(size_t)i * w + (w - 1)] = 0.0f;
	}
}

static void alphagray(inst *in, const uint8_t *infr, uint8_t *oufr)
{
	int n = in->w * in->h;
	const uint8_t *src = (in->din == 0) ? oufr : infr;
	uint32_t *dst32 = (uint32_t *)oufr;
	for (int i = 0; i < n; i++) {
		uint32_t s = src[4 * i + 3];
		dst32[i] = s | (s << 8) | (s << 16) | 0xFF000000u;
	}
}

static void grayred(inst *in, const uint8_t *infr, uint8_t *oufr)
{
	int n = in->w * in->h;
	const uint8_t *asrc = (in->din == 0) ? oufr : infr;
	uint32_t *dst32 = (uint32_t *)oufr;
	for (int i = 0; i < n; i++) {
		uint32_t r = infr[4 * i];
		uint32_t g = infr[4 * i + 1];
		uint32_t b = infr[4 * i + 2];
		uint32_t a = asrc[4 * i + 3];
		uint32_t y = (uint8_t)((r >> 2) + (g >> 1) + (b >> 2));
		y = 64u + (y >> 1);
		uint32_t rr = y + (a >> 1);
		if (rr > 255u) rr = 255u;
		dst32[i] = rr | (y << 8) | (y << 16) | 0xFF000000u;
	}
}

static void drawsel(inst *in, const uint8_t *infr, uint8_t *oufr, int bg)
{
	int w = in->w, h = in->h;
	int n = w * h;
	const uint8_t *src = (in->din == 0) ? oufr : infr;
	uint32_t *dst32 = (uint32_t *)oufr;

	if (bg != 3) {
		uint32_t bk = 0;
		switch (bg) {
		case 0: bk = 0x00; break;
		case 1: bk = 0x80; break;
		case 2: bk = 0xFF; break;
		default: break;
		}
		for (int i = 0; i < n; i++) {
			uint32_t r = src[4 * i];
			uint32_t g = src[4 * i + 1];
			uint32_t b = src[4 * i + 2];
			uint32_t a = src[4 * i + 3];
			uint32_t ia_bk = (255u - a) * bk;
			r = (a * r + ia_bk) >> 8;
			g = (a * g + ia_bk) >> 8;
			b = (a * b + ia_bk) >> 8;
			dst32[i] = r | (g << 8) | (b << 16) | 0xFF000000u;
		}
	} else {
		for (int y = 0; y < h; y++) {
			int ybit = (y >> 3) & 1;
			int row = y * w;
			for (int x = 0; x < w; x++) {
				int i = row + x;
				uint32_t bk = (((i >> 3) & 1) ^ ybit) ? 0x64u : 0x9Bu;
				uint32_t r = src[4 * i];
				uint32_t g = src[4 * i + 1];
				uint32_t b = src[4 * i + 2];
				uint32_t a = src[4 * i + 3];
				uint32_t ia_bk = (255u - a) * bk;
				r = (a * r + ia_bk) >> 8;
				g = (a * g + ia_bk) >> 8;
				b = (a * b + ia_bk) >> 8;
				dst32[i] = r | (g << 8) | (b << 16) | 0xFF000000u;
			}
		}
	}
}

//----------------------------------------------------------
//shave based on average of 8 neighbors (writes result into ab; caller swaps)
static void shave_alpha_step(const float *sl, float *ab, int w, int h)
{
	zero_border(ab, w, h);
#if defined(__SSE2__)
	const __m128 v8 = _mm_set1_ps(0.125f);
#endif
	for (int i = 1; i < h - 1; i++) {
		int p = i * w + 1;
		int j = 1;
#if defined(__SSE2__)
		for (; j + 3 < w - 1; j += 4, p += 4) {
			__m128 s = _mm_loadu_ps(sl + p - 1);
			s = _mm_add_ps(s, _mm_loadu_ps(sl + p + 1));
			s = _mm_add_ps(s, _mm_loadu_ps(sl + p - w));
			s = _mm_add_ps(s, _mm_loadu_ps(sl + p + w));
			s = _mm_add_ps(s, _mm_loadu_ps(sl + p - 1 - w));
			s = _mm_add_ps(s, _mm_loadu_ps(sl + p + 1 + w));
			s = _mm_add_ps(s, _mm_loadu_ps(sl + p + 1 - w));
			s = _mm_add_ps(s, _mm_loadu_ps(sl + p - 1 + w));
			__m128 m = _mm_mul_ps(s, v8);
			__m128 c = _mm_loadu_ps(sl + p);
			_mm_storeu_ps(ab + p, _mm_min_ps(c, m));
		}
#endif
		for (; j < w - 1; j++, p++) {
			float m = sl[p-1] + sl[p+1] + sl[p-w] + sl[p+w] + sl[p-1-w] + sl[p+1+w] + sl[p+1-w] + sl[p-1+w];
			m = m * 0.125f;
			ab[p] = (sl[p] < m) ? sl[p] : m;
		}
	}
}

#if defined(__SSE2__)
static inline __m128 sse_select_lt(__m128 c, __m128 n, __m128 cur)
{
	__m128 mask = _mm_cmplt_ps(c, n);
	return _mm_or_ps(_mm_and_ps(mask, n), _mm_andnot_ps(mask, cur));
}

static inline __m128 sse_select_gt(__m128 c, __m128 n, __m128 cur)
{
	__m128 mask = _mm_cmpgt_ps(c, n);
	return _mm_or_ps(_mm_and_ps(mask, n), _mm_andnot_ps(mask, cur));
}
#endif

static void grow_alpha_step(const float *al, float *ab, int w, int h, int mode)
{
	zero_border(ab, w, h);
	if (mode == 0) {
		for (int i = 1; i < h - 1; i++) {
			int p = i * w + 1;
			int j = 1;
#if defined(__SSE2__)
			for (; j + 3 < w - 1; j += 4, p += 4) {
				__m128 c = _mm_loadu_ps(al + p);
				__m128 r = sse_select_lt(c, _mm_loadu_ps(al + p - 1), c);
				r = sse_select_lt(c, _mm_loadu_ps(al + p + 1), r);
				r = sse_select_lt(c, _mm_loadu_ps(al + p - w), r);
				r = sse_select_lt(c, _mm_loadu_ps(al + p + w), r);
				_mm_storeu_ps(ab + p, r);
			}
#endif
			for (; j < w - 1; j++, p++) {
				float c = al[p], r = c;
				if (c < al[p-1]) r = al[p-1];
				if (c < al[p+1]) r = al[p+1];
				if (c < al[p-w]) r = al[p-w];
				if (c < al[p+w]) r = al[p+w];
				ab[p] = r;
			}
		}
	} else if (mode == 1) {
		for (int i = 1; i < h - 1; i++) {
			int p = i * w + 1;
			for (int j = 1; j < w - 1; j++, p++) {
				float c = al[p];
				float m = c;
				if (c < al[p-1]) m = al[p-1];
				if (c < al[p+1]) m = al[p+1];
				if (c < al[p-w]) m = al[p-w];
				if (c < al[p+w]) m = al[p+w];
				float md = c;
				if (c < al[p-1-w]) md = al[p-1-w];
				if (c < al[p+1-w]) md = al[p+1-w];
				if (c < al[p-1+w]) md = al[p-1+w];
				if (c < al[p+1+w]) md = al[p+1+w];
				ab[p] = (float)(0.4 * c + 0.4 * m + 0.2 * md);
			}
		}
	}
}

static void shrink_alpha_step(const float *al, float *ab, int w, int h, int mode)
{
	zero_border(ab, w, h);
	if (mode == 0) {
		for (int i = 1; i < h - 1; i++) {
			int p = i * w + 1;
			int j = 1;
#if defined(__SSE2__)
			for (; j + 3 < w - 1; j += 4, p += 4) {
				__m128 c = _mm_loadu_ps(al + p);
				__m128 r = sse_select_gt(c, _mm_loadu_ps(al + p - 1), c);
				r = sse_select_gt(c, _mm_loadu_ps(al + p + 1), r);
				r = sse_select_gt(c, _mm_loadu_ps(al + p - w), r);
				r = sse_select_gt(c, _mm_loadu_ps(al + p + w), r);
				_mm_storeu_ps(ab + p, r);
			}
#endif
			for (; j < w - 1; j++, p++) {
				float c = al[p], r = c;
				if (c > al[p-1]) r = al[p-1];
				if (c > al[p+1]) r = al[p+1];
				if (c > al[p-w]) r = al[p-w];
				if (c > al[p+w]) r = al[p+w];
				ab[p] = r;
			}
		}
	} else if (mode == 1) {
		for (int i = 1; i < h - 1; i++) {
			int p = i * w + 1;
			for (int j = 1; j < w - 1; j++, p++) {
				float c = al[p];
				float m = c;
				if (c > al[p-1]) m = al[p-1];
				if (c > al[p+1]) m = al[p+1];
				if (c > al[p-w]) m = al[p-w];
				if (c > al[p+w]) m = al[p+w];
				float md = c;
				if (c > al[p-1-w]) md = al[p-1-w];
				if (c > al[p+1-w]) md = al[p+1-w];
				if (c > al[p-1+w]) md = al[p-1+w];
				if (c > al[p+1+w]) md = al[p+1+w];
				ab[p] = (float)(0.4 * c + 0.4 * m + 0.2 * md);
			}
		}
	}
}

static void blur_alpha(inst *in, float *falpha)
{
	int n = in->w * in->h;
	for (int i = 0; i < n; i++) falpha[i] *= 0.0039215;
	
	fibe2o_f(falpha, in->w, in->h, in->a1, in->a2, in->rd1, in->rd2, in->rs1, in->rs2, in->rc1, in->rc2, 1);
	
	for (int i = 0; i < n; i++) {
		float v = falpha[i] * 255.0;
		if (v > 255.0f) v = 255.0f;
		if (v < 0.0f) v = 0.0f;
		falpha[i] = v;
	}
}

static float AitNev3(int t, float xt[], float yt[], float x)
{
	float p[10];
	int i,j,m;
	float zero = 0.0f;
	
	if ((x<xt[0])||(x>xt[t-1]))
		return 1.0f/zero;
	
	m=0; while (x>xt[m++]);
	m=m-4/2-1; if (m<0) m=0; if ((m+4)>(t-1)) m=t-4;
	
	for (i=0;i<4;i++)
		p[i]=yt[i+m];
	
	for (j=1;j<4;j++)
		for (i=(4-1);i>=j;i--)
			p[i]=p[i]+(x-xt[i+m])/(xt[i+m]-xt[i-j+m])*(p[i]-p[i-1]);
	return p[4-1];
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
	info->name="alpha0ps";
	info->author="Marko Cebokli";
	info->plugin_type=F0R_PLUGIN_TYPE_FILTER;
	info->color_model=F0R_COLOR_MODEL_RGBA8888;
	info->frei0r_version=FREI0R_MAJOR_VERSION;
	info->major_version=0;
	info->minor_version=4;
	info->num_params=6;
	info->explanation="Display and manipulation of the alpha channel";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
	switch(param_index)
	{
	case 0:
		info->name = "Display";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "";
		break;
	case 1:
		info->name = "Display input alpha";
		info->type = F0R_PARAM_BOOL;
		info->explanation = "";
		break;
	case 2:
		info->name = "Operation";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "";
		break;
	case 3:
		info->name = "Threshold";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "";
		break;
	case 4:
		info->name = "Shrink/Grow/Blur amount";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "";
		break;
	case 5:
		info->name = "Invert";
		info->type = F0R_PARAM_BOOL;
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
	
	in->disp=0;
	in->din=0;
	in->op=0;
	in->thr=0.5f;
	in->sga=1.0f;
	in->inv=0;
	
	in->f=0.05f; in->q=0.55f;
	calcab_lp1(in->f, in->q, &in->a0, &in->a1, &in->a2, &in->b0, &in->b1, &in->b2);
	in->a1=in->a1/in->a0; in->a2=in->a2/in->a0;
	rep(-0.5f, 0.5f, 0.0f, &in->rd1, &in->rd2, 256, in->a1, in->a2);
	rep(1.0f, 1.0f, 0.0f, &in->rs1, &in->rs2, 256, in->a1, in->a2);
	rep(0.0f, 0.0f, 1.0f, &in->rc1, &in->rc2, 256, in->a1, in->a2);

	in->falpha = NULL;
	in->ab = NULL;
	
	return (f0r_instance_t)in;
}

void f0r_destruct(f0r_instance_t instance)
{
	inst *in = (inst*)instance;
	free(in->falpha);
	free(in->ab);
	free(instance);
}

void f0r_set_param_value(f0r_instance_t instance, f0r_param_t parm, int param_index)
{
	inst *p;
	double tmpf;
	int tmpi,chg;
	
	p=(inst*)instance;
	
	float am1[]={0.499999f,0.7f,1.0f,1.5f,2.0f,3.0f,4.0f,5.0f,7.0f,10.0f,15.0f,20.0f,30.0f,40.0f,50.0f,70.0f,100.0f,150.0f,200.00001f};
	float iir2f[]={0.475f,0.39f,0.325f,0.26f,0.21f,0.155f,0.112f,0.0905f,0.065f,0.0458f,0.031f,0.0234f,0.01575f,0.0118f,0.0093f,0.00725f,0.00505f,0.0033f,0.0025f};
	float iir2q[]={0.53f,0.53f,0.54f,0.54f,0.54f,0.55f,0.6f,0.6f,0.6f,0.6f,0.6f,0.6f,0.6f,0.6f,0.6f,0.6f,0.6f,0.6f,0.6f};
	
	chg=0;
	switch(param_index)
	{
	case 0:
		tmpi=map_value_forward(*((double*)parm), 0.0f, 6.9999f);
		if (p->disp != tmpi) chg=1;
		p->disp=tmpi;
		break;
	case 1:
		tmpi=map_value_forward(*((double*)parm), 0.0f, 1.0f);
		if (p->din != tmpi) chg=1;
		p->din=tmpi;
		break;
	case 2:
		tmpi=map_value_forward(*((double*)parm), 0.0f, 7.9999f);
		if (p->op != tmpi) chg=1;
		p->op=tmpi;
		break;
	case 3:
		tmpf=*(double*)parm;
		if (tmpf!=p->thr) chg=1;
		p->thr=tmpf;
		break;
	case 4:
		tmpf=map_value_forward(*((double*)parm), 0.0f, 4.9999f);
		if (p->sga != tmpf) chg=1;
		p->sga=tmpf;
		break;
	case 5:
		tmpi=map_value_forward(*((double*)parm), 0.0f, 1.0f);
		if (p->inv != tmpi) chg=1;
		p->inv=tmpi;
		break;
	}
	
	if (chg==0) return;
	
	if (param_index==4)
	{
		p->f=AitNev3(19, am1, iir2f, 0.5+3.0*p->sga);
		p->q=AitNev3(19, am1, iir2q, 0.5+3.0*p->sga);
		calcab_lp1(p->f, p->q, &p->a0, &p->a1, &p->a2, &p->b0, &p->b1, &p->b2);
		p->a1=p->a1/p->a0; p->a2=p->a2/p->a0;
		rep(-0.5f, 0.5f, 0.0f, &p->rd1, &p->rd2, 256, p->a1, p->a2);
		rep(1.0f, 1.0f, 0.0f, &p->rs1, &p->rs2, 256, p->a1, p->a2);
		rep(0.0f, 0.0f, 1.0f, &p->rc1, &p->rc2, 256, p->a1, p->a2);
	}
}

void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index)
{
	inst *p;
	
	p=(inst*)instance;
	
	switch(param_index)
	{
	case 0:
		*((double*)param)=map_value_backward(p->disp, 0.0f, 6.9999f);
		break;
	case 1:
		*((double*)param)=map_value_backward(p->din, 0.0f, 1.0f);
		break;
	case 2:
		*((double*)param)=map_value_backward(p->op, 0.0f, 7.9999f);
		break;
	case 3:
		*((double*)param)=p->thr;
		break;
	case 4:
		*((double*)param)=map_value_backward(p->sga, 0.0f, 4.9999f);
		break;
	case 5:
		*((double*)param)=map_value_backward(p->inv, 0.0f, 1.0f);
		break;
	}
}

void f0r_update(f0r_instance_t instance, double time, const uint32_t* inframe, uint32_t* outframe)
{
	inst *in;
	assert(instance);
	in=(inst*)instance;

	const int w = in->w, h = in->h;
	const int n = w * h;
	const uint8_t *infr = (const uint8_t *)inframe;
	uint8_t *oufr = (uint8_t *)outframe;

	/* Fast pointwise path for op == 0 (NOP) and op == 6 (threshold): zero float buffers */
	if (in->op == 0 || in->op == 6) {
		if (in->op == 0) {
			if (in->inv == 1) {
				for (int i = 0; i < n; i++) {
					uint32_t px = inframe[i];
					uint32_t a = 255u - (px >> 24);
					outframe[i] = (px & 0x00FFFFFFu) | (a << 24);
				}
			} else {
				if (outframe != inframe) {
					memcpy(outframe, inframe, (size_t)n * sizeof(uint32_t));
				}
			}
		} else { /* op == 6: threshold */
			float thr_val = (float)(255.0 * in->thr);
			uint32_t hi = (in->inv == 1) ? 0x00000000u : 0xFF000000u;
			uint32_t lo = (in->inv == 1) ? 0xFF000000u : 0x00000000u;
			for (int i = 0; i < n; i++) {
				uint32_t px = inframe[i];
				float a = (float)(px >> 24);
				outframe[i] = (px & 0x00FFFFFFu) | ((a > thr_val) ? hi : lo);
			}
		}
	} else {
		if (!in->falpha) {
			in->falpha = (float *)malloc((size_t)n * sizeof(float));
			in->ab = (float *)malloc((size_t)n * sizeof(float));
			if (!in->falpha || !in->ab) return;
		}
		float *cur = in->falpha;
		float *nxt = in->ab;

		int i = 0;
#if defined(__SSE2__)
		for (; i + 3 < n; i += 4) {
			__m128i px = _mm_loadu_si128((const __m128i *)(inframe + i));
			__m128 fa = _mm_cvtepi32_ps(_mm_srli_epi32(px, 24));
			_mm_storeu_ps(cur + i, fa);
		}
#endif
		for (; i < n; i++) {
			cur[i] = (float)(inframe[i] >> 24);
		}

		switch (in->op) {
		case 1:
			for (int iter = 0; iter < in->sga; iter++) {
				shave_alpha_step(cur, nxt, w, h);
				float *tmp = cur; cur = nxt; nxt = tmp;
			}
			break;
		case 2:
			for (int iter = 0; iter < in->sga; iter++) {
				shrink_alpha_step(cur, nxt, w, h, 0);
				float *tmp = cur; cur = nxt; nxt = tmp;
			}
			break;
		case 3:
			for (int iter = 0; iter < in->sga; iter++) {
				shrink_alpha_step(cur, nxt, w, h, 1);
				float *tmp = cur; cur = nxt; nxt = tmp;
			}
			break;
		case 4:
			for (int iter = 0; iter < in->sga; iter++) {
				grow_alpha_step(cur, nxt, w, h, 0);
				float *tmp = cur; cur = nxt; nxt = tmp;
			}
			break;
		case 5:
			for (int iter = 0; iter < in->sga; iter++) {
				grow_alpha_step(cur, nxt, w, h, 1);
				float *tmp = cur; cur = nxt; nxt = tmp;
			}
			break;
		case 7:
			blur_alpha(in, cur);
			break;
		default:
			break;
		}

		if (in->inv == 1) {
			i = 0;
#if defined(__SSE2__)
			const __m128 v255 = _mm_set1_ps(255.0f);
			const __m128i rgb_mask = _mm_set1_epi32(0x00FFFFFF);
			for (; i + 3 < n; i += 4) {
				__m128i px = _mm_loadu_si128((const __m128i *)(inframe + i));
				__m128 fa = _mm_sub_ps(v255, _mm_loadu_ps(cur + i));
				__m128i a32 = _mm_slli_epi32(_mm_cvttps_epi32(fa), 24);
				_mm_storeu_si128((__m128i *)(outframe + i), _mm_or_si128(_mm_and_si128(px, rgb_mask), a32));
			}
#endif
			for (; i < n; i++) {
				uint32_t a = (uint32_t)(uint8_t)(255.0f - cur[i]);
				outframe[i] = (inframe[i] & 0x00FFFFFFu) | (a << 24);
			}
		} else {
			i = 0;
#if defined(__SSE2__)
			const __m128i rgb_mask = _mm_set1_epi32(0x00FFFFFF);
			for (; i + 3 < n; i += 4) {
				__m128i px = _mm_loadu_si128((const __m128i *)(inframe + i));
				__m128i a32 = _mm_slli_epi32(_mm_cvttps_epi32(_mm_loadu_ps(cur + i)), 24);
				_mm_storeu_si128((__m128i *)(outframe + i), _mm_or_si128(_mm_and_si128(px, rgb_mask), a32));
			}
#endif
			for (; i < n; i++) {
				uint32_t a = (uint32_t)(uint8_t)cur[i];
				outframe[i] = (inframe[i] & 0x00FFFFFFu) | (a << 24);
			}
		}
	}

	switch (in->disp) {
	case 0:
		break;
	case 1:
		alphagray(in, infr, oufr);
		break;
	case 2:
		grayred(in, infr, oufr);
		break;
	case 3:
		drawsel(in, infr, oufr, 0);
		break;
	case 4:
		drawsel(in, infr, oufr, 1);
		break;
	case 5:
		drawsel(in, infr, oufr, 2);
		break;
	case 6:
		drawsel(in, infr, oufr, 3);
		break;
	default:
		break;
	}
}
