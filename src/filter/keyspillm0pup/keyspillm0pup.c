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

//Version 0.2

//compile: gcc -c -fPIC -Wall keyspillm0pup.c -o keyspillm0pup.o
//link: gcc -shared -o keyspillm0pup.so keyspillm0pup.o

//*******************************************************************

#include <stdio.h>
#include <math.h>
#include <frei0r.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <string.h>

double PI=3.14159265358979;

typedef struct
	{
	float r;
	float g;
	float b;
	float a;
	} float_rgba;

//----------------------------------------------------
void RGBA8888_2_float(const uint32_t* in, float_rgba *out, int w, int h)
{
uint8_t *cin;
int i;
float f1;

cin=(uint8_t *)in;
f1=1.0/255.0;
for (i=0;i<w*h;i++)
	{
	out[i].r=f1*(float)*cin++;
	out[i].g=f1*(float)*cin++;
	out[i].b=f1*(float)*cin++;
	out[i].a=f1*(float)*cin++;
	}
}

//------------------------------------------------------------------
void float_2_RGBA8888(const float_rgba *in, uint32_t* out, int w, int h)
{
uint8_t *cout;
int i;

cout=(uint8_t *)out;

for (i=0;i<w*h;i++)
	{
	*cout++=(uint8_t)(in[i].r*255.0);
	*cout++=(uint8_t)(in[i].g*255.0);
	*cout++=(uint8_t)(in[i].b*255.0);
	*cout++=(uint8_t)(in[i].a*255.0);
	}
}

//------------------------------------------------
//color coeffs according to rec 601 or rec 701
void cocos(int cm, float *kr, float *kg, float *kb)
{
*kr=0.30; *kg=0.59; *kb=0.11;	//da compiler ne jamra
switch (cm)
    {
    case 0:
	{
	*kr=0.30; *kg=0.59; *kb=0.11;	//rec 601
	break;
	}
    case 1:
	{
	*kr=0.2126; *kg=0.7152; *kb=0.0722; //rec 709
	break;
	}
    default:
	{
	fprintf(stderr,"Unknown color model %d\n",cm);
	break;
	}
    }
}

//-------------------------------------------------
//premakne barvo radialno stran od key
//sorazmerno maski
//*mask=float maska [0...1]
//k=key
//am=amount  [0...1]
void clean_rad_m(float_rgba *s, int w, int h, float_rgba k, float *mask, float am)
{
int i;
float aa,min;
min=0.5;	//min aa = max color change

for (i=0;i<w*h;i++)
	{
	if (mask[i]==0.0) continue;
	aa=1.0-am*(1.0-min)*mask[i];
	s[i].r=(s[i].r-(1.0-aa)*k.r)/aa;
	s[i].g=(s[i].g-(1.0-aa)*k.g)/aa;
	s[i].b=(s[i].b-(1.0-aa)*k.b)/aa;
	if (s[i].r<0.0) s[i].r=0.0;
	if (s[i].g<0.0) s[i].g=0.0;
	if (s[i].b<0.0) s[i].b=0.0;
	if (s[i].r>1.0) s[i].r=1.0;
	if (s[i].g>1.0) s[i].g=1.0;
	if (s[i].b>1.0) s[i].b=1.0;
	}
}

//-------------------------------------------------
//premakne barvo proti target
//sorazmerno maski
//*mask=float maska [0...1]
//k=key
//am=amount  [0...1]
void clean_tgt_m(float_rgba *s, int w, int h, float_rgba k, float *mask, float am, float_rgba tgt)
{
int i;
float a,aa,min;

min=0.5;	//min aa = max color change

for (i=0;i<w*h;i++)
	{
	if (mask[i]==0.0) continue;

	a=mask[i];
	aa=a*am;
	s[i].r=s[i].r+(tgt.r-s[i].r)*aa;
	s[i].g=s[i].g+(tgt.g-s[i].g)*aa;
	s[i].b=s[i].b+(tgt.b-s[i].b)*aa;
	if (s[i].r<0.0) s[i].r=0.0;
	if (s[i].g<0.0) s[i].g=0.0;
	if (s[i].b<0.0) s[i].b=0.0;
	if (s[i].r>1.0) s[i].r=1.0;
	if (s[i].g>1.0) s[i].g=1.0;
	if (s[i].b>1.0) s[i].b=1.0;
	}
}

