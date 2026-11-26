/*
keyspillm0pup.c

This Frei0r plugin cleans key color residue from composited video

Version 0.1	mar 2012

Copyright (C) 2012  Marko Cebokli    http://lea.hamradio.si/~s57uuu


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
#include <math.h>
#include <frei0r.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#if defined(__SSE2__)
#include <emmintrin.h>
#endif

static const double PI=3.14159265358979;

typedef struct
{
	float r;
	float g;
	float b;
	float a;
} float_rgba;

static void cocos(int cm, float *kr, float *kg, float *kb)
{
	*kr=0.30f; *kg=0.59f; *kb=0.11f;
	switch (cm)
	{
	case 0:
		*kr=0.30f; *kg=0.59f; *kb=0.11f;	//rec 601
		break;
	case 1:
		*kr=0.2126f; *kg=0.7152f; *kb=0.0722f; //rec 709
		break;
	default:
		fprintf(stderr,"Unknown color model %d\n",cm);
		break;
	}
}

static inline void apply_op_pixel(int op, float am, float m,
                                  float *r, float *g, float *b,
                                  float kr, float kg, float kb,
                                  float tr, float tg, float tb,
                                  float cr_k, float cg_k, float cb_k, float ikg)
{
	if (op == 0) return;
	if (op == 1) { /* De-Key */
		float aa = (float)(1.0 - am * 0.5 * m);
		float nr = (float)((*r - (1.0 - aa) * kr) / aa);
		float ng = (float)((*g - (1.0 - aa) * kg) / aa);
		float nb = (float)((*b - (1.0 - aa) * kb) / aa);
		if (nr < 0.0f) nr = 0.0f; else if (nr > 1.0f) nr = 1.0f;
		if (ng < 0.0f) ng = 0.0f; else if (ng > 1.0f) ng = 1.0f;
		if (nb < 0.0f) nb = 0.0f; else if (nb > 1.0f) nb = 1.0f;
		*r = nr; *g = ng; *b = nb;
	} else if (op == 2) { /* Target */
		float aa = m * am;
		float nr = *r + (tr - *r) * aa;
		float ng = *g + (tg - *g) * aa;
		float nb = *b + (tb - *b) * aa;
		if (nr < 0.0f) nr = 0.0f; else if (nr > 1.0f) nr = 1.0f;
		if (ng < 0.0f) ng = 0.0f; else if (ng > 1.0f) ng = 1.0f;
		if (nb < 0.0f) nb = 0.0f; else if (nb > 1.0f) nb = 1.0f;
		*r = nr; *g = ng; *b = nb;
	} else if (op == 3) { /* Desaturate */
		float y = cr_k * (*r) + cg_k * (*g) + cb_k * (*b);
		float cr = *r - y;
		float cb = *b - y;
		float ds = (float)(1.0 - am * m);
		ds = ds * ds;
		float nr = cr * ds + y;
		float nb = cb * ds + y;
		float ng = (y - cr_k * nr - cb_k * nb) * ikg;
		if (nr < 0.0f) nr = 0.0f; else if (nr > 1.0f) nr = 1.0f;
		if (ng < 0.0f) ng = 0.0f; else if (ng > 1.0f) ng = 1.0f;
		if (nb < 0.0f) nb = 0.0f; else if (nb > 1.0f) nb = 1.0f;
		*r = nr; *g = ng; *b = nb;
	} else if (op == 4) { /* Luma adjust */
		float mv = (float)(2.0 * am);
		float y = cr_k * (*r) + cg_k * (*g) + cb_k * (*b);
		float cr = *r - y;
		float cb = *b - y;
		float mm = (float)((mv - 1.0) * m + 1.0);
		if (mv >= 1.0f) y = (float)(mm - 1.0 + y * (2.0 - mm));
		else y = mm * y;
		float nr = cr + y;
		float nb = cb + y;
		float ng = (y - cr_k * nr - cb_k * nb) * ikg;
		if (nr < 0.0f) nr = 0.0f; else if (nr > 1.0f) nr = 1.0f;
		if (ng < 0.0f) ng = 0.0f; else if (ng > 1.0f) ng = 1.0f;
		if (nb < 0.0f) nb = 0.0f; else if (nb > 1.0f) nb = 1.0f;
		*r = nr; *g = ng; *b = nb;
	}
}

