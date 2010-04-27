/* defish0r.c

 * Copyright (C) 2010 Marko Cebokli   http://lea.hamradio.si/~s57uuu
 * This file is a Frei0r plugin.
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



//compile:	gcc -Wall -std=c99 -c -fPIC defish0r.c -o defish0r.o

//link: gcc -lm -shared -o defish0r.so defish0r.o

//skaliranje za center=1 se ne dela!!!!

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <frei0r.h>

#include "interp.h"


double PI=3.14159265358979;

//---------------------------------------------------------
//	r = 0...1    izhod = 0...maxr
//ta funkcija da popacenje v odvisnosti od r
float fish(int n, float r, float f)
{
float rr,ff;

//printf("ff=%f\n",ff);
switch (n)
	{
	case 0:		//equidistant
		ff=f*2.0/PI;
		rr=tanf(r/ff);
		break;
	case 1:		//ortographic
		ff=r/f;
		if (ff>1.0)
			rr=-1.0;
		else
			rr=tanf(asinf(ff));
		break;
	case 2:		//equiarea
		ff=r/2.0/f;
		if (ff>1.0)
			rr=-1.0;
		else
			rr=tanf(2.0*asinf(r/2.0/f));
		break;
	case 3:		//stereographic
		ff=f*2.0/PI;
		rr=tanf(2.0*atanf(r/2.0/ff));
		break;
	default:
//		printf("Neznana fishitvena funkcija %d\n",n);
		break;
	}
return rr;
}

//---------------------------------------------------------
//ta funkcija da popacenje v odvisnosti od r
//	r = 0...1    izhod = 0...1
float defish(int n, float r, float f, float mr)
{
float rr;

switch (n)
	{
	case 0:		//equidistant
		rr=f*2.0/PI*atanf(r*mr);
		break;
	case 1:		//ortographic
		rr=f*sinf(atanf(r*mr));
		break;
	case 2:		//equiarea
		rr=2.0*f*sinf(atanf(r*mr)/2.0);
		break;
	case 3:		//stereographic
		rr=f*4.0/PI*tanf(atanf(r*mr)/2.0);
		break;
	default:
//		printf("Neznana fishitvena funkcija %d\n",n);
		break;
	}
return rr;
}

//----------------------------------------------------------------
//nafila array map s polozaji pikslov
//locena funkcija, da jo poklicem samo enkrat na zacetku,
//array map[] potem uporablja funkcija remap()
//tako ni treba za vsak frame znova racunat teh sinusov itd...
//wi,hi,wo ho = input.output image width/height
//n = 0..3	function select
//f = focal ratio (amount of distortion)
//scal = scaling factor
//pari, paro = pixel aspect ratio (input / output)
//dx, dy   offset on input (for non-cosited chroma subsampling)
void fishmap(int wi, int hi, int wo, int ho, int n, float f, float scal, float pari, float paro, float dx, float dy, float *map)
{
float rmax,maxr,r,kot,x,y,imax;
int i,j,ww,wi2,hi2,wo2,ho2;
float ii,jj,sc;

rmax=hypotf(ho/2.0,wo/2.0*paro);
maxr=fish(n,1.0,f);
imax=hypotf(hi/2.0,wi/2.0*pari);
sc=imax/maxr;

//printf("Fishmap: F=%5.2f  Rmax= %7.2f  Maxr=%6.2f  sc=%6.2f  scal=%6.2f\n",f,rmax,maxr,sc,scal);

wi2=wi/2; hi2=hi/2; wo2=wo/2; ho2=ho/2;
for (i=0;i<ho;i++)
	{
	ii=i-ho2;
	for (j=0;j<wo;j++)
		{
		jj=(j-wo2)*paro;
		r=hypotf(ii,jj);
		kot=atan2f(ii,jj);
		r=fish(n,r/rmax*scal,f)*sc;
		ww=2*(wo*i+j);
		if (r<0.0)
			{
			map[ww]=-1;
			map[ww+1]=-1;
			}
		else
			{
			x=wi2+r*cosf(kot)/pari;
			y=hi2+r*sinf(kot);
			if ((x>0)&(x<(wi-1))&(y>0)&(y<(hi-1)))
				{
				map[ww]=x+dx;
				map[ww+1]=y+dy;
				}
			else
				{
				map[ww]=-1;
				map[ww+1]=-1;
				}
			}
		}
	}

}

//----------------------------------------------------------------
//nafila array map s polozaji pikslov
//locena funkcija, da jo poklicem samo enkrat na zacetku,
//array map[] potem uporablja funkcija remap()
//tako ni treba za vsak frame znova racunat teh sinusov itd...
//wi,hi,wo ho = input.output image width/height
//n = 0..3	function select
//f = focal ratio (amount of distortion)
//scal = scaling factor
//pari,paro = pixel aspect ratio (input / output)
//dx, dy   offset on input (for non-cosited chroma subsampling)
void defishmap(int wi, int hi, int wo, int ho, int n, float f, float scal, float pari, float paro, float dx, float dy, float *map)
{
float rmax,maxr,r,kot,x,y,imax;
int i,j,ww,wi2,hi2,wo2,ho2;
float ii,jj,sc;

rmax=hypotf(ho/2.0,wo/2.0*paro);
maxr=fish(n,1.0,f);
imax=hypotf(hi/2.0,wi/2.0*pari);
sc=imax/maxr;

//printf("Defishmap: F=%f  rmax= %f  Maxr=%f   sc=%6.2f  scal=%6.2f\n",f,rmax,maxr,sc,scal);

wi2=wi/2; hi2=hi/2; wo2=wo/2; ho2=ho/2;
for (i=0;i<ho;i++)
	{
	ii=i-ho2;
	for (j=0;j<wo;j++)
		{
		jj=(j-wo2)*paro;	//aspect....
		r=hypotf(ii,jj)/scal;
		kot=atan2f(ii,jj);
		ww=2*(wo*i+j);
		r=defish(n,r/sc,f,1.0)*rmax;
		if (r<0.0)
			{
			map[ww]=-1;
			map[ww+1]=-1;
			}
		else
			{
			x=wi2+r*cosf(kot)/pari;
			y=hi2+r*sinf(kot);
			if ((x>0)&(x<(wi-1))&(y>0)&(y<(hi-1)))
				{
				map[ww]=x;
				map[ww+1]=y;
				}
			else
				{
				map[ww]=-1;
				map[ww+1]=-1;
				}
			}
		}
	}
}

//=====================================================
//kao instanca frei0r
//w,h:	image dimensions in pixels
//f:	focal ratio
//dir:	0=defish  1=fish
//type:	0..3	equidistant, ortographic, equiarea, stereographic
//scal:	0..3	image to fill, keep center scale, image to fit, manu
//intp: 0..6	type of interpolator
//aspt:	0..4	aspect type square, PAL, NTSC, HDV, manual
//par:	pixel aspect ratio
typedef struct
	{
	int w;
	int h;
	float f;
	int dir;
	int type;
	int scal;
	int intp;
	float mscale;
	int aspt;
	float mpar;
	float par;
	float *map;
	interpp interpol;
	} param;



//-------------------------------------------------------
interpp set_intp(param p)
{
switch (p.intp)	//katero interpolacijo bo uporabil
	{
//	case -1:return interpNNpr_b;	//nearest neighbor+print
	case 0:	return interpNN_b32;	//nearest neighbor
	case 1: return interpBL_b32;	//bilinear
	case 2:	return interpBC_b32;	//bicubic smooth
	case 3:	return interpBC2_b32;	//bicibic sharp
	case 4:	return interpSP4_b32;	//spline 4x4
	case 5:	return interpSP6_b32;	//spline 6x6
	case 6: return interpSC16_b32;	//lanczos 8x8
	default: return NULL;
	}
}

//--------------------------------------------------------
void make_map(param p)
{
float rmax,maxr,imax,fscal,dscal;

rmax=hypotf(p.h/2.0,p.w/2.0*p.par);
maxr=fish(p.type,1.0,p.f);
imax=hypotf(p.h/2.0,p.w/2.0*p.par);

if (p.dir==0)		//defish
    {
    switch (p.scal)
	{
	case 0:		//fill
	    dscal=maxr*p.h/2.0/rmax/fish(p.type,p.h/2.0/rmax,p.f);
	    break;
	case 1:		//keep
	    dscal=maxr*p.f;
	    if ((p.type==0)||(p.type==3)) dscal=dscal/PI*2.0;break;
	case 2:		//fit
	    dscal=1.0; break;
	case 3:		//manual
	    dscal=p.mscale; break;
	}
	defishmap(p.w ,p.h ,p.w ,p.h, p.type, p.f, dscal, p.par, p.par, 0.0, 0.0,  p.map);
    }
else		//fish
    {
    switch (p.scal)
	{
	case 0:		//fill
	    fscal=1.0;break;
	case 1:		//keep
	    fscal=maxr*p.f;
	    if ((p.type==0)||(p.type==3)) fscal=fscal/PI*2.0;
	    break;
	case 2:		//fit
	    fscal=2.0*defish(p.type,p.h/2.0*maxr/imax,p.f,1.0)/p.h*rmax;
	    break;
	case 3:		//manual
	    fscal=1.0/p.mscale; break;
	}
	fishmap(p.w, p.h, p.w ,p.h, p.type, p.f, fscal, p.par, p.par, 0.0, 0.0,  p.map);
    }

}

//*********************************************************
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

info->name="Defish0r";
info->author="Marko Cebokli";
info->plugin_type=F0R_PLUGIN_TYPE_FILTER;
info->color_model=F0R_COLOR_MODEL_RGBA8888;
info->frei0r_version=FREI0R_MAJOR_VERSION;
info->major_version=0;
info->minor_version=2;
info->num_params=8;
info->explanation="Non rectilinear lens mappings";
}

//--------------------------------------------------
void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
switch(param_index)
  {
  case 0:
    info->name = "Amount";
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "Focal Ratio";
    break;
  case 1:
    info->name = "DeFish";
    info->type = F0R_PARAM_BOOL;
    info->explanation = "Fish or Defish";
    break;
  case 2:
    info->name = "Type";
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "Mapping function";
    break;
  case 3:
    info->name = "Scaling";
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "Scaling method";
    break;
  case 4:
    info->name = "Manual Scale";
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "Manual Scale";
    break;
  case 5:
    info->name = "Interpolator";
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "Quality of interpolation";
    break;
  case 6:
    info->name = "Aspect type";
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "Pixel aspect ratio presets";
    break;
  case 7:
    info->name = "Manual Aspect";
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "Manual Pixel Aspect ratio";
    break;
  }

}

//--------------------------------------------------------
//kao constructor za frei0r
f0r_instance_t  f0r_construct(unsigned int width, unsigned int height)
{
param *p;

p=calloc(1, sizeof(param));

p->w=width;
p->h=height;
p->f=20.0;		//defaults (not used??)
p->dir=1;
p->type=2;
p->scal=2;
p->intp=1;
p->mscale=1.0;
p->aspt=0;		//square pixels
p->par=1.0;		//square pixels
p->mpar=1.0;

p->map=calloc(1, sizeof(float)*(p->w*p->h*2+2));
p->interpol=set_intp(*p);

make_map(*p);

//printf("Construct, w=%d h=%d\n",width,height);

return (f0r_instance_t)p;
}

//---------------------------------------------------
void f0r_destruct(f0r_instance_t instance)
{
param *p;
p=(param*)instance;

free(p->map);
free(instance);
}

//----------------------------------------------------
//not used in frei0r plugin
void change_param(param *p, int w, int h, float f, int dir, int type, int scal, int intp)
{
p->f=f;
p->dir=dir;
p->type=type;
p->scal=scal;
p->intp=intp;

if ((w!=p->w)||(h!=p->h))
	{
	free(p->map);
	p->map=calloc(1, sizeof(float)*(w*h*2+2));
	p->w=w;
	p->h=h;
	}

p->interpol=set_intp(*p);
make_map(*p);
}

//-----------------------------------------------------
void print_param(param p)
//not used in frei0r plugin
{
printf("Param: w=%d h=%d f=%f dir=%d",p.w, p.h, p.f, p.dir);
printf(" type=%d scal=%d intp=%d",p.type, p.scal, p.intp);
printf(" mscale=%f par=%f mpar=%f\n",p.mscale, p.par, p.mpar);
}

//------------------------------------------------------
//computes x to the power p
//only for positive x
float pwr(float x, float p)
{
if (x<=0) return 0;
//printf("exp(%f)=%f\n",x,expf(p*logf(x)));
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
//smisele vrednosti za parameter f:  (za fish)
// tip 0: (0.3) 1.001...10;    tip 1: 1.000...10
// tip 2: (0.5) 0.75...10.0    tip 3: (0.1) 0.78...10
//za defish:
// tip 0: 0.1...10             tip 1: 1.0...10
// tip 2: 0.5...10             tip 3: (0.1) 0.5...10
void f0r_set_param_value(f0r_instance_t instance, f0r_param_t parm, int param_index)
{
param *p;
int chg,tmpi;
float tmpf;

p=(param*)instance;

//printf("set parm: index=%d, value=%f\n",param_index,*(float*)parm);
chg=0;
switch(param_index)
  {
  case 0:	//f
    tmpf=pwr(*((double*)parm),1.0/5.0);
//    tmpf=map_value_forward(*((double*)parm), 10.0, 0.1);
    tmpf=map_value_forward(tmpf, 20.0, 0.1);
    if (p->f != tmpf) chg=1;
    p->f=tmpf;
    break;
  case 1:	//fish
    tmpi=map_value_forward(*((double*)parm), 1.0, 0.0);//BOOL!!
    if (p->dir != tmpi) chg=1;
    p->dir=tmpi;
    break;
  case 2:	//type
    tmpi=map_value_forward(*((double*)parm), 0.0, 3.999);
    if (p->type != tmpi) chg=1;
    p->type=tmpi;
    break;
  case 3:	//scaling
    tmpi=map_value_forward(*((double*)parm), 0.0, 3.999);
    if (p->scal != tmpi) chg=1;
    p->scal=tmpi;
    break;
  case 4:	//manual scale
    tmpf=map_value_forward_log(*((double*)parm), 0.01, 100.0);
    if (p->mscale != tmpf) chg=1;
    p->mscale=tmpf;
    break;
  case 5:	//interpolator
    tmpi=map_value_forward(*((double*)parm), 0.0, 6.999);
    if (p->intp != tmpi) chg=1;
    p->intp=tmpi;
    break;
  case 6:	//aspect type
    tmpi=map_value_forward(*((double*)parm), 0.0, 4.999);
    if (p->aspt != tmpi) chg=1;
    p->aspt=tmpi;
    break;
  case 7:	//manual aspect
    tmpf=map_value_forward_log(*((double*)parm), 0.5, 2.0);
    if (p->mpar != tmpf) chg=1;
    p->mpar=tmpf;
    break;
  }

if (chg!=0)
  {
  switch (p->aspt)	//pixel aspect ratio
    {
    case 0: p->par=1.000;break;		//square pixels
    case 1: p->par=1.067;break;		//PAL DV
    case 2: p->par=0.889;break;		//NTSC DV
    case 3: p->par=1.333;break;		//HDV
    case 4: p->par=p->mpar;break;	//manual
    }
  p->interpol=set_intp(*p);
  make_map(*p);
  }

//print_param(*p);
}

//--------------------------------------------------
void f0r_get_param_value(f0r_instance_t instance, f0r_param_t parm, int param_index)
{
param *p;
float tmpf;

p=(param*)instance;

switch(param_index)
  {
  case 0:	//f
//    *((double*)parm)=map_value_backward(p->f, 10.0, 0.1);
    tmpf=map_value_backward(p->f, 20.0, 0.1);
    *((double*)parm)=pwr(tmpf, 5.0);
    break;
  case 1:	//fish
    *((double*)parm)=map_value_backward(p->dir, 1.0, 0.0); //BOOL!!
    break;
  case 2:	//type
    *((double*)parm)=map_value_backward(p->type, 0.0, 3.0);
    break;
  case 3:	//scaling
    *((double*)parm)=map_value_backward(p->scal, 0.0, 3.0);
    break;
  case 4:	//manual scale
    *((double*)parm)=map_value_backward_log(p->mscale, 0.01, 100.0);
    break;
  case 5:	//interpolator
    *((double*)parm)=map_value_backward(p->intp, 0.0, 6.0);
    break;
  case 6:	//aspect type
    *((double*)parm)=map_value_backward(p->aspt, 0.0, 4.999);
    break;
  case 7:	//manual aspect
    *((double*)parm)=map_value_backward_log(p->mpar, 0.5, 2.0);
    break;
  }
}

//-------------------------------------------------
void f0r_update(f0r_instance_t instance, double time, const uint32_t* inframe, uint32_t* outframe)
{
param *p;
int size;
int i,j;

p=(param*)instance;

remap32(p->w, p->h, p->w, p->h, (unsigned char*) inframe, (unsigned char*) outframe, p->map, 0, p->interpol);

}