//----------------------------------------------------------
//desaturate colors according to mask
void desat_m(float_rgba *s, int w, int h, float *mask, float des, int cm)
{
float a,y,cr,cb,kr,kg,kb,ikg;
int i;
float ds;

cocos(cm,&kr,&kg,&kb);    
ikg=1.0/kg;

for (i=0;i<w*h;i++)
	{
	if (mask[i]==0.0) continue;

	a=mask[i];
//separate luma/chroma
	y=kr*s[i].r+kg*s[i].g+kb*s[i].b;
	cr=s[i].r-y;	// +-
	cb=s[i].b-y;	// +-
//desaturate
	ds=1.0-des*a;
	ds=ds*ds;
	cr=cr*ds; cb=cb*ds;
//back to RGB
	s[i].r=cr+y;
	s[i].b=cb+y;
	s[i].g=(y-kr*s[i].r-kb*s[i].b)*ikg;

	if (s[i].r<0.0) s[i].r=0.0;
	if (s[i].g<0.0) s[i].g=0.0;
	if (s[i].b<0.0) s[i].b=0.0;
	if (s[i].r>1.0) s[i].r=1.0;
	if (s[i].g>1.0) s[i].g=1.0;
	if (s[i].b>1.0) s[i].b=1.0;
	}
}

//----------------------------------------------------------
//adjust luma according to mask
void luma_m(float_rgba *s, int w, int h, float *mask, float lad, int cm)
{
float a,m,mm,y,cr,cb,kr,kg,kb,ikg;
int i;

cocos(cm,&kr,&kg,&kb);    
ikg=1.0/kg;
m=2.0*lad;

for (i=0;i<w*h;i++)
	{
	if (mask[i]==0.0) continue;

	a=mask[i];
//separate luma/chroma
	y=kr*s[i].r+kg*s[i].g+kb*s[i].b;
	cr=s[i].r-y;	// +-
	cb=s[i].b-y;	// +-
//adjust luma
	mm=(m-1.0)*a+1.0;
	if (m>=1.0) y=mm-1.0+y*(2.0-mm); else y=mm*y;
//back to RGB
	s[i].r=cr+y;
	s[i].b=cb+y;
	s[i].g=(y-kr*s[i].r-kb*s[i].b)*ikg;

	if (s[i].r<0.0) s[i].r=0.0;
	if (s[i].g<0.0) s[i].g=0.0;
	if (s[i].b<0.0) s[i].b=0.0;
	if (s[i].r>1.0) s[i].r=1.0;
	if (s[i].g>1.0) s[i].g=1.0;
	if (s[i].b>1.0) s[i].b=1.0;
	}
}

