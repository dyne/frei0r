/*
test_pat_R
This frei0r plugin generates resolution test patterns
Version 0.1	may 2010

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

/***********************************************************
Test patterns: Resolution and spatial frequency response

The patterns are drawn into a temporary float array, for two reasons:
1. drawing routines are color model independent,
2. drawing is done only when a parameter changes.

only the function float2color()
needs to care about color models, endianness, DV legality etc.

*************************************************************/

//compile:	gcc -Wall -std=c99 -c -fPIC test_pat_R.c -o test_pat_R.o

//link: gcc -lm -shared -o test_pat_R.so test_pat_R.o

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include "frei0r.h"



double PI=3.14159265358979;

typedef struct
	{
	float r;
	float g;
	float b;
	float a;
	} float_rgba;


//----------------------------------------------------------
void draw_rectangle(float *sl, int w, int h, int x, int y, int wr, int hr, float gray)
{
int i,j;
int zx,kx,zy,ky;

zx=x;  if (zx<0) zx=0;
zy=y;  if (zy<0) zy=0;
kx=x+wr;  if (kx>w) kx=w;
ky=y+hr;  if (ky>h) ky=h;
for (i=zy;i<ky;i++)
	for (j=zx;j<kx;j++)
		sl[w*i+j]=gray;

}

//-------------------------------------------------------
//draw one numerical digit, 7-segment style
//v=size in x direction (in y it is 2*v)
//d= number [0...9]
void disp7s(float *sl, int w, int h, int x, int y, int v, int d, float gray)
{
char seg[10]={0xEE,0x24,0xBA,0xB6,0x74,0xD6,0xDE,0xA4,0xFE,0xF6};

if ((d<0)||(d>9)) return;

if ((seg[d]&0x80)!=0) draw_rectangle(sl,w,h,x,  y-2*v,v,1,gray);
if ((seg[d]&0x40)!=0) draw_rectangle(sl,w,h,x,  y-2*v,1,v,gray);
if ((seg[d]&0x20)!=0) draw_rectangle(sl,w,h,x+v,y-2*v,1,v,gray);
if ((seg[d]&0x10)!=0) draw_rectangle(sl,w,h,x,  y-v,  v,1,gray);
if ((seg[d]&0x08)!=0) draw_rectangle(sl,w,h,x,  y-v,  1,v,gray);
if ((seg[d]&0x04)!=0) draw_rectangle(sl,w,h,x+v,y-v,  1,v,gray);
if ((seg[d]&0x02)!=0) draw_rectangle(sl,w,h,x  ,y,    v,1,gray);
}

//-----------------------------------------------------------------
//draw a floating point number, using disp7s()
//v=size
//n=number
//f=format (as in printf, for example %5.1f)
void dispF(float *sl, int w, int h, int x, int y, int v, float n, char *f, float gray)
{
char str[64];
int i;

sprintf(str,f,n);
i=0;
while (str[i]!=0)
	{
	if (str[i]=='-')
		draw_rectangle(sl,w,h,x+i*(v+v/3+1),y-v,v,1,gray);
	else
		disp7s(sl,w,h,x+i*(v+v/3+1),y,v,str[i]-48,gray);
	i++;
	}

}

//-----------------------------------------------------------
//sweep parallel with bars
//x,y,wr,hr same as in draw_rectangle()
//f1, f2 as fraction of Nyquist
//a = amplitude,     1.0=100% mod [0.0...1.0]
//dir:		0=vertical  1=horizontal    sweep
//linp:   0=linear frequency sweep   1=linear period sweep
void draw_sweep_1(float *sl, int w, int h, int x, int y, int wr, int hr, float f1, float f2, float a, int dir, int linp)
{
int i,j;
int zx,kx,zy,ky;
double p,dp,dp1,dp2,dt,dt1,dt2;

zx=x;  if (zx<0) zx=0;
zy=y;  if (zy<0) zy=0;
kx=x+wr;  if (kx>w) kx=w;
ky=y+hr;  if (ky>h) ky=h;

dp1=PI*f1; dp2=PI*f2;  //phase steps
dt1=1.0/dp1; dt2=1.0/dp2;
a=a/2.0;

p=0;
if (dir==0)
	{
	for (i=zy;i<ky;i++)
		{
		if (linp==0)
			dp=dp1+(dp2-dp1)*(double)(i-zy)/(double)(ky-zy);
		else
			{
			dt=dt1+(dt2-dt1)*(double)(i-zy)/(double)(ky-zy);
			dp=1.0/dt;
			}
		//zacetna faza tako, da bo v sredini 0
		p=-(double)wr/2.0*dp;
		for (j=zx;j<kx;j++)
			{
			sl[w*i+j]=0.5+a*cos(p);
			p=p+dp;
			}
		}
	}
else
	{
	for (j=zx;j<kx;j++)
		{
		if (linp==0)
			dp=dp1+(dp2-dp1)*(double)(j-zx)/(double)(kx-zx);
		else
			{
			dt=dt1+(dt2-dt1)*(double)(j-zy)/(double)(kx-zx);
			dp=1.0/dt;
			}
		//zacetna faza tako, da bo v sredini 0
		p=-(double)hr/2.0*dp;
		for (i=zy;i<ky;i++)
			{
			sl[w*i+j]=0.5+a*cos(p);
			p=p+dp;
			}
		}
	}
}

