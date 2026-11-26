/*
hqdn3d.c

This frei0r plugin is a port of Mplayer's hqdb3d denoiser
original by Daniel Moreno <comac@comac.darktech.org>

Version 0.1	nov 2010

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


//compile: gcc -c -fPIC -Wall hqdn3d.c -o hqdn3d.o
//link: gcc -shared -o hqdn3d.so hqdn3d.o

//#include <stdio.h>
#include <frei0r.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <inttypes.h>

#define MIN_MATRIX_SIZE 3
#define MAX_MATRIX_SIZE 63


//----------------------------------------
typedef struct {
        int Coefs[4][512*16];
        unsigned int *Line;      /* interleaved [3 * width] */
	unsigned short *Frame;   /* interleaved [3 * width * height] */
} vf_priv_s;

//----------------------------------------
//struktura za instanco efekta
typedef struct
{
int h;
int w;

double LumSpac,LumTmp;
vf_priv_s vps;
} inst;


//========================================================
//functions LowPassMul, deNoiseTemporal, deNoiseSpacial,
//deNoise and PrecalcCoefs are from Mplayer "hqdn3d" filter
//by Daniel Moreno <comac@comac.darktech.org>

static inline unsigned int LowPassMul(unsigned int PrevMul, unsigned int CurrMul, const int* Coef){
    int dMul = (int)(PrevMul - CurrMul);
    unsigned int d = ((unsigned int)(dMul + 0x10007FF) >> 12);
    return CurrMul + (unsigned int)Coef[d];
}

static void deNoiseTemporalRGB(
                    const uint32_t *inframe,
                    uint32_t *outframe,
                    unsigned short *FrameAnt,
                    int W, int H,
                    const int *Temporal)
{
    size_t n = (size_t)W * H;
    size_t i;
    for (i = 0; i < n; i++) {
        uint32_t px = inframe[i];
        unsigned short *fa = FrameAnt + i * 3;

        unsigned int dstR = LowPassMul((unsigned int)fa[0] << 8, (px & 0xFFu) << 16, Temporal);
        unsigned int dstG = LowPassMul((unsigned int)fa[1] << 8, ((px >> 8) & 0xFFu) << 16, Temporal);
        unsigned int dstB = LowPassMul((unsigned int)fa[2] << 8, ((px >> 16) & 0xFFu) << 16, Temporal);

        fa[0] = (unsigned short)((dstR + 0x1000007Fu) >> 8);
        fa[1] = (unsigned short)((dstG + 0x1000007Fu) >> 8);
        fa[2] = (unsigned short)((dstB + 0x1000007Fu) >> 8);

        outframe[i] = (((dstR + 0x10007FFFu) >> 16) & 0xFFu)
                    | ((((dstG + 0x10007FFFu) >> 16) & 0xFFu) << 8)
                    | ((((dstB + 0x10007FFFu) >> 16) & 0xFFu) << 16)
                    | (px & 0xFF000000u);
    }
}