//---------------------------------------------------------
//do the blur for edge mask
static void fibe1o_f(float *s, int w, int h, float a, int ec)
{
	int i,j;
	float b,g,g4,avg,avg1,cr,g4a,g4b;
	int p,pw,pj,pwj,pww,pmw;
	
	avg=8;
	avg1=1.0f/avg;
	
	g=1.0f/(1.0f-a);
	g4=1.0f/g/g/g/g;
	
	b=1.0f/(1.0f-a)/(1.0f+a);
	
	for (i=0;i<avg;i++)
	{
		p=i*w;pw=p+w;
		if (ec!=0)
		{
			cr=0.0f;
			for (j=0;j<avg;j++)
				cr=cr+s[p+j];
			cr=cr*avg1;
			s[p]=cr*g+b*(s[p]-cr);
		}
		
		for (j=1;j<w;j++)
			s[p+j]=s[p+j]+a*s[p+j-1];
		
		if (ec!=0)
		{
			cr=0.0f;
			for (j=w-avg;j<w;j++)
				cr=cr+s[p+j];
			cr=cr*avg1;
			s[pw-1]=cr*g+b*(s[pw-1]-cr);
		}
		else
			s[pw-1]=b*s[pw-1];
		
		for (j=w-2;j>=0;j--)
			s[p+j]=a*s[p+j+1]+s[p+j];
	}
	
	for (i=0;i<w;i++)
	{
		if (ec!=0)
		{
			cr=0.0f;
			for (j=0;j<avg;j++)
				cr=cr+s[i+w*j];
			cr=cr*avg1;
			s[i]=cr*g+b*(s[i]-cr);
		}
		for (j=1;j<avg;j++)
			s[i+j*w]=s[i+j*w]+a*s[i+w*(j-1)];
	}
	
	for (i=avg;i<h-1;i=i+2)
	{
		p=i*w; pw=p+w; pww=pw+w; pmw=p-w;
		if (ec!=0)
		{
			cr=0.0f;
			for (j=0;j<avg;j++)
				cr=cr+s[p+j];
			cr=cr*avg1;
			s[p]=cr*g+b*(s[p]-cr);
			cr=0.0f;
			for (j=0;j<avg;j++)
				cr=cr+s[pw+j];
			cr=cr*avg1;
			s[pw]=cr*g+b*(s[pw]-cr);
		}
		for (j=1;j<w;j++)
		{
			pj=p+j;pwj=pw+j;
			s[pj]=s[pj]+a*s[pj-1];
			s[pwj]=s[pwj]+a*s[pwj-1];
		}
		
		if (ec!=0)
		{
			cr=0.0f;
			for (j=w-avg;j<w;j++)
				cr=cr+s[p+j];
			cr=cr*avg1;
			s[pw-1]=cr*g+b*(s[pw-1]-cr);
			cr=0.0f;
			for (j=w-avg;j<w;j++)
				cr=cr+s[pw+j];
			cr=cr*avg1;
			s[pww-1]=cr*g+b*(s[pww-1]-cr);
		}
		else
		{
			s[pw-1]=b*s[pw-1];
			s[pww-1]=b*s[pww-1];
		}
		
		s[pw-2]=s[pw-2]+a*s[pw-1];
		s[pw-1]=s[pw-1]+a*s[p-1];
		
		for (j=w-2;j>=1;j--)
		{
			pj=p+j;pwj=pw+j;
			s[pj-1]=a*s[pj]+s[pj-1];
			s[pwj]=a*s[pwj+1]+s[pwj];
			s[pj]=s[pj]+a*s[pmw+j];
			s[pwj+1]=s[pwj+1]+a*s[pj+1];
		}
		s[pw]=s[pw]+a*s[pw+1];
		s[p]=s[p]+a*s[pmw];
		s[pw+1]=s[pw+1]+a*s[p+1];
		s[pw]=s[pw]+a*s[p];
	}
	
	if (i!=h)
	{
		p=i*w; pw=p+w;
		for (j=1;j<w;j++)
			s[p+j]=s[p+j]+a*s[p+j-1];
		
		s[pw-1]=b*s[pw-1];
		
		for (j=w-2;j>=0;j--)
		{
			s[p+j]=a*s[p+j+1]+s[p+j];
			s[p+j+1]=s[p+j+1]+a*s[p-w+j+1];
		}
		s[p]=s[p]+a*s[p-w];
	}
	
	g4b=g4*b;
	g4a=g4/(1.0f-a);
	p=(h-1)*w;
	if (ec!=0)
	{
		for (i=0;i<w;i++)
		{
			cr=0.0f;
			for (j=h-avg;j<h;j++)
				cr=cr+s[i+w*j];
			cr=cr*avg1;
			s[i+p]=g4a*cr+g4b*(s[i+p]-cr);
		}
	}
	else
	{
		for (j=0;j<w;j++)
			s[j+p]=g4b*s[j+p];
	}
	
	for (i=h-2;i>=0;i--)
	{
		p=i*w; pw=p+w;
		for (j=0;j<w;j++)
			s[p+j]=a*s[pw+j]+g4*s[p+j];
	}
}