//-----------------------------------------------------------
//sweep perpendicular to bars
//x,y,wr,hr same as in draw_rectangle()
//f1, f2 as fraction of Nyquist
//a = amplitude,     1.0=100% mod [0.0...1.0]
//dir:		0=vertical  1=horizontal    sweep
//linp:   0=linear frequency sweep   1=linear period sweep
void draw_sweep_2(float *sl, int w, int h, int x, int y, int wr, int hr, float f1, float f2, float a, int dir, int linp)
{
int i,j;
int zx,kx,zy,ky;
double p,dp,dp1,dp2,dt,dt1,dt2;
float s;

zx=x;  if (zx<0) zx=0;
zy=y;  if (zy<0) zy=0;
kx=x+wr;  if (kx>w) kx=w;
ky=y+hr;  if (ky>h) ky=h;

dp1=PI*f1; dp2=PI*f2;  //phase steps
dt1=1.0/dp1; dt2=1.0/dp2;
a=a/2.0;

p=0;
if (dir==0)
	{
	for (i=zy;i<ky;i++)
		{
		if (linp==0)
			dp=dp1+(dp2-dp1)*(double)(i-zy)/(double)(ky-zy);
		else
			{
			dt=dt1+(dt2-dt1)*(double)(i-zy)/(double)(ky-zy);
			dp=1.0/dt;
			}
		p=p+dp;
		s=0.5+a*cos(p);
		for (j=zx;j<kx;j++)
			sl[w*i+j]=s;
		}
	}
else
	{
	for (j=zx;j<kx;j++)
		{
		if (linp==0)
			dp=dp1+(dp2-dp1)*(double)(j-zx)/(double)(kx-zx);
		else
			{
			dt=dt1+(dt2-dt1)*(double)(j-zy)/(double)(kx-zx);
			dp=1.0/dt;
			}
		p=p+dp;
		s=0.5+a*cos(p);
		for (i=zy;i<ky;i++)
			sl[w*i+j]=s;
		}
	}
}

