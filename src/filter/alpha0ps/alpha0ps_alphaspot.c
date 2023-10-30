/*
alphaspot.c

This frei0r plugin draws simple shapes into the alpha channel
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


//compile: gcc -c -fPIC -Wall alphaspot.c -o alphaspot.o
//link: gcc -shared -o alphaspot.so alphaspot.o

//#include <stdio.h>
#include <frei0r.h>
#include <frei0r_math.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <string.h>


//----------------------------------------
//struktura za instanco efekta
typedef struct
{
    int h;
    int w;

    float pozx,pozy,sizx,sizy,wdt,tilt,min,max;
    int shp,op;

    uint8_t *gr8;

} inst;


//----------------------------------------------------------
//general (rotated) rectangle with soft border
void gen_rec_s(uint8_t* sl, int w, int h, float siz1, float siz2, float tilt, float pozx, float pozy, float min, float max, float wb)
{
    int i,j;
    float d1,d2,d,db,st,ct,g,is1,is2;

    if ((siz1 == 0.0f) || (siz2 == 0.0f)) return;
    st = sinf(tilt);
    ct = cosf(tilt);
    is1 = 1.0f/siz1;
    is2 = 1.0f/siz2;

    for (i = 0; i < h; i++) {
        for (j = 0; j < w; j++) {
            d1 = (i-pozy)*st+(j-pozx)*ct;
            d2 = (i-pozy)*ct-(j-pozx)*st;
            d1 = fabsf(d1)*is1;
            d2 = fabsf(d2)*is2;
            d = (d1<d2)?d2:d1;
            d1 = 1.0f - (1.0f - d1)*is2/is1;
            db = (d1<d2)?d2:d1;
            if (fabsf(d) > 1.0f) {
                g = min;
            } else {
                if (db <= 1.004f - wb) {
                    g = max;
                } else {
                    g = min + (1.0f - wb-db)/wb*(max-min);
                }
            }
            sl[i*w+j] = lrintf(g * 255.0f);
        }
    }
}

//----------------------------------------------------------
//general (rotated) ellipse with soft border
void gen_eli_s(uint8_t* sl, int w, int h, float siz1, float siz2, float tilt, float pozx, float pozy, float min, float max, float wb)
{
    int i,j;
    float d1,d2,d,db,st,ct,is1,is2,g;

    if ((siz1 == 0.0f) || (siz2 == 0.0f)) return;
    st = sinf(tilt);
    ct = cosf(tilt);
    is1 = 1.0f/siz1;
    is2 = 1.0f/siz2;

    for (i = 0; i < h; i++) {
        for (j = 0; j < w; j++) {
            d1 = (i-pozy)*st+(j-pozx)*ct;
            d2 = (i-pozy)*ct-(j-pozx)*st;
            d = hypotf(d1*is1,d2*is2);

            db = d;	//neenakomeren rob!!!

            if (d > 1.0f) {
                g = min;
            } else {
                if (db <= 1.004f - wb) {
                    g = max;
                } else {
                    g = min + (1.0f - wb-db)/wb*(max-min);
                }
            }
            sl[i*w+j] = lrintf(g * 255.0f);
        }
    }
}

//----------------------------------------------------------
//general (rotated) triangle with soft border
void gen_tri_s(uint8_t* sl, int w, int h, float siz1, float siz2, float tilt, float pozx, float pozy, float min, float max, float wb)
{
    int i,j;
    float d1,d2,d3,d4,d,st,ct,is1,is2,k5,lim,db,g;

    if ((siz1 == 0.0f) || (siz2 == 0.0f)) return;
    st =sinf(tilt);
    ct = cosf(tilt);
    is1 = 1.0f/siz1;
    is2 = 1.0f/siz2;
    k5 = 1.0f/sqrtf(5.0f);
    lim = 0.82f;

    for (i = 0; i < h; i++) {
        for (j = 0; j < w; j++) {
            d1 = (i-pozy)*st+(j-pozx)*ct;
            d2 = (i-pozy)*ct-(j-pozx)*st;
            d1 = d1*is1;
            d2 = d2*is2;
            d3 = (2.0f * d1+d2 + 1.0f)*k5;
            d4 = (2.0f* d1-d2 - 1.0f)*k5;
            d2 = fabsf(d2);
            d3 = fabsf(d3);
            d4 = fabsf(d4);
            d = d2;
            if (d3 > d) d=d3;
            if (d4 > d) d=d4;
            db = d;

            if (fabsf(d) > lim) {
                g = min;
            } else {
                if (db <= 1.004f * lim-wb) {
                    g = max;
                } else {
                    g = min + (lim-wb-db)/wb*(max-min);
                }
            }
            sl[i*w+j] = lrintf(g * 255.0f);
        }
    }
}

//----------------------------------------------------------
//general (rotated) diamond shape with soft border
void gen_dia_s(uint8_t* sl, int w, int h, float siz1, float siz2, float tilt, float pozx, float pozy, float min, float max, float wb)
{
    int i,j;
    float d1,d2,d,db,st,ct,is1,is2,g;

    if ((siz1 == 0.0f) || (siz2 == 0.0f)) return;
    st = sinf(tilt);
    ct = cosf(tilt);
    is1 = 1.0f/siz1;
    is2 = 1.0f/siz2;

    for (i = 0; i < h; i++) {
        for (j = 0; j < w; j++) {
            d1 = (i-pozy)*st+(j-pozx)*ct;
            d2 = (i-pozy)*ct-(j-pozx)*st;
            d = fabsf(d1*is1)+fabsf(d2*is2);
            db = d;
            if (fabsf(d) > 1.0f) {
                g = min;
            } else {
                if (db <= 1.004f - wb) {
                    g = max;
                } else {
                    g = min + (1.0f - wb-db)/wb*(max-min);
                }
            }
            sl[i*w+j] = lrintf(g * 255.0f);
        }
    }
}

//-----------------------------------------------------
void draw(inst *in)
{
    switch (in->shp)
    {
    case 0:
        gen_rec_s(in->gr8, in->w, in->h, in->sizx*in->w, in->sizy*in->h, in->tilt, in->pozx*in->w, in->pozy*in->h, in->min, in->max, in->wdt);
        break;
    case 1:
        gen_eli_s(in->gr8, in->w, in->h, in->sizx*in->w, in->sizy*in->h, in->tilt, in->pozx*in->w, in->pozy*in->h, in->min, in->max, in->wdt);
        break;
    case 2:
        gen_tri_s(in->gr8, in->w, in->h, in->sizx*in->w, in->sizy*in->h, in->tilt, in->pozx*in->w, in->pozy*in->h, in->min, in->max, in->wdt);
        break;
    case 3:
        gen_dia_s(in->gr8, in->w, in->h, in->sizx*in->w, in->sizy*in->h, in->tilt, in->pozx*in->w, in->pozy*in->h, in->min, in->max, in->wdt);
        break;
    default:
        break;
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
// MANDATORY FREI0R FUNCTIONS

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

    info->name = "alphaspot";
    info->author = "Marko Cebokli";
    info->plugin_type = F0R_PLUGIN_TYPE_FILTER;
    info->color_model = F0R_COLOR_MODEL_RGBA8888;
    info->frei0r_version = FREI0R_MAJOR_VERSION;
    info->major_version = 0;
    info->minor_version = 2;
    info->num_params = 10;
    info->explanation = "Draws simple shapes into the alpha channel";
}

//--------------------------------------------------
void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
    switch(param_index) {
    case 0:
        info->name = "Shape";
        info->type = F0R_PARAM_DOUBLE;
        info->explanation = "";
        break;
    case 1:
        info->name = "Position X";
        info->type = F0R_PARAM_DOUBLE;
        info->explanation = "";
        break;
    case 2:
        info->name = "Position Y";
        info->type = F0R_PARAM_DOUBLE;
        info->explanation = "";
        break;
    case 3:
        info->name = "Size X";
        info->type = F0R_PARAM_DOUBLE;
        info->explanation = "";
        break;
    case 4:
        info->name = "Size Y";
        info->type = F0R_PARAM_DOUBLE;
        info->explanation = "";
        break;
    case 5:
        info->name = "Tilt";
        info->type = F0R_PARAM_DOUBLE;
        info->explanation = "";
        break;
    case 6:
        info->name = "Transition width";
        info->type = F0R_PARAM_DOUBLE;
        info->explanation = "";
        break;
    case 7:
        info->name = "Min";
        info->type = F0R_PARAM_DOUBLE;
        info->explanation = "";
        break;
    case 8:
        info->name = "Max";
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

//----------------------------------------------
f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
    inst *in;

    in = calloc(1, sizeof(inst));
    in->w = width;
    in->h = height;

    in->shp = 0;
    in->pozx = 0.5f;
    in->pozy = 0.5f;
    in->sizx = 0.1f;
    in->sizy = 0.1f;
    in->wdt = 0.2f;
    in->tilt = 0.0f;
    in->min = 0.0f;
    in->max = 1.0f;
    in->op = 0;

    in->gr8 = calloc(in->w*in->h, sizeof(*in->gr8));

    draw(in);

    return (f0r_instance_t)in;
}

//---------------------------------------------------
void f0r_destruct(f0r_instance_t instance)
{
    inst *in;

    in = (inst*) instance;

    free(in->gr8);
    free(instance);
}

//-----------------------------------------------------
void f0r_set_param_value(f0r_instance_t instance, f0r_param_t parm, int param_index)
{
    inst *p;
    double tmpf;
    int tmpi,chg;

    p = (inst*) instance;

    chg = 0;
    switch (param_index) {
    case 0:
        tmpi = map_value_forward(*((double*)parm), 0.0f, 3.9999f);
        if (p->shp != tmpi) chg=1;
        p->shp = tmpi;
        break;
    case 1:
        tmpf = *(double*)parm;
        if (tmpf != p->pozx) chg = 1;
        p->pozx = tmpf;
        break;
    case 2:
        tmpf = *(double*)parm;
        if (tmpf != p->pozy) chg = 1;
        p->pozy = tmpf;
        break;
    case 3:
        tmpf = *(double*)parm;
        if (tmpf != p->sizx) chg = 1;
        p->sizx = tmpf;
        break;
    case 4:
        tmpf = *(double*)parm;
        if (tmpf != p->sizy) chg = 1;
        p->sizy = tmpf;
        break;
    case 5:
        tmpf = map_value_forward(*((double*)parm), -3.15f, 3.15f);
        if (tmpf != p->tilt) chg = 1;
        p->tilt = tmpf;
        break;
    case 6:
        tmpf = *(double*)parm;
        if (tmpf != p->wdt) chg = 1;
        p->wdt = tmpf;
        break;
    case 7:
        tmpf = *(double*)parm;
        if (tmpf != p->min) chg = 1;
        p->min = tmpf;
        break;
    case 8:
        tmpf = *(double*)parm;
        if (tmpf != p->max) chg = 1;
        p->max = tmpf;
        break;
    case 9:
        tmpi = map_value_forward(*((double*)parm), 0.0f, 4.9999f);
        if (p->op != tmpi) chg = 1;
        p->op = tmpi;
        break;
    }

    if (chg == 0) return;

    draw(p);

}

//--------------------------------------------------
void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index)
{
    inst *p;

    p=(inst*)instance;

    switch(param_index) {
    case 0:
        *((double*)param) = map_value_backward(p->shp, 0.0f, 3.9999f);
        break;
    case 1:
        *((double*)param) = p->pozx;
        break;
    case 2:
        *((double*)param) = p->pozy;
        break;
    case 3:
        *((double*)param) = p->sizx;
        break;
    case 4:
        *((double*)param) = p->sizy;
        break;
    case 5:
        *((double*)param) = map_value_backward(p->tilt, -3.15f, 3.15f);
        break;
    case 6:
        *((double*)param) = p->wdt;
        break;
    case 7:
        *((double*)param) = p->min;
        break;
    case 8:
        *((double*)param) = p->max;
        break;
    case 9:
        *((double*)param) = map_value_backward(p->op, 0.0f, 4.9999f);
        break;
    }
}

//-------------------------------------------------
void f0r_update(f0r_instance_t instance, double time, const uint32_t* inframe, uint32_t* outframe)
{
    inst *in;
    int i;
    uint8_t *inp = (uint8_t*) inframe;
    uint8_t *outp = (uint8_t*) outframe;

    assert(instance);
    in = (inst*)instance;
    memcpy(outframe, inframe, sizeof(*inframe) * in->w * in->h);

    switch (in->op) {
    case 0:		//write on clear
        for (i = 0; i < in->h*in->w; i++) {
            outp[4*i+3] = in->gr8[i];
        }
        break;
    case 1:		//max
        for (i = 0; i < in->h*in->w; i++) {
            outp[4*i+3] = (inp[4*i+3] > in->gr8[i]) ? inp[4*i+3] : in->gr8[i];
        }
        break;
    case 2:		//min
        for (i = 0; i < in->h*in->w; i++) {
            outp[4*i+3] = (inp[4*i+3] < in->gr8[i]) ? inp[4*i+3] : in->gr8[i];
        }
        break;
    case 3:		//add
        for (i = 0;i < in->h*in->w; i++) {
            outp[4*i+3] = MAX255(inp[4*i+3] + in->gr8[i]);
        }
        break;
    case 4:		//subtract
        for (i = 0; i < in->h*in->w; i++) {
            outp[4*i+3] = (inp[4*i+3] > in->gr8[i]) ? inp[4*i+3] - in->gr8[i] : 0;
        }
        break;
    default:
        break;
    }
}