//---------------------------------------------------------
//do the blur for edge mask
//This is the fibe1o_8 function from the IIRblur plugin
//converted to scalar float (for planar color)
// 1-tap IIR v 4 smereh
//optimized for speed
//loops rearanged for more locality (better cache hit ratio)
//outer (vertical) loop 2x unroll to break dependency chain
//simplified indexes
void fibe1o_f(float *s, int w, int h, float a, int ec)
{
int i,j;
float b,g,g4,avg,avg1,cr,g4a,g4b;
int p,pw,pj,pwj,pww,pmw;

avg=8;	//koliko vzorcev za povprecje pri edge comp
avg1=1.0/avg;

g=1.0/(1.0-a);
g4=1.0/g/g/g/g;

//predpostavimo, da je "zunaj" crnina (nicle)
b=1.0/(1.0-a)/(1.0+a);

//prvih avg vrstic
for (i=0;i<avg;i++)
	{
	p=i*w;pw=p+w;
	if (ec!=0)
		{
		cr=0.0;
		for (j=0;j<avg;j++)
			cr=cr+s[p+j];
		cr=cr*avg1;
		s[p]=cr*g+b*(s[p]-cr);
		}

	for (j=1;j<w;j++)	//tja
		s[p+j]=s[p+j]+a*s[p+j-1];

	if (ec!=0)
		{
		cr=0.0;
		for (j=w-avg;j<w;j++)
			cr=cr+s[p+j];
		cr=cr*avg1;
		s[pw-1]=cr*g+b*(s[pw-1]-cr);
		}
	else
		s[pw-1]=b*s[pw-1];

	for (j=w-2;j>=0;j--)	//nazaj
		s[p+j]=a*s[p+j+1]+s[p+j];
	}

//prvih avg vrstic samo navzdol (nazaj so ze)
for (i=0;i<w;i++)
	{
	if (ec!=0)
		{
		cr=0.0;
		for (j=0;j<avg;j++)
			cr=cr+s[i+w*j];
		cr=cr*avg1;
		s[i]=cr*g+b*(s[i]-cr);
		}
	for (j=1;j<avg;j++)	//dol
		s[i+j*w]=s[i+j*w]+a*s[i+w*(j-1)];
	}

for (i=avg;i<h-1;i=i+2)	//po vrsticah navzdol
	{
	p=i*w; pw=p+w; pww=pw+w; pmw=p-w;
	if (ec!=0)
		{
		cr=0.0;
		for (j=0;j<avg;j++)
			cr=cr+s[p+j];
		cr=cr*avg1;
		s[p]=cr*g+b*(s[p]-cr);
		cr=0.0;
		for (j=0;j<avg;j++)
			cr=cr+s[pw+j];
		cr=cr*avg1;
		s[pw]=cr*g+b*(s[pw]-cr);
		}
	for (j=1;j<w;j++)	//tja
		{
		pj=p+j;pwj=pw+j;
		s[pj]=s[pj]+a*s[pj-1];
		s[pwj]=s[pwj]+a*s[pwj-1];
		}

	if (ec!=0)
		{
		cr=0.0;
		for (j=w-avg;j<w;j++)
			cr=cr+s[p+j];
		cr=cr*avg1;
		s[pw-1]=cr*g+b*(s[pw-1]-cr);
		cr=0.0;
		for (j=w-avg;j<w;j++)
			cr=cr+s[pw+j];
		cr=cr*avg1;
		s[pww-1]=cr*g+b*(s[pww-1]-cr);
		}
	else
		{
		s[pw-1]=b*s[pw-1];	//rep H
		s[pww-1]=b*s[pww-1];
		}

//zacetek na desni
	s[pw-2]=s[pw-2]+a*s[pw-1];	//nazaj
	s[pw-1]=s[pw-1]+a*s[p-1];	//dol

	for (j=w-2;j>=1;j--)	//nazaj
		{
		pj=p+j;pwj=pw+j;
		s[pj-1]=a*s[pj]+s[pj-1];
		s[pwj]=a*s[pwj+1]+s[pwj];
//zdaj naredi se en piksel vertikalno dol, za vse stolpce
//dva nazaj, da ne vpliva na H nazaj
		s[pj]=s[pj]+a*s[pmw+j];
		s[pwj+1]=s[pwj+1]+a*s[pj+1];
		}
//konec levo
	s[pw]=s[pw]+a*s[pw+1];		//nazaj
	s[p]=s[p]+a*s[pmw];		//dol
	s[pw+1]=s[pw+1]+a*s[p+1];	//dol
	s[pw]=s[pw]+a*s[p];		//dol
	}

//ce je sodo stevilo vrstic, moras zadnjo posebej
if (i!=h)
	{
	p=i*w; pw=p+w;
	for (j=1;j<w;j++)	//tja
		s[p+j]=s[p+j]+a*s[p+j-1];

	s[pw-1]=b*s[pw-1];	//rep H

	for (j=w-2;j>=0;j--)	//nazaj in dol
		{
		s[p+j]=a*s[p+j+1]+s[p+j];
//zdaj naredi se en piksel vertikalno dol, za vse stolpce
//dva nazaj, da ne vpliva na H nazaj
		s[p+j+1]=s[p+j+1]+a*s[p-w+j+1];
		}
//levi piksel vert
	s[p]=s[p]+a*s[p-w];
	}

//zadnja vrstica (h-1)
g4b=g4*b;
g4a=g4/(1.0-a);
p=(h-1)*w;
if (ec!=0)
	{
	for (i=0;i<w;i++)	//po stolpcih
		{
		cr=0.0;
		for (j=h-avg;j<h;j++)
			cr=cr+s[i+w*j];
		cr=cr*avg1;
		s[i+p]=g4a*cr+g4b*(s[i+p]-cr);
		}
	}
else
	{
	for (j=0;j<w;j++)	//po stolpcih
		s[j+p]=g4b*s[j+p];	//rep V
	}

for (i=h-2;i>=0;i--)	//po vrsticah navzgor
	{
	p=i*w; pw=p+w;
	for (j=0;j<w;j++)	//po stolpcih
		s[p+j]=a*s[pw+j]+g4*s[p+j];
	}

}