//--------------------------------------------------------------
//vertical sweep with labels (frequency changes with y)
//a=axis	0=horizontal frequencies   1=vert freqs
//amp = amplitude
//lps:  0=lin f sweep    1=lin p sweep
//ar = pixel aspect ratio (used only for LPPH labels on hor f)
//sf,ef		start,end freqs in Nyquists
//LPPH (lines per picture height) labels are in "TV lines",
//not line pairs. Line pairs = TV lines / 2.0
//lf* arrays determine where the labels will be drawn
void sweep_v(float *sl, int w, int h, int a, float amp, int lps, float ar, float sf, float ef)
{
float xl,nf;
float lf1[]={0.05,0.1,0.2,0.3,0.4,0.5,0.6,0.7}; //label lin f  nyq
float lf2[]={0.05,0.07,0.1,0.15,0.3,0.7};    //label lin p nyq
float lf3[]={100.0,200.0,300.0,400.0,500.0,600.0,700.0,800.0,900.0};
float lf4[]={10.0,25.0,50.0,100.0,200.0,400.0,800.0};
int i,x,y;

for (x=0;x<w*h;x++) sl[x]=0.0;	//black background

if (a==0)
	draw_sweep_1(sl,w,h,w/8,h/16,6*w/8,14*h/16, sf, ef, amp, 0, lps);
else
	draw_sweep_2(sl,w,h,w/8,h/16,6*w/8,14*h/16, sf, ef, amp, 0, lps);

//labels
if (lps==0)	//lin freq sweep
	{
	for (i=0;i<sizeof(lf1)/sizeof(float);i++)	//Nyquist
		{
		xl=(lf1[i]-sf)/(ef-sf);
		if ((xl>=0.0)&&(xl<=1.0))
			{
			x=w/8-60;
			y=h/16+xl*(14*h/16);
			draw_rectangle(sl,w,h,w/8-15,y,10,3,0.9);
			dispF(sl,w,h,x,y+6,6,lf1[i],"%5.2f",0.9);
			}
		}
	for (i=0;i<sizeof(lf3)/sizeof(float);i++)	//LPPH
		{
		nf=lf3[i]/(float)h;	//lpph to nyquist
		if (a==0) nf=nf*ar;
		xl=(nf-sf)/(ef-sf);
		if ((xl>=0.0)&&(xl<=1.0))
			{
			x=7*w/8+10;
			y=h/16+xl*(14*h/16);
			draw_rectangle(sl,w,h,7*w/8+5,y,10,3,0.9);
			dispF(sl,w,h,x,y+6,6,lf3[i],"%4.0f",0.9);
			}
		}
	}
else	//lin period sweep
	{
	for (i=0;i<sizeof(lf2)/sizeof(float);i++)	//Nyquist
		{
		xl=(1.0/lf2[i]-1.0/sf)/(1.0/ef-1.0/sf);
		if ((xl>=0.0)&&(xl<=1.0))
			{
			x=w/8-60;
			y=h/16+xl*(14*h/16);
			draw_rectangle(sl,w,h,w/8-15,y,10,3,0.9);
			dispF(sl,w,h,x,y+6,6,lf2[i],"%5.2f",0.9);
			}
		}
	for (i=0;i<sizeof(lf4)/sizeof(float);i++)	//LPPH
		{
		nf=lf4[i]/(float)h;	//lpph to nyquist
		if (a==0) nf=nf*ar;
		xl=(1.0/nf-1.0/sf)/(1.0/ef-1.0/sf);
		if ((xl>=0.0)&&(xl<=1.0))
			{
			x=7*w/8+10;
			y=h/16+xl*(14*h/16);
			draw_rectangle(sl,w,h,7*w/8+5,y,10,3,0.9);
			dispF(sl,w,h,x,y+6,6,lf4[i],"%4.0f",0.9);
			}
		}
	}
}

//--------------------------------------------------------------
//horizontal sweep with labels (frequency changes with x)
//a=axis	0=horizontal frequencies   1=vert freqs
//amp = amplitude
//lps:  0=lin f sweep    1=lin p sweep
//ar = pixel aspect ratio (used only for LPPH labels on hor f)
//sf,ef		start,end freqs in Nyquists
//LPPH (lines per picture height) labels are in "TV lines",
//not line pairs. Line pairs = TV lines / 2.0
//lf* arrays determine where the labels will be drawn
void sweep_h(float *sl, int w, int h, int a, float amp, int lps, float ar, float sf, float ef)
{
float xl,nf;
float lf1[]={0.05,0.2,0.3,0.4,0.5,0.6,0.7};	 //label lin f nyq
float lf2[]={0.05,0.07,0.1,0.15,0.3,0.7};   //label lin p nyq
float lf3[]={100.0,200.0,300.0,400.0,500.0,600.0,700.0,800.0,900.0};
float lf4[]={10.0,25.0,50.0,100.0,200.0,400.0,800.0};
int i,x,y;

for (x=0;x<w*h;x++) sl[x]=0.0;	//black background

if (a==0)
	draw_sweep_2(sl,w,h,w/16,h/8,14*w/16,6*h/8, sf, ef, amp, 1, lps);
else
	draw_sweep_1(sl,w,h,w/16,h/8,14*w/16,6*h/8, sf, ef, amp, 1, lps);

//labels
if (lps==0)	//lin freq sweep
	{
	for (i=0;i<sizeof(lf1)/sizeof(float);i++)	//Nyquist
		{
		xl=(lf1[i]-sf)/(ef-sf);
		if ((xl>=0.0)&&(xl<=1.0))
			{
			y=7*h/8+25;
			x=w/16+xl*(14*w/16);
			draw_rectangle(sl,w,h,x,y-20,3,10,0.9);
			dispF(sl,w,h,x-20,y+6,6,lf1[i],"%5.2f",0.9);
			}
		}
	for (i=0;i<sizeof(lf3)/sizeof(float);i++)	//LPPH
		{
		nf=lf3[i]/(float)h;	//lpph to nyquist
		if (a==0) nf=nf*ar;
		xl=(nf-sf)/(ef-sf);
		if ((xl>=0.0)&&(xl<=1.0))
			{
			y=h/8-25;
			x=w/16+xl*(14*w/16);
			draw_rectangle(sl,w,h,x,y+8,3,10,0.9);
			dispF(sl,w,h,x-20,y+2,6,lf3[i],"%4.0f",0.9);
			}
		}
	}
else	//lin period sweep
	{
	for (i=0;i<sizeof(lf2)/sizeof(float);i++)	//Nyquist
		{
		xl=(1.0/lf2[i]-1.0/sf)/(1.0/ef-1.0/sf);
		if ((xl>=0.0)&&(xl<=1.0))
			{
			y=7*h/8+25;
			x=w/16+xl*(14*w/16);
			draw_rectangle(sl,w,h,x,y-20,3,10,0.9);
			dispF(sl,w,h,x-20,y+6,6,lf2[i],"%5.2f",0.9);
			}
		}
	for (i=0;i<sizeof(lf4)/sizeof(float);i++)	//LPPH
		{
		nf=lf4[i]/(float)h;	//lpph to nyquist
		if (a==0) nf=nf*ar;
		xl=(1.0/nf-1.0/sf)/(1.0/ef-1.0/sf);
		if ((xl>=0.0)&&(xl<=1.0))
			{
			y=h/8-25;
			x=w/16+xl*(14*w/16);
			draw_rectangle(sl,w,h,x,y+8,3,10,0.9);
			dispF(sl,w,h,x-20,y+2,6,lf4[i],"%4.0f",0.9);
			}
		}
	}
}

