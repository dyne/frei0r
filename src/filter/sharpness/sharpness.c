/*
sharpness.c

This frei0r plugin is a port of Mplayer's unsharp mask filter
original by by Remi Guyomarch <rguyom@pobox.com>

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


//compile: gcc -c -fPIC -Wall sharpness.c -o sharpness.o
//link: gcc -shared -o sharpness.so sharpness.o

//#include <stdio.h>
#include <frei0r.h>
#include <stdlib.h>
//#include <math.h>
#include <assert.h>
#include <string.h>

#define MIN_MATRIX_SIZE 3
#define MAX_MATRIX_SIZE 63

typedef struct FilterParam {
    int msizeX, msizeY;
    double amount;
    uint32_t *SC[MAX_MATRIX_SIZE-1];
} FilterParam;


//----------------------------------------
//struktura za instanco efekta
typedef struct
{
int h;
int w;

FilterParam fp;
int size,ac;
unsigned char *Rplani,*Gplani,*Bplani,*Rplano,*Gplano,*Bplano;

} inst;


//========================================================
//unsharp() function from Mplayer unsharp filter
//by Remi Guyomarch <rguyom@pobox.com>

/* This code is based on :

An Efficient algorithm for Gaussian blur using finite-state machines
Frederick M. Waltz and John W. V. Miller

SPIE Conf. on Machine Vision Systems for Inspection and Metrology VII
Originally published Boston, Nov 98

*/

void unsharp( uint8_t *dst, uint8_t *src, int dstStride, int srcStride, int width, int height, FilterParam *fp ) {

    uint32_t **SC = fp->SC;
    uint32_t SR[MAX_MATRIX_SIZE-1], Tmp1, Tmp2;
    uint8_t* src2 = src; // avoid gcc warning

    int32_t res;
    int x, y, z;
    int amount = fp->amount * 65536.0;
    int stepsX = fp->msizeX/2;
    int stepsY = fp->msizeY/2;
    int scalebits = (stepsX+stepsY)*2;
    int32_t halfscale = 1 << ((stepsX+stepsY)*2-1);

    if( !fp->amount ) {
	if( src == dst )
	    return;
	if( dstStride == srcStride )
//	    fast_memcpy( dst, src, srcStride*height );
	    memcpy( dst, src, srcStride*height );
	else
	    for( y=0; y<height; y++, dst+=dstStride, src+=srcStride )
//		fast_memcpy( dst, src, width );
		memcpy( dst, src, width );
	return;
    }

    for( y=0; y<2*stepsY; y++ )
	memset( SC[y], 0, sizeof(SC[y][0]) * (width+2*stepsX) );

    for( y=-stepsY; y<height+stepsY; y++ ) {
	if( y < height ) src2 = src;
	memset( SR, 0, sizeof(SR[0]) * (2*stepsX-1) );
	for( x=-stepsX; x<width+stepsX; x++ ) {
	    Tmp1 = x<=0 ? src2[0] : x>=width ? src2[width-1] : src2[x];
	    for( z=0; z<stepsX*2; z+=2 ) {
		Tmp2 = SR[z+0] + Tmp1; SR[z+0] = Tmp1;
		Tmp1 = SR[z+1] + Tmp2; SR[z+1] = Tmp2;
	    }
	    for( z=0; z<stepsY*2; z+=2 ) {
		Tmp2 = SC[z+0][x+stepsX] + Tmp1; SC[z+0][x+stepsX] = Tmp1;
		Tmp1 = SC[z+1][x+stepsX] + Tmp2; SC[z+1][x+stepsX] = Tmp2;
	    }
	    if( x>=stepsX && y>=stepsY ) {
		uint8_t* srx = src - stepsY*srcStride + x - stepsX;
		uint8_t* dsx = dst - stepsY*dstStride + x - stepsX;

		res = (int32_t)*srx + ( ( ( (int32_t)*srx - (int32_t)((Tmp1+halfscale) >> scalebits) ) * amount ) >> 16 );
		*dsx = res>255 ? 255 : res<0 ? 0 : (uint8_t)res;
	    }
	}
	if( y >= 0 ) {
	    dst += dstStride;
	    src += srcStride;
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

info->name="Sharpness";
info->author="Marko Cebokli, Remi Guyomarch";
info->plugin_type=F0R_PLUGIN_TYPE_FILTER;
info->color_model=F0R_COLOR_MODEL_RGBA8888;
info->frei0r_version=FREI0R_MAJOR_VERSION;
info->major_version=0;
info->minor_version=1;
info->num_params=2;
info->explanation="Unsharp masking (port from Mplayer)";
}

//--------------------------------------------------
void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
switch(param_index)
	{
	case 0:
		info->name = "Amount";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "";
		break;
	case 1:
		info->name = "Size";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "";
		break;
	}
}

//----------------------------------------------
f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
inst *in;
int z;

in=calloc(1,sizeof(inst));
in->w=width;
in->h=height;

in->Rplani=calloc(width*height,sizeof(unsigned char));
in->Gplani=calloc(width*height,sizeof(unsigned char));
in->Bplani=calloc(width*height,sizeof(unsigned char));
in->Rplano=calloc(width*height,sizeof(unsigned char));
in->Gplano=calloc(width*height,sizeof(unsigned char));
in->Bplano=calloc(width*height,sizeof(unsigned char));

//defaults
in->fp.amount=0.0;
in->size=3;
in->fp.msizeX=3;
in->fp.msizeY=3;
in->ac=0;

memset(in->fp.SC,0,sizeof(in->fp.SC));
for( z=0; z<in->fp.msizeY; z++ )
    in->fp.SC[z] = calloc(in->w+in->fp.msizeX , sizeof(*(in->fp.SC[z])));

return (f0r_instance_t)in;
}

