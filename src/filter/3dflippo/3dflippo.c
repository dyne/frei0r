/* 3dflippo.c */

/*
 * 25/01/2006 c.e. prelz
 *
 * My second frei0r effect - more complex flipping
 *
 * Copyright (C) 2006 BEK - Bergen Senter for Elektronisk Kunst <bek@bek.no>
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

#include "frei0r.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(_MSC_VER)
#define _USE_MATH_DEFINES
#endif /* _MSC_VER */
#include <math.h>

#define MSIZE 4
#define TWO_PI (M_PI*2.0)

enum axis
{
  AXIS_X,
  AXIS_Y,
  AXIS_Z
};

#include <assert.h>

typedef struct tdflippo_instance
{
  unsigned int width,height,fsize;
  int *mask;
  float flip[3],rate[3],center[2];
  unsigned char invertrot,dontblank,fillblack,mustrecompute;
} tdflippo_instance_t;

static float **newmat(unsigned char unit_flg);
static void matfree(float **mat);
static float **mat_translate(float tx,float ty,float tz);
static float **mat_rotate(enum axis ax,float angle);
static float **matmult(float **mat1,float **mat2);
static void vetmat(float **mat,float *x,float *y,float *z);
static void recompute_mask(tdflippo_instance_t* inst);

int f0r_init()
{
  return 1;
}

void f0r_deinit()
{
}

void f0r_get_plugin_info(f0r_plugin_info_t* flippoInfo)
{
  flippoInfo->name="3dflippo";
  flippoInfo->author="c.e. prelz AS FLUIDO <fluido@fluido.as>";
  flippoInfo->plugin_type=F0R_PLUGIN_TYPE_FILTER;
  flippoInfo->color_model=F0R_COLOR_MODEL_PACKED32;
  flippoInfo->frei0r_version=FREI0R_MAJOR_VERSION;
  flippoInfo->major_version=0;
  flippoInfo->minor_version=1;
  flippoInfo->num_params=11;
  flippoInfo->explanation="Frame rotation in 3d-space";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
  switch(param_index)
  {
  case 0:
    info->name="X axis rotation";
    info->type=F0R_PARAM_DOUBLE;
    info->explanation="Rotation on the X axis";
    break;
  case 1:
    info->name="Y axis rotation";
    info->type=F0R_PARAM_DOUBLE;
    info->explanation="Rotation on the Y axis";
    break;
  case 2:
    info->name="Z axis rotation";
    info->type=F0R_PARAM_DOUBLE;
    info->explanation="Rotation on the Z axis";
    break;
  case 3:
    info->name="X axis rotation rate";
    info->type=F0R_PARAM_DOUBLE;
    info->explanation="Rotation rate on the X axis";
    break;
  case 4:
    info->name="Y axis rotation rate";
    info->type=F0R_PARAM_DOUBLE;
    info->explanation="Rotation rate on the Y axis";
    break;
  case 5:
    info->name="Z axis rotation rate";
    info->type=F0R_PARAM_DOUBLE;
    info->explanation="Rotation rate on the Z axis";
    break;
  case 6:
    info->name="Center position (X)";
    info->type=F0R_PARAM_DOUBLE;
    info->explanation="Position of the center of rotation on the X axis";
    break;
  case 7:
    info->name="Center position (Y)";
    info->type=F0R_PARAM_DOUBLE;
    info->explanation="Position of the center of rotation on the Y axis";
    break;
  case 8:
    info->name="Invert rotation assignment";
    info->type=F0R_PARAM_BOOL;
    info->explanation="If true, when mapping rotation, make inverted (wrong) assignment";
    break;
  case 9:
    info->name="Don't blank mask";
    info->type=F0R_PARAM_BOOL;
    info->explanation="Mask for frame transposition is not blanked, so a trace of old transpositions is maintained";
    break;
  case 10:
    info->name="Fill with image or black";
    info->type=F0R_PARAM_BOOL;
    info->explanation="If true, pixels that are not transposed are black, otherwise, they are copied with the original";
    break;
  }
}