//----------------------------------------------------------
//draws a "Siemens star" pattern
//ar = pixel aspect ratio (not used currently)
//np = numbers of periods around the circle
void radials(float *sl, int w, int h, float a, float ar, float np)
{
float an,s,c,g,da,r,rmin,rmax;
int x,y;

da=PI/2000.0;

for (x=0;x<w*h;x++) sl[x]=0.5;	//gray background

a=a/2.0;
rmin=np/0.7/2.0/PI;	//inner edge at 0.7 Nyquist
rmax=(float)h/2.4;
for (an=0.0; an<2.0*PI; an=an+da)
	{
	g=0.5+a*cosf(np*an);
	c=cosf(an); s=sinf(an);
	for (r=rmin; r<rmax; r=r+1.0)
		{
		x=w/2+r*c;
		y=h/2+r*s;
		sl[y*w+x]=g;
		}
	}

}

//----------------------------------------------------------
//dir: 0=LF inside
//linp==1  lin period sweep
//sf,ef		start,end freqs in Nyquists
//ar = pixel aspect ratio (not used currently)
void rings(float *sl, int w, int h, float a, float ar, int linp, float sf, float ef)
{
float k,m,g,p,da,r,rmax;
int x,y;

da=PI/2000.0;

a=a/2.0;
rmax=(float)h/2.1;
if (linp==0)
	{
	m=sf*PI;
	k=0.5*(ef-sf)*PI/rmax;

	p=(k*rmax+m)*rmax;
	g=0.5+a*cosf(p);
	for (x=0;x<w*h;x++) sl[x]=g; //match background to outer rim

	for (x=-rmax;x<rmax;x++)
		for (y=-rmax;y<rmax;y++)
			{
			r=sqrtf(x*x+y*y);
			if (r<rmax)
				{
				p=(k*r+m)*r;
				g=0.5+a*cosf(p);
				sl[(y+h/2)*w+x+w/2]=g;
				}
			}
	}
else
	{
	m=1.0/sf;
	k=(1.0/ef-1.0/sf)/rmax;

	p=PI/k*logf(fabsf(m+k*rmax));
	g=0.5+a*cosf(p);
	for (x=0;x<w*h;x++) sl[x]=g; //match background to outer rim

	for (x=-rmax;x<rmax;x++)
		for (y=-rmax;y<rmax;y++)
			{
			r=sqrtf(x*x+y*y);
			if (r<rmax)
				{
				p=PI/k*logf(fabsf(m+k*r));
				g=0.5+a*cosf(p);
				sl[(y+h/2)*w+x+w/2]=g;
				}
			}
	}

}

//----------------------------------------------------------
//fills frame with constand 2D spatial frequency
//ar = pixel aspect ratio (not used currently)
void diags(float *sl, int w, int h, float a, float ar, float fh, float fv)
{
int x,y;
float p1,p;

a=a/2.0;
p1=0;
for (y=0;y<h;y++)
	{
	p=p1;
	for (x=0;x<w;x++)
		{
		p=p+PI*fh;
		sl[y*w+x]=0.5+a*cosf(p);
		}
	p1=p1+PI*fv;
	}
}