static void deNoiseSpacialRGB(
                    const uint32_t *inframe,
                    uint32_t *outframe,
                    unsigned int *LineAnt,
                    int W, int H,
                    const int *Horizontal, const int *Vertical)
{
    int X, Y;
    unsigned int antR, antG, antB;

    /* First pixel has no left nor top neighbor. */
    {
        uint32_t px = inframe[0];
        LineAnt[0] = antR = (px & 0xFFu) << 16;
        LineAnt[1] = antG = ((px >> 8) & 0xFFu) << 16;
        LineAnt[2] = antB = ((px >> 16) & 0xFFu) << 16;
        outframe[0] = (((antR + 0x10007FFFu) >> 16) & 0xFFu)
                    | ((((antG + 0x10007FFFu) >> 16) & 0xFFu) << 8)
                    | ((((antB + 0x10007FFFu) >> 16) & 0xFFu) << 16)
                    | (px & 0xFF000000u);
    }

    /* First line has no top neighbor, only left. */
    for (X = 1; X < W; X++) {
        uint32_t px = inframe[X];
        unsigned int dstR = LineAnt[X * 3 + 0] = LowPassMul(antR, (px & 0xFFu) << 16, Horizontal);
        unsigned int dstG = LineAnt[X * 3 + 1] = LowPassMul(antG, ((px >> 8) & 0xFFu) << 16, Horizontal);
        unsigned int dstB = LineAnt[X * 3 + 2] = LowPassMul(antB, ((px >> 16) & 0xFFu) << 16, Horizontal);
        outframe[X] = (((dstR + 0x10007FFFu) >> 16) & 0xFFu)
                    | ((((dstG + 0x10007FFFu) >> 16) & 0xFFu) << 8)
                    | ((((dstB + 0x10007FFFu) >> 16) & 0xFFu) << 16)
                    | (px & 0xFF000000u);
    }

    for (Y = 1; Y < H; Y++) {
        const uint32_t *in_row = inframe + (size_t)Y * W;
        uint32_t *out_row = outframe + (size_t)Y * W;

        /* First pixel on each line doesn't have previous pixel */
        {
            uint32_t px = in_row[0];
            antR = (px & 0xFFu) << 16;
            antG = ((px >> 8) & 0xFFu) << 16;
            antB = ((px >> 16) & 0xFFu) << 16;
            unsigned int dstR = LineAnt[0] = LowPassMul(LineAnt[0], antR, Vertical);
            unsigned int dstG = LineAnt[1] = LowPassMul(LineAnt[1], antG, Vertical);
            unsigned int dstB = LineAnt[2] = LowPassMul(LineAnt[2], antB, Vertical);
            out_row[0] = (((dstR + 0x10007FFFu) >> 16) & 0xFFu)
                       | ((((dstG + 0x10007FFFu) >> 16) & 0xFFu) << 8)
                       | ((((dstB + 0x10007FFFu) >> 16) & 0xFFu) << 16)
                       | (px & 0xFF000000u);
        }

        for (X = 1; X < W; X++) {
            uint32_t px = in_row[X];
            unsigned int *la = LineAnt + X * 3;
            antR = LowPassMul(antR, (px & 0xFFu) << 16, Horizontal);
            antG = LowPassMul(antG, ((px >> 8) & 0xFFu) << 16, Horizontal);
            antB = LowPassMul(antB, ((px >> 16) & 0xFFu) << 16, Horizontal);

            unsigned int dstR = la[0] = LowPassMul(la[0], antR, Vertical);
            unsigned int dstG = la[1] = LowPassMul(la[1], antG, Vertical);
            unsigned int dstB = la[2] = LowPassMul(la[2], antB, Vertical);

            out_row[X] = (((dstR + 0x10007FFFu) >> 16) & 0xFFu)
                       | ((((dstG + 0x10007FFFu) >> 16) & 0xFFu) << 8)
                       | ((((dstB + 0x10007FFFu) >> 16) & 0xFFu) << 16)
                       | (px & 0xFF000000u);
        }
    }
}