//----------------------------------------------------------
//mask based on euclidean RGB distance  (alpha independent)
//mask values [0...1]
//fo=1 foreground only (alpha>0.005)
void rgb_mask(float_rgba *s, int w, int h, float *mask, float_rgba k, float t, float p, int fo)
{
int i;
float dr,dg,db,d,ip,tr,a,de;

ip = (p>0.000001) ? 1.0/p : 1000000.0;
tr=1.0/3.0;
for (i=0;i<w*h;i++)
	{
	if ((fo==1)&&(s[i].a<0.005)) {mask[i]=0.0; continue;}

//euclidean RGB distance
	dr=s[i].r-k.r;
	dg=s[i].g-k.g;
	db=s[i].b-k.b;
	de=dr*dr+dg*dg+db*db;
	de=de*tr;	//max mozno=1.0

	d=de;

	if (d>(t+p))
		a=1.0;		//notranjost (alfa=1)
	else
		a=(d-t)*ip;
	if (d<t) a=0.0;		//blizu key, max efekt

	mask[i]=1.0-a;
	}
}

//----------------------------------------------------------
//mask based on hue difference   (alpha independent)
//mask values [0...1]
//fo=1 foreground only (alpha>0.005)
void hue_mask(float_rgba *s, int w, int h, float *mask, float_rgba k, float t, float p, int fo)
{
int i;
float d,ip,tr,a;
float ka,k32,kh,kbb,ipi,b,hh;
float sa;

k32=sqrtf(3.0)/2.0;
ipi=1.0/PI;
ka=k.r-0.5*k.g-0.5*k.b;
kbb=k32*(k.g-k.b);
kh=atan2f(kbb,ka)*ipi;		//  [-1...+1]
sa=0.0;		//da compiler ne jamra
//printf("color mask, key hue = %6.3f\n",kh);
//printf("color mask, key = %6.3f %6.3f %6.3f\n",k.r, k.g, k.b);
//printf("color mask, key a.b = %6.3f %6.3f\n",ka,kbb);

ip = (p>0.000001) ? 1.0/p : 1000000.0;
tr=1.0/3.0;
for (i=0;i<w*h;i++)
	{
	if ((fo==1)&&(s[i].a<0.005)) {mask[i]=0.0; continue;}

//difference of hue		2.6X pocasneje kot RGB...
	a=s[i].r-0.5*s[i].g-0.5*s[i].b;
	b=k32*(s[i].g-s[i].b);
	hh=atan2f(b,a)*ipi;		//  [-1...1]
	d = (hh>kh) ? hh-kh : kh-hh;	//  [0...2]
	d = (d>1.0) ? 2.0-d : d;	// [0...1] cir
//	sa=hypotf(b,a)/(s[i].r+s[i].g+s[i].b+1.0E-6)*3.0;

	if (d>(t+p))
		a=1.0;		//notranjost (alfa=1)
	else
		a=(d-t)*ip;
	if (d<t) a=0.0;		//blizu key, max efekt

	mask[i]=1.0-a;
//	mask[i]=hh;	//debug
	}
}