//-----------------------------------------------------------
//Nyquist blocks (horizontal, checkerboard and vertical)
//  N and N/2 square wave
//a = amplitude
void nblocks(float *sl, int w, int h, float a)
{
int x,y;
float g1,g2;

for (x=0;x<w*h;x++) sl[x]=0.5;	//gray background

g1=(1.0+a)/2.0; g2=(1.0-a)/2.0;
for (y=h/7;y<3*h/7;y++)
	{
	for (x=w/13;x<4*w/13;x++)
		sl[y*w+x]=(y%2==0)?g1:g2;
	for (x=5*w/13;x<8*w/13;x++)
		sl[y*w+x]=((x+y)%2==0)?g1:g2;
	for (x=9*w/13;x<12*w/13;x++)
		sl[y*w+x]=(x%2==0)?g1:g2;
	}
for (y=4*h/7;y<6*h/7;y++)
	{
	for (x=w/13;x<4*w/13;x++)
		sl[y*w+x]=((y/2)%2==0)?g1:g2;
	for (x=5*w/13;x<8*w/13;x++)
		sl[y*w+x]=((x/2+y/2)%2==0)?g1:g2;
	for (x=9*w/13;x<12*w/13;x++)
		sl[y*w+x]=((x/2)%2==0)?g1:g2;
	}
}

//---------------------------------------------------------
//square wave bars at integer fractions of Nyquist
void sqbars(float *sl, int w, int h, float a)
{
int x,y;
float g1,g2;

for (x=0;x<w*h;x++) sl[x]=0.5;	//gray background

g1=(1.0+a)/2.0; g2=(1.0-a)/2.0;

//horizontals
for (y=h/5;y<2*h/5;y++)
	{
	for (x=w/10;x<2*w/10-w/100;x++)		//Nyquist
		if (x%2==0) sl[y*w+x]=g1; else sl[y*w+x]=g2;
	for (x=2*w/10;x<3*w/10-w/100;x++)	//Nyquist/2
		if ((x/2)%2==0) sl[y*w+x]=g1; else sl[y*w+x]=g2;
	for (x=3*w/10;x<4*w/10-w/100;x++)	//Nyquist/3
		if ((x/3)%2==0) sl[y*w+x]=g1; else sl[y*w+x]=g2;
	for (x=4*w/10;x<5*w/10-w/100;x++)	//Nyquist/4
		if ((x/4)%2==0) sl[y*w+x]=g1; else sl[y*w+x]=g2;
	for (x=5*w/10;x<6*w/10-w/100;x++)	//Nyquist/5
		if ((x/5)%2==0) sl[y*w+x]=g1; else sl[y*w+x]=g2;
	for (x=6*w/10;x<7*w/10-w/100;x++)	//Nyquist/6
		if ((x/6)%2==0) sl[y*w+x]=g1; else sl[y*w+x]=g2;
	for (x=7*w/10;x<8*w/10-w/100;x++)	//Nyquist/7
		if ((x/7)%2==0) sl[y*w+x]=g1; else sl[y*w+x]=g2;
	for (x=8*w/10;x<9*w/10-w/100;x++)	//Nyquist/8
		if ((x/8)%2==0) sl[y*w+x]=g1; else sl[y*w+x]=g2;
	}

//verticals
for (y=3*h/5;y<4*h/5;y++)
	{
	for (x=w/10;x<2*w/10-w/100;x++)		//Nyquist
		if (y%2==0) sl[y*w+x]=g1; else sl[y*w+x]=g2;
	for (x=2*w/10;x<3*w/10-w/100;x++)	//Nyquist/2
		if ((y/2)%2==0) sl[y*w+x]=g1; else sl[y*w+x]=g2;
	for (x=3*w/10;x<4*w/10-w/100;x++)	//Nyquist/3
		if ((y/3)%2==0) sl[y*w+x]=g1; else sl[y*w+x]=g2;
	for (x=4*w/10;x<5*w/10-w/100;x++)	//Nyquist/4
		if ((y/4)%2==0) sl[y*w+x]=g1; else sl[y*w+x]=g2;
	for (x=5*w/10;x<6*w/10-w/100;x++)	//Nyquist/5
		if ((y/5)%2==0) sl[y*w+x]=g1; else sl[y*w+x]=g2;
	for (x=6*w/10;x<7*w/10-w/100;x++)	//Nyquist/6
		if ((y/6)%2==0) sl[y*w+x]=g1; else sl[y*w+x]=g2;
	for (x=7*w/10;x<8*w/10-w/100;x++)	//Nyquist/7
		if ((y/7)%2==0) sl[y*w+x]=g1; else sl[y*w+x]=g2;
	for (x=8*w/10;x<9*w/10-w/100;x++)	//Nyquist/8
		if ((y/8)%2==0) sl[y*w+x]=g1; else sl[y*w+x]=g2;
	}
}



