/*
select0r.c

This frei0r plugin   makes a color based alpha selection
Version 0.4	apr 2012

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

//	apr 2012	added slope parameter

#include <frei0r.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#if defined(__SSE2__)
#include <emmintrin.h>
#endif

typedef struct
{
	float r;
	float g;
	float b;
	float a;
} float_rgba;

typedef struct
{
	float x;
	float y;
	float z;
} triplet;

static const double PI=3.14159265358979;

#define SELECT0R_TILE 256

//-----------------------------------------------------------
//inline functions for subspace metrics
static inline float dist_box(float cx, float cy, float cz, float dx, float dy, float dz, float x, float y, float z)
{
	float ax,ay,az,r;
	
	ax=fabsf(x-cx)*dx;
	ay=fabsf(y-cy)*dy;
	az=fabsf(z-cz)*dz;
	r=ax;
	if (ay>r) r=ay;
	if (az>r) r=az;
	r=r*r;
	return r;
}

static inline float dist_eli(float cx, float cy, float cz, float dx, float dy, float dz, float x, float y, float z)
{
	float ax,ay,az,r;
	
	ax=(x-cx)*dx;
	ay=(y-cy)*dy;
	az=(z-cz)*dz;
	r=ax*ax+ay*ay+az*az;
	return r;
}

static inline float dist_oct(float cx, float cy, float cz, float dx, float dy, float dz, float x, float y, float z)
{
	float ax,ay,az,r;
	
	ax=fabsf(x-cx)*dx;
	ay=fabsf(y-cy)*dy;
	az=fabsf(z-cz)*dz;
	r=ax+ay+az;
	r=r*r;
	return r;
}

static inline float dist_box_c(float chue, float cy, float cz, float dhue, float dy, float dz, float hue, float y, float z)
{
	float ax,ay,az,r;
	
	ax = 0.5f - fabsf(fabsf(hue - chue) - 0.5f);
	ax=ax*dhue;
	ay=fabsf(y-cy)*dy;
	az=fabsf(z-cz)*dz;
	r=ax;
	if (ay>r) r=ay;
	if (az>r) r=az;
	r=r*r;
	return r;
}

static inline float dist_eli_c(float chue, float cy, float cz, float dhue, float dy, float dz, float hue, float y, float z)
{
	float ax,ay,az,r;
	
	ax = 0.5f - fabsf(fabsf(hue - chue) - 0.5f);
	ax=ax*dhue;
	ay=(y-cy)*dy;
	az=(z-cz)*dz;
	r=ax*ax+ay*ay+az*az;
	return r;
}

static inline float dist_oct_c(float chue, float cy, float cz, float dhue, float dy, float dz, float hue, float y, float z)
{
	float ax,ay,az,r;
	
	ax = 0.5f - fabsf(fabsf(hue - chue) - 0.5f);
	ax=ax*dhue;
	ay=fabsf(y-cy)*dy;
	az=fabsf(z-cz)*dz;
	r=ax+ay+az;
	r=r*r;
	return r;
}

//----------------------------------------------------------
static inline void rgb2abi(float k32, float r, float g, float b, float *a, float *bb, float *i)
{
	*a=r-0.5*g-0.5*b;
	*bb=k32*(g-b);
	*i=0.3333*(r+g+b);
}

static inline void rgb2hci(float ipi2, float k32, float r, float g, float b, float *h, float *c, float *i)
{
	float a,bb;
	a=r-0.5*g-0.5*b;
	bb=k32*(g-b);
	*h=atan2(bb,a)*ipi2;
	*c=hypotf(a,bb);
	*i=0.3333*(r+g+b);
}

//------------------------------------------------------
static inline float thres(float a)
{
	return (a<1.0) ? 1.0 : 0.0;
}

static inline float fat(float a)
{
	a=a*a*a*a;
	return (a<1.0) ? 1.0-a : 0.0;
}

static inline float norm(float a)
{
	a=a*a;
	return (a<1.0) ? 1.0-a : 0.0;
}

static inline float skiny(float a)
{
	return (a<1.0) ? 1.0-a : 0.0;
}

static inline float slope(float a, float is)
{
	a = (a<1.0) ? 1.0 : 1.0-is*(a-1);
	return (a>=0) ? a : 0.0;
}

#if defined(__SSE2__)
static inline __m128 sse_fabs_ps(__m128 v)
{
	const __m128 sign_mask = _mm_castsi128_ps(_mm_set1_epi32(0x7FFFFFFF));
	return _mm_and_ps(v, sign_mask);
}

static inline void unpack_4px_rgb_ps(const uint32_t *src, __m128 *vr, __m128 *vg, __m128 *vb)
{
	const __m128i mask = _mm_set1_epi32(0xFF);
	const __m128 vf1 = _mm_set1_ps(1.0f / 256.0f);
	__m128i px = _mm_loadu_si128((const __m128i *)src);
	*vr = _mm_mul_ps(_mm_cvtepi32_ps(_mm_and_si128(px, mask)), vf1);
	*vg = _mm_mul_ps(_mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(px, 8), mask)), vf1);
	*vb = _mm_mul_ps(_mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(px, 16), mask)), vf1);
}

static inline __m128 sse_dist_box(__m128 cx, __m128 cy, __m128 cz, __m128 dx, __m128 dy, __m128 dz, __m128 x, __m128 y, __m128 z)
{
	__m128 ax = _mm_mul_ps(sse_fabs_ps(_mm_sub_ps(x, cx)), dx);
	__m128 ay = _mm_mul_ps(sse_fabs_ps(_mm_sub_ps(y, cy)), dy);
	__m128 az = _mm_mul_ps(sse_fabs_ps(_mm_sub_ps(z, cz)), dz);
	__m128 r = _mm_max_ps(_mm_max_ps(ax, ay), az);
	return _mm_mul_ps(r, r);
}

static inline __m128 sse_dist_eli(__m128 cx, __m128 cy, __m128 cz, __m128 dx, __m128 dy, __m128 dz, __m128 x, __m128 y, __m128 z)
{
	__m128 ax = _mm_mul_ps(_mm_sub_ps(x, cx), dx);
	__m128 ay = _mm_mul_ps(_mm_sub_ps(y, cy), dy);
	__m128 az = _mm_mul_ps(_mm_sub_ps(z, cz), dz);
	return _mm_add_ps(_mm_add_ps(_mm_mul_ps(ax, ax), _mm_mul_ps(ay, ay)), _mm_mul_ps(az, az));
}

static inline __m128 sse_dist_oct(__m128 cx, __m128 cy, __m128 cz, __m128 dx, __m128 dy, __m128 dz, __m128 x, __m128 y, __m128 z)
{
	__m128 ax = _mm_mul_ps(sse_fabs_ps(_mm_sub_ps(x, cx)), dx);
	__m128 ay = _mm_mul_ps(sse_fabs_ps(_mm_sub_ps(y, cy)), dy);
	__m128 az = _mm_mul_ps(sse_fabs_ps(_mm_sub_ps(z, cz)), dz);
	__m128 r = _mm_add_ps(_mm_add_ps(ax, ay), az);
	return _mm_mul_ps(r, r);
}

static inline __m128 sse_edge(__m128 dd, __m128 vislp, int thr)
{
	const __m128 vone = _mm_set1_ps(1.0f);
	const __m128 vzero = _mm_setzero_ps();
	switch (thr) {
	case 0: /* thres */
		return _mm_and_ps(_mm_cmplt_ps(dd, vone), vone);
	case 1: { /* fat */
		__m128 d2 = _mm_mul_ps(dd, dd);
		__m128 a = _mm_mul_ps(d2, d2);
		__m128 mask = _mm_cmplt_ps(a, vone);
		return _mm_and_ps(mask, _mm_sub_ps(vone, a));
	}
	case 2: { /* norm */
		__m128 a = _mm_mul_ps(dd, dd);
		__m128 mask = _mm_cmplt_ps(a, vone);
		return _mm_and_ps(mask, _mm_sub_ps(vone, a));
	}
	case 3: { /* skiny */
		__m128 mask = _mm_cmplt_ps(dd, vone);
		return _mm_and_ps(mask, _mm_sub_ps(vone, dd));
	}
	case 4: { /* slope */
		__m128 lt = _mm_cmplt_ps(dd, vone);
		__m128 sl = _mm_sub_ps(vone, _mm_mul_ps(vislp, _mm_sub_ps(dd, vone)));
		__m128 a = _mm_or_ps(_mm_and_ps(lt, vone), _mm_andnot_ps(lt, sl));
		return _mm_max_ps(a, vzero);
	}
	default:
		return vzero;
	}
}
#endif

