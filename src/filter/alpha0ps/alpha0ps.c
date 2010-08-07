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

*/


//compile: gcc -c -fPIC -Wall alpha0ps.c -o alpha0ps.o
//link: gcc -shared -o alpha0ps.so alpha0ps.o

#include <stdio.h>
#include <frei0r.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>



//----------------------------------------
//struktura za instanco efekta
typedef struct
{
int h;
int w;

int disp;
int din;
int op;
float thr;
int sga;
int inv;

float *falpha,*ab;
} inst;


//---------------------------------------------------
//RGBA8888 little endian
void alphagray(inst *in, const uint32_t* inframe, uint32_t* outframe)
{
uint32_t s;
int i;

if (in->din==0)
	for (i=0;i<in->w*in->h;i++)
		{
		s=(outframe[i]&0xFF000000)>>24;
		s=s+(s<<8)+(s<<16);
		outframe[i]=(outframe[i]&0xFF000000)|s;
		}
else
	for (i=0;i<in->w*in->h;i++)
		{
		s=(inframe[i]&0xFF000000)>>24;
		s=s+(s<<8)+(s<<16);
		outframe[i]=(outframe[i]&0xFF000000)+s;
		}
}

//---------------------------------------------------
//RGBA8888 little endian
void grayred(inst *in, const uint32_t* inframe, uint32_t* outframe)
{
int i;
uint32_t r,g,b,a,y,s;

if (in->din==0)
	for (i=0;i<in->w*in->h;i++)
		{
		b=(inframe[i]&0x00FF0000)>>16;
		g=(inframe[i]&0x0000FF00)>>8;
		r=inframe[i]&0x000000FF;
		a=(outframe[i]&0xFF000000)>>24;
		y=(r>>2)+(g>>1)+(b>>2);	//approx luma
		y=64+(y>>1);
		r=y+(a>>1);
		if (r>255) r=255;
		s=r+(y<<8)+(y<<16);
		outframe[i]=(inframe[i]&0xFF000000)+s;
		}
else
	for (i=0;i<in->w*in->h;i++)
		{
		b=(inframe[i]&0x00FF0000)>>16;
		g=(inframe[i]&0x0000FF00)>>8;
		r=inframe[i]&0x000000FF;
		a=(inframe[i]&0xFF000000)>>24;
		y=(r>>2)+(g>>1)+(b>>2);
		y=64+(y>>1);
		r=y+(a<<1);
		if (r>255) r=255;
		s=r+(y<<8)+(y<<16);
		outframe[i]=(inframe[i]&0xFF000000)+s;
		}
}

//---------------------------------------------------
void drawsel(inst *in, const uint32_t* inframe, uint32_t* outframe, int bg)
{
int i;
uint32_t bk;
uint32_t r,g,b,a,s;

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
		b=(outframe[i]&0x00FF0000)>>16;
		g=(outframe[i]&0x0000FF00)>>8;
		r=outframe[i]&0x000000FF;
		a=(outframe[i]&0xFF000000)>>24;
		r=a*r+(255-a)*bk;
		r=r>>8;
		g=a*g+(255-a)*bk;
		g=g>>8;
		b=a*b+(255-a)*bk;
		b=b>>8;
		s=r+(g<<8)+(b<<16);
		outframe[i]=(inframe[i]&0xFF000000)+s;
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
		b=(inframe[i]&0x00FF0000)>>16;
		g=(inframe[i]&0x0000FF00)>>8;
		r=inframe[i]&0x000000FF;
		a=(inframe[i]&0xFF000000)>>24;
		r=a*r+(255-a)*bk;
		r=r>>8;
		g=a*g+(255-a)*bk;
		g=g>>8;
		b=a*b+(255-a)*bk;
		b=b>>8;
		s=r+(g<<8)+(b<<16);
		outframe[i]=(inframe[i]&0xFF000000)+s;
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
info->minor_version=1;
info->num_params=6;
info->explanation="Diplay and manipulation of the alpha channel";
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
		info->name = "Shrink/grow amount";
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
in->sga=1;
in->inv=0;

in->falpha=(float*)calloc(in->w*in->h,sizeof(float));
in->ab=(float*)calloc(in->w*in->h,sizeof(float));

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

chg=0;
switch(param_index)
	{
	case 0:
                tmpi=map_value_forward(*((double*)parm), 0.0, 6.9999);
                if (p->disp != tmpi) chg=1;
                p->disp=tmpi;
		break;
	case 1:
                tmpi=map_value_forward(*((double*)parm), 0.0, 1.0); //BOOL!!
                if (p->din != tmpi) chg=1;
                p->din=tmpi;
		break;
	case 2:
                tmpi=map_value_forward(*((double*)parm), 0.0, 6.9999);
                if (p->op != tmpi) chg=1;
                p->op=tmpi;
		break;
	case 3:
		tmpf=*(double*)parm;
		if (tmpf!=p->thr) chg=1;
		p->thr=tmpf;
		break;
	case 4:
                tmpi=map_value_forward(*((double*)parm), 0.0, 2.9999);
                if (p->sga != tmpi) chg=1;
                p->sga=tmpi;
		break;
	case 5:
                tmpi=map_value_forward(*((double*)parm), 0.0, 1.0); //BOOL!!
                if (p->inv != tmpi) chg=1;
                p->inv=tmpi;
		break;
	}

if (chg==0) return;

}

//--------------------------------------------------
void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index)
{
inst *p;
double tmpf;
int tmpi;

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
//RGBA8888 little endian
void f0r_update(f0r_instance_t instance, double time, const uint32_t* inframe, uint32_t* outframe)
{
inst *in;
int i;

assert(instance);
in=(inst*)instance;

//printf("update, op=%d, inv=%d disp=%d\n",in->op,in->inv,in->disp);

for (i=0;i<in->w*in->h;i++)
	in->falpha[i]=(inframe[i]&0xFF000000)>>24;

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
	default:
		break;
	}

if (in->inv==1)
	for (i=0;i<in->w*in->h;i++)
		in->falpha[i]=255.0-in->falpha[i];

for (i=0;i<in->w*in->h;i++)
	outframe[i] = (inframe[i]&0x00FFFFFF) | (((uint32_t)in->falpha[i])<<24);

switch (in->disp)
	{
	case 0:
		break;
	case 1:
		alphagray(in, inframe, outframe);
		break;
	case 2:
		grayred(in, inframe, outframe);
		break;
	case 3:
		drawsel(in, inframe, outframe, 0);
		break;
	case 4:
		drawsel(in, inframe, outframe, 1);
		break;
	case 5:
		drawsel(in, inframe, outframe, 2);
		break;
	case 6:
		drawsel(in, inframe, outframe, 3);
		break;
	default:
		break;
		}

}

//**********************************************************