static void edge_mask_u8(const uint32_t *inframe, int w, int h, float *mask, float wd, int io)
{
	int i;
	int n = w * h;
	float a;
	float lim = 0.05f;
	const float f1 = (float)(1.0 / 255.0);

	for (i = 0; i < n; i++) {
		float al = f1 * (float)(inframe[i] >> 24);
		mask[i] = (al > 0.996f) ? 1.0f : 0.0f;
	}

	a = expf(logf(lim) / wd);
	fibe1o_f(mask, w, h, a, 1);

	if (io == -1) {
		for (i = 0; i < n; i++) {
			float mv = (mask[i] > 0.5f) ? (float)(2.0 * (1.0 - mask[i])) : 0.0f;
			mask[i] = (mv < lim) ? 0.0f : mv;
		}
	} else if (io == 1) {
		for (i = 0; i < n; i++) {
			float mv = (mask[i] < 0.5f) ? (float)(2.0 * mask[i]) : 0.0f;
			mask[i] = (mv < lim) ? 0.0f : mv;
		}
	}
}

//***********************************************************
//		FREI0R FUNKCIJE

typedef struct
{
	int w,h;
	
	f0r_param_color_t key;
	f0r_param_color_t tgt;
	int maskType;
	float tol;
	float slope;
	float Hgate;
	float Sthresh;
	int op1;
	float am1;
	int op2;
	float am2;
	int showmask;
	int m2a;
	int fo;
	int cm;
	
	float_rgba krgb;
	float_rgba trgb;
	char *liststr;
	float *edge_buf;
	float u8_to_f[256];
} inst;

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
	info->name="keyspillm0pup";
	info->author="Marko Cebokli";
	info->plugin_type=F0R_PLUGIN_TYPE_FILTER;
	info->color_model=F0R_COLOR_MODEL_RGBA8888;
	info->frei0r_version=FREI0R_MAJOR_VERSION;
	info->major_version=0;
	info->minor_version=3;
	info->num_params=13;
	info->explanation="Reduces the visibility of key color spill in chroma keying";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
	switch(param_index)
	{
	case 0:
		info->name = "Key color";
		info->type = F0R_PARAM_COLOR;
		info->explanation = "Key color that was used for chroma keying";
		break;
	case 1:
		info->name = "Target color";
		info->type = F0R_PARAM_COLOR;
		info->explanation = "Desired color to replace key residue with";
		break;
	case 2:
		info->name = "Mask type";
		info->type = F0R_PARAM_STRING;
		info->explanation = "Which mask to apply [0,1,2,3]";
		break;
	case 3:
		info->name = "Tolerance";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "Range of colors around the key, where effect is full strength";
		break;
	case 4:
		info->name = "Slope";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "Range of colors around the key where effect gradually decreases";
		break;
	case 5:
		info->name = "Hue gate";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "Restrict mask to hues close to key";
		break;
	case 6:
		info->name = "Saturation threshold";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "Restrict mask to saturated colors";
		break;
	case 7:
		info->name = "Operation 1";
		info->type = F0R_PARAM_STRING;
		info->explanation = "First operation 1 [0,1,2]";
		break;
	case 8:
		info->name = "Amount 1";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "";
		break;
	case 9:
		info->name = "Operation 2";
		info->type = F0R_PARAM_STRING;
		info->explanation = "Second operation 2 [0,1,2]";
		break;
	case 10:
		info->name = "Amount 2";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "";
		break;
	case 11:
		info->name = "Show mask";
		info->type = F0R_PARAM_BOOL;
		info->explanation = "Replace image with the mask";
		break;
	case 12:
		info->name = "Mask to Alpha";
		info->type = F0R_PARAM_BOOL;
		info->explanation = "Replace alpha channel with the mask";
		break;
	}
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
	inst *in;
	
	in=calloc(1,sizeof(inst));
	in->w=width;
	in->h=height;
	
	in->key.r = 0.1f; 
	in->key.g = 0.8f; 
	in->key.b = 0.1f; 
	in->tgt.r = 0.78f; 
	in->tgt.g = 0.5f; 
	in->tgt.b = 0.4f; 
	in->maskType=0;
	in->tol=0.12f;
	in->slope=0.2f;
	in->Hgate=0.25f;
	in->Sthresh=0.15f;
	in->op1=1;
	in->am1=0.55f;
	in->op2=0;
	in->am2=0.0f;
	in->showmask=0;
	in->m2a=0;
	in->fo=1;
	in->cm=1;
	
	const char* sval = "0";
	in->liststr = (char*)malloc( strlen(sval) + 1 );
	strcpy( in->liststr, sval );

	const float f1 = (float)(1.0 / 255.0);
	for (int i = 0; i < 256; i++) {
		in->u8_to_f[i] = f1 * (float)i;
	}
	in->edge_buf = NULL;
	
	return (f0r_instance_t)in;
}