static void deNoiseRGB(const uint32_t *inframe,
                       uint32_t *outframe,
                       unsigned int *LineAnt,
                       unsigned short **FrameAntPtr,
                       int W, int H,
                       const int *Horizontal, const int *Vertical, const int *Temporal)
{
    int X, Y;
    unsigned short *FrameAnt = *FrameAntPtr;

    if (!FrameAnt) {
        size_t n = (size_t)W * H;
        size_t i;
        *FrameAntPtr = FrameAnt = (unsigned short*)malloc(n * 3 * sizeof(unsigned short));
        for (i = 0; i < n; i++) {
            uint32_t px = inframe[i];
            FrameAnt[i * 3 + 0] = (unsigned short)((px & 0xFFu) << 8);
            FrameAnt[i * 3 + 1] = (unsigned short)(((px >> 8) & 0xFFu) << 8);
            FrameAnt[i * 3 + 2] = (unsigned short)(((px >> 16) & 0xFFu) << 8);
        }
    }

    if (!Horizontal[0] && !Vertical[0]) {
        deNoiseTemporalRGB(inframe, outframe, FrameAnt, W, H, Temporal);
        return;
    }
    if (!Temporal[0]) {
        deNoiseSpacialRGB(inframe, outframe, LineAnt, W, H, Horizontal, Vertical);
        return;
    }

    /* First pixel has no left nor top neighbor. Only previous frame */
    unsigned int antR, antG, antB;
    {
        uint32_t px = inframe[0];
        LineAnt[0] = antR = (px & 0xFFu) << 16;
        LineAnt[1] = antG = ((px >> 8) & 0xFFu) << 16;
        LineAnt[2] = antB = ((px >> 16) & 0xFFu) << 16;

        unsigned int dstR = LowPassMul((unsigned int)FrameAnt[0] << 8, antR, Temporal);
        unsigned int dstG = LowPassMul((unsigned int)FrameAnt[1] << 8, antG, Temporal);
        unsigned int dstB = LowPassMul((unsigned int)FrameAnt[2] << 8, antB, Temporal);

        FrameAnt[0] = (unsigned short)((dstR + 0x1000007Fu) >> 8);
        FrameAnt[1] = (unsigned short)((dstG + 0x1000007Fu) >> 8);
        FrameAnt[2] = (unsigned short)((dstB + 0x1000007Fu) >> 8);

        outframe[0] = (((dstR + 0x10007FFFu) >> 16) & 0xFFu)
                    | ((((dstG + 0x10007FFFu) >> 16) & 0xFFu) << 8)
                    | ((((dstB + 0x10007FFFu) >> 16) & 0xFFu) << 16)
                    | (px & 0xFF000000u);
    }

    /* First line has no top neighbor. Only left one for each pixel and last frame */
    for (X = 1; X < W; X++) {
        uint32_t px = inframe[X];
        unsigned int *la = LineAnt + X * 3;
        unsigned short *fa = FrameAnt + X * 3;

        la[0] = antR = LowPassMul(antR, (px & 0xFFu) << 16, Horizontal);
        la[1] = antG = LowPassMul(antG, ((px >> 8) & 0xFFu) << 16, Horizontal);
        la[2] = antB = LowPassMul(antB, ((px >> 16) & 0xFFu) << 16, Horizontal);

        unsigned int dstR = LowPassMul((unsigned int)fa[0] << 8, antR, Temporal);
        unsigned int dstG = LowPassMul((unsigned int)fa[1] << 8, antG, Temporal);
        unsigned int dstB = LowPassMul((unsigned int)fa[2] << 8, antB, Temporal);

        fa[0] = (unsigned short)((dstR + 0x1000007Fu) >> 8);
        fa[1] = (unsigned short)((dstG + 0x1000007Fu) >> 8);
        fa[2] = (unsigned short)((dstB + 0x1000007Fu) >> 8);

        outframe[X] = (((dstR + 0x10007FFFu) >> 16) & 0xFFu)
                    | ((((dstG + 0x10007FFFu) >> 16) & 0xFFu) << 8)
                    | ((((dstB + 0x10007FFFu) >> 16) & 0xFFu) << 16)
                    | (px & 0xFF000000u);
    }

    for (Y = 1; Y < H; Y++) {
        const uint32_t *in_row = inframe + (size_t)Y * W;
        uint32_t *out_row = outframe + (size_t)Y * W;
        unsigned short *LinePrev = FrameAnt + (size_t)Y * W * 3;

        /* First pixel on each line doesn't have previous pixel */
        {
            uint32_t px = in_row[0];
            antR = (px & 0xFFu) << 16;
            antG = ((px >> 8) & 0xFFu) << 16;
            antB = ((px >> 16) & 0xFFu) << 16;

            unsigned int vR = LineAnt[0] = LowPassMul(LineAnt[0], antR, Vertical);
            unsigned int vG = LineAnt[1] = LowPassMul(LineAnt[1], antG, Vertical);
            unsigned int vB = LineAnt[2] = LowPassMul(LineAnt[2], antB, Vertical);

            unsigned int dstR = LowPassMul((unsigned int)LinePrev[0] << 8, vR, Temporal);
            unsigned int dstG = LowPassMul((unsigned int)LinePrev[1] << 8, vG, Temporal);
            unsigned int dstB = LowPassMul((unsigned int)LinePrev[2] << 8, vB, Temporal);

            LinePrev[0] = (unsigned short)((dstR + 0x1000007Fu) >> 8);
            LinePrev[1] = (unsigned short)((dstG + 0x1000007Fu) >> 8);
            LinePrev[2] = (unsigned short)((dstB + 0x1000007Fu) >> 8);

            out_row[0] = (((dstR + 0x10007FFFu) >> 16) & 0xFFu)
                       | ((((dstG + 0x10007FFFu) >> 16) & 0xFFu) << 8)
                       | ((((dstB + 0x10007FFFu) >> 16) & 0xFFu) << 16)
                       | (px & 0xFF000000u);
        }

        for (X = 1; X < W; X++) {
            uint32_t px = in_row[X];
            unsigned int *la = LineAnt + X * 3;
            unsigned short *lp = LinePrev + X * 3;

            antR = LowPassMul(antR, (px & 0xFFu) << 16, Horizontal);
            antG = LowPassMul(antG, ((px >> 8) & 0xFFu) << 16, Horizontal);
            antB = LowPassMul(antB, ((px >> 16) & 0xFFu) << 16, Horizontal);

            unsigned int vR = la[0] = LowPassMul(la[0], antR, Vertical);
            unsigned int vG = la[1] = LowPassMul(la[1], antG, Vertical);
            unsigned int vB = la[2] = LowPassMul(la[2], antB, Vertical);

            unsigned int dstR = LowPassMul((unsigned int)lp[0] << 8, vR, Temporal);
            unsigned int dstG = LowPassMul((unsigned int)lp[1] << 8, vG, Temporal);
            unsigned int dstB = LowPassMul((unsigned int)lp[2] << 8, vB, Temporal);

            lp[0] = (unsigned short)((dstR + 0x1000007Fu) >> 8);
            lp[1] = (unsigned short)((dstG + 0x1000007Fu) >> 8);
            lp[2] = (unsigned short)((dstB + 0x1000007Fu) >> 8);

            out_row[X] = (((dstR + 0x10007FFFu) >> 16) & 0xFFu)
                       | ((((dstG + 0x10007FFFu) >> 16) & 0xFFu) << 8)
                       | ((((dstB + 0x10007FFFu) >> 16) & 0xFFu) << 16)
                       | (px & 0xFF000000u);
        }
    }
}

