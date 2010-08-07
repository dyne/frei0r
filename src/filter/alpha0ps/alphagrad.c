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


//compile: gcc -c -fPIC -Wall alphagrad.c -o alphagrad.o
//link: gcc -shared -o alphagrad.so alphagrad.o

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

float poz,wdt,tilt,min,max;
uint32_t *gr8;
int op;

} inst;

//-----------------------------------------------------
//RGBA8888 little endian
void fill_grad(inst *in)
{
int i,j;
float st,ct,po,wd,d,a;

st=sinf(in->tilt);
ct=cosf(in->tilt);
po=(-in->w/2.0+in->poz*in->w)*1.5;
wd=in->wdt*in->w;

if (in->min==in->max)
	{
	for (i=0;i<in->h*in->w;i++)
		in->gr8[i]=(((uint32_t)(in->min*255.0))<<24)&0xFF000000;
	return;
	}

for (i=0;i<in->h;i++)
	for (j=0;j<in->w;j++)
		{
		d=(i-in->h/2)*ct+(j-in->w/2)*st-po;
		if (fabsf(d)>wd/2.0)
			{
			if (d>0.0)
				a=in->min;
			else
				a=in->max;
			}
		else
			{
			if (d>wd/2.0) d=wd/2.0;
			a = in->min+(wd/2.0-d) / wd*(in->max-in->min);
			}
		a=255.0*a;
		in->gr8[i*in->w+j] = (((uint32_t)a)<<24)&0xFF000000;
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

info->name="alphagrad";
info->author="Marko Cebokli";
info->plugin_type=F0R_PLUGIN_TYPE_FILTER;
info->color_model=F0R_COLOR_MODEL_RGBA8888;
info->frei0r_version=FREI0R_MAJOR_VERSION;
info->major_version=0;
info->minor_version=1;
info->num_params=6;
info->explanation="Fills alpha channel with a gradient";
}

//--------------------------------------------------
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
		info->type = F0R_PARAM_BOOL;
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

//----------------------------------------------
f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
inst *in;

in=calloc(1,sizeof(inst));
in->w=width;
in->h=height;

in->poz=0.5;
in->wdt=0.5;
in->tilt=0.0;
in->min=0.0;
in->max=1.0;
in->op=0;

in->gr8 = (uint32_t*)calloc(in->w*in->h, sizeof(uint32_t));

fill_grad(in);

return (f0r_instance_t)in;
}

//---------------------------------------------------
void f0r_destruct(f0r_instance_t instance)
{
inst *in;

in=(inst*)instance;

free(in->gr8);
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
                tmpf=map_value_forward(*((double*)parm), -3.15, 3.15);
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
                tmpi=map_value_forward(*((double*)parm), 0.0, 4.9999);
                if (p->op != tmpi) chg=1;
                p->op=tmpi;
		break;
	}

if (chg==0) return;

fill_grad(p);
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
                *((double*)param)=p->poz;
		break;
	case 1:
                *((double*)param)=p->wdt;
		break;
	case 2:
                *((double*)param)=map_value_backward(p->tilt, -3.15, 3.15);
		break;
	case 3:
                *((double*)param)=p->min;
		break;
	case 4:
                *((double*)param)=p->max;
		break;
	case 5:
                *((double*)param)=map_value_backward(p->op, 0.0, 4.9999);
		break;
	}
}

//-------------------------------------------------
//RGBA8888 little endian
void f0r_update(f0r_instance_t instance, double time, const uint32_t* inframe, uint32_t* outframe)
{
inst *in;
int i,j;
float a,d,st,ct;
float po,wd;
uint32_t t;

assert(instance);
in=(inst*)instance;

switch (in->op)
	{
	case 0:		//write on clear
		for (i=0;i<in->h*in->w;i++)
			outframe[i] = (inframe[i]&0x00FFFFFF) | in->gr8[i];
		break;
	case 1:		//max
		for (i=0;i<in->h*in->w;i++)
			{
			t=((inframe[i]&0xFF000000)>in->gr8[i]) ? inframe[i]&0xFF000000 : in->gr8[i];
			outframe[i] = (inframe[i]&0x00FFFFFF) | t;
			}
		break;
	case 2:		//min
		for (i=0;i<in->h*in->w;i++)
			{
			t=((inframe[i]&0xFF000000)<in->gr8[i]) ? inframe[i]&0xFF000000 : in->gr8[i];
			outframe[i] = (inframe[i]&0x00FFFFFF) | t;
			}
		break;
	case 3:		//add
		for (i=0;i<in->h*in->w;i++)
			{
			t=((inframe[i]&0xFF000000)>>1)+(in->gr8[i]>>1);
			t = (t>0x7F800000) ? 0xFF000000 : t<<1;
			outframe[i] = (inframe[i]&0x00FFFFFF) | t;
			}
		break;
	case 4:		//subtract
		for (i=0;i<in->h*in->w;i++)
			{
			t= ((inframe[i]&0xFF000000)>in->gr8[i]) ? (inframe[i]&0xFF000000)-in->gr8[i] : 0;
			outframe[i] = (inframe[i]&0x00FFFFFF) | t;
			}
		break;
	default:
		break;
	}
}

//**********************************************************