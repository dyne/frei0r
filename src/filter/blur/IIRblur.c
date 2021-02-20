/*
IIRblur.c

This Frei0r plugin implements fast IIR blurring of video

Version 0.1	jul 2011

Copyright (C) 2011  Marko Cebokli    http://lea.hamradio.si/~s57uuu


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


//compile: gcc -c -fPIC -Wall IIRblur.c -o IIRblur.o
//link: gcc -shared -o IIRblur.so IIRblur.o

//stdio samo za debug izpise
//#include <stdio.h>
#include <frei0r.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <inttypes.h>
#include <string.h>

double PI=3.14159265358979;

typedef struct
{
    float r;
    float g;
    float b;
    float a;
} float_rgba;

#include "fibe.h"


//----------------------------------------
//struktura za instanco efekta
typedef struct
{
    //status
    int h;
    int w;

    //parameters
    float am;	//amount of blur
    int ty;		//type of blur [0..2]
    int ec;		//edge compensation (BOOL)

    //video buffers
    float_rgba *img;

    //internal variables
    float a1,a2,a3;
    float rd1,rd2,rs1,rs2,rc1,rc2;

} inst;

//--------------------------------------------------------
//Aitken-Neville interpolacija iz 4 tock (tretjega reda)
//t = stevilo tock v arrayu
//array xt naj bo v rastocem zaporedju, lahko neekvidistanten
float AitNev3(int t, float xt[], float yt[], float x)
{
    float p[10];
    int i,j,m;
    float zero = 0.0f; // MSVC doesn't allow division through a zero literal, but allows it through non-const variable set to zero

    if ((x<xt[0])||(x>xt[t-1]))
    {
        //	printf("\n\n x=%f je izven mej tabele!",x);
        return 1.0/zero;
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

//-----------------------------------------------------
//collapse from parameter range [min...max] to [0...1] logarithmic
//min and max must be positive!
double map_value_backward_log(float v, float min, float max)
{
    float sr,k;

    sr=sqrtf(min*max);
    k=2.0*log(max/sr);
    return logf(v/sr)/k+0.5;
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

    info->name="IIR blur";
    info->author="Marko Cebokli";
    info->plugin_type=F0R_PLUGIN_TYPE_FILTER;
    info->color_model=F0R_COLOR_MODEL_RGBA8888;
    info->frei0r_version=FREI0R_MAJOR_VERSION;
    info->major_version=0;
    info->minor_version=2;
    info->num_params=3;
    info->explanation="Three types of fast IIR blurring";
}

//--------------------------------------------------
void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
    switch(param_index)
    {
    case 0:
        info->name = "Amount";
        info->type = F0R_PARAM_DOUBLE;
        info->explanation = "Amount of blur";
        break;
    case 1:
        info->name = "Type";
        info->type = F0R_PARAM_DOUBLE;
        info->explanation = "Blur type";
        break;
    case 2:
        info->name = "Edge";
        info->type = F0R_PARAM_BOOL;
        info->explanation = "Edge compensation";
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

    in->img=calloc(width*height*4,sizeof(float));

    in->am=map_value_forward_log(0.2, 0.5, 100.0);
    in->a1=-0.796093; in->a2=0.186308;
    in->ty=1;
    in->ec=1;

    return (f0r_instance_t)in;
}

//---------------------------------------------------
void f0r_destruct(f0r_instance_t instance)
{
    inst *in;

    in=(inst*)instance;

    free(in->img);

    free(instance);
}

//-----------------------------------------------------
void f0r_set_param_value(f0r_instance_t instance, f0r_param_t parm, int param_index)
{
    inst *p;
    double tmpf;
    int chg,tmpi;
    float a0,b0,b1,b2,f,q,s;

    float am1[]={0.499999,0.7,1.0,1.5,2.0,3.0,4.0,5.0,7.0,10.0,
                 15.0,20.0,30.0,40.0,50.0,70.0,100.0,150.0,200.00001};
    //float iir2f[]={0.448,0.4,0.31,0.25,0.21,0.15,0.1,0.075,
    //	0.055,0.039,0.026,0.02,0.013,0.01,0.008,0.006,
    //	0.0042,0.0029,0.00205};
    //float iir2q[]={0.53,0.53,0.54,0.54,0.54,0.55,0.6,0.7,0.7,
    //	0.7,0.7,0.7,0.7,0.7,0.7,0.7,0.7,0.7,0.7};
    float iir2q[]={0.53,0.53,0.54,0.54,0.54,0.55,0.6,0.6,0.6,
                   0.6,0.6,0.6,0.6,0.6,0.6,0.6,0.6,0.6,0.6};
    //float iir1a1[]={0.167,0.3,0.5,0.65,0.7,0.8,0.88,0.92,0.95,
    //	0.96,0.97,0.98,0.985,0.988,0.99,0.992,0.993,0.9955,
    //	0.997};

    float iir1a1[]={0.138,0.24,0.34,0.45,0.55, 0.65,0.728,0.775,0.834,0.88,
                    0.92,0.937,0.958,0.968,0.9745,
                    0.98,0.986,0.991,0.9931};		//po sigmi

    float iir2f[]={0.475,0.39,0.325,0.26,0.21,
                   0.155,0.112,0.0905,0.065,0.0458,
                   0.031,0.0234,0.01575,0.0118,0.0093,
                   0.00725,0.00505,0.0033,0.0025};		//po sigmi

    float iir3si[]={0.5,0.7,1.0,1.5,2.0,
                    3.0,4.0,5.0,7.0,10.0,
                    15.0,20.0,30.0,40.0,50.0,
                    70.0,100.0,150.0,186.5};


    p=(inst*)instance;

    chg=0;
    switch(param_index)
    {
    case 0:
        //		tmpf=map_value_forward(*((double*)parm), 0.5, 100.0);
        if (*((double*)parm) == 0.0)
            tmpf = 0.0;
        else
            tmpf=map_value_forward_log(*((double*)parm), 0.5, 100.0);
        if (tmpf!=p->am) chg=1;
        p->am=tmpf;
        break;
    case 1:
        tmpf=*((double*)parm);
        if (tmpf>=1.0)
            tmpi=(int)tmpf;
        else
            tmpi = map_value_forward(tmpf, 0.0, 2.9999);
        if ((tmpi<0)||(tmpi>2.0)) break;
        if (p->ty != tmpi) chg=1;
        p->ty = tmpi;
        break;
    case 2:
        p->ec=map_value_forward(*((double*)parm), 0.0, 1.0); //BOOL!!
        break;
    }

    if (chg==0) return;

    switch(p->ty)
    {
    case 0:		//FIBE-1
        p->a1=AitNev3(19, am1, iir1a1, p->am);
        //printf("Set parm FIBE-1 a1=%f (p->am=%f)\n",p->a1,p->am);
        break;
    case 1:		//FIBE-2
        f=AitNev3(19, am1, iir2f, p->am);
        q=AitNev3(19, am1, iir2q, p->am);
        calcab_lp1(f, q, &a0, &p->a1, &p->a2, &b0, &b1, &b2);
        p->a1=p->a1/a0; p->a2=p->a2/a0;
        rep(-0.5, 0.5, 0.0, &p->rd1, &p->rd2, 256, p->a1, p->a2);
        rep(1.0, 1.0, 0.0, &p->rs1, &p->rs2, 256, p->a1, p->a2);
        rep(0.0, 0.0, 1.0, &p->rc1, &p->rc2, 256, p->a1, p->a2);
        //printf("Set parm FIBE-2 a1=%f a2=%f\n",p->a1,p->a2);
        break;
    case 2:		//FIBE-3
        s=AitNev3(19, am1, iir3si, p->am);
        young_vliet(s, &a0, &p->a1, &p->a2, &p->a3);
        p->a1=-p->a1/a0;
        p->a2=-p->a2/a0;
        p->a3=-p->a3/a0;
        //printf("Set parm FIBE-3 a1=%f a2=%f a3=%f\n",p->a1,p->a2,p->a3);
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
        //		*((double*)param)=map_value_backward(p->am, 0.5, 100.0);
        *((double*)param)=map_value_backward_log(p->am, 0.5, 100.0);
        break;
    case 1:
        *((double*)param)=map_value_backward(p->ty, 0.0, 2.9999);
        break;
    case 2:
        *((double*)param)=map_value_backward(p->ec, 0.0, 1.0);//BOOL!!
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

    if (in->am==0.0)	//zero blur, just copy and return
    {
        memcpy(outframe, inframe, in->w * in->h * sizeof(uint32_t));
        return;
    }
    //do the blur
    switch(in->ty)
    {
    case 0:
        fibe1o_8(inframe, outframe, in->img, in->w, in->h, in->a1, in->ec);
        break;
    case 1:
        fibe2o_8(inframe, outframe, in->img, in->w, in->h, in->a1, in->a2, in->rd1, in->rd2, in->rs1, in->rs2, in->rc1, in->rc2, in->ec);
        break;
    case 2:
        fibe3_8(inframe, outframe, in->img, in->w, in->h, in->a1, in->a2, in->a3, in->ec);
        // The bottom 3 lines were not updated, and outframe may be initialized with garbage.
        // Copy the 4th line from the bottom to the bottom 3 lines.
        for (i = 0; i < 3; i++)
            memcpy(&outframe[in->w * (in->h - 3 + i)], &outframe[in->w * (in->h - 4)], in->w * 4);
        break;
    }
    //copy alpha
    for (i=0;i<in->w*in->h;i++)
    {
        outframe[i]=(outframe[i]&0x00FFFFFF) | (inframe[i]&0xFF000000);
    }
}