f0r_instance_t f0r_construct(unsigned int width,unsigned int height)
{
  tdflippo_instance_t *inst=(tdflippo_instance_t*)calloc(1, sizeof(*inst));
  inst->width=width;
  inst->height=height;
  inst->fsize=width*height;

  inst->flip[0]=inst->flip[1]=inst->flip[2]=inst->rate[0]=inst->rate[1]=inst->rate[2]=0.5;
  
  inst->mask=(int*)malloc(sizeof(int)*inst->fsize);

  return (f0r_instance_t)inst;
}

void f0r_destruct(f0r_instance_t instance)
{
  tdflippo_instance_t* inst=(tdflippo_instance_t*)instance;

  free(inst->mask);
  free(inst);
}

void f0r_set_param_value(f0r_instance_t instance,
                         f0r_param_t param,int param_index)
{
  assert(instance);

  tdflippo_instance_t *inst=(tdflippo_instance_t*)instance;

  switch(param_index)
  {
  case 0:
    inst->flip[0]=(float)(*((double*)param));
    break;
  case 1:
    inst->flip[1]=(float)(*((double*)param));
    break;
  case 2:
    inst->flip[2]=(float)(*((double*)param));
    break;
  case 3:
    inst->rate[0]=(float)(*((double*)param));
    break;
  case 4:
    inst->rate[1]=(float)(*((double*)param));
    break;
  case 5:
    inst->rate[2]=(float)(*((double*)param));
    break;
  case 6:
    inst->center[0]=(float)(*((double*)param));
    break;
  case 7:
    inst->center[1]=(float)(*((double*)param));
    break;
  case 8:
    inst->invertrot=(*((double*)param)>=0.5);
    break;
  case 9:
    inst->dontblank=(*((double*)param)>=0.5);
    break;
  case 10:
    inst->fillblack=(*((double*)param)>=0.5);
    break;
  }

  if((param_index>=0 && param_index<=2) || (param_index>=6 && param_index<=9))
    inst->mustrecompute=1;
}

void f0r_get_param_value(f0r_instance_t instance,
                         f0r_param_t param,int param_index)
{
  assert(instance);

  tdflippo_instance_t *inst=(tdflippo_instance_t*)instance;

  switch(param_index)
  {
  case 0:
    *((double*)param)=inst->flip[0];
    break;
  case 1:
    *((double*)param)=inst->flip[1];
    break;
  case 2:
    *((double*)param)=inst->flip[2];
    break;
  case 3:
    *((double*)param)=inst->rate[0];
    break;
  case 4:
    *((double*)param)=inst->rate[1];
    break;
  case 5:
    *((double*)param)=inst->rate[2];
    break;
  case 6:
    *((double*)param)=inst->center[0];
    break;
  case 7:
    *((double*)param)=inst->center[1];
    break;
  case 8:
    *((double*)param)=(inst->invertrot ? 1.0 : 0.0);
    break;
  case 9:
    *((double*)param)=(inst->dontblank ? 1.0 : 0.0);
    break;
  case 10:
    *((double*)param)=(inst->fillblack ? 1.0 : 0.0);
    break;    
  }
}

void f0r_update(f0r_instance_t instance,double time,
                const uint32_t *inframe, uint32_t *outframe)
{
  assert(instance);

  tdflippo_instance_t* inst=(tdflippo_instance_t*)instance;  
  int i;

  if(inst->rate[0]!=0.5 || inst->rate[1]!=0.5 || inst->rate[2]!=0.5 || inst->mustrecompute)
  {
    inst->mustrecompute=0;
    
/*
 * We are changing: apply change and recompute mask
 */

    for(i=0;i<3;i++)
    {
      inst->flip[i]+=inst->rate[i]-0.5;
      if(inst->flip[i]<0.0)
	inst->flip[i]+=1.0;
      else if(inst->flip[i]>=1.0)
	inst->flip[i]-=1.0;
    }
    recompute_mask(inst);
  }

  for(i=0;i<inst->fsize;i++)
  {
    if(inst->mask[i]>=0)
      outframe[i]=inframe[inst->mask[i]];
    else if(!inst->fillblack)
      outframe[i]=inframe[i];
    else
      outframe[i]=0;
  }
}

static float **newmat(unsigned char unit_flg)
{
  int i;
  float **to_ret=(float**)malloc(sizeof(float *)*MSIZE);

  for(i=0;i<MSIZE;i++)
  {
    to_ret[i]=(float*)calloc(MSIZE,sizeof(float));
    if(unit_flg)
      to_ret[i][i]=1.0;
  }

  return to_ret;
}