//-----------------------------------------------------
//converts the internal monochrome float image into
//Frei0r rgba8888 color
//ch selects the channel   0=all  1=R  2=G  3=B
//sets alpha to opaque
void float2color(float *sl, uint32_t* outframe, int w , int h, int ch)
{
int i,ri,gi,bi;
uint32_t p;
float r,g,b;

switch (ch)
	{
	case 0:		//all (gray)
		for (i=0;i<w*h;i++)
			{
			p=(uint32_t)(255.0*sl[i]) & 0xFF;
			outframe[i] = (p<<16)+(p<<8)+p+0xFF000000;
			}
		break;
	case 1:		//R
		for (i=0;i<w*h;i++)
			{
			p=(uint32_t)(255.0*sl[i]) & 0xFF;
			outframe[i] = p+0xFF000000;
			}
		break;
	case 2:		//G
		for (i=0;i<w*h;i++)
			{
			p=(uint32_t)(255.0*sl[i]) & 0xFF;
			outframe[i] = (p<<8)+0xFF000000;
			}
		break;
	case 3:		//B
		for (i=0;i<w*h;i++)
			{
			p=(uint32_t)(255.0*sl[i]) & 0xFF;
			outframe[i] = (p<<16)+0xFF000000;
			}
		break;
	case 4:		//ccir rec 601  R-Y   on 50 gray
		for (i=0;i<w*h;i++)
			{
			r=sl[i];
			b=0.5;
			g=(0.5-0.299*r-0.114*b)/0.587;
			ri=(int)(255.0*r);
			gi=(int)(255.0*g);
			bi=(int)(255.0*b);
			outframe[i] = (bi<<16)+(gi<<8)+ri+0xFF000000;
			}
		break;
	case 5:		//ccir rec 601  B-Y   on 50% gray
		for (i=0;i<w*h;i++)
			{
			b=sl[i];
			r=0.5;
			g=(0.5-0.299*r-0.114*b)/0.587;
			ri=(int)(255.0*r);
			gi=(int)(255.0*g);
			bi=(int)(255.0*b);
			outframe[i] = (bi<<16)+(gi<<8)+ri+0xFF000000;
			}
		break;
	case 6:		//ccir rec 709  R-Y   on 50 gray
		for (i=0;i<w*h;i++)
			{
			r=sl[i];
			b=0.5;
			g=(0.5-0.2126*r-0.0722*b)/0.7152;
			ri=(int)(255.0*r);
			gi=(int)(255.0*g);
			bi=(int)(255.0*b);
			outframe[i] = (bi<<16)+(gi<<8)+ri+0xFF000000;
			}
		break;
	case 7:		//ccir rec 709  B-Y   on 50% gray
		for (i=0;i<w*h;i++)
			{
			b=sl[i];
			r=0.5;
			g=(0.5-0.2126*r-0.0722*b)/0.7152;
			ri=(int)(255.0*r);
			gi=(int)(255.0*g);
			bi=(int)(255.0*b);
			outframe[i] = (bi<<16)+(gi<<8)+ri+0xFF000000;
			}
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

//**************************************************
//obligatory frei0r stuff follows

//------------------------------------------------
//this structure holds an instance of the test_pat_R plugin
typedef struct
{
  unsigned int w;
  unsigned int h;

  int type;
  int chan;
  float amp;
  int linp;
  float f1;
  float f2;
  int aspt;
  float mpar;

  float par;
  float *sl;

} tp_inst_t;

//----------------------------------------------------
int f0r_init()
{
  return 1;
}

//--------------------------------------------------
void f0r_deinit()
{ /* no initialization required */ }

//--------------------------------------------------
void f0r_get_plugin_info(f0r_plugin_info_t* tp_info)
{
  tp_info->name           = "test_pat_R";
  tp_info->author         = "Marko Cebokli";
  tp_info->plugin_type    = F0R_PLUGIN_TYPE_SOURCE;
//  tp_info->plugin_type    = F0R_PLUGIN_TYPE_FILTER;
  tp_info->color_model    = F0R_COLOR_MODEL_RGBA8888;
  tp_info->frei0r_version = FREI0R_MAJOR_VERSION;
  tp_info->major_version  = 0;
  tp_info->minor_version  = 1;
  tp_info->num_params     = 8;
  tp_info->explanation    = "Generates resolution test patterns";
}

//--------------------------------------------------
void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
  switch (param_index)
    {
    case 0:
      info->name        = "Type";
      info->type        = F0R_PARAM_DOUBLE;
      info->explanation = "Type of test pattern"; break;
    case 1:
      info->name	="Channel";
      info->type	= F0R_PARAM_DOUBLE;
      info->explanation = "Into which color channel to draw";
      break;
    case 2:
      info->name	= "Amplitude";
      info->type	= F0R_PARAM_DOUBLE;
      info->explanation = "Amplitude (contrast) of the pattern";
      break;
    case 3:
      info->name        = "Lin P swp";
      info->type        = F0R_PARAM_BOOL;
      info->explanation = "Use linear period sweep";
      break;
    case 4:
      info->name	= "Freq 1";
      info->type	= F0R_PARAM_DOUBLE;
      info->explanation = "Pattern 7 H frequency";
      break;
    case 5:
      info->name	= "Freq 2";
      info->type	= F0R_PARAM_DOUBLE;
      info->explanation = "Pattern 7 V frequency";
      break;
    case 6:
      info->name	="Aspect type";
      info->type	= F0R_PARAM_DOUBLE;
      info->explanation = "Pixel aspect ratio presets";
      break;
    case 7:
      info->name	= "Manual aspect";
      info->type	= F0R_PARAM_DOUBLE;
      info->explanation = "Manual pixel aspect ratio";
      break;
    }
}

//--------------------------------------------------
f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
  tp_inst_t* inst = calloc(1, sizeof(*inst));
  inst->w  = width; 
  inst->h = height;

  inst->type=0;
  inst->chan=0;
  inst->amp=0.8;
  inst->linp=0;
  inst->f1=0.03;
  inst->f2=0.03;
  inst->aspt=0;
  inst->mpar=1.0;

  inst->par=1.0;
  inst->sl=(float*)calloc(width*height,sizeof(float));

  sweep_v(inst->sl, inst->w, inst->h, 0, inst->amp, inst->linp, inst->par, 0.05, 0.7);

  return (f0r_instance_t)inst;
}

