/*
medians.c  implements several median-type filters

This frei0r plugin implements several median-type filters

Version 0.1	jan 2013

Copyright (C) 2013  Marko Cebokli    http://lea.hamradio.si/~s57uuu


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


//compile: gcc -c -fPIC -Wall medians.c -o medians.o
//link: gcc -shared -o medians.so medians.o

#include <stdio.h>
#include <frei0r.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>

#include "small_medians.h"
#include "ctmf.h"


/* ******************************************
//The following functions implement these median type filters:

               X 
cross5:       XXX
               X

                XXX 
square3x3:	XXX
                XXX
                

Bilevel:

                 X 
                XXX
dia3x3:        XXXXX
                XXX
                 X
                 
Arce BI:	multilevel spatio-temporal, see [1]

Arp ML3D:	multilevel spatio-temporal, see [1]

ML3DEX:		multilevel spatio-temporal, see [1]


[1] Anil Christopher Kokaram: Motion Picure Restoration
  phd disertation

****************************************** */

//------------------------------------------------------------
//cross5	packed char RGB image (uint32_t)
//vs = input image
//is = output image
void cross5(const uint32_t *vs, int w, int h, uint32_t *is)
{
int i,j,p;
uint32_t m[8];

for (i=1;i<h-1;i++)
    for (j=1;j<w-1;j++)
	{
	p=i*w+j;

	m[0]=vs[p-w]; m[1]=vs[p-1]; m[2]=vs[p];
	m[3]=vs[p+1]; m[4]=vs[p+w];

	is[p]=median5(m);
	}
}

//------------------------------------------------------------
//square 3x3		packed char RGB image (uint32_t)
//vs = input image
//is = output image
void sq3x3(const uint32_t *vs, int w, int h, uint32_t *is)
{
int i,j,p;
uint32_t m[16];

for (i=1;i<h-1;i++)
    for (j=1;j<w-1;j++)
	{
	p=i*w+j;

	m[0]=vs[p-w-1]; m[1]=vs[p-w]; m[2]=vs[p-w+1];
	m[3]=vs[p-1];   m[4]=vs[p];   m[5]=vs[p+1];
	m[6]=vs[p+w-1]; m[7]=vs[p+w]; m[8]=vs[p+w+1];

	is[p]=median9(m);
	}
}

//------------------------------------------------------------
//bilevel		packed char RGB image (uint32_t)
//vs = input image
//is = output image
void bilevel(const uint32_t *vs, int w, int h, uint32_t *is)
{
int i,j,p;
uint32_t m[8],mm[4];

for (i=1;i<h-1;i++)
    for (j=1;j<w-1;j++)
	{
	p=i*w+j;

	m[0]=vs[p-w-1]; m[1]=vs[p-w+1]; m[2]=vs[p];
	m[3]=vs[p+w-1]; m[4]=vs[p+w+1];
	mm[0]=median5(m);
	mm[1]=vs[p];
	m[0]=vs[p-w]; m[1]=vs[p-1]; m[2]=vs[p];
	m[3]=vs[p+1]; m[4]=vs[p+w];
	mm[2]=median5(m);

	is[p]=median3(mm);
	}
}

//------------------------------------------------------------
//diamond 3x3		packed char RGB image (uint32_t)
//vs = input image
//is = output image
void dia3x3(const uint32_t *vs, int w, int h, uint32_t *is)
{
int i,j,p;
uint32_t m[16];

for (i=2;i<h-2;i++)
    for (j=2;j<w-2;j++)
	{
	p=i*w+j;
	m[0]=vs[p-2*w]; m[1]=vs[p-w-1]; m[2]=vs[p-w];
	m[3]=vs[p-w+1]; m[4]=vs[p-2]; m[5]=vs[p-1];
	m[6]=vs[p]; m[7]=vs[p+1]; m[8]=vs[p+2];
	m[9]=vs[p+w-1]; m[10]=vs[p+w]; m[11]=vs[p+w+1];
	m[12]=vs[p+2*w];

	is[p]=median13(m);
	}
}

