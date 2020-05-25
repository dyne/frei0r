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

//------------------------------------------------------
//computes x to the power p
//only for positive x
float pwr(float x, float p)
{
if (x<=0) return 0;
return expf(p*logf(x));
}

//-----------------------------------------------------
//stretch [0...1] to parameter range [min...max] logarithmic
//min and max must be positive!
float map_value_forward_log(double v, float min, float max)
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
void make_lut1(float r, float g, float b, lut_s *lut, int norm, int cm)
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
void make_lut2(float r, float g, float b, lut_s *lut, int norm, int cm)
{
int i;
float rr,gg,bb,gama,l;

for (i=0;i<256;i++)
	{
	gama=map_value_forward_log(r,3.0,0.3333);
	rr=255.0*pwr((float)i/255.0,gama);
	gama=map_value_forward_log(g,3.0,0.3333);
	gg=255.0*pwr((float)i/255.0,gama);
	gama=map_value_forward_log(b,3.0,0.3333);
	bb=255.0*pwr((float)i/255.0,gama);

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
void make_lut3(float r, float g, float b, lut_s *lut, int norm, int cm)
{
int i;
float rr,gg,bb,l;

for (i=0;i<256;i++)
	{
	rr=(float)i*map_value_forward_log(r,0.3333,3.0);
	gg=(float)i*map_value_forward_log(g,0.3333,3.0);
	bb=(float)i*map_value_forward_log(b,0.3333,3.0);

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
void apply_lut(const uint32_t* inframe, uint32_t* outframe, int size, lut_s *lut, int ac)
{
int i;
uint32_t r,g,b,a;

if (ac==0)
	{
	for (i=0;i<size;i++)
		{
		outframe[i] = lut->r[inframe[i]&255];
		outframe[i] = outframe[i] + ((lut->g[(inframe[i]>>8)&255])<<8);
		outframe[i] = outframe[i] + ((lut->b[(inframe[i]>>16)&255])<<16);
		outframe[i] = outframe[i] + (inframe[i]&0xFF000000);
		}
	}
else		//alpha controlled
	{
	for (i=0;i<size;i++)
		{
		a=(inframe[i]>>24)&255;
		r = (255-a)*(inframe[i]&255) + a*lut->r[inframe[i]&255];
		g = (255-a)*((inframe[i]>>8)&255) + a*lut->g[(inframe[i]>>8)&255];
		b = (255-a)*((inframe[i]>>16)&255) + a*lut->b[(inframe[i]>>16)&255];
		outframe[i] = r/255 + ((g/255)<<8) + ((b/255)<<16);
		outframe[i] = outframe[i] + (inframe[i]&0xFF000000);
		}
	}
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