static void matfree(float **mat)
{
  int i;

  for(i=0;i<MSIZE;i++)
    free(mat[i]);
  
  free(mat);
}

static float **mat_translate(float tx,float ty,float tz)
{
  float **mat=newmat(1);

  mat[0][3]=tx;
  mat[1][3]=ty;
  mat[2][3]=tz;

  return mat;
}

static float **mat_rotate(enum axis ax,float angle)
{
  float **mat=newmat(1);
  float sf=sinf(angle);
  float cf=cosf(angle);

  switch(ax)
  {
    case AXIS_X:
      mat[1][1]=cf;
      mat[1][2]=-sf;
      mat[2][1]=sf;
      mat[2][2]=cf;
      break;
    case AXIS_Y:
      mat[0][0]=cf;
      mat[0][2]=sf;
      mat[2][0]=-sf;
      mat[2][2]=cf;
      break;
    case AXIS_Z:
      mat[0][0]=cf;
      mat[0][1]=-sf;
      mat[1][0]=sf;
      mat[1][1]=cf;
      break;
  }
  return mat;
}  

static float **matmult(float **mat1,float **mat2)
{
  float **mat=newmat(0);
  int i,j,k;

  for(i=0;i<MSIZE;i++)
    for(j=0;j<MSIZE;j++)
      for(k=0;k<MSIZE;k++)
	mat[i][j]+=mat1[i][k]*mat2[k][j];
  
  matfree(mat1);
  matfree(mat2);

  return mat;
}

static void vetmat(float **mat,float *x,float *y,float *z)
{
  float v;
  float vet[]={*x,*y,*z,1.0};
  float *vetp[]={x,y,z,&v};
  int i,j;

  for(i=0;i<MSIZE;i++)
  {
    *(vetp[i])=0.0;

    for(j=0;j<MSIZE;j++)
      *(vetp[i])+=mat[i][j]*vet[j];
  }
}
  
static void recompute_mask(tdflippo_instance_t* inst)
{
  float xpos=(float)inst->width*inst->center[0];
  float ypos=(float)inst->height*inst->center[1];
  float **mat=mat_translate(xpos,ypos,0.0);
  
  if(inst->flip[0]!=0.5)
    mat=matmult(mat,mat_rotate(AXIS_X,(inst->flip[0]-0.5)*TWO_PI));
  if(inst->flip[1]!=0.5)
    mat=matmult(mat,mat_rotate(AXIS_Y,(inst->flip[1]-0.5)*TWO_PI));
  if(inst->flip[2]!=0.5)
    mat=matmult(mat,mat_rotate(AXIS_Z,(inst->flip[2]-0.5)*TWO_PI));
  
  mat=matmult(mat,mat_translate(-xpos,-ypos,0.0));
  
#if 0
  fprintf(stderr,"Resarra %.2f %.2f %.2f %.2f | %.2f %.2f %.2f %.2f | %.2f %.2f %.2f %.2f | %.2f %.2f %.2f %.2f\n",
	  mat[0][0],mat[0][1],mat[0][2],mat[0][3],
	  mat[1][0],mat[1][1],mat[1][2],mat[1][3],
	  mat[2][0],mat[2][1],mat[2][2],mat[2][3],
	  mat[3][0],mat[3][1],mat[3][2],mat[3][3]);
#endif
  
  int x,y,nx,ny,pos;
  float xf,yf,zf;

  if(!inst->dontblank)
    memset(inst->mask,0xff,sizeof(int)*inst->fsize);

  for(y=0,pos=0;y<inst->height;y++)
    for(x=0;x<inst->width;x++,pos++)
    {
      xf=x;
      yf=y;
      zf=0.0;
      vetmat(mat,&xf,&yf,&zf);
      nx=(int)(xf+0.5);
      ny=(int)(yf+0.5);
      
      if(nx>=0 && nx<inst->width && ny>=0 && ny<inst->height)
      {
	if(!inst->invertrot) 
	  inst->mask[ny*inst->width+nx]=pos;
	else
	  inst->mask[pos]=ny*inst->width+nx;
      }
    }
  matfree(mat);
}