void f0r_destruct(f0r_instance_t instance)
{
	inst* in = (inst*)instance;
	free(in->edge_buf);
	free(in->liststr);
	free(instance);
}

void f0r_set_param_value(f0r_instance_t instance, f0r_param_t parm, int param_index)
{
	inst *p;
	double tmpf;
	int chg,tmpi,nc;
	f0r_param_color_t tmpc;
	char *tmpch;
	
	p=(inst*)instance;
	
	chg=0;
	switch(param_index)
	{
	case 0:
		tmpc=*(f0r_param_color_t*)parm;
		if ((tmpc.r!=p->key.r) || (tmpc.g!=p->key.g) || (tmpc.b!=p->key.b))
			chg=1;
		p->key=tmpc;
		p->krgb.r=p->key.r;
		p->krgb.g=p->key.g;
		p->krgb.b=p->key.b;
		break;
	case 1:
		tmpc=*(f0r_param_color_t*)parm;
		if ((tmpc.r!=p->tgt.r) || (tmpc.g!=p->tgt.g) || (tmpc.b!=p->tgt.b))
			chg=1;
		p->tgt=tmpc;
		p->trgb.r=p->tgt.r;
		p->trgb.g=p->tgt.g;
		p->trgb.b=p->tgt.b;
		break;
	case 2:
		tmpch = (*(f0r_param_string*)parm);
		if (tmpch && strcmp(p->liststr, tmpch)) {
			p->liststr = realloc( p->liststr, strlen(tmpch) + 1 );
			strcpy( p->liststr, tmpch );
		}
		nc=sscanf(p->liststr,"%d",&tmpi);
		if ((nc<=0)||(tmpi<0)||(tmpi>3)) break;
		if (p->maskType != tmpi) chg=1;
		p->maskType = tmpi;
		break;
	case 3:
		tmpf=map_value_forward(*((double*)parm), 0.0, 0.5);
		if (p->tol != tmpf) chg=1;
		p->tol = tmpf;
		break;
	case 4:
		tmpf=map_value_forward(*((double*)parm), 0.0, 0.5);
		if (p->slope != tmpf) chg=1;
		p->slope = tmpf;
		break;
	case 5:
		tmpf=*(double*)parm;
		if (tmpf!=p->Hgate) chg=1;
		p->Hgate=tmpf;
		break;
	case 6:
		tmpf=*(double*)parm;
		if (tmpf!=p->Sthresh) chg=1;
		p->Sthresh=tmpf;
		break;
	case 7:
		tmpch = (*(f0r_param_string*)parm);
		if (tmpch && strcmp(p->liststr, tmpch)) {
			p->liststr = realloc( p->liststr, strlen(tmpch) + 1 );
			strcpy( p->liststr, tmpch );
		}
		nc=sscanf(p->liststr,"%d",&tmpi);
		if ((nc<=0)||(tmpi<0)||(tmpi>4)) break;
		if (p->op1 != tmpi) chg=1;
		p->op1 = tmpi;
		break;
	case 8:
		tmpf=*(double*)parm;
		if (tmpf!=p->am1) chg=1;
		p->am1=tmpf;
		break;
	case 9:
		tmpch = (*(f0r_param_string*)parm);
		if (tmpch && strcmp(p->liststr, tmpch)) {
			p->liststr = realloc( p->liststr, strlen(tmpch) + 1 );
			strcpy( p->liststr, tmpch );
		}
		nc=sscanf(p->liststr,"%d",&tmpi);
		if ((nc<=0)||(tmpi<0)||(tmpi>4)) break;
		if (p->op2 != tmpi) chg=1;
		p->op2 = tmpi;
		break;
	case 10:
		tmpf=*(double*)parm;
		if (p->am2 != tmpf) chg=1;
		p->am2 = tmpf;
		break;
	case 11:
		tmpf=*(double*)parm;
		tmpi=roundf(tmpf);
		if (tmpi!=p->showmask) chg=1;
		p->showmask=tmpi;
		break;
	case 12:
		tmpf=*(double*)parm;
		tmpi=roundf(tmpf);
		if (tmpi!=p->m2a) chg=1;
		p->m2a=tmpi;
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
		*((f0r_param_color_t*)param)=p->key;
		break;
	case 1:
		*((f0r_param_color_t*)param)=p->tgt;
		break;
	case 2:
		p->liststr=realloc(p->liststr,16);
		sprintf(p->liststr,"%d",p->maskType);
		*((char**)param) = p->liststr;
		break;
	case 3:
		*((double*)param)=map_value_backward(p->tol, 0.0, 0.5);
		break;
	case 4:
		*((double*)param)=map_value_backward(p->slope, 0.0, 0.5);
		break;
	case 5:
		*((double*)param)=p->Hgate;
		break;
	case 6:
		*((double*)param)=p->Sthresh;
		break;
	case 7:
		p->liststr=realloc(p->liststr,16);
		sprintf(p->liststr,"%d",p->op1);
		*((char**)param) = p->liststr;
		break;
	case 8:
		*((double*)param)=p->am1;
		break;
	case 9:
		p->liststr=realloc(p->liststr,16);
		sprintf(p->liststr,"%d",p->op2);
		*((char**)param) = p->liststr;
		break;
	case 10:
		*((double*)param)=p->am2;
		break;
	case 11:
		*((double*)param)=(double)p->showmask;
		break;
	case 12:
		*((double*)param)=(double)p->m2a;
		break;
	}
}

