/* cfc.h
 * uchar->float and float->uchar conversion for packed RGBA video
 * with flexible gamma correction
 *
 * Copyright (C) 2012 Marko Cebokli   http://lea.hamradio.si/~s57uuu
 * This file is a part of the Frei0r package
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

//	the float to uint8_t conversion is done
//	using Spitzak type tables (upper 16 bits
//	of a float value used as table index)
//	see  http://mysite.verizon.net/spitzak/conversion/

#include <math.h>
#include <inttypes.h>


typedef struct
	{
	float r;
	float g;
	float b;
	float a;
	} float_rgba;

//------------------------------------------------------
//the following gamma functions need not be speed optimized,
//as they are only used for table generation

//--------------------------------------------------------
//rec 709 gamma forward (linear to gamma)
//a = input, should be in the 0.0 to 1.0 range
static inline float gamma_709_f(float a)
{
return (a<=0.018) ? 4.5*a : 1.099*powf(a,0.45)-0.099;
}

//--------------------------------------------------------
//rec 709 gamma backward (gamma to linear)
//a = input, should be in the 0.0 to 1.0 range
static inline float gamma_709_b(float a)
{
return (a<=0.081) ? a/4.5 : powf((a+0.099)/1.099,1.0/0.45);
}

//----------------------------------------------------
//sRGB gamma forward (linear to gamma)
//a = input, should be in the 0.0 to 1.0 range
static inline float gamma_sRGB_f(float a)
{
return (a<=0.0031308) ? 12.92*a : 1.055*powf(a,1.0/2.4)-0.055;
}

//--------------------------------------------------------
//sRGB gamma backward (gamma to linear)
//a = input, should be in the 0.0 to 1.0 range
static inline float gamma_sRGB_b(float a)
{
return (a<=0.04045) ? a/12.92 : powf((a+0.055)/1.055,2.4);
}

//----------------------------------------------------
//plain gamma (power function) forward (linear to gamma)
//a = input, should be in the 0.0 to 1.0 range
static inline float gamma_plain_f(float a, float g)
{
return powf(a,1.0/g);
}

//--------------------------------------------------------
//plain gamma (power function) backward (gamma to linear)
//a = input, should be in the 0.0 to 1.0 range
static inline float gamma_plain_b(float a, float g)
{
return powf(a,g);
}

//--------------------------------------------------------
//dummy function for linear tables (no gamma)
//g = gamma value
static inline float gamma_none(float a)
{
return a;
}

//------------------------------------------------
//expand highlights using a modified Spitzak formula
//with limited max output value (250)
//(for values up to 2500 use 1.0001 and 0.493)
//input [0...1]
//output [0...250]
static inline float exp_hl(float a)
{
return (a<=0.4781) ? a : 0.25/(1.001-a);
}

//------------------------------------------------------------
//compress highlights using a modified Spitzak formula
//with limited max input value (250)
//(for values up to 2500 use 1.0001 and 0.493)
//input [0...250]
//output [0...1]
static inline float cps_hl(float a)
{
return (a<=0.4781) ? a : 1.001-0.25/a;
}

//----------------------------------------------------------
//float to char conversion is done using the upper 16 bits
//of the float number as an index into the conversion table.
//This union is used for type punning, to avoid problems
//with compiler optimizations, as the read access in the
//cfc_tab_8 function directly follows writing
typedef union
	{
	float a;
	uint16_t i[2];
	} flint;

//--------------------------------------------------------
//generate forward and backward conversion tables
//for 8 bit (uint8_t) video
//*ft = forward table (float to uchar, linear to gamma)
//	must have space for 65536 char elements
//*bt = backward table (uchar to float, gamma to linear)
//	must have space for 256 float elements
//type = what kind of gamma function will be used:
//	0 = linear (no gamma, just multiply/divide by 255)
//	1 = plain gamma (pure power function)
//	2 = rec 709 gamma
//	3 = sRGB gamma
//	4 = sRGB gamma with highlight expansion/compression
//	types 0...3 map to [0...1] linear float range
//	type 4 maps to [0...250] linear float range
//g = gamma value, 0.3 to 3.0 (only used with type=1)
static inline void cfc_tab_8(uint8_t *ft, float *bt, int type, float g)
{
uint16_t i;
float a;
flint fi;

// *** generate float to char conversion table ***
for (i=0;i<128;i++)	//positive denormals map to zero
	ft[i]=0;

for (i=128;i<=32639;i++)  //positive numbers map to 0...255
	{
//#if FREI0R_BYTE_ORDER == FREI0R_BIG_ENDIAN
//	fi.i[0]=i; fi.i[1]=0x8000;		//big endian
//#endif
//#if FREI0R_BYTE_ORDER == FREI0R_LITTLE_ENDIAN
	fi.i[1]=i; fi.i[0]=0x8000;		//little endian
//#endif
	a=fi.a;
	switch (type)
	    {
	    case 0: a=gamma_none(a); break;
	    case 1: a=gamma_plain_f(a,g); break;
	    case 2: a=gamma_709_f(a); break;
	    case 3: a=gamma_sRGB_f(a); break;
	    case 4: a=cps_hl(a); a=gamma_sRGB_f(a); break;
	    default: break;
	    }
	if (a>0.9999) a=0.9999;
	ft[i]=(uint8_t)(256.0*a);
	}

for (i=32640;i<32768;i++) //positive NANs and infinite map to 255
	ft[i]=255;
for (i=32768;i!=65535;i++) //everything negative maps to 0
	ft[i]=0;
ft[65535]=0;


// *** generate char to float conversion table ***
for (i=0;i<=255;i++)
	{
	a=((float)i+0.5)/256.0;
	switch (type)
	    {
	    case 0: a=gamma_none(a); break;
	    case 1: a=gamma_plain_b(a,g); break;
	    case 2: a=gamma_709_b(a); break;
	    case 3: a=gamma_sRGB_b(a); break;
	    case 4: a=gamma_sRGB_b(a); a=exp_hl(a); break;
	    default: break;
	    }
	bt[i]=a;
	}
}

//--------------------------------------------------------
//convert from paked uchar RGBA to packed float RGBA
//w,h are width and height of the image
//tab = table used for RGB conversion
//atab = table used for alpha converion (usually linear)
static inline void RGBA8_2_float(const uint32_t *in, float_rgba *out, int w, int h, float *tab, float *atab)
{
int i;
uint8_t *cin=(uint8_t *)in;

for (i=0;i<w*h;i++)
	{
	out[i].r=tab[*cin++];
	out[i].g=tab[*cin++];
	out[i].b=tab[*cin++];
	out[i].a=atab[*cin++];
	}
}

//--------------------------------------------------------
//convert from paked uchar RGBA to packed float RGBA,
//covert RGB only, SKIP ALPHA
//w,h are width and height of the image
//tab = table used for RGB conversion
static inline void RGB8_2_float(const uint32_t *in, float_rgba *out, int w, int h, float *tab)
{
int i;
uint8_t *cin=(uint8_t *)in;

for (i=0;i<w*h;i++)
	{
	out[i].r=tab[*cin++];
	out[i].g=tab[*cin++];
	out[i].b=tab[*cin++];
	cin++;
	}
}


//--------------------------------------------------------
//in the following conversions, we can afford "direct" type punning
//as the array already exists from well before (I hope :-)
//
//#if FREI0R_BYTE_ORDER == FREI0R_BIG_ENDIAN
//big endian most significant 16 bit word from 32bit float
//#define MSWF(x) *(uint16_t*)x
//#endif
//
//#if FREI0R_BYTE_ORDER == FREI0R_LITTLE_ENDIAN
//little endian most significant 16 bit word from 32bit float
#define MSWF(x) *((uint16_t*)x+1)
//#endif

//----------------------------------------------------------
//convert from packed float RGBA to packed uchar RGBA
//tab = table used for RGB conversion
//atab = table used for alpha converion (usually linear)
static inline void float_2_RGBA8(const float_rgba *in, uint32_t *out, int w, int h, uint8_t *tab, uint8_t *atab)
{
int i;
uint8_t *cout=(uint8_t *)out;

for (i=0;i<w*h;i++)
	{
	*cout++ = tab[MSWF(&in[i].r)];
	*cout++ = tab[MSWF(&in[i].g)];
	*cout++ = tab[MSWF(&in[i].b)];
	*cout++ = atab[MSWF(&in[i].a)];
	}
}

//----------------------------------------------------------
//convert from packed float RGBA to packed uchar RGBA
//covert RGB only, SKIP ALPHA
//tab = table used for RGB conversion
static inline void float_2_RGB8(const float_rgba *in, uint32_t *out, int w, int h, uint8_t *tab)
{
int i;
uint8_t *cout=(uint8_t *)out;

for (i=0;i<w*h;i++)
	{
	*cout++ = tab[MSWF(&in[i].r)];
	*cout++ = tab[MSWF(&in[i].g)];
	*cout++ = tab[MSWF(&in[i].b)];
	cout++;
	}
}

#undef MSWF


//--------------------------------------------------
//  -- Single value conversion --
//there is no function for single uint8 to float, because
//that is just a simple table lookup
//
//because float_2_uint8() can be called immediately after
//the input value has been created, type punning has to be
//done properly, with a union

//#if FREI0R_BYTE_ORDER == FREI0R_LITTLE_ENDIAN
static inline uint8_t float_2_uint8(const float *in, uint8_t *tab)
{
return tab[((flint*)in)->i[1]];
}
//endif

//#if FREI0R_BYTE_ORDER == FREI0R_BIG_ENDIAN
//static inline float_2_uint8(const float *in, uint8_t *tab)
//{
//return tab[((flint*)in)->i[0]];
//}
//#endif