static void sel_rgb_tile(const uint32_t *in, float *aout, int count,
                         float kr, float kg, float kb,
                         float ddx, float ddy, float ddz,
                         float islp, int ss, int thr)
{
	int i = 0;
#if defined(__SSE2__)
	const __m128 vkr = _mm_set1_ps(kr), vkg = _mm_set1_ps(kg), vkb = _mm_set1_ps(kb);
	const __m128 vdx = _mm_set1_ps(ddx), vdy = _mm_set1_ps(ddy), vdz = _mm_set1_ps(ddz);
	const __m128 vislp = _mm_set1_ps(islp);

	if (ss == 0) {
		for (; i + 3 < count; i += 4) {
			__m128 vr, vg, vb;
			unpack_4px_rgb_ps(in + i, &vr, &vg, &vb);
			__m128 dd = sse_dist_box(vkr, vkg, vkb, vdx, vdy, vdz, vr, vg, vb);
			_mm_storeu_ps(aout + i, sse_edge(dd, vislp, thr));
		}
	} else if (ss == 1) {
		for (; i + 3 < count; i += 4) {
			__m128 vr, vg, vb;
			unpack_4px_rgb_ps(in + i, &vr, &vg, &vb);
			__m128 dd = sse_dist_eli(vkr, vkg, vkb, vdx, vdy, vdz, vr, vg, vb);
			_mm_storeu_ps(aout + i, sse_edge(dd, vislp, thr));
		}
	} else if (ss == 2) {
		for (; i + 3 < count; i += 4) {
			__m128 vr, vg, vb;
			unpack_4px_rgb_ps(in + i, &vr, &vg, &vb);
			__m128 dd = sse_dist_oct(vkr, vkg, vkb, vdx, vdy, vdz, vr, vg, vb);
			_mm_storeu_ps(aout + i, sse_edge(dd, vislp, thr));
		}
	}
#endif
	const float f1 = 1.0f / 256.0f;
	for (; i < count; i++) {
		uint32_t px = in[i];
		float r = f1 * (float)(px & 0xFFu);
		float g = f1 * (float)((px >> 8) & 0xFFu);
		float b = f1 * (float)((px >> 16) & 0xFFu);
		float dd = 0.0f;
		if (ss == 0) dd = dist_box(kr, kg, kb, ddx, ddy, ddz, r, g, b);
		else if (ss == 1) dd = dist_eli(kr, kg, kb, ddx, ddy, ddz, r, g, b);
		else if (ss == 2) dd = dist_oct(kr, kg, kb, ddx, ddy, ddz, r, g, b);
		switch (thr) {
		case 0: aout[i] = thres(dd); break;
		case 1: aout[i] = fat(dd); break;
		case 2: aout[i] = norm(dd); break;
		case 3: aout[i] = skiny(dd); break;
		case 4: aout[i] = slope(dd, islp); break;
		default: aout[i] = 0.0f; break;
		}
	}
}