//--------------------------------------------------
void f0r_destruct(f0r_instance_t instance)
{
  tp_inst_t* inst = (tp_inst_t*)instance;

  free(inst->sl);
  free(inst);
}

//--------------------------------------------------
void f0r_set_param_value(f0r_instance_t instance, f0r_param_t param, int param_index)
{
  tp_inst_t* inst = (tp_inst_t*)instance;

  f0r_param_double* p = (f0r_param_double*) param;

  int chg,tmpi;
  float tmpf;

  chg=0;
  switch (param_index)
    {
    case 0:	//type
      tmpf=*((double*)p);
      if (tmpf>=1.0)
        tmpi=(int)tmpf;
      else
        tmpi = map_value_forward(tmpf, 0.0, 9.9999);
      if ((tmpi<0)||(tmpi>9.0)) break;
      if (inst->type != tmpi) chg=1;
      inst->type = tmpi;
      break;
    case 1:	//channel
      tmpf=*((double*)p);
      if (tmpf>=1.0)
        tmpi=(int)tmpf;
      else
        tmpi = map_value_forward(tmpf, 0.0, 7.9999);
      if ((tmpi<0)||(tmpi>7.0)) break;
      if (inst->chan != tmpi) chg=1;
      inst->chan = tmpi;
    case 2:	//amplitude
      tmpf = map_value_forward(*((double*)p), 0.0, 1.0);
      if (inst->amp != tmpf) chg=1;
      inst->amp = tmpf;
      break;
    case 3:	//linear period sweep
      tmpi = map_value_forward(*((double*)p), 0.0, 1.0);
      if (inst->linp != tmpi) chg=1;
      inst->linp = tmpi;
      break;
    case 4:	//frequency 1
      tmpf = map_value_forward(*((double*)p), 0.0, 1.0);
      if (inst->f1 != tmpf) chg=1;
      inst->f1 = tmpf;
      break;
    case 5:	//frequency 2
      tmpf = map_value_forward(*((double*)p), 0.0, 1.0);
      if (inst->f2 != tmpf) chg=1;
      inst->f2 = tmpf;
      break;
    case 6:	//aspect type
      tmpf=*((double*)p);
      if (tmpf>=1.0)
        tmpi=(int)tmpf;
      else
      tmpi = map_value_forward(tmpf, 0.0, 6.9999);
      if ((tmpi<0)||(tmpi>6.0)) break;
      if (inst->aspt != tmpi) chg=1;
      inst->aspt = tmpi;
      switch (inst->aspt)	//pixel aspect ratio
        {
        case 0: inst->par=1.000;break;		//square pixels
        case 1: inst->par=1.067;break;		//PAL DV
        case 2: inst->par=1.455;break;		//PAL wide
        case 3: inst->par=0.889;break;		//NTSC DV
        case 4: inst->par=1.212;break;		//NTSC wide
        case 5: inst->par=1.333;break;		//HDV
        case 6: inst->par=inst->mpar;break;	//manual
        }
      break;
    case 7:	//manual aspect
      tmpf = map_value_forward_log(*((double*)p), 0.5, 2.0);
      if (inst->mpar != tmpf) chg=1;
      inst->mpar = tmpf;
      if (inst->aspt==6) inst->par=inst->mpar;
      break;
    }

  if (chg==0) return;

  switch (inst->type)
    {
    case 0:		 //hor freq  ver sweep
      sweep_v(inst->sl, inst->w, inst->h, 0, inst->amp, inst->linp, inst->par, 0.05, 0.7);
      break;
    case 1:		 //hor freq  hor sweep
      sweep_h(inst->sl, inst->w, inst->h, 0, inst->amp, inst->linp, inst->par, 0.05, 0.7);
      break;
    case 2:		 //ver freq  ver sweep
      sweep_v(inst->sl, inst->w, inst->h, 1, inst->amp, inst->linp, inst->par, 0.05, 0.7); //ver f  ver sw
      break;
    case 3:		 //ver freq  hor sweep
      sweep_h(inst->sl, inst->w, inst->h, 1, inst->amp, inst->linp, inst->par, 0.05, 0.7);
      break;
    case 4:		//   "Siemens star"
      radials(inst->sl, inst->w, inst->h, inst->amp,  inst->par, 60.0);
      break;
    case 5:		//rings outwards
      rings(inst->sl, inst->w, inst->h, inst->amp,  inst->par, inst->linp, 0.05, 0.7);
      break;
    case 6:		//rings inwards
      rings(inst->sl, inst->w, inst->h, inst->amp,  inst->par, inst->linp, 0.7, 0.05);
      break;
    case 7:		//uniform 2D spatial frequency
      diags(inst->sl, inst->w, inst->h, inst->amp,  inst->par, inst->f1, inst->f2);
      break;
    case 8:		//   "Nyquist blocks"
      nblocks(inst->sl, inst->w, inst->h, inst->amp);
      break;
    case 9:		//square bars at integer Nyquist fractions
      sqbars(inst->sl, inst->w, inst->h, inst->amp);
      break;
    default:
      break;
    }

}