static inline uint32_t process_active_pixel(uint32_t px, float r, float g, float b, float m,
                                            float k32, float ipi2, float kh,
                                            double u_a, double u_b,
                                            double cos_pass, double cos_stop,
                                            int use_pass, int use_stop,
                                            float hg_t, float hg_tp, float hg_ip,
                                            float st_t1, float st_t2, float st_ip,
                                            int op1, float am1, int op2, float am2,
                                            float kr, float kg, float kb,
                                            float tr, float tg, float tb,
                                            float cr_k, float cg_k, float cb_k, float ikg,
                                            int showmask, int m2a)
{
	float a_c = (float)(r - 0.5 * g - 0.5 * b);
	float b_c = k32 * (g - b);

	/* Fast opposite-quadrant reject before sqrt/atan2f */
	if (a_c < 0.0f) {
		if ((kh >= 0.0f && b_c < 0.0f && (kh + 0.25f) > hg_tp + 1e-4f) ||
		    (kh < 0.0f && b_c >= 0.0f && (-kh + 0.25f) > hg_tp + 1e-4f)) {
			if (showmask) return m2a ? 0x00000000u : 0xFF000000u;
			if (m2a) return px & 0x00FFFFFFu;
			return px;
		}
	}

	double r_len = sqrt((double)a_c * (double)a_c + (double)b_c * (double)b_c);

	/* Check sat_thres rejection before atan2f (independent of hue_gate) */
	float sa = (float)((float)r_len / (r + g + b + 1.0E-6));
	if (sa < st_t1) {
		if (showmask) return m2a ? 0x00000000u : 0xFF000000u;
		if (m2a) return px & 0x00FFFFFFu;
		return px;
	}

	/* hue_gate with exact dot-product fast-accept / fast-reject before atan2f */
	if (r_len > 0.0) {
		double dot = (double)a_c * u_a + (double)b_c * u_b;
		if (use_stop && dot < r_len * cos_stop) {
			if (showmask) return m2a ? 0x00000000u : 0xFF000000u;
			if (m2a) return px & 0x00FFFFFFu;
			return px;
		}
		int no_wrap = (use_pass == 2) || (kh >= 0.0f ? (b_c >= 0.0f) : (b_c < 0.0f));
		if (!(use_pass && no_wrap && dot > r_len * cos_pass)) {
			float hh = atan2f(b_c, a_c) * ipi2;
			float d = (hh > kh) ? (hh - kh) : (kh - hh);
			d = (d > 1.0f) ? (float)(2.0 - d) : d;
			if (d > hg_tp) {
				if (showmask) return m2a ? 0x00000000u : 0xFF000000u;
				if (m2a) return px & 0x00FFFFFFu;
				return px;
			} else if (d >= hg_t) {
				float aa = (float)(1.0 - (d - hg_t) * hg_ip);
				m = m * aa;
			}
		}
	} else {
		float hh = atan2f(b_c, a_c) * ipi2;
		float d = (hh > kh) ? (hh - kh) : (kh - hh);
		d = (d > 1.0f) ? (float)(2.0 - d) : d;
		if (d > hg_tp) {
			if (showmask) return m2a ? 0x00000000u : 0xFF000000u;
			if (m2a) return px & 0x00FFFFFFu;
			return px;
		} else if (d >= hg_t) {
			float aa = (float)(1.0 - (d - hg_t) * hg_ip);
			m = m * aa;
		}
	}

	if (m != 0.0f && sa <= st_t2) {
		m = m * (sa - st_t1) * st_ip;
	}

	if (m == 0.0f) {
		if (showmask) return m2a ? 0x00000000u : 0xFF000000u;
		if (m2a) return px & 0x00FFFFFFu;
		return px;
	}

	if (!showmask) {
		apply_op_pixel(op1, am1, m, &r, &g, &b, kr, kg, kb, tr, tg, tb, cr_k, cg_k, cb_k, ikg);
		apply_op_pixel(op2, am2, m, &r, &g, &b, kr, kg, kb, tr, tg, tb, cr_k, cg_k, cb_k, ikg);
		uint32_t ro = (uint32_t)(uint8_t)(r * 255.0);
		uint32_t go = (uint32_t)(uint8_t)(g * 255.0);
		uint32_t bo = (uint32_t)(uint8_t)(b * 255.0);
		uint32_t ao = m2a ? (uint32_t)(uint8_t)(m * 255.0) : (px >> 24);
		return ro | (go << 8) | (bo << 16) | (ao << 24);
	} else {
		uint32_t mv = (uint32_t)(uint8_t)(m * 255.0);
		uint32_t ao = m2a ? mv : 255u;
		return mv | (mv << 8) | (mv << 16) | (ao << 24);
	}
}