static void sel_abi_tile(const uint32_t *in, float *aout, int count,
                         float ka, float kb, float ki, float k32,
                         float dda, float ddb, float ddi,
                         float islp, int ss, int thr)
{
	int i = 0;
	const float f1 = 1.0f / 256.0f;
#if defined(__SSE2__)
	const __m128 vka = _mm_set1_ps(ka), vkb = _mm_set1_ps(kb), vki = _mm_set1_ps(ki);
	const __m128 vda = _mm_set1_ps(dda), vdb = _mm_set1_ps(ddb), vdi = _mm_set1_ps(ddi);
	const __m128 vk32 = _mm_set1_ps(k32);
	const __m128 vhalf = _mm_set1_ps(0.5f);
	const __m128d vthird = _mm_set1_pd(0.3333);
	const __m128 vislp = _mm_set1_ps(islp);

	for (; i + 3 < count; i += 4) {
		__m128 vr, vg, vb;
		unpack_4px_rgb_ps(in + i, &vr, &vg, &vb);
		__m128 sa = _mm_sub_ps(_mm_sub_ps(vr, _mm_mul_ps(vhalf, vg)), _mm_mul_ps(vhalf, vb));
		__m128 sb = _mm_mul_ps(vk32, _mm_sub_ps(vg, vb));
		__m128 sum = _mm_add_ps(_mm_add_ps(vr, vg), vb);
		__m128 si_lo = _mm_cvtpd_ps(_mm_mul_pd(vthird, _mm_cvtps_pd(sum)));
		__m128 si_hi = _mm_cvtpd_ps(_mm_mul_pd(vthird, _mm_cvtps_pd(_mm_movehl_ps(sum, sum))));
		__m128 si = _mm_movelh_ps(si_lo, si_hi);
		__m128 dd;
		if (ss == 0) dd = sse_dist_box(vka, vkb, vki, vda, vdb, vdi, sa, sb, si);
		else if (ss == 1) dd = sse_dist_eli(vka, vkb, vki, vda, vdb, vdi, sa, sb, si);
		else dd = sse_dist_oct(vka, vkb, vki, vda, vdb, vdi, sa, sb, si);
		_mm_storeu_ps(aout + i, sse_edge(dd, vislp, thr));
	}
#endif
	for (; i < count; i++) {
		uint32_t px = in[i];
		float r = f1 * (float)(px & 0xFFu);
		float g = f1 * (float)((px >> 8) & 0xFFu);
		float b = f1 * (float)((px >> 16) & 0xFFu);
		float sa, sb, si, dd = 0.0f;
		rgb2abi(k32, r, g, b, &sa, &sb, &si);
		if (ss == 0) dd = dist_box(ka, kb, ki, dda, ddb, ddi, sa, sb, si);
		else if (ss == 1) dd = dist_eli(ka, kb, ki, dda, ddb, ddi, sa, sb, si);
		else if (ss == 2) dd = dist_oct(ka, kb, ki, dda, ddb, ddi, sa, sb, si);
		switch (thr) {
		case 0: aout[i] = thres(dd); break;
		case 1: aout[i] = fat(dd); break;
		case 2: aout[i] = norm(dd); break;
		case 3: aout[i] = skiny(dd); break;
		case 4: aout[i] = slope(dd, islp); break;
		default: aout[i] = 0.0f; break;
		}
	}
}