//----------------------------------------------------------
//mask values [0...1]
void edge_mask(float_rgba *s, int w, int h, float *mask, float wd, int io)
{
int i;
float a;
float lim;

lim=0.05;	//clear mask below this value (good for speed)

//fully opaque areas
for (i=0;i<w*h;i++)
	if (s[i].a>0.996) mask[i]=1.0; else mask[i]=0.0;

//blur mask
a=expf(logf(lim)/wd);
fibe1o_f(mask, w, h, a, 1);

//select edge
if (io==-1)	//inside
	for (i=0;i<w*h;i++)
		{
		if (mask[i]>0.5) mask[i]=2.0*(1.0-mask[i]); else mask[i]=0.0;  //inside only
		if (mask[i]<lim) mask[i]=0.0;
		}

if (io==1)	//outside
	for (i=0;i<w*h;i++)
		{
		if (mask[i]<0.5) mask[i]=2.0*mask[i]; else mask[i]=0.0;  //outside only AURA
		if (mask[i]<lim) mask[i]=0.0;
		}

}

//----------------------------------------------------------
//mask values [0...1]
//partially transparent areas
//useful as additional clean after key or edge
//amp=amplify mask for lower transparencies [0...1]
void trans_mask(float_rgba *s, int w, int h, float *mask, float amp)
{
int i;
float ia;
//partially transparent areas
ia=1.0-amp;
for (i=0;i<w*h;i++)
	if ((s[i].a<0.996)&&(s[i].a>0.004))
		mask[i]=1.0-ia*s[i].a;
	else
		mask[i]=0.0;
}

//------------------------------------------------
//gate the mask based on similarity of hue to key
void hue_gate(float_rgba *s, int w, int h, float *mask, float_rgba k, float t, float p)
{
float k32,ka,kb,kh,ipi2,a,b,hh,d,aa,ip;
int i;

k32=sqrtf(3.0)/2.0;
ipi2=0.5/PI;
ip = (p>0.000001) ? 1.0/p : 1000000.0;

ka=k.r-0.5*k.g-0.5*k.b;
kb=k32*(k.g-k.b);
kh=atan2f(kb,ka)*ipi2;		//  +- 1.0

for (i=0;i<w*h;i++)
	{
	if (mask[i]==0.0) continue;

	a=s[i].r-0.5*s[i].g-0.5*s[i].b;
	b=k32*(s[i].g-s[i].b);
	hh=atan2f(b,a)*ipi2;

	d = (hh>kh) ? hh-kh : kh-hh;	//  [0...2]
	d = (d>1.0) ? 2.0-d : d;

	if (d>(t+p)) {mask[i]=0.0; continue;}
	if (d<t) continue;

	aa=1.0-(d-t)*ip;
	mask[i]=mask[i]*aa;
	}
}

//------------------------------------------------
//reduce the mask based on a saturation threshold
//intensity normalized saturation
void sat_thres(float_rgba *s, int w, int h, float *mask,  float th)
{
float k32,ipi2,a,b,ip,sa;
float t1,t2;
int i;
float p=0.1;	//sirina prehodnega pasu

t2=(1.0+p)*th;	//above this, no mask change
t1=t2-p;	//below this, mask=0

k32=sqrtf(3.0)/2.0;
ipi2=0.5/PI;
ip = (p>0.000001) ? 1.0/p : 1000000.0;

for (i=0;i<w*h;i++)
	{
	if (mask[i]==0.0) continue;

	a=s[i].r-0.5*s[i].g-0.5*s[i].b;
	b=k32*(s[i].g-s[i].b);
	sa=hypotf(b,a)/(s[i].r+s[i].g+s[i].b+1.0E-6);

	if (sa>t2) continue;
	if (sa<t1) {mask[i]=0.0; continue;}
	mask[i]=mask[i]*(sa-t1)*ip;
	}
}

