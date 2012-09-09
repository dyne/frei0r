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


//compile: gcc -c -fPIC -Wall alpha0ps.c -o alpha0ps.o
//link: gcc -shared -o alpha0ps.so alpha0ps.o

#include <stdio.h>
#include <frei0r.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>


#include "fibe_f.h"


//----------------------------------------
//struktura za instanco efekta
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

//buffers & pointers
float *falpha,*ab;
uint8_t *infr,*oufr;

//auxilliary variables for fibe2o
float f,q,a0,a1,a2,b0,b1,b2,rd1,rd2,rs1,rs2,rc1,rc2;

} inst;


//---------------------------------------------------
void alphagray(inst *in)
{
uint8_t s;
int i;

if (in->din==0)
	for (i=0;i<in->w*in->h;i++)
		{
		s=in->oufr[4*i+3];
		in->oufr[4*i]=s;
		in->oufr[4*i+1]=s;
		in->oufr[4*i+2]=s;
		in->oufr[4*i+3]=0xFF;
		}
else
	for (i=0;i<in->w*in->h;i++)
		{
		s=in->infr[4*i+3];
		in->oufr[4*i]=s;
		in->oufr[4*i+1]=s;
		in->oufr[4*i+2]=s;
		in->oufr[4*i+3]=0xFF;
		}
}

//---------------------------------------------------
void grayred(inst *in)
{
int i,rr;
uint8_t r,g,b,a,y;

if (in->din==0)
	for (i=0;i<in->w*in->h;i++)
		{
		b=in->infr[4*i+2];
		g=in->infr[4*i+1];
		r=in->infr[4*i];
		a=in->oufr[4*i+3];
		y=(r>>2)+(g>>1)+(b>>2);	//approx luma
		y=64+(y>>1);
		rr=y+(a>>1);
		if (rr>255) rr=255;
		in->oufr[4*i]=rr;
		in->oufr[4*i+1]=y;
		in->oufr[4*i+2]=y;
		in->oufr[4*i+3]=0xFF;
		}
else
	for (i=0;i<in->w*in->h;i++)
		{
		b=in->infr[4*i+2];
		g=in->infr[4*i+1];
		r=in->infr[4*i];
		a=in->infr[4*i+3];
		y=(r>>2)+(g>>1)+(b>>2);	//approx luma
		y=64+(y>>1);
		rr=y+(a>>1);
		if (rr>255) rr=255;
		in->oufr[4*i]=rr;
		in->oufr[4*i+1]=y;
		in->oufr[4*i+2]=y;
		in->oufr[4*i+3]=0xFF;
		}
}

//---------------------------------------------------
void drawsel(inst *in, int bg)
{
int i;
uint32_t bk;
uint32_t r,g,b,a;

switch (bg)
	{
	case 0: bk=0x00; break;
	case 1: bk=0x80; break;
	case 2: bk=0xFF; break;
	default: break;
	}

if (in->din==0)
	for (i=0;i<in->w*in->h;i++)
		{
		if (bg==3)
			{
			if (((i/8)%2)^((i/8/in->w)%2))
				bk=0x64;
			else
				bk=0x9B;
			}
		b=in->oufr[4*i+2];
		g=in->oufr[4*i+1];
		r=in->oufr[4*i];
		a=in->oufr[4*i+3];
		r=(a*r+(255-a)*bk)>>8;
		g=(a*g+(255-a)*bk)>>8;
		b=(a*b+(255-a)*bk)>>8;
		in->oufr[4*i]=r;
		in->oufr[4*i+1]=g;
		in->oufr[4*i+2]=b;
		in->oufr[4*i+3]=0xFF;
		}
else
	for (i=0;i<in->w*in->h;i++)
		{
		if (bg==3)
			{
			if (((i/8)%2)^((i/8/in->w)%2))
				bk=0x64;
			else
				bk=0x9B;
			}
		b=in->infr[4*i+2];
		g=in->infr[4*i+1];
		r=in->infr[4*i];
		a=in->infr[4*i+3];
		r=(a*r+(255-a)*bk)>>8;
		g=(a*g+(255-a)*bk)>>8;
		b=(a*b+(255-a)*bk)>>8;
		in->oufr[4*i]=r;
		in->oufr[4*i+1]=g;
		in->oufr[4*i+2]=b;
		in->oufr[4*i+3]=0xFF;
		}
}