//---------------------------------------------------
void f0r_destruct(f0r_instance_t instance)
{
inst *in;
int i;

in=(inst*)instance;

free(in->Rplani);
free(in->Gplani);
free(in->Bplani);
free(in->Rplano);
free(in->Gplano);
free(in->Bplano);
for (i=0;i<in->fp.msizeY;i++) free(in->fp.SC[i]);

free(instance);
}

//-----------------------------------------------------
void f0r_set_param_value(f0r_instance_t instance, f0r_param_t parm, int param_index)
{
inst *p;
double tmpf;
int tmpi,chg,z;

p=(inst*)instance;

chg=0;
switch(param_index)
	{
	case 0:
                tmpf=map_value_forward(*((double*)parm), -1.5, 3.5); 
		if (tmpf!=p->fp.amount) chg=1;
		p->fp.amount=tmpf;
		break;
	case 1:
		tmpi=map_value_forward(*((double*)parm), 3.0, 11.0);
		if (p->size != tmpi) chg=1;
		p->size=tmpi;
		break;
	}

if (chg==0) return;


for( z=0; z<p->fp.msizeY; z++ )
    free(p->fp.SC[z]);

p->fp.msizeX=p->size;
p->fp.msizeY=p->size;

memset(p->fp.SC,0,sizeof(p->fp.SC));
for( z=0; z<p->fp.msizeY; z++ )
    p->fp.SC[z] = calloc(p->w+p->fp.msizeX , sizeof(*(p->fp.SC[z])));

}

//--------------------------------------------------
void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index)
{
inst *p;

p=(inst*)instance;

switch(param_index)
	{
	case 0:
                *((double*)param)=map_value_backward(p->fp.amount, -1.5, 3.5);
		break;
	case 1:
                *((double*)param)=map_value_backward(p->size, 3.0, 11.0);
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
for (i=0;i<(in->w*in->h);i++)	//copy to planar
	{
	in->Rplani[i]=inframe[i]&255;
	in->Gplani[i]=(inframe[i]>>8)&255;
	in->Bplani[i]=(inframe[i]>>16)&255;
	}

unsharp(in->Rplano, in->Rplani, in->w, in->w, in->w, in->h, &in->fp);
unsharp(in->Gplano, in->Gplani, in->w, in->w, in->w, in->h, &in->fp);
unsharp(in->Bplano, in->Bplani, in->w, in->w, in->w, in->h, &in->fp);

for (i=0;i<(in->w*in->h);i++)	//copy to packed, preserve alpha
	{
	outframe[i]=((uint32_t)in->Rplano[i])|((uint32_t)in->Gplano[i]<<8)|((uint32_t)in->Bplano[i]<<16)|(inframe[i]&0xFF000000);
	}


}