//------------------------------------------------------------
//square 5x5		packed char RGB image (uint32_t)
//vs = input image
//is = output image
void sq5x5(const uint32_t *vs, int w, int h, uint32_t *is)
{
int i,j,p;
uint32_t m[32];

for (i=2;i<h-2;i++)
    for (j=2;j<w-2;j++)
	{
	p=i*w+j;

	m[0]=vs[p-2*w-2]; m[1]=vs[p-2*w-1]; m[2]=vs[p-2*w];
	m[3]=vs[p-2*w+1]; m[4]=vs[p-2*w+2]; m[5]=vs[p-w-2];
	m[6]=vs[p-w-1];   m[7]=vs[p-w];     m[8]=vs[p-w+1];
	m[9]=vs[p-w+2];	  m[10]=vs[p-2];    m[11]=vs[p-1];
	m[12]=vs[p];      m[13]=vs[p+1];    m[14]=vs[p+2];
	m[15]=vs[p+w-2];  m[16]=vs[p+w-1];  m[17]=vs[p+w];
	m[18]=vs[p+w+1];  m[19]=vs[p+w+2];  m[20]=vs[p+2*w-2];
	m[21]=vs[p+2*w-1];m[22]=vs[p+2*w];  m[23]=vs[p+2*w+1];
	m[24]=vs[p+2*w+2];

	is[p]=median25(m);
	}
}


//--------------------------------------------------------
//temporal 3 frames
void temp3(uint32_t *s1, uint32_t *s2, uint32_t *s3, int w, int h, uint32_t *is)
{
int i;
uint32_t m[32];

for (i=0;i<w*h;i++)
    {
    m[0]=s1[i]; m[1]=s2[i]; m[2]=s3[i];
    is[i]=median3(m);
    }
}


//--------------------------------------------------------
//temporal 5 frames
void temp5(uint32_t *s1, uint32_t *s2, uint32_t *s3, uint32_t *s4, uint32_t *s5, int w, int h, uint32_t *is)
{
int i;
uint32_t m[32];

for (i=0;i<w*h;i++)
    {
    m[0]=s1[i]; m[1]=s2[i]; m[2]=s3[i]; m[3]=s4[i]; m[4]=s5[i];
    is[i]=median5(m);
    }
  
}


//------------------------------------------------------------
//Arce BI	packed char RGB image (uint32_t)
//s1,s2,s3 = previous, current, next frame
//is = output image
void ArceBI(uint32_t *s1, uint32_t *s2, uint32_t *s3, int w, int h, uint32_t *is)
{
int i,j,p;
uint32_t mm[8],m[16];

for (i=1;i<h-1;i++)
    for (j=1;j<w-1;j++)
	{
	p=i*w+j;
//grupa C
	mm[0]=s1[p];
//GRUPA W1
	m[0]=s1[p]; m[1]=s2[p-w-1]; m[2]=s2[p];
	m[3]=s2[p+w+1]; m[4]=s3[p];
	mm[3]=median5(m);
//grupa W2
	m[0]=s1[p]; m[1]=s2[p-w]; m[2]=s2[p];
	m[3]=s2[p+w]; m[4]=s3[p];
	mm[4]=median5(m);
//grupa W3
	m[0]=s1[p]; m[1]=s2[p-1]; m[2]=s2[p];
	m[3]=s2[p+1]; m[4]=s3[p];
	mm[5]=median5(m);
//grupa W4
	m[0]=s1[p]; m[1]=s2[p-w+1]; m[2]=s2[p];
	m[3]=s2[p+w-1]; m[4]=s3[p];
	mm[6]=median5(m);
//max
	mm[1]=mm[3]; if (mm[4]>mm[1]) mm[1]=mm[4];
	if (mm[5]>mm[1]) mm[1]=mm[5];
	if (mm[6]>mm[1]) mm[1]=mm[6];
//min
	mm[2]=mm[3]; if (mm[4]<mm[2]) mm[2]=mm[4];
	if (mm[5]<mm[2]) mm[2]=mm[5];
	if (mm[6]<mm[2]) mm[2]=mm[6];
//izhod
	is[p]=median3(mm);
	}
}