//-------------------------------------------------
void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index)
{
  tp_inst_t* inst = (tp_inst_t*)instance;

  f0r_param_double* p = (f0r_param_double*) param;

  switch (param_index)
    {
    case 0:	//type
      *p = map_value_backward(inst->type, 0.0, 9.9999);
      break;
    case 1:	//channel
      *p = map_value_backward(inst->chan, 0.0, 7.9999);
      break;
    case 2:	//amplitude
      *p = map_value_backward(inst->amp, 0.0, 1.0);
      break;
    case 3:	//linear period sweep
      *p = map_value_backward(inst->linp, 0.0, 1.0);
      break;
    case 4:	//frequency 1
      *p = map_value_backward(inst->f1, 0.0, 1.0);
      break;
    case 5:	//frequency 2
      *p = map_value_backward(inst->f2, 0.0, 1.0);
      break;
    case 6:	//aspect type
      *p = map_value_backward(inst->aspt, 0.0, 6.9999);
      break;
    case 7:	//manual aspect
      *p = map_value_backward_log(inst->mpar, 0.5, 2.0);
      break;
    }
}

//---------------------------------------------------
void f0r_update(f0r_instance_t instance, double time, const uint32_t* inframe, uint32_t* outframe)
{

  assert(instance);
  tp_inst_t* inst = (tp_inst_t*)instance;

  float2color(inst->sl, outframe, inst->w , inst->h, inst->chan);

}