//==============================================================
void f0r_update(f0r_instance_t instance, double time, const uint32_t* inframe, uint32_t* outframe)
{
	inst *in;
	assert(instance);
	in=(inst*)instance;

	const int total = in->w * in->h;
	const float *u8_to_f = in->u8_to_f;
	const float kr = in->krgb.r, kg = in->krgb.g, kb = in->krgb.b;
	const float tr = in->trgb.r, tg = in->trgb.g, tb = in->trgb.b;
	const int op1 = in->op1, op2 = in->op2;
	const float am1 = in->am1, am2 = in->am2;
	const int showmask = in->showmask, m2a = in->m2a;

	float cr_k, cg_k, cb_k;
	cocos(in->cm, &cr_k, &cg_k, &cb_k);
	const float ikg = (float)(1.0 / cg_k);

	const float k32 = (float)(sqrtf(3.0f) / 2.0);
	const float ipi2 = (float)(0.5 / PI);
	const float hg_t = in->Hgate;
	const float hg_p = (float)(0.5 * in->Hgate);
	const float hg_tp = hg_t + hg_p;
	const float hg_ip = (hg_p > 0.000001f) ? (float)(1.0 / hg_p) : 1000000.0f;
	const float ka = (float)(kr - 0.5 * kg - 0.5 * kb);
	const float kbb = k32 * (kg - kb);
	const float kh = atan2f(kbb, ka) * ipi2;
	const double theta_k = (double)kh * (2.0 * PI);
	const double u_a = cos(theta_k);
	const double u_b = sin(theta_k);
	const int use_pass = (hg_t > 0.001f && hg_t < 0.499f)
	                   ? (((kh + hg_t) < 0.498f && (kh - hg_t) > -0.498f) ? 2 : 1) : 0;
	const int use_stop = (hg_tp > 0.001f && hg_tp < 0.499f);
	const double cos_pass = use_pass ? (cos((double)hg_t * (2.0 * PI)) + 1e-4) : 2.0;
	const double cos_stop = use_stop ? (cos((double)hg_tp * (2.0 * PI)) - 1e-4) : -2.0;

	const float st_p = 0.1f;
	const float st_t2 = (float)((1.0 + st_p) * in->Sthresh);
	const float st_t1 = st_t2 - st_p;
	const float st_ip = (float)(1.0 / st_p);

	const uint32_t zero_mask_out = showmask ? (m2a ? 0x00000000u : 0xFF000000u) : 0u;

	if (in->maskType == 0) {
		/* RGB distance mask - fused single pass with SSE2 4-pixel fast-reject */
		const float tol = in->tol;
		const float slp = in->slope;
		const float tol_slp = tol + slp;
		const float ip = (slp > 0.000001f) ? (float)(1.0 / slp) : 1000000.0f;
		const float third = (float)(1.0 / 3.0);
		const int fo = in->fo;
		int i = 0;

#if defined(__SSE2__)
		const __m128i maskff = _mm_set1_epi32(0xFF);
		const __m128 vf1 = _mm_set1_ps((float)(1.0 / 255.0));
		const __m128 vkr = _mm_set1_ps(kr), vkg = _mm_set1_ps(kg), vkb = _mm_set1_ps(kb);
		const __m128 vthird = _mm_set1_ps(third);
		const __m128 vtol_slp = _mm_set1_ps(tol_slp);
		const __m128 vfo_min = _mm_set1_ps(0.005f);
		const __m128i vzero_out = _mm_set1_epi32((int)zero_mask_out);
		const __m128i vrgb_mask = _mm_set1_epi32(0x00FFFFFF);

		for (; i + 3 < total; i += 4) {
			__m128i px = _mm_loadu_si128((const __m128i *)(inframe + i));
			__m128 vr = _mm_mul_ps(_mm_cvtepi32_ps(_mm_and_si128(px, maskff)), vf1);
			__m128 vg = _mm_mul_ps(_mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(px, 8), maskff)), vf1);
			__m128 vb = _mm_mul_ps(_mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(px, 16), maskff)), vf1);
			__m128 dr = _mm_sub_ps(vr, vkr);
			__m128 dg = _mm_sub_ps(vg, vkg);
			__m128 db = _mm_sub_ps(vb, vkb);
			__m128 de = _mm_mul_ps(_mm_add_ps(_mm_add_ps(_mm_mul_ps(dr, dr), _mm_mul_ps(dg, dg)), _mm_mul_ps(db, db)), vthird);

			/* Active if de <= tol_slp (and if fo==1, alpha >= 0.005) */
			__m128 active = _mm_cmple_ps(de, vtol_slp);
			if (fo == 1) {
				__m128 va = _mm_mul_ps(_mm_cvtepi32_ps(_mm_srli_epi32(px, 24)), vf1);
				active = _mm_and_ps(active, _mm_cmpge_ps(va, vfo_min));
			}

			int act_mask = _mm_movemask_ps(active);
			if (act_mask == 0) {
				if (showmask) {
					_mm_storeu_si128((__m128i *)(outframe + i), vzero_out);
				} else if (m2a) {
					_mm_storeu_si128((__m128i *)(outframe + i), _mm_and_si128(px, vrgb_mask));
				} else {
					_mm_storeu_si128((__m128i *)(outframe + i), px);
				}
				continue;
			}

			float de_arr[4];
			_mm_storeu_ps(de_arr, de);
			for (int k = 0; k < 4; k++) {
				uint32_t p = inframe[i + k];
				if (!(act_mask & (1 << k))) {
					if (showmask) outframe[i + k] = zero_mask_out;
					else if (m2a) outframe[i + k] = p & 0x00FFFFFFu;
					else outframe[i + k] = p;
					continue;
				}
				float d = de_arr[k];
				float a = (d > tol_slp) ? 1.0f : (d - tol) * ip;
				if (d < tol) a = 0.0f;
				float m = (float)(1.0 - a);
				if (m == 0.0f) {
					if (showmask) outframe[i + k] = zero_mask_out;
					else if (m2a) outframe[i + k] = p & 0x00FFFFFFu;
					else outframe[i + k] = p;
					continue;
				}
				float r = u8_to_f[p & 0xFFu];
				float g = u8_to_f[(p >> 8) & 0xFFu];
				float b = u8_to_f[(p >> 16) & 0xFFu];
				outframe[i + k] = process_active_pixel(p, r, g, b, m,
					k32, ipi2, kh, u_a, u_b, cos_pass, cos_stop, use_pass, use_stop, hg_t, hg_tp, hg_ip, st_t1, st_t2, st_ip,
					op1, am1, op2, am2, kr, kg, kb, tr, tg, tb,
					cr_k, cg_k, cb_k, ikg, showmask, m2a);
			}
		}