//--------------------------------------------------
void copy_mask_i(float_rgba *sl, int w, int h, float *mask)
{
int i;

for (i=0;i<w*h;i++)
	{
	sl[i].r=mask[i];
	sl[i].g=mask[i];
	sl[i].b=mask[i];
	sl[i].a=1.0;
	}
}

//--------------------------------------------------
void copy_mask_a(float_rgba *sl, int w, int h, float *mask)
{
int i;

for (i=0;i<w*h;i++)
	{
	sl[i].a=mask[i];
	}
}

//***********************************************************
//		FREI0R FUNKCIJE

//----------------------------------------
//struktura za instanco efekta
typedef struct
{
//status
int w,h;

//parameters
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
int fo;		//foreground only (speed)
int cm;		//color model 0=rec601  1=rec 709

//video buffers
float_rgba *sl;
float *mask;

//internal variables
float_rgba krgb;
float_rgba trgb;
char *liststr;

} inst;

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

info->name="keyspillm0pup";
info->author="Marko Cebokli";
info->plugin_type=F0R_PLUGIN_TYPE_FILTER;
info->color_model=F0R_COLOR_MODEL_RGBA8888;
info->frei0r_version=FREI0R_MAJOR_VERSION;
info->major_version=0;
info->minor_version=2;
info->num_params=13;
info->explanation="Reduces the visibility of key color spill in chroma keying";
}

//--------------------------------------------------
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

//----------------------------------------------
f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
inst *in;

in=calloc(1,sizeof(inst));
in->w=width;
in->h=height;

in->sl=calloc(in->w*in->h,sizeof(float_rgba));
in->mask=calloc(in->w*in->h,sizeof(float));

//defaults
in->key.r = 0.1; 
in->key.g = 0.8; 
in->key.b = 0.1; 
in->tgt.r = 0.78; 
in->tgt.g = 0.5; 
in->tgt.b = 0.4; 
in->maskType=0;
in->tol=0.12;
in->slope=0.2;
in->Hgate=0.25;
in->Sthresh=0.15;
in->op1=1;
in->am1=0.55;
in->op2=0;
in->am2=0.0;
in->showmask=0;
in->m2a=0;
in->fo=1;
in->cm=1;

const char* sval = "0";
in->liststr = (char*)malloc( strlen(sval) + 1 );
strcpy( in->liststr, sval );

return (f0r_instance_t)in;
}

//---------------------------------------------------
void f0r_destruct(f0r_instance_t instance)
{
inst *in;

in=(inst*)instance;

free(in->sl);
free(in->mask);
free(in->liststr);
free(instance);
}

//-----------------------------------------------------
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
	case 0:		//key color
		tmpc=*(f0r_param_color_t*)parm;
		if ((tmpc.r!=p->key.r) || (tmpc.g!=p->key.g) || (tmpc.b!=p->key.b))
			chg=1;
		p->key=tmpc;
		p->krgb.r=p->key.r;
		p->krgb.g=p->key.g;
		p->krgb.b=p->key.b;
		break;
	case 1:		//target color
		tmpc=*(f0r_param_color_t*)parm;
		if ((tmpc.r!=p->tgt.r) || (tmpc.g!=p->tgt.g) || (tmpc.b!=p->tgt.b))
			chg=1;
		p->tgt=tmpc;
		p->trgb.r=p->tgt.r;
		p->trgb.g=p->tgt.g;
		p->trgb.b=p->tgt.b;
		break;
	case 2:		//Mask type (list)
		tmpch = (*(char**)parm);
		p->liststr = (char*)realloc( p->liststr, strlen(tmpch) + 1 );
		strcpy( p->liststr, tmpch );
		nc=sscanf(p->liststr,"%d",&tmpi);
