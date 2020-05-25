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
        unsigned int *Line;
	unsigned short *Frame[3];
}vf_priv_s;

//----------------------------------------
//struktura za instanco efekta
typedef struct
{
int h;
int w;

double LumSpac,LumTmp;
vf_priv_s vps;

unsigned char *Rplani,*Gplani,*Bplani,*Rplano,*Gplano,*Bplano;
} inst;


//========================================================
//functions LowPassMul, deNoiseTemporal, deNoiseSpacial,
//deNoise and PrecalaCoefs  are from Mplayer "hqdn3d" filter
//by Daniel Moreno <comac@comac.darktech.org>

static inline unsigned int LowPassMul(unsigned int PrevMul, unsigned int CurrMul, int* Coef){
//    int dMul= (PrevMul&0xFFFFFF)-(CurrMul&0xFFFFFF);
    int dMul= PrevMul-CurrMul;
    unsigned int d=((dMul+0x10007FF)>>12);
    return CurrMul + Coef[d];
}

void deNoiseTemporal(
                    unsigned char *Frame,        // mpi->planes[x]
                    unsigned char *FrameDest,    // dmpi->planes[x]
                    unsigned short *FrameAnt,
                    int W, int H, int sStride, int dStride,
                    int *Temporal)
{
    long X, Y;
    unsigned int PixelDst;

    for (Y = 0; Y < H; Y++){
        for (X = 0; X < W; X++){
            PixelDst = LowPassMul(FrameAnt[X]<<8, Frame[X]<<16, Temporal);
            FrameAnt[X] = ((PixelDst+0x1000007F)>>8);
            FrameDest[X]= ((PixelDst+0x10007FFF)>>16);
        }
        Frame += sStride;
        FrameDest += dStride;
        FrameAnt += W;
    }
}

void deNoiseSpacial(
                    unsigned char *Frame,        // mpi->planes[x]
                    unsigned char *FrameDest,    // dmpi->planes[x]
                    unsigned int *LineAnt,       // vf->priv->Line (width bytes)
                    int W, int H, int sStride, int dStride,
                    int *Horizontal, int *Vertical)
{
    long X, Y;
    long sLineOffs = 0, dLineOffs = 0;
    unsigned int PixelAnt;
    unsigned int PixelDst;

    /* First pixel has no left nor top neighbor. */
    PixelDst = LineAnt[0] = PixelAnt = Frame[0]<<16;
    FrameDest[0]= ((PixelDst+0x10007FFF)>>16);

    /* First line has no top neighbor, only left. */
    for (X = 1; X < W; X++){
        PixelDst = LineAnt[X] = LowPassMul(PixelAnt, Frame[X]<<16, Horizontal);
        FrameDest[X]= ((PixelDst+0x10007FFF)>>16);
    }

    for (Y = 1; Y < H; Y++){
	unsigned int PixelAnt;
	sLineOffs += sStride, dLineOffs += dStride;
        /* First pixel on each line doesn't have previous pixel */
        PixelAnt = Frame[sLineOffs]<<16;
        PixelDst = LineAnt[0] = LowPassMul(LineAnt[0], PixelAnt, Vertical);
        FrameDest[dLineOffs]= ((PixelDst+0x10007FFF)>>16);

        for (X = 1; X < W; X++){
            unsigned int PixelDst;
            /* The rest are normal */
            PixelAnt = LowPassMul(PixelAnt, Frame[sLineOffs+X]<<16, Horizontal);
            PixelDst = LineAnt[X] = LowPassMul(LineAnt[X], PixelAnt, Vertical);
            FrameDest[dLineOffs+X]= ((PixelDst+0x10007FFF)>>16);
        }
    }
}