//------------------------------------------------------------
//Arp ML3D	packed char RGB image (uint32_t)
//s1,s2,s3 = previous, current, next frame
//is = output image
void ml3d(uint32_t *s1, uint32_t *s2, uint32_t *s3, int w, int h, uint32_t *is)
{
int i,j,p;
uint32_t mm[8],m[16];

for (i=1;i<h-1;i++)
    for (j=1;j<w-1;j++)
	{
	p=i*w+j;
//grupa C
	mm[0]=s1[p];
//grupa W6
	m[0]=s1[p]; m[1]=s2[p-w-1]; m[2]=s2[p-w+1];
	m[3]=s2[p]; m[4]=s2[p+w-1]; m[5]=s2[p+w+1];
	m[6]=s3[p];
	mm[1]=median7(m);
//grupa W5
	m[0]=s1[p]; m[1]=s2[p-w]; m[2]=s2[p-1];
	m[3]=s2[p]; m[4]=s2[p+1]; m[5]=s2[p+w];
	m[6]=s3[p];
	mm[2]=median7(m);
//izhod = median medianov
	is[p]=median3(mm);
	}
}

//------------------------------------------------------------
//Kokaram ML3Dex	packed char RGB image (uint32_t)
//s1,s2,s3 = previous, current, next frame
//is = output image
void ml3dex(uint32_t *s1, uint32_t *s2, uint32_t *s3, int w, int h, uint32_t *is)
{
int i,j,p;
uint32_t mm[8],m[16];

for (i=1;i<h-1;i++)
    for (j=1;j<w-1;j++)
	{
	p=i*w+j;
//grupa W9
	m[0]=s1[p-w-1]; m[1]=s1[p-w+1]; m[2]=s1[p];
	m[3]=s1[p+w-1]; m[4]=s1[p+w+1]; m[5]=s2[p];
	m[6]=s3[p-w-1]; m[7]=s3[p-w+1]; m[8]=s3[p];
	m[9]=s3[p+w-1]; m[10]=s3[p+w+1];
	mm[0]=median11(m);
//grupa W8
	m[0]=s1[p-w]; m[1]=s1[p-1]; m[2]=s1[p];
	m[3]=s1[p+w]; m[4]=s1[p+1]; m[5]=s2[p];
	m[6]=s3[p-w]; m[7]=s3[p-1]; m[8]=s3[p];
	m[9]=s3[p+w]; m[10]=s3[p+1];
	mm[1]=median11(m);
//grupa W7
	m[0]=s1[p]; m[1]=s2[p]; m[2]=s3[p];
	mm[2]=median3(m);
//grupa W6
	m[0]=s1[p]; m[1]=s2[p-w-1]; m[2]=s2[p-w+1];
	m[3]=s2[p]; m[4]=s2[p+w-1]; m[5]=s2[p+w+1];
	m[6]=s3[p];
	mm[3]=median7(m);
//grupa W5
	m[0]=s1[p]; m[1]=s2[p-w]; m[2]=s2[p-1];
	m[3]=s2[p]; m[4]=s2[p+1]; m[5]=s2[p+w];
	m[6]=s3[p];
	mm[4]=median7(m);
//izhod = median medianov
	is[p]=median5(mm);
	}
}

//****************************************************

//----------------------------------------
//struktura za instanco efekta
typedef struct
{
int h;
int w;

//parameters
int type;
int size;

//internal variables
uint32_t *ppf,*pf,*cf,*nf,*nnf;

//image buffers
uint32_t *f1;
uint32_t *f2;
uint32_t *f3;
uint32_t *f4;
uint32_t *f5;


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

info->name="Medians";
info->author="Marko Cebokli";
info->plugin_type=F0R_PLUGIN_TYPE_FILTER;
info->color_model=F0R_COLOR_MODEL_RGBA8888;
info->frei0r_version=FREI0R_MAJOR_VERSION;
info->major_version=0;
info->minor_version=1;
info->num_params=2;
info->explanation="Implements several median-type filters";
}