static void sel_hci_tile(const uint32_t *in, float *aout, int count,
                         float kh, float kc, float ki, float ipi2, float k32,
                         float ddh, float ddc, float ddi,
                         float islp, int ss, int thr)
{
	const float f1 = 1.0f / 256.0f;
	for (int i = 0; i < count; i++) {
		uint32_t px = in[i];
		float r = f1 * (float)(px & 0xFFu);
		float g = f1 * (float)((px >> 8) & 0xFFu);
		float b = f1 * (float)((px >> 16) & 0xFFu);
		float sh, sc, si, dd = 0.0f;
		rgb2hci(ipi2, k32, r, g, b, &sh, &sc, &si);
		if (ss == 0) dd = dist_box_c(kh, kc, ki, ddh, ddc, ddi, sh, sc, si);
		else if (ss == 1) dd = dist_eli_c(kh, kc, ki, ddh, ddc, ddi, sh, sc, si);
		else if (ss == 2) dd = dist_oct_c(kh, kc, ki, ddh, ddc, ddi, sh, sc, si);
		switch (thr) {
		case 0: aout[i] = thres(dd); break;
		case 1: aout[i] = fat(dd); break;
		case 2: aout[i] = norm(dd); break;
		case 3: aout[i] = skiny(dd); break;
		case 4: aout[i] = slope(dd, islp); break;
		default: aout[i] = 0.0f; break;
		}
	}
}

