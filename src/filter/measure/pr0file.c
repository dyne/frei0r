/*
pr0file.c

This frei0r plugin ia an "2D video oscilloscope"
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

//compile: gcc -c -fPIC -Wall pr0file.c -o pr0file.o
//link: gcc -shared -o pr0file.so pr0file.o

#include <frei0r.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <assert.h>

#include "font2.h"
#include "measure.h"

double PI=3.14159265358979;

//---------------------------------------------------------------
void draw_rectangle(float_rgba *s, int w, int h, float x, float y, float wr, float hr, float_rgba c)
{
int i,j;
int zx,kx,zy,ky;

zx=x;  if (zx<0) zx=0;
zy=y;  if (zy<0) zy=0;
kx=x+wr;  if (kx>w) kx=w;
ky=y+hr;  if (ky>h) ky=h;
for (i=zy;i<ky;i++)
	for (j=zx;j<kx;j++)
		s[w*i+j]=c;

}

//---------------------------------------------------------------
//d=dim value   0.0=black   1.0=no dimming
void darken_rectangle(float_rgba *s, int w, int h, float x, float y, float wr, float hr, float d)
{
int i,j;
int zx,kx,zy,ky;

zx=x;  if (zx<0) zx=0;
zy=y;  if (zy<0) zy=0;
kx=x+wr;  if (kx>w) kx=w;
ky=y+hr;  if (ky>h) ky=h;
for (i=zy;i<ky;i++)
	for (j=zx;j<kx;j++)
		{
		s[w*i+j].r=d*s[w*i+j].r;
		s[w*i+j].g=d*s[w*i+j].g;
		s[w*i+j].b=d*s[w*i+j].b;
		}
}

//--------------------------------------------------------
//uses a 8x16 font from a .xbm image 32 char wide x 3 char high
void draw_char(float_rgba *sl, int w, int h, int x, int y, unsigned char c, float_rgba col)
{
int i,j,z;

if ((c<32)||(c>127)) return;
if (x<0) return;
if ((x+8)>=w) return;
if (y<0) return;
if ((y+16)>=h)return;

z=(c-32)%32+((c-32)/32)*512;	//position in font image

for (i=0;i<16;i++)
	for (j=0;j<8;j++)
		if ((font2_bits[z+32*i]&(1<<j))!=0)
			sl[(i+y)*w+j+x]=col;

}

//-----------------------------------------------------------
void draw_string(float_rgba *sl, int w, int h, int x, int y, char *c, float_rgba col)
{
int i;

i=0;
while (c[i]!=0)
	{
	draw_char(sl,w,h,x+8*i,y,c[i],col);
	i++;
	}
}

//--------------------------------------------------------
//generates format string for 6 chars wide number, right
//justified
//p=0 one decimal place    p=1 three decimal places
//m=1 always show sign
void forstr(float a, int p, int m, char *s)
{
float b;
char *p3=" %5.3f";
char *p3m="%+5.3f";
char *p1=" %5.1f";
char *p1m1=" %+4.1f";
char *p1m2="  %+3.1f";
char *ss;

b=fabsf(a);

if (p==1)
	{
	if (m==0) ss=p3; else ss=p3m;
	}
else
	{
	if (m==0)
		ss=p1;
	else
		{
		if (b<10.0) ss=p1m2;
		if ((b>=10.0)&&(b<100.0)) ss=p1m1;
		ss=p3m;
		}
	}
sprintf(s,"%s",ss);
}

//-------------------------------------------------------------
//draws a simple line (no antialiasing)
//xz,yz=start point
//xk,yk=end point
void draw_line(float_rgba *s, int w, int h, int xz, int yz, int xk, int yk, float_rgba c)
{
int x,y,d,i;

d =  (abs(xk-xz)>abs(yk-yz)) ? abs(xk-xz) : abs(yk-yz);
if (d==0) return;
for (i=0;i<d;i++)
  {
  x=xz+(float)i/d*(xk-xz);
  y=yz+(float)i/d*(yk-yz);
  if((x>=0)&&(x<w)&&(y>=0)&&(y<h))
    s[y*w+x]=c;
  }
}

//-------------------------------------------------------------
//mark position of the profile on the image
void pmarker(float_rgba *s, int w, int h, int xz, int yz, int xk, int yk, int sir, float_rgba c, float m1, float m2)
{
float dx,dy,dd;
float s2,s3,xm,ym;

dx=(float)(xk-xz);
dy=(float)(yk-yz);
dd=sqrtf(dx*dx+dy*dy);
if (dd==0.0) return;
dx=dx/dd;
dy=dy/dd;

s2=1.415;
s3=10.0;

//lower line
draw_line(s, w, h, xz-s2*dy, yz+s2*dx, xk-s2*dy, yk+s2*dx, c);
//upper line
draw_line(s, w, h, xz+s2*dy, yz-s2*dx, xk+s2*dy, yk-s2*dx, c);

//left end mark
draw_line(s, w, h, xz-s3*dy, yz+s3*dx, xz+s3*dy, yz-s3*dx, c);
//right end mark
draw_line(s, w, h, xk+s3*dy, yk-s3*dx, xk-s3*dy, yk+s3*dx, c);

//marker ticks
s2=2.5;
if (m1>0.0)
  {
  xm=xz+dd*dx*m1;
  ym=yz+dd*dy*m1;
  draw_line(s, w, h, xm+s2*dy, ym-s2*dx, xm+s3*dy, ym-s3*dx, c);
  draw_line(s, w, h, xm-s2*dy, ym+s2*dx, xm-s3*dy, ym+s3*dx, c);
  }
if (m2>0.0)
  {
  xm=xz+dd*dx*m2;
  ym=yz+dd*dy*m2;
  draw_line(s, w, h, xm+s2*dy, ym-s2*dx, xm+s3*dy, ym-s3*dx, c);
  draw_line(s, w, h, xm-s2*dy, ym+s2*dx, xm-s3*dy, ym+s3*dx, c);
  }
}

//--------------------------------------------------------
//select one of 8 colors for the crosshair
float_rgba mcolor(int c)
{
float_rgba wh={1.0,1.0,1.0,1.0};
float_rgba ye={1.0,1.0,0.0,1.0};
float_rgba cy={0.0,1.0,1.0,1.0};
float_rgba gr={0.0,1.0,0.0,1.0};
float_rgba mg={1.0,0.0,1.0,1.0};
float_rgba rd={1.0,0.0,0.0,1.0};
float_rgba bl={0.0,0.0,1.0,1.0};
float_rgba bk={0.0,0.0,0.0,1.0};

switch (c)
  {
  case 0: return wh;	//white
  case 1: return ye;	//yellow
  case 2: return cy;	//cyan
  case 3: return gr;	//green
  case 4: return mg;	//magenta
  case 5: return rd;	//red
  case 6: return bl;	//blue
  case 7: return bk;	//black
  default: return bk;	//black
  }
}

//--------------------------------------------------------
//graph p[], p[]+ofs should be between 0.0 and 1.0
void draw_trace(float_rgba *s, int w, int h, int x0, int y0, int vx, int vy, float p[], int n, float ofs, float_rgba c)
{
int i,x,y,xs,ys;

if (n==0) return;
xs=x0; ys=y0+vy*(1.0-p[0]-ofs);
for (i=0;i<n;i++)
  {
  x=x0+(i+1)*vx/n;
  if (x<0) x=0; if (x>=w) x=w-1;
  y=y0+(vy-1)*(1.0-p[i]-ofs)+1;
  if (y<y0) y=y0; if (y>=(y0+vy)) y=y0+vy-1; if (y>=h) y=h-1;
  draw_line(s, w, h, xs, ys, xs, y, c);
  draw_line(s, w, h, xs, y, x, y, c);
  xs=x; ys=y;
  }
}

//-------------------------------------------------------------
//numeric display below the "oscilloscope"
//m=which channel to display (one of seven)
//dit=what data to display (display items flags)
//m1,m2 marker positions as indexes into p.x arrays
//output is written into string *str
void izpis(profdata p, char *str, int m, int u, int m1, int m2, int dit)
{
int i;
char fs[256],frs[16];
float data[8];

for (i=0;i<8;i++) data[i]=0;

switch (m>>24)	//select channel  (r,g,b....)  & copy data
  {
  case 0:	//display nothing
    return;
  case 1:	//display R channel
    data[0]=p.r[m1]; data[1]=p.r[m2]; data[2]=data[1]-data[0];
    data[3]=p.sr.avg; data[4]=p.sr.rms; data[5]=p.sr.min;
    data[6]=p.sr.max;
    break;
  case 2:	//display G channel
    data[0]=p.g[m1]; data[1]=p.g[m2]; data[2]=data[1]-data[0];
    data[3]=p.sg.avg; data[4]=p.sg.rms; data[5]=p.sg.min;
    data[6]=p.sg.max;
    break;
  case 3:	//display B channel
    data[0]=p.b[m1]; data[1]=p.b[m2]; data[2]=data[1]-data[0];
    data[3]=p.sb.avg; data[4]=p.sb.rms; data[5]=p.sb.min;
    data[6]=p.sb.max;
    break;
  case 4:	//display Y channel
    data[0]=p.y[m1]; data[1]=p.y[m2]; data[2]=data[1]-data[0];
    data[3]=p.sy.avg; data[4]=p.sy.rms; data[5]=p.sy.min;
    data[6]=p.sy.max;
    break;
  case 5:	//display Pr channel
    data[0]=p.u[m1]; data[1]=p.u[m2]; data[2]=data[1]-data[0];
    data[3]=p.su.avg; data[4]=p.su.rms; data[5]=p.su.min;
    data[6]=p.su.max;
    break;
  case 6:	//display Pb channel
    data[0]=p.v[m1]; data[1]=p.v[m2]; data[2]=data[1]-data[0];
    data[3]=p.sv.avg; data[4]=p.sv.rms; data[5]=p.sv.min;
    data[6]=p.sv.max;
    break;
  case 7:	//display alpha channel
    data[0]=p.a[m1]; data[1]=p.a[m2]; data[2]=data[1]-data[0];
    data[3]=p.sa.avg; data[4]=p.sa.rms; data[5]=p.sa.min;
    data[6]=p.sa.max;
    break;
  default:
    break;
  }

if (u!=0) for (i=0;i<8;i++) data[i]=data[i]*255.0;

for (i=0;i<256;i++) {fs[i]=0; str[i]=0;}
if ((dit&0x00000001)!=0)	//marker 1 value
  {
  if (m1>=0)
    {
    forstr(data[0],1-u,0,frs);
    sprintf(fs,"%%s Mk1=%s", frs);
    sprintf(str,fs,str,data[0]);
    }
  else
    sprintf(str,"%s %s",str,"Mk1= -----");
  }
if ((dit&0x00000004)!=0)	//marker 2 value
  {
  if (m2>=0)
    {
    forstr(data[1],1-u,0,frs);
    sprintf(fs,"%%s Mk2=%s", frs);
    sprintf(str,fs,str,data[1]);
    }
  else
    sprintf(str,"%s %s",str,"Mk2= -----");
  }
if ((dit&0x00000010)!=0)	//difference marker2-marker1
  {
  if ((m2>=0)&&(m1>=0))
    {
    forstr(data[2],1-u,0,frs);
    sprintf(fs,"%%s D=%s", frs);
    sprintf(str,fs,str,data[2]);
    }
  else
    sprintf(str,"%s %s",str,"D= -----");
  }
if ((dit&0x00000020)!=0)	//average of profile
  {
  forstr(data[3],1-u,0,frs);
  sprintf(fs,"%%s Avg=%s", frs);
  sprintf(str,fs,str,data[3]);
  }
if ((dit&0x00000040)!=0)	//RMS of profile
  {
  forstr(data[4],1-u,0,frs);
  sprintf(fs,"%%s RMS=%s", frs);
  sprintf(str,fs,str,data[4]);
  }
if ((dit&0x00000080)!=0)	//MIN of profile
  {
  forstr(data[5],1-u,0,frs);
  sprintf(fs,"%%s Min=%s", frs);
  sprintf(str,fs,str,data[5]);
  }
if ((dit&0x00000100)!=0)	//MAX of profile
  {
  forstr(data[6],1-u,0,frs);
  sprintf(fs,"%%s Max=%s", frs);
  sprintf(str,fs,str,data[6]);
  }
}

//--------------------------------------------------------------
//draw info window
//sx,sy=size of probe   (must be odd)
//poz=position of info window	0=left   1=right
//m=measurement channel, trace/numeric display
//u=units    0=0.0-1.0    1=0-255
//as = auto scale
//m1,m2=marker positions
//dit=display items flags
//cc=crosshair color [0...7]
//cm=0 rec 601, cm=1 rec 709
void prof(float_rgba *s, int w, int h, int *poz, int x, int y, float tilt, int len, int sir, int m, int u, int as, int m1, int m2, int dit, int cc, int cm, profdata *p)
{
int x0,y0,vx,vy;
int xz,xk,yz,yk;	//zacetna in koncna tocka
char string[256];
int i,sl;
float_rgba white={1.0,1.0,1.0,1.0};
float_rgba lgray={0.7,0.7,0.7,1.0};
float_rgba gray={0.5,0.5,0.5,1.0};
float_rgba dgray={0.3,0.3,0.3,1.0};
float_rgba red={1.0,0.0,0.0,1.0};
float_rgba dgreen={0.0,0.7,0.0,1.0};
float_rgba lblue={0.3,0.3,1.0,1.0};
float_rgba yellow={0.7,0.7,0.0,1.0};
float_rgba pink={0.8,0.4,0.5,1.0};
float_rgba magenta={0.8,0.0,0.8,1.0};
float_rgba cyan={0.0,0.7,0.8,1.0};

//position and size of info window
if (y<h/2-20) *poz=1;	//bottom
if (y>h/2+20) *poz=0;	//top
x0=h/20;
vx=w*15/16;
vy = h*6/16; 
y0 = (*poz==0) ? h/20 : h-h/20-vy;

//end points of profile
xz=x-len/2.0*cosf(tilt);
xk=x+len/2.0*cosf(tilt);
yz=y-len/2.0*sinf(tilt);
yk=y+len/2.0*sinf(tilt);

//measure
meriprof(s, w, h, xz, yz, xk, yk, sir, p);
prof_yuv(p,cm);
prof_stat(p);

//draw crosshair
pmarker(s, w, h, xz, yz, xk, yk, sir, mcolor(cc), (float)m1/p->n, (float)m2/p->n);

//info window background
darken_rectangle(s, w, h, x0, y0, vx, vy, 0.4);

//draw scope
//background
//draw_rectangle(s, w, h, x0+50, y0+5, vx-55, vy-40, black);
//grid
yz=y0+6; yk=y0+vy-36;
for (i=0;i<9;i++)
  {
  xz=x0+49+(i+1)*(vx-55)/10;
  draw_line(s, w, h, xz, yz, xz, yk, dgray);
  }
xz=x0+50;xk=x0+vx-6;
for (i=0;i<3;i++)
  {
  yz=y0+5+(i+1)*(vy-40)/4;
  draw_line(s, w, h, xz, yz, xk, yz, dgray);
  }
//traces
if ((m&0x00000001)!=0)		//R
  draw_trace(s, w, h, x0+50, y0+5, vx-55, vy-40, p->r, p->n, 0.0, red);
if ((m&0x00000002)!=0)		//G
  draw_trace(s, w, h, x0+50, y0+5, vx-55, vy-40, p->g, p->n, 0.0, dgreen);
if ((m&0x00000004)!=0)		//B
  draw_trace(s, w, h, x0+50, y0+5, vx-55, vy-40, p->b, p->n, 0.0, lblue);
if ((m&0x00000008)!=0)		//Y
  draw_trace(s, w, h, x0+50, y0+5, vx-55, vy-40, p->y, p->n, 0.0, lgray);
if ((m&0x00000010)!=0)		//Pr
  draw_trace(s, w, h, x0+50, y0+5, vx-55, vy-40, p->u, p->n, 0.5, magenta);
if ((m&0x00000020)!=0)		//Pb
  draw_trace(s, w, h, x0+50, y0+5, vx-55, vy-40, p->v, p->n, 0.5, cyan);
if ((m&0x00000040)!=0)		//alpha
  draw_trace(s, w, h, x0+50, y0+5, vx-55, vy-40, p->a, p->n, 0.0, gray);
//markers
if ((m1>=0)&&(m1<p->n))
  {
  draw_line(s, w, h, x0+50+(m1+0.5)*(vx-55)/p->n, y0+5, x0+50+(m1+0.5)*(vx-55)/p->n, y0+vy-35, yellow);
  }
if ((m2>=0)&&(m2<p->n))
  {
  draw_line(s, w, h, x0+50+(m2+0.5)*(vx-55)/p->n, y0+5, x0+50+(m2+0.5)*(vx-55)/p->n, y0+vy-35, pink);
  }
//frame
draw_line(s, w, h, x0+49, y0+5, x0+vx-5, y0+5, gray);
draw_line(s, w, h, x0+49, y0+vy-35, x0+vx-5, y0+vy-35, gray);
draw_line(s, w, h, x0+49, y0+5, x0+49, y0+vy-35, gray);
draw_line(s, w, h, x0+vx-5, y0+5, x0+vx-5, y0+vy-35, gray);

//numeric display
izpis(*p,string,m,u,m1,m2,dit);
sl=strlen(string);
if (sl>((vx-55)/8))
  {
  sprintf(string,"<- NOT ENOUGH SPACE ->");
  draw_string(s, w, h, x0+vx/2-88, y0+vy-25, string, white);
  return;
  }
switch (m>>24)	//which channel data under the scope
  {
  case 0:
    break;
  case 1:
    draw_string(s, w, h, x0+20, y0+vy-25, "R", red);
    draw_string(s, w, h, x0+60, y0+vy-25, string, red);
    break;
  case 2:
    draw_string(s, w, h, x0+20, y0+vy-25, "G", dgreen);
    draw_string(s, w, h, x0+60, y0+vy-25, string, dgreen);
    break;
  case 3:
    draw_string(s, w, h, x0+20, y0+vy-25, "B", lblue);
    draw_string(s, w, h, x0+60, y0+vy-25, string, lblue);
    break;
  case 4:
    draw_string(s, w, h, x0+20, y0+vy-25, "Y", lgray);
    draw_string(s, w, h, x0+60, y0+vy-25, string, lgray);
    break;
  case 5:
    draw_string(s, w, h, x0+20, y0+vy-25, "Pr", magenta);
    draw_string(s, w, h, x0+60, y0+vy-25, string, magenta);
    break;
  case 6:
    draw_string(s, w, h, x0+20, y0+vy-25, "Pb", cyan);
    draw_string(s, w, h, x0+60, y0+vy-25, string, cyan);
    break;
  case 7:
    draw_string(s, w, h, x0+20, y0+vy-25, "a", gray);
    draw_string(s, w, h, x0+60, y0+vy-25, string, gray);
    break;
  default:
    break;
  }

}

//-----------------------------------------------------
//converts the internal RGBA float image into
//Frei0r rgba8888 color
void floatrgba2color(float_rgba *sl, uint32_t* outframe, int w , int h)
{
int i;
uint32_t p;

for (i=0;i<w*h;i++)
	{
	p=(uint32_t)(255.0*sl[i].a) & 0xFF;
	p=(p<<8) + ((uint32_t)(255.0*sl[i].b) & 0xFF);
	p=(p<<8) + ((uint32_t)(255.0*sl[i].g) & 0xFF);
	p=(p<<8) + ((uint32_t)(255.0*sl[i].r) & 0xFF);
	outframe[i]=p;
	}
}

//-----------------------------------------------------
//converts the Frei0r rgba8888 color image into
//internal float RGBA
void color2floatrgba(uint32_t* inframe, float_rgba *sl, int w , int h)
{
int i;

for (i=0;i<w*h;i++)
	{
	sl[i].r=((float)(inframe[i] & 0x000000FF))*0.00392157;
	sl[i].g=((float)((inframe[i] & 0x0000FF00)>>8))*0.00392157;
	sl[i].b=((float)((inframe[i] & 0x00FF0000)>>16))*0.00392157;
	sl[i].a=((float)((inframe[i] & 0xFF000000)>>24))*0.00392157;
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

//----------------------------------------
//struktura za instanco efekta
typedef struct
{
int h;
int w;

int x;		//horizontal position
int y;		//vertical position
float tilt;	//tilt of profile
int len;	//length of profile
int chn;	//channel for numeric display
int m1;		//marker 1 position along profile
int m2;		//marker 2 position along profile
int rt;		//show r trace		BOOL
int gt;		//show g trace		BOOL
int bt;		//show b trace		BOOL
int yt;		//show Y' trace		BOOL
int ut;		//show Pr trace		BOOL
int vt;		//show Pb trace		BOOL
int at;		//show alpha trace	BOOL
int davg;	//display average	BOOL
int drms;	//display rms		BOOL
int dmin;	//display minimum	BOOL
int dmax;	//display maximum	BOOL
int un;		//0...255 units		BOOL
int col;	//color, rec 601 or rec 709
int chc;	//crosshair color  [0...7]

int poz;
int mer;	//display channel + trace flags
int dit;	//numeric display items flags

float_rgba *sl;
profdata *p;

} inst;

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

info->name="pr0file";
info->author="Marko Cebokli";
info->plugin_type=F0R_PLUGIN_TYPE_FILTER;
info->color_model=F0R_COLOR_MODEL_RGBA8888;
info->frei0r_version=FREI0R_MAJOR_VERSION;
info->major_version=0;
info->minor_version=1;
info->num_params=21;
info->explanation="2D video oscilloscope";
}

//--------------------------------------------------
void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{

switch(param_index)
	{
	case 0:
		info->name = "X";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "X position of profile";
		break;
	case 1:
		info->name = "Y";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "Y position of profile";
		break;
	case 2:
		info->name = "Tilt";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "Tilt of profile";
		break;
	case 3:
		info->name = "Length";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "Length of profile";
		break;
	case 4:
		info->name = "Channel";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "Channel to numerically display";
		break;
	case 5:
		info->name = "Marker 1";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "Position of marker 1";
		break;
	case 6:
		info->name = "Marker 2";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "Position of marker 2";
		break;
	case 7:
		info->name = "R trace";
		info->type = F0R_PARAM_BOOL;
		info->explanation = "Show R trace on scope";
		break;
	case 8:
		info->name = "G trace";
		info->type = F0R_PARAM_BOOL;
		info->explanation = "Show G trace on scope";
		break;
	case 9:
		info->name = "B trace";
		info->type = F0R_PARAM_BOOL;
		info->explanation = "Show B trace on scope";
		break;
	case 10:
		info->name = "Y trace";
		info->type = F0R_PARAM_BOOL;
		info->explanation = "Show Y' trace on scope";
		break;
	case 11:
		info->name = "Pr trace";
		info->type = F0R_PARAM_BOOL;
		info->explanation = "Show Pr trace on scope";
		break;
	case 12:
		info->name = "Pb trace";
		info->type = F0R_PARAM_BOOL;
		info->explanation = "Show Pb trace on scope";
		break;
	case 13:
		info->name = "Alpha trace";
		info->type = F0R_PARAM_BOOL;
		info->explanation = "Show Alpha trace on scope";
		break;
	case 14:
		info->name = "Display average";
		info->type = F0R_PARAM_BOOL;
		info->explanation = "e";
		break;
	case 15:
		info->name = "Display RMS";
		info->type = F0R_PARAM_BOOL;
		info->explanation = "";
		break;
	case 16:
		info->name = "Display minimum";
		info->type = F0R_PARAM_BOOL;
		info->explanation = "";
		break;
	case 17:
		info->name = "Display maximum";
		info->type = F0R_PARAM_BOOL;
		info->explanation = "";
		break;
	case 18:
		info->name = "256 scale";
		info->type = F0R_PARAM_BOOL;
		info->explanation = "use 0-255 instead of 0.0-1.0";
		break;
	case 19:
		info->name = "Color";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "rec 601 or rec 709";
		break;
	case 20:
		info->name = "Crosshair color";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "Color of the profile marker";
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

in->x=width/2;
in->y=height/2;
in->tilt=0.0;
in->len=3*width/4;
in->chn=3;
in->m1=-1;
in->m2=-1;
in->rt=1;
in->gt=1;
in->bt=1;
in->yt=0;
in->ut=0;
in->vt=0;
in->at=0;
in->davg=1;
in->drms=1;
in->dmin=0;
in->dmax=0;
in->un=0;
in->col=0;
in->chc=0;

in->poz=1;
in->mer=(3<<24)+7;	//Y display + R,G,B traces
in->dit=32+64;		//avg+RMS

in->sl=(float_rgba*)calloc(width*height,sizeof(float_rgba));
in->p=(profdata*)calloc(1,sizeof(profdata));

in->p->n=5;

return (f0r_instance_t)in;
}

//---------------------------------------------------
void f0r_destruct(f0r_instance_t instance)
{
inst *in;

in=(inst*)instance;

free(in->sl);
free(in->p);
free(instance);
}

//-----------------------------------------------------
void f0r_set_param_value(f0r_instance_t instance, f0r_param_t parm, int param_index)
{
inst *p;
double tmpf;
int tmpi,chg;

p=(inst*)instance;

chg=0;
switch(param_index)
	{
	case 0:		//X
                tmpi=map_value_forward(*((double*)parm), 0.0, p->w);
		if (tmpi != p->x) chg=1;
		p->x=tmpi;
		break;
	case 1:		//Y
                tmpi=map_value_forward(*((double*)parm), 0.0, p->h);
		if (tmpi != p->y) chg=1;
		p->y=tmpi;
		break;
	case 2:		//tilt
                tmpf=map_value_forward(*((double*)parm), -PI/2.0, PI/2.0);
		if (tmpf != p->tilt) chg=1;
		p->tilt=tmpf;
		break;
	case 3:		//length
                tmpi=map_value_forward(*((double*)parm), 20.0, sqrtf(p->w*p->w+p->h*p->h));
		if (tmpi != p->len) chg=1;
		p->len=tmpi;
		break;
	case 4:		//channel
                tmpi=map_value_forward(*((double*)parm), 1.0, 7.9999);
		if (tmpi != p->chn) chg=1;
		p->chn=tmpi;
		break;
	case 5:		//marker 1
                tmpi=map_value_forward(*((double*)parm), -1.0, p->p->n);
		if (tmpi != p->m1) chg=1;
		p->m1=tmpi;
		break;
	case 6:		//marker 2
                tmpi=map_value_forward(*((double*)parm), -1.0, p->p->n);
		if (tmpi != p->m2) chg=1;
		p->m2=tmpi;
		break;
	case 7:		//R trace
                tmpi=map_value_forward(*((double*)parm), 0.0, 1.0); //BOOL!!
                if (p->rt != tmpi) chg=1;
                p->rt=tmpi;
                break;
	case 8:		//G trace
                tmpi=map_value_forward(*((double*)parm), 0.0, 1.0); //BOOL!!
                if (p->gt != tmpi) chg=1;
                p->gt=tmpi;
                break;
	case 9:		//B trace
                tmpi=map_value_forward(*((double*)parm), 0.0, 1.0); //BOOL!!
                if (p->bt != tmpi) chg=1;
                p->bt=tmpi;
                break;
	case 10:	//Y trace
                tmpi=map_value_forward(*((double*)parm), 0.0, 1.0); //BOOL!!
                if (p->yt != tmpi) chg=1;
                p->yt=tmpi;
                break;
	case 11:	//U trace
                tmpi=map_value_forward(*((double*)parm), 0.0, 1.0); //BOOL!!
                if (p->ut != tmpi) chg=1;
                p->ut=tmpi;
                break;
	case 12:	//V trace
                tmpi=map_value_forward(*((double*)parm), 0.0, 1.0); //BOOL!!
                if (p->vt != tmpi) chg=1;
                p->vt=tmpi;
                break;
	case 13:	//alpha trace
                tmpi=map_value_forward(*((double*)parm), 0.0, 1.0); //BOOL!!
                if (p->at != tmpi) chg=1;
                p->at=tmpi;
                break;
	case 14:	//display avg
                tmpi=map_value_forward(*((double*)parm), 0.0, 1.0); //BOOL!!
                if (p->davg != tmpi) chg=1;
                p->davg=tmpi;
                break;
	case 15:	//display RMS
                tmpi=map_value_forward(*((double*)parm), 0.0, 1.0); //BOOL!!
                if (p->drms != tmpi) chg=1;
                p->drms=tmpi;
                break;
	case 16:	//display min
                tmpi=map_value_forward(*((double*)parm), 0.0, 1.0); //BOOL!!
                if (p->dmin != tmpi) chg=1;
                p->dmin=tmpi;
                break;
	case 17:	//display max
                tmpi=map_value_forward(*((double*)parm), 0.0, 1.0); //BOOL!!
                if (p->dmax != tmpi) chg=1;
                p->dmax=tmpi;
                break;
	case 18:	//256 units
                tmpi=map_value_forward(*((double*)parm), 0.0, 1.0); //BOOL!!
                if (p->un != tmpi) chg=1;
                p->un=tmpi;
                break;
	case 19:	//color mode
                tmpi=map_value_forward(*((double*)parm), 0.0, 1.9999);
                if (p->col != tmpi) chg=1;
                p->col=tmpi;
                break;
	case 20:	//Crosshair color
                tmpi=map_value_forward(*((double*)parm), 0.0, 7.9999);
                if (p->chc != tmpi) chg=1;
                p->chc=tmpi;
                break;
	}

if (chg==0) return;

p->mer=p->chn<<24;
p->mer=p->mer+p->rt;
p->mer=p->mer+2*p->gt;
p->mer=p->mer+4*p->bt;
p->mer=p->mer+8*p->yt;
p->mer=p->mer+16*p->ut;
p->mer=p->mer+32*p->vt;
p->mer=p->mer+64*p->at;

p->dit=0;
if (p->m1>=0) p->dit=p->dit+1;
if (p->m2>=0) p->dit=p->dit+4;
if ((p->m1>=0)&&(p->m2>=0)) p->dit=p->dit+16;
p->dit=p->dit+32*p->davg;
p->dit=p->dit+64*p->drms;
p->dit=p->dit+128*p->dmin;
p->dit=p->dit+256*p->dmax;

}

//--------------------------------------------------
void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index)
{
inst *p;
double tmpf;
int tmpi;

p=(inst*)instance;

switch(param_index)
	{
	case 0:
		*((double*)param)=map_value_backward(p->x, 0.0, p->w);
		break;
	case 1:
		*((double*)param)=map_value_backward(p->y, 0.0, p->h);
		break;
	case 2:
		*((double*)param)=map_value_backward(p->tilt, -PI/2.0, PI/2.0);
		break;
	case 3:
		*((double*)param)=map_value_backward(p->len, 20.0, sqrtf(p->w*p->w+p->h*p->h));
		break;
	case 4:
		*((double*)param)=map_value_backward(p->chn, 0.0, 7.9999);
		break;
	case 5:
		*((double*)param)=map_value_backward(p->m1, 0.0, p->p->n);
		break;
	case 6:
		*((double*)param)=map_value_backward(p->m2, 0.0, p->p->n);
		break;
	case 7:
                *((double*)param)=map_value_backward(p->rt, 0.0, 1.0);//BOOL!!
		break;
	case 8:
                *((double*)param)=map_value_backward(p->gt, 0.0, 1.0);//BOOL!!
		break;
	case 9:
                *((double*)param)=map_value_backward(p->bt, 0.0, 1.0);//BOOL!!
		break;
	case 10:
                *((double*)param)=map_value_backward(p->yt, 0.0, 1.0);//BOOL!!
		break;
	case 11:
                *((double*)param)=map_value_backward(p->ut, 0.0, 1.0);//BOOL!!
		break;
	case 12:
                *((double*)param)=map_value_backward(p->vt, 0.0, 1.0);//BOOL!!
		break;
	case 13:
                *((double*)param)=map_value_backward(p->at, 0.0, 1.0);//BOOL!!
		break;
	case 14:
                *((double*)param)=map_value_backward(p->davg, 0.0, 1.0);//BOOL!!
		break;
	case 15:
                *((double*)param)=map_value_backward(p->drms, 0.0, 1.0);//BOOL!!
		break;
	case 16:
                *((double*)param)=map_value_backward(p->dmin, 0.0, 1.0);//BOOL!!
		break;
	case 17:
                *((double*)param)=map_value_backward(p->dmax, 0.0, 1.0);//BOOL!!
		break;
	case 18:
                *((double*)param)=map_value_backward(p->un, 0.0, 1.0);//BOOL!!
		break;
	case 19:
                *((double*)param)=map_value_backward(p->col, 0.0, 1.9999);
		break;
	case 20:
                *((double*)param)=map_value_backward(p->chc, 0.0, 7.9999);
		break;
	}
}

//-------------------------------------------------
void f0r_update(f0r_instance_t instance, double time, const uint32_t* inframe, uint32_t* outframe)
{
inst *in;
float l;
unsigned char lc;
int i;

assert(instance);
in=(inst*)instance;

color2floatrgba(inframe, in->sl, in->w , in->h);

prof(in->sl, in->w, in->h, &in->poz, in->x, in->y, in->tilt, in->len, 1, in->mer, in->un, 0, in->m1, in->m2, in->dit, in->chc, in->col, in->p);

floatrgba2color(in->sl, outframe, in->w , in->h);
}