//		if ((nc<=0)||(tmpi<0)||(tmpi>3)) tmpi=1;
		if ((nc<=0)||(tmpi<0)||(tmpi>3)) break;
		if (p->maskType != tmpi) chg=1;
		p->maskType = tmpi;
		break;
	case 3:		//tolerance
		tmpf=map_value_forward(*((double*)parm), 0.0, 0.5);
		if (p->tol != tmpf) chg=1;
		p->tol = tmpf;
		break;
	case 4:		//slope
		tmpf=map_value_forward(*((double*)parm), 0.0, 0.5);
		if (p->slope != tmpf) chg=1;
		p->slope = tmpf;
		break;
	case 5:		//Hue gate
		tmpf=*(double*)parm;
		if (tmpf!=p->Hgate) chg=1;
		p->Hgate=tmpf;
		break;
	case 6:		//Saturation threshold
		tmpf=*(double*)parm;
		if (tmpf!=p->Sthresh) chg=1;
		p->Sthresh=tmpf;
		break;
	case 7:		//Operation 1 (list)
		tmpch = (*(char**)parm);
		p->liststr = (char*)realloc( p->liststr, strlen(tmpch) + 1 );
		strcpy( p->liststr, tmpch );
		nc=sscanf(p->liststr,"%d",&tmpi);
//		if ((nc<=0)||(tmpi<0)||(tmpi>4)) tmpi=0;
		if ((nc<=0)||(tmpi<0)||(tmpi>4)) break;
		if (p->op1 != tmpi) chg=1;
		p->op1 = tmpi;
		break;
	case 8:		//Amount 1
		tmpf=*(double*)parm;
		if (tmpf!=p->am1) chg=1;
		p->am1=tmpf;
		break;
	case 9:		//Operation 2 (list)
		tmpch = (*(char**)parm);
		p->liststr = (char*)realloc( p->liststr, strlen(tmpch) + 1 );
		strcpy( p->liststr, tmpch );
		nc=sscanf(p->liststr,"%d",&tmpi);
//		if ((nc<=0)||(tmpi<0)||(tmpi>4)) tmpi=0;
		if ((nc<=0)||(tmpi<0)||(tmpi>4)) break;
		if (p->op2 != tmpi) chg=1;
		p->op2 = tmpi;
		break;
	case 10:	//Amount 2
		tmpf=*(double*)parm;
		if (p->am2 != tmpf) chg=1;
		p->am2 = tmpf;
		break;
	case 11:	//Show mask  (bool)
		tmpf=*(double*)parm;
		tmpi=roundf(tmpf);
		if (tmpi!=p->showmask) chg=1;
		p->showmask=tmpi;
		break;
	case 12:	//Mask to Alpha  (bool)
		tmpf=*(double*)parm;
		tmpi=roundf(tmpf);
		if (tmpi!=p->m2a) chg=1;
		p->m2a=tmpi;
		break;
	}

if (chg==0) return;

}