//----------------------------------------------------------
//shave based on average of 8 neighbors
void shave_alpha(float *sl, float *ab, int w, int h)
{
int i,j,p;
float m;

for (i=1;i<h-1;i++)
	{
	p=i*w+1;
	for (j=1;j<w-1;j++)
		{
		m = sl[p-1] + sl[p+1] + sl[p-w] + sl[p+w] + sl[p-1-w] + sl[p+1+w] + sl[p+1-w] + sl[p-1+w];
		m=m*0.125;
		ab[p] = (sl[p]<m) ? sl[p] : m;
		p++;
		}
	}

for (i=0;i<w*h;i++) sl[i]=ab[i];
}


//----------------------------------------------------------
void grow_alpha(float *al, float *ab,
int w, int h, int mode)
{
int i,j,p;
float m,md;

switch(mode)
	{
	case 0:
		for (i=1;i<h-1;i++)
			{
			p=i*w+1;
			for (j=1;j<w-1;j++)
				{
				ab[p]=al[p];
				if (al[p]<al[p-1])
					ab[p]=al[p-1];
				if (al[p]<al[p+1])
					ab[p]=al[p+1];
				if (al[p]<al[p-w])
					ab[p]=al[p-w];
				if (al[p]<al[p+w])
					ab[p]=al[p+w];
				p++;
				}
			}
		break;
	case 1:
		for (i=1;i<h-1;i++)
			{
			p=i*w+1;
			for (j=1;j<w-1;j++)
				{
				m=al[p];
				if (al[p]<al[p-1])
					m=al[p-1];
				if (al[p]<al[p+1])
					m=al[p+1];
				if (al[p]<al[p-w])
					m=al[p-w];
				if (al[p]<al[p+w])
					m=al[p+w];
				md=al[p];
				if (al[p]<al[p-1-w])
					md=al[p-1-w];
				if (al[p]<al[p+1-w])
					md=al[p+1-w];
				if (al[p]<al[p-1+w])
					md=al[p-1+w];
				if (al[p]<al[p+1+w])
					md=al[p+1+w];

				ab[p]=0.4*al[p]+0.4*m+0.2*md;
//				ab[p]=0.3*al[p]+0.4*m+0.3*md;
				p++;
				}
			}
		break;
	}



for (i=0;i<w*h;i++) al[i]=ab[i];
}

//----------------------------------------------------------
void shrink_alpha(float *al, float *ab,
int w, int h, int mode)
{
int i,j,p;
float m,md;

switch(mode)
	{
	case 0:
		for (i=1;i<h-1;i++)
			{
			p=i*w+1;
			for (j=1;j<w-1;j++)
				{
				ab[p]=al[p];
				if (al[p]>al[p-1])
					ab[p]=al[p-1];
				if (al[p]>al[p+1])
					ab[p]=al[p+1];
				if (al[p]>al[p-w])
					ab[p]=al[p-w];
				if (al[p]>al[p+w])
					ab[p]=al[p+w];
				p++;
				}
			}
		break;
	case 1:
		for (i=1;i<h-1;i++)
			{
			p=i*w+1;
			for (j=1;j<w-1;j++)
				{
				m=al[p];
				if (al[p]>al[p-1])
					m=al[p-1];
				if (al[p]>al[p+1])
					m=al[p+1];
				if (al[p]>al[p-w])
					m=al[p-w];
				if (al[p]>al[p+w])
					m=al[p+w];
				md=al[p];
				if (al[p]>al[p-1-w])
					md=al[p-1-w];
				if (al[p]>al[p+1-w])
					md=al[p+1-w];
				if (al[p]>al[p-1+w])
					md=al[p-1+w];
				if (al[p]>al[p+1+w])
					md=al[p+1+w];

				ab[p]=0.4*al[p]+0.4*m+0.2*md;
//				ab[p]=0.3*al[p]+0.4*m+0.3*md;
				p++;
				}
			}
		break;
	}



for (i=0;i<w*h;i++) al[i]=ab[i];
}

//---------------------------------------------------------
void threshold_alpha(float *al, int w, int h, float thr, float hi, float lo)
{
int i;

for (i=0;i<w*h;i++)
	al[i] = (al[i]>thr) ? hi : lo;
}

//----------------------------------------------------------
void blur_alpha(inst *in)
{
int i;

for (i=0;i<in->w*in->h;i++) in->falpha[i]*=0.0039215;

fibe2o_f(in->falpha, in->w, in->h, in->a1, in->a2, in->rd1, in->rd2, in->rs1, in->rs2, in->rc1, in->rc2, 1);

for (i=0;i<in->w*in->h;i++)
	{
	in->falpha[i]*=255.0;
	if (in->falpha[i]>255.0) in->falpha[i]=255.0;
	if (in->falpha[i]<0.0) in->falpha[i]=0.0;
	}
}