#define ABS(A) ( (A) > 0 ? (A) : -(A) )

static void PrecalcCoefs(int *Ct, double Dist25)
{
    int i;
    double Gamma, Simil, C;

    Gamma = log(0.25) / log(1.0 - Dist25/255.0 - 0.00001);

    for (i = -255*16; i <= 255*16; i++)
    {
        Simil = 1.0 - ABS(i) / (16*255.0);
        C = pow(Simil, Gamma) * 65536.0 * (double)i / 16.0;
        Ct[16*256+i] = (C<0) ? (C-0.5) : (C+0.5);
    }

    Ct[0] = (Dist25 != 0);
}

//end of hqdn3d functions
//===============================================



//-----------------------------------------------------
//stretch [0...1] to parameter range [min...max] linear
static float map_value_forward(double v, float min, float max)
{
return min+(max-min)*v;
}

//-----------------------------------------------------
//collapse from parameter range [min...max] to [0...1] linear
static double map_value_backward(float v, float min, float max)
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

info->name="hqdn3d";
info->author="Marko Cebokli, Daniel Moreno";
info->plugin_type=F0R_PLUGIN_TYPE_FILTER;
info->color_model=F0R_COLOR_MODEL_RGBA8888;
info->frei0r_version=FREI0R_MAJOR_VERSION;
info->major_version=0;
info->minor_version=1;
info->num_params=2;
info->explanation="High quality 3D denoiser from Mplayer";
}

//--------------------------------------------------
void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
switch(param_index)
	{
	case 0:
		info->name = "Spatial";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "Amount of spatial filtering";
		break;
	case 1:
		info->name = "Temporal";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "Amount of temporal filtering";
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

in->LumSpac=4;
in->LumTmp=6;
in->vps.Line=calloc((size_t)width * 3, sizeof(unsigned int));

PrecalcCoefs(in->vps.Coefs[0],in->LumSpac);
PrecalcCoefs(in->vps.Coefs[1],in->LumTmp);

return (f0r_instance_t)in;
}

//---------------------------------------------------
void f0r_destruct(f0r_instance_t instance)
{
inst *in;

in=(inst*)instance;

free(in->vps.Line);
free(in->vps.Frame);

free(instance);
}

//-----------------------------------------------------
void f0r_set_param_value(f0r_instance_t instance, f0r_param_t parm, int param_index)
{
inst *p;
double tmpf;
int chg;

p=(inst*)instance;

chg=0;
switch(param_index)
	{
	case 0:
		tmpf=map_value_forward(*((double*)parm), 0.0, 100.0); 
		if (tmpf!=p->LumSpac) chg=1;
		p->LumSpac=tmpf;
		break;
	case 1:
		tmpf=map_value_forward(*((double*)parm), 0.0, 100.0); 
		if (tmpf!=p->LumTmp) chg=1;
		p->LumTmp=tmpf;
		break;
	}

if (chg==0) return;

PrecalcCoefs(p->vps.Coefs[0],p->LumSpac);
PrecalcCoefs(p->vps.Coefs[1],p->LumTmp);

}

//--------------------------------------------------
void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index)
{
inst *p;

p=(inst*)instance;

switch(param_index)
	{
	case 0:
		*((double*)param)=map_value_backward(p->LumSpac, 0.0, 100.0);//BOOL!!
		break;
	case 1:
		*((double*)param)=map_value_backward(p->LumTmp, 0.0, 100.0);//BOOL!!
		break;
	}
}

//-------------------------------------------------
void f0r_update(f0r_instance_t instance, double time, const uint32_t* inframe, uint32_t* outframe)
{
inst *in;

assert(instance);
in=(inst*)instance;

deNoiseRGB(inframe, outframe, in->vps.Line, &in->vps.Frame, in->w, in->h,
           in->vps.Coefs[0], in->vps.Coefs[0], in->vps.Coefs[1]);
}