//----------------------------------------
typedef struct
{
	int h;
	int w;
	f0r_param_color_t col;
	int subsp;
	int sshape;
	float del1,del2,del3;
	float slp;
	float nud1,nud2,nud3;
	int soft;
	int inv;
	int op;
} inst;

static float map_value_forward(double v, float min, float max)
{
	return min+(max-min)*v;
}

static double map_value_backward(float v, float min, float max)
{
	return (v-min)/(max-min);
}

//***********************************************
// OBVEZNE FREI0R FUNKCIJE

int f0r_init()
{
	return 1;
}

void f0r_deinit()
{
}

void f0r_get_plugin_info(f0r_plugin_info_t* info)
{
	info->name="select0r";
	info->author="Marko Cebokli";
	info->plugin_type=F0R_PLUGIN_TYPE_FILTER;
	info->color_model=F0R_COLOR_MODEL_RGBA8888;
	info->frei0r_version=FREI0R_MAJOR_VERSION;
	info->major_version=0;
	info->minor_version=6;
	info->num_params=10;
	info->explanation="Color based alpha selection";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
	switch(param_index)
	{
	case 0:
		info->name = "Color to select";
		info->type = F0R_PARAM_COLOR;
		info->explanation = "";
		break;
	case 1:
		info->name = "Invert selection";
		info->type = F0R_PARAM_BOOL;
		info->explanation = "";
		break;
	case 2:
		info->name = "Delta R / A / Hue";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "";
		break;
	case 3:
		info->name = "Delta G / B / Chroma";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "";
		break;
	case 4:
		info->name = "Delta B / I / I";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "";
		break;
	case 5:
		info->name = "Slope";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "";
		break;
	case 6:
		info->name = "Selection subspace";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "";
		break;
	case 7:
		info->name = "Subspace shape";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "";
		break;
	case 8:
		info->name = "Edge mode";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "";
		break;
	case 9:
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
	
	in->col.r=0.0;
	in->col.g=0.8;
	in->col.b=0.0;
	in->subsp=0;
	in->sshape=0;
	in->del1=0.2; in->del2=0.2; in->del3=0.2;
	in->nud1=0.0; in->nud2=0.0; in->nud3=0.0;
	in->slp=0.0;
	in->soft=0;
	in->inv=0;
	in->op=0;
	
	return (f0r_instance_t)in;
}

void f0r_destruct(f0r_instance_t instance)
{
	free(instance);
}

void f0r_set_param_value(f0r_instance_t instance, f0r_param_t parm, int param_index)
{
	inst *p;
	double tmpf;
	int tmpi,chg;
	f0r_param_color_t tmpc;
	
	p=(inst*)instance;
	
	chg=0;
	switch(param_index)
	{
	case 0:		//color
		tmpc=*(f0r_param_color_t*)parm;
		if ((tmpc.r!=p->col.r) || (tmpc.g!=p->col.g) || (tmpc.b!=p->col.b))
			chg=1;
		p->col=tmpc;
		break;
	case 1:		//invert
		tmpi=map_value_forward(*((double*)parm), 0.0, 1.0); //BOOL!!
		if (p->inv != tmpi) chg=1;
		p->inv=tmpi;
		break;
	case 2:		//delta 1
		tmpf=*(double*)parm;
		if (tmpf!=p->del1) chg=1;
		p->del1=tmpf;
		break;
	case 3:		//delta 2
		tmpf=*(double*)parm;
		if (tmpf!=p->del2) chg=1;
		p->del2=tmpf;
		break;
	case 4:		//delta 3
		tmpf=*(double*)parm;
		if (tmpf!=p->del3) chg=1;
		p->del3=tmpf;
		break;
	case 5:		//slope 1
		tmpf=*(double*)parm;
		if (tmpf!=p->slp) chg=1;
		p->slp=tmpf;
		break;
	case 6:		//subspace
		tmpi = map_value_forward(*(double*)parm, 0.0, 2.9999);
		if (p->subsp != tmpi) chg=1;
		p->subsp = tmpi;
		break;
	case 7:		//shape
		tmpi = map_value_forward(*(double*)parm, 0.0, 2.9999);
		if (p->sshape != tmpi) chg=1;
		p->sshape = tmpi;
		break;
	case 8:		//edge mode
		tmpi = map_value_forward(*(double*)parm, 0.0, 4.9999);
		if (p->soft != tmpi) chg=1;
		p->soft = tmpi;
		break;
	case 9:		//operation
		tmpi = map_value_forward(*(double*)parm, 0.0, 4.9999);
		if (p->op != tmpi) chg=1;
		p->op = tmpi;
		break;
	}
	
	if (chg==0) return;
}

void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index)
{
	inst *p;
	
	p=(inst*)instance;
	
	switch(param_index)
	{
	case 0:
		*((f0r_param_color_t*)param)=p->col;
		break;
	case 1:
		*((double*)param)=map_value_backward(p->inv, 0.0, 1.0);//BOOL!!
		break;
	case 2:
		*((double*)param)=p->del1;
		break;
	case 3:
		*((double*)param)=p->del2;
		break;
	case 4:
		*((double*)param)=p->del3;
		break;
	case 5:
		*((double*)param)=p->slp;
		break;
	case 6:
		*((double*)param)=map_value_backward(p->subsp, 0.0, 2.9999);
		break;
	case 7:
		*((double*)param)=map_value_backward(p->sshape, 0.0, 2.9999);
		break;
	case 8:
		*((double*)param)=map_value_backward(p->soft, 0.0, 4.9999);
		break;
	case 9:
		*((double*)param)=map_value_backward(p->op, 0.0, 4.9999);
		break;
	}
}