//--------------------------------------------------
void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
switch(param_index)
	{
	case 0:
		info->name = "Type";
		info->type = F0R_PARAM_STRING;
		info->explanation = "Choose type of median: Cross5, Square3x3, Bilevel, Diamond3x3, Square5x5, Temp3, Temp5, ArceBI, ML3D, ML3dEX, VarSize";
		break;
	case 1:
		info->name = "Size";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "Size for 'var size' type filter";
		break;
	case 2:
		info->name = "";
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

in->type=1;
in->liststr=calloc(1,strlen("Square3x3")+1);
strcpy(in->liststr,"Square3x3");
in->size=5;

in->f1=calloc(in->w*in->h,sizeof(uint32_t));
in->f2=calloc(in->w*in->h,sizeof(uint32_t));
in->f3=calloc(in->w*in->h,sizeof(uint32_t));
in->f4=calloc(in->w*in->h,sizeof(uint32_t));
in->f5=calloc(in->w*in->h,sizeof(uint32_t));

in->ppf=in->f1;
in->pf=in->f2;
in->cf=in->f3;
in->nf=in->f4;
in->nnf=in->f5;

return (f0r_instance_t)in;
}

//---------------------------------------------------
void f0r_destruct(f0r_instance_t instance)
{
inst *in;

in=(inst*)instance;

free(in->f1);
free(in->f2);
free(in->f3);
free(in->f4);
free(in->f5);

free(in->liststr);
free(instance);
}

//-----------------------------------------------------
void f0r_set_param_value(f0r_instance_t instance, f0r_param_t parm, int param_index)
{
inst *p;
double tmpf;
int chg;
char *tmpch;
char list1[][11]={"Cross5", "Square3x3", "Bilevel", "Diamond3x3", "Square5x5", "Temp3", "Temp5", "ArceBI", "ML3D", "ML3dEX", "VarSize"};

p=(inst*)instance;

chg=0;
switch(param_index)
	{
	case 0:		//(string based list)
		tmpch = (*(char**)parm);
		p->liststr = (char*)realloc( p->liststr, strlen(tmpch) + 1 );
		strcpy( p->liststr, tmpch );
		p->type=0;
		while ((strcmp(p->liststr,list1[p->type])!=0)&&(p->type<10)) p->type++;
		break;
	case 1:
                tmpf=map_value_forward(*((double*)parm), 0.0, 50); 
		if (tmpf!=p->size) chg=1;
		p->size=tmpf;
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
	case 0:		//(string based list)
		*((char**)param) = p->liststr;
		break;
	case 1:
		*((double*)param)=map_value_backward(p->size, 0.0, 50);
		break;
	}
}

//-------------------------------------------------
void f0r_update(f0r_instance_t instance, double time, const uint32_t* inframe, uint32_t* outframe)
{
inst *in;

assert(instance);
in=(inst*)instance;
uint32_t *tmpp;
uint8_t *cin,*cout;
int step,i;

memcpy(in->ppf, inframe, 4*in->w*in->h);
tmpp=in->nnf;
in->nnf=in->ppf;
in->ppf=in->pf;
in->pf=in->cf;
in->cf=in->nf;
in->nf=tmpp;

cin=(uint8_t*)inframe;
cout=(uint8_t*)outframe;

switch (in->type)
	{
	case 0:
		cross5(inframe, in->w, in->h, outframe);
		break;
	case 1:
		sq3x3(inframe, in->w, in->h, outframe);
		break;
	case 2:
		bilevel(inframe, in->w, in->h, outframe);
		break;
	case 3:
		dia3x3(inframe, in->w, in->h, outframe);
		break;
	case 4:
		sq5x5(inframe, in->w, in->h, outframe);
		break;
	case 5:
		temp3(in->cf, in->nf, in->nnf, in->w, in->h, outframe);
		break;
	case 6:
		temp5(in->ppf, in->pf, in->cf, in->nf, in->nnf, in->w, in->h, outframe);
		break;
	case 7:
		ArceBI(in->cf, in->nf, in->nnf, in->w, in->h, outframe);
		break;
	case 8:
		ml3d(in->cf, in->nf, in->nnf, in->w, in->h, outframe);
		break;
	case 9:
		ml3dex(in->cf, in->nf, in->nnf, in->w, in->h, outframe);
		break;
	case 10:
		//varsize
		step=in->w*4;
		ctmf(cin,cout,in->w,in->h,step,step,in->size,4,512*1024);
		break;
	default:
		break;
	}
//COPY ALPHA
for (i = 3; i < 4 * in->w * in->h; i += 4)
	cout[i]=cin[i];

}