//--------------------------------------------------------
//Aitken-Neville interpolacija iz 4 tock (tretjega reda)
//t = stevilo tock v arrayu
//array xt naj bo v rastocem zaporedju, lahko neekvidistanten
float AitNev3(int t, float xt[], float yt[], float x)
{
float p[10];
int i,j,m;

if ((x<xt[0])||(x>xt[t-1]))
	{
//	printf("\n\n x=%f je izven mej tabele!",x);
	return 1.0/0.0;
	}

//poisce, katere tocke bo uporabil
m=0; while (x>xt[m++]);
m=m-4/2-1; if (m<0) m=0; if ((m+4)>(t-1)) m=t-4;

for (i=0;i<4;i++)
	p[i]=yt[i+m];

for (j=1;j<4;j++)
	for (i=(4-1);i>=j;i--)
		{
		p[i]=p[i]+(x-xt[i+m])/(xt[i+m]-xt[i-j+m])*(p[i]-p[i-1]);
		}
return p[4-1];
}

//-----------------------------------------------------
//stretch [0...1] to parameter range [min...max] linear
float map_value_forward(double v, float min, float max)
{
return min+(max-min)*v;
}

//-----------------------------------------------------
//collapse from parameter range [min...max] to [0...1] linear
double map_value_backward(float v, float min, float max)
{
return (v-min)/(max-min);
}

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

info->name="alpha0ps";
info->author="Marko Cebokli";
info->plugin_type=F0R_PLUGIN_TYPE_FILTER;
info->color_model=F0R_COLOR_MODEL_RGBA8888;
info->frei0r_version=FREI0R_MAJOR_VERSION;
info->major_version=0;
info->minor_version=3;
info->num_params=6;
info->explanation="Display and manipulation of the alpha channel";
}

//--------------------------------------------------
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

//----------------------------------------------
f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
inst *in;

in=calloc(1,sizeof(inst));
in->w=width;
in->h=height;

in->disp=0;
in->din=0;
in->op=0;
in->thr=0.5;
in->sga=1.0;
in->inv=0;

in->falpha=(float*)calloc(in->w*in->h,sizeof(float));
in->ab=(float*)calloc(in->w*in->h,sizeof(float));

in->f=0.05; in->q=0.55;		//blur
calcab_lp1(in->f, in->q, &in->a0, &in->a1, &in->a2, &in->b0, &in->b1, &in->b2);
in->a1=in->a1/in->a0; in->a2=in->a2/in->a0;
rep(-0.5, 0.5, 0.0, &in->rd1, &in->rd2, 256, in->a1, in->a2);
rep(1.0, 1.0, 0.0, &in->rs1, &in->rs2, 256, in->a1, in->a2);
rep(0.0, 0.0, 1.0, &in->rc1, &in->rc2, 256, in->a1, in->a2);

return (f0r_instance_t)in;
}

//---------------------------------------------------
void f0r_destruct(f0r_instance_t instance)
{
inst *in;

in=(inst*)instance;

free(in->falpha);
free(in->ab);
free(instance);
}