//--------------------------------------------------
void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index)
{
inst *p;

p=(inst*)instance;

switch(param_index)
	{
	case 0:		//key color
		*((f0r_param_color_t*)param)=p->key;
		break;
	case 1:		//target color
		*((f0r_param_color_t*)param)=p->tgt;
		break;
	case 2:		//Mask type   (list)
		p->liststr=realloc(p->liststr,16);
		sprintf(p->liststr,"%d",p->maskType);
		*((char**)param) = p->liststr;
		break;
	case 3:		//tolerance
		*((double*)param)=map_value_backward(p->tol, 0.0, 0.5);
		break;
	case 4:		//slope
		*((double*)param)=map_value_backward(p->slope, 0.0, 0.5);
		break;
	case 5:		//Hue gate
		*((double*)param)=p->Hgate;
		break;
	case 6:		//Saturation threshold
		*((double*)param)=p->Sthresh;
		break;
	case 7:		//Operation 1  (list)
		p->liststr=realloc(p->liststr,16);
		sprintf(p->liststr,"%d",p->op1);
		*((char**)param) = p->liststr;
		break;
	case 8:		//Amount 1
		*((double*)param)=p->am1;
		break;
	case 9:		//Operation 2  (list)
		p->liststr=realloc(p->liststr,16);
		sprintf(p->liststr,"%d",p->op2);
		*((char**)param) = p->liststr;
		break;
	case 10:	//Amount 2
		*((double*)param)=p->am2;
		break;
	case 11:	//Show mask (bool)
		*((double*)param)=(double)p->showmask;
		break;
	case 12:	//Mask 2 Alpha (bool)
		*((double*)param)=(double)p->m2a;
		break;
	}
}

//==============================================================
void f0r_update(f0r_instance_t instance, double time, const uint32_t* inframe, uint32_t* outframe)
{
inst *in;

assert(instance);
in=(inst*)instance;

RGBA8888_2_float(inframe, in->sl, in->w, in->h);

switch(in->maskType)		//GENERATE MASK
    {
    case 0:		//Color distance based mask
	{
	rgb_mask(in->sl, in->w, in->h, in->mask, in->krgb, in->tol, in->slope, in->fo);
        break;
	}
    case 1:		//Transparency based mask
	{
	trans_mask(in->sl, in->w, in->h, in->mask, in->tol);
	break;
	}
    case 2:		//Edge based mask inwards
	{
	edge_mask(in->sl, in->w, in->h, in->mask, in->tol*200.0, -1);
	break;
	}
    case 3:		//Edge based mask outwards
	{
	edge_mask(in->sl, in->w, in->h, in->mask, in->tol*200.0, 1);
	break;
	}
    }

hue_gate(in->sl, in->w, in->h, in->mask, in->krgb, in->Hgate, 0.5*in->Hgate);
sat_thres(in->sl, in->w, in->h, in->mask, in->Sthresh);

switch(in->op1)		//OPERATION 1
    {
    case 0: break;
    case 1:	//De-Key
	{
	clean_rad_m(in->sl, in->w, in->h, in->krgb, in->mask, in->am1);
	break;
	}
    case 2:	//Target
	{
	clean_tgt_m(in->sl, in->w, in->h, in->krgb, in->mask, in->am1, in->trgb);
	break;
	}
    case 3:	//Desaturate
	{
	desat_m(in->sl, in->w, in->h, in->mask, in->am1, in->cm);
	break;
	}
    case 4:	//Luma adjust
	{
	luma_m(in->sl, in->w, in->h, in->mask, in->am1, in->cm);
	break;
	}
    }
    
switch(in->op2)		//OPERATION 2
    {
    case 0: break;
    case 1:	//De-Key
	{
	clean_rad_m(in->sl, in->w, in->h, in->krgb, in->mask, in->am2);
	break;
	}
    case 2:	//Target
	{
	clean_tgt_m(in->sl, in->w, in->h, in->krgb, in->mask, in->am2, in->trgb);
	break;
	}
    case 3:	//Desaturate
	{
	desat_m(in->sl, in->w, in->h, in->mask, in->am2, in->cm);
	break;
	}
    case 4:	//Luma adjust
	{
	luma_m(in->sl, in->w, in->h, in->mask, in->am2, in->cm);
	break;
	}
    }

if (in->showmask)	//REPLACE IMAGE WITH THE MASK
    {
    copy_mask_i(in->sl, in->w, in->h, in->mask);
    }
    
if (in->m2a)		//REPLACE ALPHA WITH THE MASK
    {
    copy_mask_a(in->sl, in->w, in->h, in->mask);
    }      
      
      
float_2_RGBA8888(in->sl, outframe, in->w, in->h);
}