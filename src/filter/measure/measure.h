/*
measure.c 

measures video pixel and profile values with averaging

Version 0.1	jun 2010

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

//measurement functions for direct inclusion in pr0be.c, pr0file.c

typedef struct		//float pixel
	{
	float r;
	float g;
	float b;
	float a;
	} float_rgba;

typedef struct		//statistics
	{
	float avg;
	float rms;
	float min;
	float max;
	} stat;

typedef struct		//profile data and statistics
	{
	int n;		//number of points used
	float r[8192];
	float g[8192];
	float b[8192];
	float a[8192];
	float y[8192];
	float u[8192];
	float v[8192];
	stat sr;
	stat sg;
	stat sb;
	stat sa;
	stat sy;
	stat su;
	stat sv;
	int xz,xk,yz,yk;	//start and end point
	} profdata;

//-----------------------------------------------------
//luminance/luma statistics of a float_rgba pixel group
//color:
//0=use rec 601
//1=use rec 709
//w=width of image (stride)
//x,y=position of the center of the group in pixels
//sx,sy=size of the group in pixels
void meri_y(float_rgba *s, stat *yy, int color, int x, int y, int w, int sx, int sy)
{
float wr,wg,wb,luma,nf;
int xp,yp;
float_rgba pix;
int i,j;

switch (color)
	{
	case 0:		//CCIR rec 601
		wr=0.299;wg=0.587;wb=0.114;
		break;
	case 1:		//CCIR rec 709
		wr=0.2126;wg=0.7152;wb=0.0722;
		break;
	default:	//unknown color model
//		printf("Unknown color model %d\n",color);
		break;
	}

yy->avg=0.0; yy->rms=0.0; yy->min=1.0E9; yy->max=-1.0E9;
for (i=0;i<sy;i++)
	for (j=0;j<sx;j++)
		{
		xp=x-sx/2+j;
		yp=y-sy/2+i;
		if (xp<0) xp=0; if (xp>=w) xp=w-1;
		if (yp<0) yp=0; //if (yp>=h) xp=h-1;
		pix=s[yp*w+xp];
		luma=wr*pix.r+wg*pix.g+wb*pix.b;
		if (luma<yy->min) yy->min=luma;
		if (luma>yy->max) yy->max=luma;
		yy->avg=yy->avg+luma;
		yy->rms=yy->rms+luma*luma;
		}
nf=(float)(sx*sy);
yy->avg=yy->avg/nf;
yy->rms=sqrtf((yy->rms-nf*yy->avg*yy->avg)/nf);

}

//-------------------------------------------------------
//R,G,B statistics of a float_rgba pixel group
//w=width of image (stride)
//x,y=position of the center of the group in pixels
//sx,sy=size of the group in pixels
void meri_rgb(float_rgba *s, stat *r, stat *g, stat *b, int x, int y, int w, int sx, int sy)
{
float nf;
int xp,yp;
float_rgba pix;
int i,j;

r->avg=0.0; r->rms=0.0; r->min=1.0E9; r->max=-1.0E9;
g->avg=0.0; g->rms=0.0; g->min=1.0E9; g->max=-1.0E9;
b->avg=0.0; b->rms=0.0; b->min=1.0E9; b->max=-1.0E9;
for (i=0;i<sy;i++)
	for (j=0;j<sx;j++)
		{
		xp=x-sx/2+j;
		yp=y-sy/2+i;
		if (xp<0) xp=0; if (xp>=w) xp=w-1;
		if (yp<0) yp=0; //if (yp>=h) xp=h-1;
		pix=s[yp*w+xp];

		if (pix.r<r->min) r->min=pix.r;
		if (pix.r>r->max) r->max=pix.r;
		r->avg=r->avg+pix.r;
		r->rms=r->rms+pix.r*pix.r;

		if (pix.g<g->min) g->min=pix.g;
		if (pix.g>g->max) g->max=pix.g;
		g->avg=g->avg+pix.g;
		g->rms=g->rms+pix.g*pix.g;

		if (pix.b<b->min) b->min=pix.b;
		if (pix.b>b->max) b->max=pix.b;
		b->avg=b->avg+pix.b;
		b->rms=b->rms+pix.b*pix.b;
		}
nf=(float)(sx*sy);

r->avg=r->avg/nf;
r->rms=sqrtf((r->rms-nf*r->avg*r->avg)/nf);

g->avg=g->avg/nf;
g->rms=sqrtf((g->rms-nf*g->avg*g->avg)/nf);

b->avg=b->avg/nf;
b->rms=sqrtf((b->rms-nf*b->avg*b->avg)/nf);

}

//--------------------------------------------------------
//alpha channel statistics of a float_rgba pixel group
//w=width of image (stride)
//x,y=position of the center of the group in pixels
//sx,sy=size of the group in pixels
void meri_a(float_rgba *s, stat *a, int x, int y, int w, int sx, int sy)
{
float nf;
int xp,yp;
float_rgba pix;
int i,j;

a->avg=0.0; a->rms=0.0; a->min=1.0E9; a->max=-1.0E9;
for (i=0;i<sy;i++)
	for (j=0;j<sx;j++)
		{
		xp=x-sx/2+j;
		yp=y-sy/2+i;
		if (xp<0) xp=0; if (xp>=w) xp=w-1;
		if (yp<0) yp=0; //if (yp>=h) xp=h-1;
		pix=s[yp*w+xp];

		if (pix.a<a->min) a->min=pix.a;
		if (pix.a>a->max) a->max=pix.a;
		a->avg=a->avg+pix.a;
		a->rms=a->rms+pix.a*pix.a;
		}
nf=(float)(sx*sy);

a->avg=a->avg/nf;
a->rms=sqrtf((a->rms-nf*a->avg*a->avg)/nf);

}

//--------------------------------------------------------
//R-Y, B-Y statistics of a float_rgba pixel group
//color:
//0=use rec 601
//1=use rec 709
//w=width of image (stride)
//x,y=position of the center of the group in pixels
//sx,sy=size of the group in pixels
void meri_uv(float_rgba *s, stat *u, stat *v, int color, int x, int y, int w, int sx, int sy)
{
float wr,wg,wb,uu,vv,nf;
int xp,yp;
float_rgba pix;
int i,j;

switch (color)
	{
	case 0:		//CCIR rec 601
		wr=0.299;wg=0.587;wb=0.114;
		break;
	case 1:		//CCIR rec 709
		wr=0.2126;wg=0.7152;wb=0.0722;
		break;
	default:	//unknown color model
//		printf("Unknown color model %d\n",color);
		break;
	}

u->avg=0.0; u->rms=0.0; u->min=1.0E9; u->max=-1.0E9;
v->avg=0.0; v->rms=0.0; v->min=1.0E9; v->max=-1.0E9;
for (i=0;i<sy;i++)
	for (j=0;j<sx;j++)
		{
		xp=x-sx/2+j;
		yp=y-sy/2+i;
		if (xp<0) xp=0; if (xp>=w) xp=w-1;
		if (yp<0) yp=0; //if (yp>=h) xp=h-1;
		pix=s[yp*w+xp];

		//R-Y
		uu=(1.0-wr)*pix.r-wg*pix.g-wb*pix.b;
		if (uu<u->min) u->min=uu;
		if (uu>u->max) u->max=uu;
		u->avg=u->avg+uu;
		u->rms=u->rms+uu*uu;

		//B-Y
		vv=(1.0-wb)*pix.b-wr*pix.r-wg*pix.g;
		if (vv<v->min) v->min=vv;
		if (vv>v->max) v->max=vv;
		v->avg=v->avg+vv;
		v->rms=v->rms+vv*vv;
		}
nf=(float)(sx*sy);

u->avg=u->avg/nf;
u->rms=sqrtf((u->rms-nf*u->avg*u->avg)/nf);

v->avg=v->avg/nf;
v->rms=sqrtf((v->rms-nf*v->avg*v->avg)/nf);

}

//------------------------------------------------------------
//statistics of a float_rgba pixel profile
//color:
//0=use rec 601
//1=use rec 709
//w,h=size of image
//xz,yy,xk,yk=end points of the profile line
//sir=width of profile in pixels (averaging)
void meriprof(float_rgba *s, int w, int h, int xz, int yz, int xk, int yk, int sir, profdata *p)
{
int x,y,d,i;
float_rgba pix;

d =  abs(xk-xz)>abs(yk-yz) ? abs(xk-xz) : abs(yk-yz);
p->n=d;
for (i=0;i<d;i++)
	{
	x=xz+(float)i/d*(xk-xz);
	y=yz+(float)i/d*(yk-yz);
	if ((x>=0)&&(x<w)&&(y>=0)&&(y<h))
		pix=s[w*y+x];
	else
		{pix.r=0.0;pix.g=0.0;pix.b=0.0;pix.a=0.0;}
	p->r[i]=pix.r;
	p->g[i]=pix.g;
	p->b[i]=pix.b;
	p->a[i]=pix.a;
	}

}

//-----------------------------------------------------
//c=0 rec 601      c=1 rec 709
void prof_yuv(profdata *p, int color)
{
int i;
float wr,wg,wb;

switch (color)
	{
	case 0:		//CCIR rec 601
		wr=0.299;wg=0.587;wb=0.114;
		break;
	case 1:		//CCIR rec 709
		wr=0.2126;wg=0.7152;wb=0.0722;
		break;
	default:	//unknown color model
//		printf("Unknown color model %d\n",color);
		break;
	}

for (i=0;i<p->n;i++)
	{
	p->y[i]=wr*p->r[i]+wg*p->g[i]+wb*p->b[i];
	p->u[i]=p->r[i]-p->y[i];
	p->v[i]=p->b[i]-p->y[i];
	}
}

//---------------------------------------------------------
//calculates AVG, RMS, MIN, MAX
//for r,g,b,a,y,u,v profiles
void prof_stat(profdata *p)
{
int i;
float nf;

p->sr.avg=0.0; p->sr.rms=0.0; p->sr.min=1.0E9; p->sr.max=-1.0E9;
p->sg.avg=0.0; p->sg.rms=0.0; p->sg.min=1.0E9; p->sg.max=-1.0E9;
p->sb.avg=0.0; p->sb.rms=0.0; p->sb.min=1.0E9; p->sb.max=-1.0E9;
p->sa.avg=0.0; p->sa.rms=0.0; p->sa.min=1.0E9; p->sa.max=-1.0E9;
p->sy.avg=0.0; p->sy.rms=0.0; p->sy.min=1.0E9; p->sy.max=-1.0E9;
p->su.avg=0.0; p->su.rms=0.0; p->su.min=1.0E9; p->su.max=-1.0E9;
p->sv.avg=0.0; p->sv.rms=0.0; p->sv.min=1.0E9; p->sv.max=-1.0E9;
for (i=0;i<p->n;i++)
	{
	if (p->r[i]<p->sr.min) p->sr.min=p->r[i];
	if (p->r[i]>p->sr.max) p->sr.max=p->r[i];
	p->sr.avg=p->sr.avg+p->r[i];
	p->sr.rms=p->sr.rms+p->r[i]*p->r[i];

	if (p->g[i]<p->sg.min) p->sg.min=p->g[i];
	if (p->g[i]>p->sg.max) p->sg.max=p->g[i];
	p->sg.avg=p->sg.avg+p->g[i];
	p->sg.rms=p->sg.rms+p->g[i]*p->g[i];

	if (p->b[i]<p->sb.min) p->sb.min=p->b[i];
	if (p->b[i]>p->sb.max) p->sb.max=p->b[i];
	p->sb.avg=p->sb.avg+p->b[i];
	p->sb.rms=p->sb.rms+p->b[i]*p->b[i];

	if (p->a[i]<p->sa.min) p->sa.min=p->a[i];
	if (p->a[i]>p->sa.max) p->sa.max=p->a[i];
	p->sa.avg=p->sa.avg+p->a[i];
	p->sa.rms=p->sa.rms+p->a[i]*p->a[i];

	if (p->y[i]<p->sy.min) p->sy.min=p->y[i];
	if (p->y[i]>p->sy.max) p->sy.max=p->y[i];
	p->sy.avg=p->sy.avg+p->y[i];
	p->sy.rms=p->sy.rms+p->y[i]*p->y[i];

	if (p->u[i]<p->su.min) p->su.min=p->u[i];
	if (p->u[i]>p->su.max) p->su.max=p->u[i];
	p->su.avg=p->su.avg+p->u[i];
	p->su.rms=p->su.rms+p->u[i]*p->u[i];

	if (p->v[i]<p->sv.min) p->sv.min=p->v[i];
	if (p->v[i]>p->sv.max) p->sv.max=p->v[i];
	p->sv.avg=p->sv.avg+p->v[i];
	p->sv.rms=p->sv.rms+p->v[i]*p->v[i];
	}
nf=(float)(p->n);

p->sr.avg=p->sr.avg/nf;
p->sr.rms=sqrtf((p->sr.rms-nf*p->sr.avg*p->sr.avg)/nf);

p->sg.avg=p->sg.avg/nf;
p->sg.rms=sqrtf((p->sg.rms-nf*p->sg.avg*p->sg.avg)/nf);

p->sb.avg=p->sb.avg/nf;
p->sb.rms=sqrtf((p->sb.rms-nf*p->sb.avg*p->sb.avg)/nf);

p->sa.avg=p->sa.avg/nf;
p->sa.rms=sqrtf((p->sa.rms-nf*p->sa.avg*p->sa.avg)/nf);

p->sy.avg=p->sy.avg/nf;
p->sy.rms=sqrtf((p->sy.rms-nf*p->sy.avg*p->sy.avg)/nf);

p->su.avg=p->su.avg/nf;
p->su.rms=sqrtf((p->su.rms-nf*p->su.avg*p->su.avg)/nf);

p->sv.avg=p->sv.avg/nf;
p->sv.rms=sqrtf((p->sv.rms-nf*p->sv.avg*p->sv.avg)/nf);

}