//-----------------------------------------------------
void f0r_set_param_value(f0r_instance_t instance, f0r_param_t parm, int param_index)
{
inst *p;
double tmpf;
int tmpi,chg;

p=(inst*)instance;

float am1[]={0.499999,0.7,1.0,1.5,2.0,3.0,4.0,5.0,7.0,10.0,15.0,20.0,30.0,40.0,50.0,70.0,100.0,150.0,200.00001};
float iir2f[]={0.475,0.39,0.325,0.26,0.21,0.155,0.112,0.0905,0.065,0.0458,0.031,0.0234,0.01575,0.0118,0.0093,0.00725,0.00505,0.0033,0.0025};
float iir2q[]={0.53,0.53,0.54,0.54,0.54,0.55,0.6,0.6,0.6,0.6,0.6,0.6,0.6,0.6,0.6,0.6,0.6,0.6,0.6};

chg=0;
switch(param_index)
	{
	case 0:		//Display
                tmpi=map_value_forward(*((double*)parm), 0.0, 6.9999);
                if (p->disp != tmpi) chg=1;
                p->disp=tmpi;
		break;
	case 1:		//Display input alpha
                tmpi=map_value_forward(*((double*)parm), 0.0, 1.0); //BOOL!!
                if (p->din != tmpi) chg=1;
                p->din=tmpi;
		break;
	case 2:		//Operation
                tmpi=map_value_forward(*((double*)parm), 0.0, 7.9999);
                if (p->op != tmpi) chg=1;
                p->op=tmpi;
		break;
	case 3:		//Threshold
		tmpf=*(double*)parm;
		if (tmpf!=p->thr) chg=1;
		p->thr=tmpf;
		break;
	case 4:		//Shrink/Grow/Blur amount
                tmpf=map_value_forward(*((double*)parm), 0.0, 4.9999);
                if (p->sga != tmpf) chg=1;
                p->sga=tmpf;
		break;
	case 5:		//Invert
                tmpi=map_value_forward(*((double*)parm), 0.0, 1.0); //BOOL!!
                if (p->inv != tmpi) chg=1;
                p->inv=tmpi;
		break;
	}

if (chg==0) return;

if (param_index==4)	// blur amount changed
	{
	p->f=AitNev3(19, am1, iir2f, 0.5+3.0*p->sga);
	p->q=AitNev3(19, am1, iir2q, 0.5+3.0*p->sga);
	calcab_lp1(p->f, p->q, &p->a0, &p->a1, &p->a2, &p->b0, &p->b1, &p->b2);
	p->a1=p->a1/p->a0; p->a2=p->a2/p->a0;
	rep(-0.5, 0.5, 0.0, &p->rd1, &p->rd2, 256, p->a1, p->a2);
	rep(1.0, 1.0, 0.0, &p->rs1, &p->rs2, 256, p->a1, p->a2);
	rep(0.0, 0.0, 1.0, &p->rc1, &p->rc2, 256, p->a1, p->a2);
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
                *((double*)param)=map_value_backward(p->disp, 0.0, 6.9999);
		break;
	case 1:
                *((double*)param)=map_value_backward(p->din, 0.0, 1.0);//BOOL!!
		break;
	case 2:
                *((double*)param)=map_value_backward(p->op, 0.0, 6.9999);
		break;
	case 3:
		*((double*)param)=p->thr;
		break;
	case 4:
                *((double*)param)=map_value_backward(p->sga, 0.0, 2.9999);
		break;
	case 5:
                *((double*)param)=map_value_backward(p->inv, 0.0, 1.0);//BOOL!!
		break;
	}
}

//-------------------------------------------------
void f0r_update(f0r_instance_t instance, double time, const uint32_t* inframe, uint32_t* outframe)
{
inst *in;
int i;

assert(instance);
in=(inst*)instance;
in->infr=(uint8_t*)inframe;
in->oufr=(uint8_t*)outframe;

for (i=0;i<in->w*in->h;i++)
	in->falpha[i]=in->infr[4*i+3];

switch (in->op)
	{
	case 0: break;
	case 1:
		for (i=0;i<in->sga;i++)
			shave_alpha(in->falpha, in->ab, in->w, in->h);
		break;
	case 2:
		for (i=0;i<in->sga;i++)
			shrink_alpha(in->falpha, in->ab, in->w, in->h, 0);
		break;
	case 3:
		for (i=0;i<in->sga;i++)
			shrink_alpha(in->falpha, in->ab, in->w, in->h, 1);
		break;
	case 4:
		for (i=0;i<in->sga;i++)
			grow_alpha(in->falpha, in->ab, in->w, in->h, 0);
		break;
	case 5:
		for (i=0;i<in->sga;i++)
			grow_alpha(in->falpha, in->ab, in->w, in->h, 1);
		break;
	case 6:
		threshold_alpha(in->falpha, in->w, in->h, 255.0*in->thr, 255.0, 0.0);
		break;
	case 7:
		blur_alpha(in);
		break;
	default:
		break;
	}

if (in->inv==1)
	for (i=0;i<in->w*in->h;i++)
		in->falpha[i]=255.0-in->falpha[i];

for (i=0;i<in->w*in->h;i++)
	{
	outframe[i] = inframe[i];
	in->oufr[4*i+3] = (uint8_t)in->falpha[i];
	}
	
switch (in->disp)
	{
	case 0:
		break;
	case 1:
		alphagray(in);
		break;
	case 2:
		grayred(in);
		break;
	case 3:
		drawsel(in, 0);
		break;
	case 4:
		drawsel(in, 1);
		break;
	case 5:
		drawsel(in, 2);
		break;
	case 6:
		drawsel(in, 3);
		break;
	default:
		break;
		}

}

//**********************************************************