#endif
		for (; i < total; i++) {
			uint32_t p = inframe[i];
			float al = u8_to_f[p >> 24];
			if (fo == 1 && al < 0.005f) {
				if (showmask) outframe[i] = zero_mask_out;
				else if (m2a) outframe[i] = p & 0x00FFFFFFu;
				else outframe[i] = p;
				continue;
			}
			float r = u8_to_f[p & 0xFFu];
			float g = u8_to_f[(p >> 8) & 0xFFu];
			float b = u8_to_f[(p >> 16) & 0xFFu];
			float dr = r - kr, dg = g - kg, db = b - kb;
			float d = (dr * dr + dg * dg + db * db) * third;
			float a = (d > tol_slp) ? 1.0f : (d - tol) * ip;
			if (d < tol) a = 0.0f;
			float m = (float)(1.0 - a);
			if (m == 0.0f) {
				if (showmask) outframe[i] = zero_mask_out;
				else if (m2a) outframe[i] = p & 0x00FFFFFFu;
				else outframe[i] = p;
				continue;
			}
			outframe[i] = process_active_pixel(p, r, g, b, m,
				k32, ipi2, kh, u_a, u_b, cos_pass, cos_stop, use_pass, use_stop, hg_t, hg_tp, hg_ip, st_t1, st_t2, st_ip,
				op1, am1, op2, am2, kr, kg, kb, tr, tg, tb,
				cr_k, cg_k, cb_k, ikg, showmask, m2a);
		}
	} else if (in->maskType == 1) {
		/* Transparency mask - fused single pass */
		const float ia = (float)(1.0 - in->tol);
		for (int i = 0; i < total; i++) {
			uint32_t p = inframe[i];
			float al = u8_to_f[p >> 24];
			float m = (al < 0.996 && al > 0.004) ? (float)(1.0 - ia * al) : 0.0f;
			if (m == 0.0f) {
				if (showmask) outframe[i] = zero_mask_out;
				else if (m2a) outframe[i] = p & 0x00FFFFFFu;
				else outframe[i] = p;
				continue;
			}
			float r = u8_to_f[p & 0xFFu];
			float g = u8_to_f[(p >> 8) & 0xFFu];
			float b = u8_to_f[(p >> 16) & 0xFFu];
			outframe[i] = process_active_pixel(p, r, g, b, m,
				k32, ipi2, kh, u_a, u_b, cos_pass, cos_stop, use_pass, use_stop, hg_t, hg_tp, hg_ip, st_t1, st_t2, st_ip,
				op1, am1, op2, am2, kr, kg, kb, tr, tg, tb,
				cr_k, cg_k, cb_k, ikg, showmask, m2a);
		}
	} else {
		/* Edge mask (maskType 2 or 3) - reuse cached mask buffer, no float_rgba buffer */
		if (!in->edge_buf) {
			in->edge_buf = (float *)malloc((size_t)total * sizeof(float));
			if (!in->edge_buf) return;
		}
		float *mask = in->edge_buf;
		if (in->maskType == 2) {
			edge_mask_u8(inframe, in->w, in->h, mask, in->tol * 200.0f, -1);
		} else if (in->maskType == 3) {
			edge_mask_u8(inframe, in->w, in->h, mask, in->tol * 200.0f, 1);
		} else {
			memset(mask, 0, (size_t)total * sizeof(float));
		}

		for (int i = 0; i < total; i++) {
			uint32_t p = inframe[i];
			float m = mask[i];
			if (m == 0.0f) {
				if (showmask) outframe[i] = zero_mask_out;
				else if (m2a) outframe[i] = p & 0x00FFFFFFu;
				else outframe[i] = p;
				continue;
			}
			float r = u8_to_f[p & 0xFFu];
			float g = u8_to_f[(p >> 8) & 0xFFu];
			float b = u8_to_f[(p >> 16) & 0xFFu];
			outframe[i] = process_active_pixel(p, r, g, b, m,
				k32, ipi2, kh, u_a, u_b, cos_pass, cos_stop, use_pass, use_stop, hg_t, hg_tp, hg_ip, st_t1, st_t2, st_ip,
				op1, am1, op2, am2, kr, kg, kb, tr, tg, tb,
				cr_k, cg_k, cb_k, ikg, showmask, m2a);
		}
	}
}