//-------------------------------------------------
//RGBA8888 little endian
void f0r_update(f0r_instance_t instance, double time, const uint32_t* inframe, uint32_t* outframe)
{
	inst *in;
	assert(instance);
	in=(inst*)instance;

	const int total = in->w * in->h;
	const float kr = in->col.r, kg = in->col.g, kb = in->col.b;
	const float d1 = (in->del1 != 0.0f) ? (float)(1.0 / in->del1) : 1.0E6f;
	const float d2 = (in->del2 != 0.0f) ? (float)(1.0 / in->del2) : 1.0E6f;
	const float d3 = (in->del3 != 0.0f) ? (float)(1.0 / in->del3) : 1.0E6f;
	const float islp = (in->slp > 0.000001f) ? (float)(0.2 / in->slp) : 200000.0f;

	const float k32 = (float)(sqrtf(3.0f) / 2.0);
	const float ipi2 = (float)(0.5 / PI);

	float c1 = 0.0f, c2 = 0.0f, c3 = 0.0f;
	if (in->subsp == 0) {
		c1 = kr + in->nud1;
		c2 = kg + in->nud2;
		c3 = kb + in->nud3;
	} else if (in->subsp == 1) {
		c1 = (float)(kr - 0.5 * kg - 0.5 * kb + in->nud1);
		c2 = k32 * (kg - kb) + in->nud2;
		c3 = (float)(0.3333 * (kr + kg + kb) + in->nud3);
	} else if (in->subsp == 2) {
		float ka = (float)(kr - 0.5 * kg - 0.5 * kb);
		float kbb = k32 * (kg - kb);
		c1 = atan2f(kbb, ka) * ipi2 + in->nud1;
		c2 = hypotf(ka, kbb) + in->nud2;
		c3 = (float)(0.3333 * (kr + kg + kb) + in->nud3);
	}

	float a_tile[SELECT0R_TILE];

	for (int base = 0; base < total; base += SELECT0R_TILE) {
		int cnt = total - base;
		if (cnt > SELECT0R_TILE) cnt = SELECT0R_TILE;
		const uint32_t *src = inframe + base;
		uint32_t *dst = outframe + base;

		switch (in->subsp) {
		case 0:
			sel_rgb_tile(src, a_tile, cnt, c1, c2, c3, d1, d2, d3, islp, in->sshape, in->soft);
			break;
		case 1:
			sel_abi_tile(src, a_tile, cnt, c1, c2, c3, k32, d1, d2, d3, islp, in->sshape, in->soft);
			break;
		case 2:
			sel_hci_tile(src, a_tile, cnt, c1, c2, c3, ipi2, k32, d1, d2, d3, islp, in->sshape, in->soft);
			break;
		default:
			for (int k = 0; k < cnt; k++) a_tile[k] = 0.0f;
			break;
		}

		if (in->inv == 1) {
			int k = 0;
#if defined(__SSE2__)
			const __m128 vone = _mm_set1_ps(1.0f);
			for (; k + 3 < cnt; k += 4) {
				__m128 va = _mm_loadu_ps(a_tile + k);
				_mm_storeu_ps(a_tile + k, _mm_sub_ps(vone, va));
			}
#endif
			for (; k < cnt; k++) {
				a_tile[k] = (float)(1.0 - a_tile[k]);
			}
		}

		switch (in->op) {
		case 0: { /* write on clear */
			int k = 0;
#if defined(__SSE2__)
			const __m128 v255 = _mm_set1_ps(255.0f);
			const __m128i rgb_mask = _mm_set1_epi32(0x00FFFFFF);
			for (; k + 3 < cnt; k += 4) {
				__m128i px = _mm_loadu_si128((const __m128i *)(src + k));
				__m128i a2 = _mm_cvttps_epi32(_mm_mul_ps(_mm_loadu_ps(a_tile + k), v255));
				__m128i res = _mm_or_si128(_mm_and_si128(px, rgb_mask), _mm_slli_epi32(a2, 24));
				_mm_storeu_si128((__m128i *)(dst + k), res);
			}
#endif
			for (; k < cnt; k++) {
				uint32_t a2 = (uint32_t)(uint8_t)(a_tile[k] * 255.0);
				dst[k] = (src[k] & 0x00FFFFFFu) | (a2 << 24);
			}
			break;
		}
		case 1: { /* max */
			int k = 0;
#if defined(__SSE2__)
			const __m128 v255 = _mm_set1_ps(255.0f);
			const __m128i rgb_mask = _mm_set1_epi32(0x00FFFFFF);
			for (; k + 3 < cnt; k += 4) {
				__m128i px = _mm_loadu_si128((const __m128i *)(src + k));
				__m128i a2 = _mm_slli_epi32(_mm_cvttps_epi32(_mm_mul_ps(_mm_loadu_ps(a_tile + k), v255)), 24);
				__m128i mx = _mm_max_epu8(px, a2);
				__m128i res = _mm_or_si128(_mm_and_si128(px, rgb_mask), _mm_andnot_si128(rgb_mask, mx));
				_mm_storeu_si128((__m128i *)(dst + k), res);
			}
#endif
			for (; k < cnt; k++) {
				uint32_t a1 = src[k] >> 24;
				uint32_t a2 = (uint8_t)(a_tile[k] * 255.0);
				dst[k] = (src[k] & 0x00FFFFFFu) | (((a1 > a2) ? a1 : a2) << 24);
			}
			break;
		}
		case 2: { /* min */
			int k = 0;
#if defined(__SSE2__)
			const __m128 v255 = _mm_set1_ps(255.0f);
			const __m128i rgb_mask = _mm_set1_epi32(0x00FFFFFF);
			for (; k + 3 < cnt; k += 4) {
				__m128i px = _mm_loadu_si128((const __m128i *)(src + k));
				__m128i a2 = _mm_slli_epi32(_mm_cvttps_epi32(_mm_mul_ps(_mm_loadu_ps(a_tile + k), v255)), 24);
				__m128i mn = _mm_min_epu8(px, a2);
				__m128i res = _mm_or_si128(_mm_and_si128(px, rgb_mask), _mm_andnot_si128(rgb_mask, mn));
				_mm_storeu_si128((__m128i *)(dst + k), res);
			}
#endif
			for (; k < cnt; k++) {
				uint32_t a1 = src[k] >> 24;
				uint32_t a2 = (uint8_t)(a_tile[k] * 255.0);
				dst[k] = (src[k] & 0x00FFFFFFu) | (((a1 < a2) ? a1 : a2) << 24);
			}
			break;
		}
		case 3: { /* add */
			int k = 0;
#if defined(__SSE2__)
			const __m128 v255 = _mm_set1_ps(255.0f);
			const __m128i rgb_mask = _mm_set1_epi32(0x00FFFFFF);
			for (; k + 3 < cnt; k += 4) {
				__m128i px = _mm_loadu_si128((const __m128i *)(src + k));
				__m128i a2 = _mm_slli_epi32(_mm_cvttps_epi32(_mm_mul_ps(_mm_loadu_ps(a_tile + k), v255)), 24);
				__m128i ad = _mm_adds_epu8(px, a2);
				__m128i res = _mm_or_si128(_mm_and_si128(px, rgb_mask), _mm_andnot_si128(rgb_mask, ad));
				_mm_storeu_si128((__m128i *)(dst + k), res);
			}
#endif
			for (; k < cnt; k++) {
				uint32_t a1 = src[k] >> 24;
				uint32_t a2 = (uint8_t)(a_tile[k] * 255.0);
				uint32_t t = a1 + a2;
				dst[k] = (src[k] & 0x00FFFFFFu) | (((t <= 255u) ? t : 255u) << 24);
			}
			break;
		}
		case 4: { /* subtract */
			int k = 0;
#if defined(__SSE2__)
			const __m128 v255 = _mm_set1_ps(255.0f);
			const __m128i rgb_mask = _mm_set1_epi32(0x00FFFFFF);
			for (; k + 3 < cnt; k += 4) {
				__m128i px = _mm_loadu_si128((const __m128i *)(src + k));
				__m128i a2 = _mm_slli_epi32(_mm_cvttps_epi32(_mm_mul_ps(_mm_loadu_ps(a_tile + k), v255)), 24);
				__m128i sb = _mm_subs_epu8(px, a2);
				__m128i res = _mm_or_si128(_mm_and_si128(px, rgb_mask), _mm_andnot_si128(rgb_mask, sb));
				_mm_storeu_si128((__m128i *)(dst + k), res);
			}
#endif
			for (; k < cnt; k++) {
				uint32_t a1 = src[k] >> 24;
				uint32_t a2 = (uint8_t)(a_tile[k] * 255.0);
				dst[k] = (src[k] & 0x00FFFFFFu) | (((a1 > a2) ? (a1 - a2) : 0u) << 24);
			}
			break;
		}
		default:
			break;
		}
	}
}