void deNoise(unsigned char *Frame,        // mpi->planes[x]
                    unsigned char *FrameDest,    // dmpi->planes[x]
                    unsigned int *LineAnt,      // vf->priv->Line (width bytes)
		    unsigned short **FrameAntPtr,
                    int W, int H, int sStride, int dStride,
                    int *Horizontal, int *Vertical, int *Temporal)
{
    long X, Y;
    long sLineOffs = 0, dLineOffs = 0;
    unsigned int PixelAnt;
    unsigned int PixelDst;
    unsigned short* FrameAnt=(*FrameAntPtr);

    if(!FrameAnt){
	(*FrameAntPtr)=FrameAnt=malloc(W*H*sizeof(unsigned short));
	for (Y = 0; Y < H; Y++){
	    unsigned short* dst=&FrameAnt[Y*W];
	    unsigned char* src=Frame+Y*sStride;
	    for (X = 0; X < W; X++) dst[X]=src[X]<<8;
	}
    }

    if(!Horizontal[0] && !Vertical[0]){
        deNoiseTemporal(Frame, FrameDest, FrameAnt,
                        W, H, sStride, dStride, Temporal);
        return;
    }
    if(!Temporal[0]){
        deNoiseSpacial(Frame, FrameDest, LineAnt,
                       W, H, sStride, dStride, Horizontal, Vertical);
        return;
    }

    /* First pixel has no left nor top neighbor. Only previous frame */
    LineAnt[0] = PixelAnt = Frame[0]<<16;
    PixelDst = LowPassMul(FrameAnt[0]<<8, PixelAnt, Temporal);
    FrameAnt[0] = ((PixelDst+0x1000007F)>>8);
    FrameDest[0]= ((PixelDst+0x10007FFF)>>16);

    /* First line has no top neighbor. Only left one for each pixel and
     * last frame */
    for (X = 1; X < W; X++){
        LineAnt[X] = PixelAnt = LowPassMul(PixelAnt, Frame[X]<<16, Horizontal);
        PixelDst = LowPassMul(FrameAnt[X]<<8, PixelAnt, Temporal);
        FrameAnt[X] = ((PixelDst+0x1000007F)>>8);
        FrameDest[X]= ((PixelDst+0x10007FFF)>>16);
    }

    for (Y = 1; Y < H; Y++){
	unsigned int PixelAnt;
	unsigned short* LinePrev=&FrameAnt[Y*W];
	sLineOffs += sStride, dLineOffs += dStride;
        /* First pixel on each line doesn't have previous pixel */
        PixelAnt = Frame[sLineOffs]<<16;
        LineAnt[0] = LowPassMul(LineAnt[0], PixelAnt, Vertical);
	PixelDst = LowPassMul(LinePrev[0]<<8, LineAnt[0], Temporal);
        LinePrev[0] = ((PixelDst+0x1000007F)>>8);
        FrameDest[dLineOffs]= ((PixelDst+0x10007FFF)>>16);

        for (X = 1; X < W; X++){
            unsigned int PixelDst;
            /* The rest are normal */
            PixelAnt = LowPassMul(PixelAnt, Frame[sLineOffs+X]<<16, Horizontal);
            LineAnt[X] = LowPassMul(LineAnt[X], PixelAnt, Vertical);
	    PixelDst = LowPassMul(LinePrev[X]<<8, LineAnt[X], Temporal);
            LinePrev[X] = ((PixelDst+0x1000007F)>>8);
            FrameDest[dLineOffs+X]= ((PixelDst+0x10007FFF)>>16);
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
in->vps.Line=calloc(width,sizeof(int));
in->Rplani=calloc(width*height,sizeof(unsigned char));
in->Gplani=calloc(width*height,sizeof(unsigned char));
in->Bplani=calloc(width*height,sizeof(unsigned char));
in->Rplano=calloc(width*height,sizeof(unsigned char));
in->Gplano=calloc(width*height,sizeof(unsigned char));
in->Bplano=calloc(width*height,sizeof(unsigned char));

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
free(in->Rplani);
free(in->Gplani);
free(in->Bplani);
free(in->Rplano);
free(in->Gplano);
free(in->Bplano);

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
int i;

assert(instance);
in=(inst*)instance;

//Frei0r works with packed color, Mplayer with planar color
//I decided to copy data rather than modify the hqdn3d functions
//this takes some time, but future inmprovements in hqdn3d
//functions can be simply copied here without modification

for (i=0;i<(in->w*in->h);i++)	//copy to planar
	{
	in->Rplani[i]=inframe[i]&255;
	in->Gplani[i]=(inframe[i]>>8)&255;
	in->Bplani[i]=(inframe[i]>>16)&255;
	}

deNoise(in->Rplani, in->Rplano, in->vps.Line, &in->vps.Frame[0], in->w, in->h, in->w, in->w, in->vps.Coefs[0], in->vps.Coefs[0], in->vps.Coefs[1]);
deNoise(in->Gplani, in->Gplano, in->vps.Line, &in->vps.Frame[1], in->w, in->h, in->w, in->w, in->vps.Coefs[0], in->vps.Coefs[0], in->vps.Coefs[1]);
deNoise(in->Bplani, in->Bplano, in->vps.Line, &in->vps.Frame[2], in->w, in->h, in->w, in->w, in->vps.Coefs[0], in->vps.Coefs[0], in->vps.Coefs[1]);

for (i=0;i<(in->w*in->h);i++)	//copy to packed, preserve alpha
	{
	outframe[i]=((uint32_t)in->Rplano[i])|((uint32_t)in->Gplano[i]<<8)|((uint32_t)in->Bplano[i]<<16)|(inframe[i]&0xFF000000);
	}


}

