/*
pr0be.c

This frei0r plugin measures pixels in video
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

//compile: gcc -c -fPIC -Wall pr0be.c -o pr0be.o
//link: gcc -shared -o pr0be.so pr0be.o

#include <stdio.h>
#include <frei0r.h>
#include <stdlib.h>
#include <math.h>
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

z=(c-32)%32+((c-32)/32)*512;

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

//----------------------------------------------------------
//marker   (crosshair)
//v=size of cross legs
void crosshair(float_rgba *s, int w, int h, int x, int y, int sx, int sy, int v)
{
float x1,y1;
float_rgba white={1.0,1.0,1.0,1.0};
float_rgba black={0.0,0.0,0.0,1.0};

x1=x-1;  y1=y-v-sy/2;
draw_rectangle(s, w, h, x1,  y1, 3, v, white);	//top
draw_rectangle(s, w, h, x-sx/2,  y-sy/2, sx, 1, white);
draw_rectangle(s, w, h, x1+1,  y1, 1, v, black);
x1=x-v-sx/2;  y1=y-1;
draw_rectangle(s, w, h, x1, y1, v,  3, white);	//left
draw_rectangle(s, w, h, x-sx/2,  y-sy/2, 1, sy, white);
draw_rectangle(s, w, h, x1, y1+1, v,  1, black);
x1=x-1;  y1=y+sy/2+1;
draw_rectangle(s, w, h, x1,  y1,  3, v, white);	//bottom
draw_rectangle(s, w, h, x-sx/2,  y+sy/2, sx, 1, white);
draw_rectangle(s, w, h, x1+1,  y1, 1, v, black);
x1=x+sx/2+1;  y1=y-1;
draw_rectangle(s, w, h, x1,  y1, v,  3, white);	//right
draw_rectangle(s, w, h, x+sx/2,  y-sy/2, 1, sy, white);
draw_rectangle(s, w, h, x1, y1+1, v,  1, black);
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
//print avg,rms,min,max into a string
//u=units    0=0.0-1.0    1=0-255
//m=sign  0=unsigned
//mm=1  print min/max
void izpis(char *str, char *lab, stat s, int u, int m, int mm)
{
char fs[256],as[16],rs[16],ns[16],xs[16];

if (u==1)
	{
	s.avg=255.0*s.avg;
	s.rms=255.0*s.rms;
	s.min=255.0*s.min;
	s.max=255.0*s.max;
	}

if (mm==1)
  {
  forstr(s.avg,1-u,m,as);
  forstr(s.rms,1-u,0,rs);
  forstr(s.min,1-u,m,ns);
  forstr(s.max,1-u,m,xs);
  sprintf(fs,"%s%s%s %s%s", lab, as, rs, ns, xs);
  sprintf(str,fs,s.avg,s.rms,s.min,s.max);
  }
else
  {
  forstr(s.avg,1-u,m,as);
  forstr(s.rms,1-u,0,rs);
  sprintf(fs,"%s%s%s", lab, as, rs);
  sprintf(str,fs,s.avg,s.rms);
  }
}

//-----------------------------------------------------------
//probe size markers in the magnifier diaplay
void sxmarkers(float_rgba *s, int w, int h, int x0, int y0, int np, int sx, int sy, int vp)
{
int np2,x,y,i,j;
float_rgba white={1.0,1.0,1.0,1.0};

np2=np/2+1;

//top left
x=x0+(np2-sx/2)*vp-1;    if (sx>np) x=x0;
y=y0+(np2-sy/2)*vp-1;    if (sy>np) y=y0;
if (sx<=np) draw_rectangle(s, w, h, x, y, 1, vp, white);
if (sy<=np) draw_rectangle(s, w, h, x, y, vp, 1, white);
//top right
x=x0+(np2+sx/2+1)*vp-1;
y=y0+(np2-sy/2)*vp-1;    if (sy>np) y=y0;
if (sx<=np) draw_rectangle(s, w, h, x, y, 1, vp, white);
x=x0+(np2+sx/2)*vp;
y=y0+(np2-sy/2)*vp-1;    if (sx>np) x=x0+(np+1)*vp-1;
if (sy<=np) draw_rectangle(s, w, h, x, y, vp, 1, white);
//bottom left
x=x0+(np2-sx/2)*vp-1;
y=y0+(np2+sy/2)*vp;      if (sy>np) y=y0+(np+1)*vp;
if (sx<=np)  draw_rectangle(s, w, h, x, y, 1, vp, white);
x=x0+(np2-sx/2)*vp-1;    if (sx>np) x=x0;
y=y0+(np2+sy/2+1)*vp-1;
if (sy<=np) draw_rectangle(s, w, h, x, y, vp, 1, white);
//bottom right
x=x0+(np2+sx/2)*vp+vp-1;
y=y0+(np2+sy/2)*vp;      if (sy>np) y=y0+(np+1)*vp;
if (sx<=np)  draw_rectangle(s, w, h, x, y, 1, vp, white);
x=x0+(np2+sx/2)*vp;
y=y0+(np2+sy/2+1)*vp-1;  if (sx>np) x=x0+(np+1)*vp-1;
if (sy<=np) draw_rectangle(s, w, h, x, y, vp, 1, white);

//"out of box" arrows
if (sx>np)
  {
  for (i=1;i<vp;i++)
    for (j=-i/2;j<=i/2;j++)
      {
      y=y0+np2*vp+j+vp/2;
      x=x0+i;
      s[w*y+x]=white;
      x=x0+(np+2)*vp-i-1;
      s[w*y+x]=white;
      }	
  }
if (sy>np)
  {
  for (i=1;i<vp;i++)
    for (j=-i/2;j<=i/2;j++)
      {
      x=x0+np2*vp+j+vp/2;
      y=y0+i;
      s[w*y+x]=white;
      y=y0+(np+2)*vp-i-1;
      s[w*y+x]=white;
      }	
  }

}

//--------------------------------------------------------------
//draw info window
//sx,sy=size of probe   (must be odd)
//poz=position of info window	0=left   1=right
//m=type of measurement:  0=rgba  1=Y'CrCba  2=HSVa
//u=units    0=0.0-1.0    1=0-255
//sha=1 show alpha
//bw=1  big window
void sonda(float_rgba *s, int w, int h, int x, int y, int sx, int sy, int *poz, int m, int u, int sha, int bw)
{
int x0,y0,vx,vy,vp,np,np2,xn,yn;
int i,j,xp,yp;
char string[256];
stat yy,rr,gg,bb,aa,uu,vv;
float al,be,h2,c1,c2,ss,va,li;

float_rgba white={1.0,1.0,1.0,1.0};
float_rgba gray={0.5,0.5,0.5,1.0};
float_rgba black={0.0,0.0,0.0,1.0};
float_rgba red={1.0,0.0,0.0,1.0};
float_rgba dgreen={0.0,0.7,0.0,1.0};
float_rgba lblue={0.3,0.3,1.0,1.0};

//position and size of info window
//if (x<5*w/12) *poz=1;	//right
//if (x>7*w/12) *poz=0;	//left
if (x<w/2-30) *poz=1;	//right
if (x>w/2+30) *poz=0;	//left
vp=9;			//pixel size in magnifier
y0=h/20;
if (bw==1)		//big window
  {
  vx=240;
  vy = (m<=2) ? 320 : 300; 
  x0 = (*poz==0) ? h/20 : w-h/20-vx;
  np=25;  		//size of magnifier
  xn = (m<=2) ? x0+8 : x0+70;
  yn=y0+(np+1)*vp+8;
  }
else			//small window
  {
  vx=152;
  vy = (m<=2) ? 230 : 210;
  x0 = (*poz==0) ? h/20 : w-h/20-vx;
  np=15;  		//size of magnifier
  xn = (m<=2) ? x0+15 : x0+25;
  yn=y0+(np+1)*vp+8;
  }
np2=np/2+1;
if (sha==1) vy=vy+20;

//keep probe inside
if (x<sx/2) x=sx/2;
if (x>=(w-sx/2)) x=w-sx/2-1;
if (y<sy/2) y=sy/2;
if (y>=(h-sy/2)) y=h-sy/2-1;

//info window background
darken_rectangle(s, w, h, x0, y0, vx, vy, 0.4);

//magnifier background
draw_rectangle(s, w, h, x0+vp-1, y0+vp-1, np*vp+1, np*vp+1, black);
//sx,sy marks
sxmarkers(s, w, h, x0, y0, np, sx, sy, vp);
//magnifier pixels
for (i=0;i<np;i++)
	for (j=0;j<np;j++)
		{
		xp=x-np2+j+1;
		yp=y-np2+i+1;
		if ((xp>=0)&&(xp<w)&&(yp>=0)&&(yp<h))
			draw_rectangle(s, w, h, x0+(j+1)*vp, y0+(i+1)*vp, vp-1, vp-1, s[yp*w+xp]);
		}

//title
if (m<=2)
  {
  if (bw==1)
    draw_string(s, w, h, xn, yn+5, "CHN  AVG   RMS    MIN   MAX", white);
  else
    draw_string(s, w, h, xn, yn+5, "CHN  AVG   RMS", white);
  }
//measurements
switch (m)
  {
  case 0:	//display RGBa
    meri_rgb(s, &rr, &gg, &bb, x, y, w, sx, sy);
    izpis(string, " R ", rr, u, 0, bw);
    draw_string(s, w, h, xn, yn+22, string, red);
    izpis(string, " G ", gg, u, 0, bw);
    draw_string(s, w, h, xn, yn+39, string, dgreen);
    izpis(string, " B ", bb, u, 0, bw);
    draw_string(s, w, h, xn, yn+56, string, lblue);
    if (sha==1)
      {
      meri_a(s, &aa, x, y, w, sx, sy);
      izpis(string, " a ", aa, u, 0, bw);
      draw_string(s, w, h, xn, yn+73, string, gray);
      }
    break;
  case 1:	//display Y'PrPba
  case 2:
    if (m==1)
      {
      meri_y(s, &yy, 0, x, y, w, sx, sy);
      meri_uv(s, &uu, &vv, 0, x, y, w, sx, sy);
      }
    else
      {
      meri_y(s, &yy, 1, x, y, w, sx, sy);
      meri_uv(s, &uu, &vv, 1, x, y, w, sx, sy);
      }
    izpis(string, " Y ", yy, u, 0, bw);
    draw_string(s, w, h, xn, yn+22, string, white);
    izpis(string, " Pr", uu, u, 1, bw);
    draw_string(s, w, h, xn, yn+39, string, red);
    izpis(string, " Pb", vv, u, 1, bw);
    draw_string(s, w, h, xn, yn+56, string, lblue);
    if (sha==1)
      {
      meri_a(s, &aa, x, y, w, sx, sy);
      izpis(string, " a ", aa, u, 0, bw);
      draw_string(s, w, h, xn, yn+73, string, gray);
      }
    break;
  case 3:	//display HSV
    meri_rgb(s, &rr, &gg, &bb, x, y, w, sx, sy);
    al=(rr.avg+gg.avg+bb.avg)/2.0;
    be=sqrtf(3.0)/2.0*(gg.avg-bb.avg);
    h2=atan2f(be,al);
    c2=sqrtf(al*al+be*be);
    va=rr.avg;
    if (gg.avg>va) va=gg.avg; if (bb.avg>va) va=bb.avg;
    li=rr.avg;
    if (gg.avg<li) li=gg.avg; if (bb.avg<li) li=bb.avg;
    c1=va-li;
    if (c1==0.0) ss=0.0; else ss=c1/va;
    h2=h2*180.0/PI; if (h2<0.0) h2=h2+180.0;
    sprintf(string," Hue = %5.1f",h2);
    draw_string(s, w, h, xn, yn+5, string, white);
    sprintf(string," Sat = %5.3f",ss);
    draw_string(s, w, h, xn, yn+22, string, white);
    sprintf(string," Val = %5.3f",va);
    draw_string(s, w, h, xn, yn+39, string, white);
    if (sha==1)
      {
      meri_a(s, &aa, x, y, w, sx, sy);
      sprintf(string,"  a  = %5.3f",aa.avg);
      draw_string(s, w, h, xn, yn+56, string, gray);
      }
    break;
  case 4:	//display HSL
    meri_rgb(s, &rr, &gg, &bb, x, y, w, sx, sy);
    al=(rr.avg+gg.avg+bb.avg)/2.0;
    be=sqrtf(3.0)/2.0*(gg.avg-bb.avg);
    h2=atan2f(be,al);
    c2=sqrtf(al*al+be*be);
    va=rr.avg;
    if (gg.avg>va) va=gg.avg; if (bb.avg>va) va=bb.avg;
    li=rr.avg;
    if (gg.avg<li) li=gg.avg; if (bb.avg<li) li=bb.avg;
    c1=va-li;
    li=(li+va)/2.0;
    if (c1==0.0)
      ss=0.0;
    else
      {
      if (li<=0.5) ss=c1/2.0/li; else ss=c1/(2.0-2.0*li);
      }
    h2=h2*180.0/PI; if (h2<0.0) h2=h2+180.0;
    sprintf(string," Hue = %5.1f",h2);
    draw_string(s, w, h, xn, yn+5, string, white);
    sprintf(string," Sat = %5.3f",ss);
    draw_string(s, w, h, xn, yn+22, string, white);
    sprintf(string," Lgt = %5.3f",li);
    draw_string(s, w, h, xn, yn+39, string, white);
    if (sha==1)
      {
      meri_a(s, &aa, x, y, w, sx, sy);
      sprintf(string,"  a  = %5.3f",aa.avg);
      draw_string(s, w, h, xn, yn+56, string, gray);
      }
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
void color2floatrgba(const uint32_t* inframe, float_rgba *sl, int w , int h)
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

int mer;
int x;
int y;
int sx;
int sy;
int un;
int sha;
int bw;

int poz;
float_rgba *sl;
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

info->name="pr0be";
info->author="Marko Cebokli";
info->plugin_type=F0R_PLUGIN_TYPE_FILTER;
info->color_model=F0R_COLOR_MODEL_RGBA8888;
info->frei0r_version=FREI0R_MAJOR_VERSION;
info->major_version=0;
info->minor_version=1;
info->num_params=8;
info->explanation="Measure video values";
}

//--------------------------------------------------
void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
switch(param_index)
	{
	case 0:
		info->name = "Measurement";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "What measurement to display";
		break;
	case 1:
		info->name = "X";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "X position of probe";
		break;
	case 2:
		info->name = "Y";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "Y position of probe";
		break;
	case 3:
		info->name = "X size";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "X size of probe";
		break;
	case 4:
		info->name = "Y size";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "Y size of probe";
		break;
	case 5:
		info->name = "256 scale";
		info->type = F0R_PARAM_BOOL;
		info->explanation = "use 0-255 instead of 0.0-1.0";
		break;
	case 6:
		info->name = "Show alpha";
		info->type = F0R_PARAM_BOOL;
		info->explanation = "Display alpha value too";
		break;
	case 7:
		info->name = "Big window";
		info->type = F0R_PARAM_BOOL;
		info->explanation = "Display more data";
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

in->mer=0;
in->x=width/2;
in->y=height/2;
in->sx=3;
in->sy=3;
in->un=0;
in->sha=0;
in->bw=0;

in->poz=0;
in->sl=(float_rgba*)calloc(width*height,sizeof(float_rgba));

return (f0r_instance_t)in;
}

//---------------------------------------------------
void f0r_destruct(f0r_instance_t instance)
{
inst *in;

in=(inst*)instance;

free(in->sl);
free(instance);
}

//-----------------------------------------------------
void f0r_set_param_value(f0r_instance_t instance, f0r_param_t parm, int param_index)
{
inst *p;
int tmpi,chg;

p=(inst*)instance;

chg=0;
switch(param_index)
	{
	case 0:
                tmpi=map_value_forward(*((double*)parm), 0.0, 4.9999);
		if (tmpi != p->mer) chg=1;
		p->mer=tmpi;
		break;
	case 1:
                tmpi=map_value_forward(*((double*)parm), 0.0, p->w);
		if (tmpi != p->x) chg=1;
		p->x=tmpi;
		break;
	case 2:
                tmpi=map_value_forward(*((double*)parm), 0.0, p->h);
		if (tmpi != p->y) chg=1;
		p->y=tmpi;
		break;
	case 3:
                tmpi=map_value_forward(*((double*)parm), 0.0, 12.0);
		if (tmpi != p->sx) chg=1;
		p->sx=tmpi;
		break;
	case 4:
                tmpi=map_value_forward(*((double*)parm), 0.0, 12.0);
		if (tmpi != p->sy) chg=1;
		p->sy=tmpi;
		break;
	case 5:
                tmpi=map_value_forward(*((double*)parm), 0.0, 1.0); //BOOL!!
                if (p->un != tmpi) chg=1;
                p->un=tmpi;
                break;
	case 6:
                tmpi=map_value_forward(*((double*)parm), 0.0, 1.0); //BOOL!!
                if (p->sha != tmpi) chg=1;
                p->sha=tmpi;
                break;
	case 7:
                tmpi=map_value_forward(*((double*)parm), 0.0, 1.0); //BOOL!!
                if (p->bw != tmpi) chg=1;
                p->bw=tmpi;
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
	case 0:
		*((double*)param)=map_value_backward(p->mer, 0.0, 4.9999);
		break;
	case 1:
		*((double*)param)=map_value_backward(p->x, 0.0, p->w);
		break;
	case 2:
		*((double*)param)=map_value_backward(p->y, 0.0, p->h);
		break;
	case 3:
		*((double*)param)=map_value_backward(p->sx, 0.0, 12.0);
		break;
	case 4:
		*((double*)param)=map_value_backward(p->sy, 0.0, 12.0);
		break;
	case 5:
                *((double*)param)=map_value_backward(p->un, 0.0, 1.0);//BOOL!!
		break;
	case 6:
                *((double*)param)=map_value_backward(p->sha, 0.0, 1.0);//BOOL!!
		break;
	case 7:
                *((double*)param)=map_value_backward(p->bw, 0.0, 1.0);//BOOL!!
		break;
	}
}

//-------------------------------------------------
void f0r_update(f0r_instance_t instance, double time, const uint32_t* inframe, uint32_t* outframe)
{
inst *in;

assert(instance);
in=(inst*)instance;

color2floatrgba(inframe, in->sl, in->w , in->h);

sonda(in->sl, in->w, in->h, in->x, in->y, 2*in->sx+1, 2*in->sy+1, &in->poz, in->mer, in->un, in->sha, in->bw);
crosshair(in->sl, in->w, in->h, in->x, in->y, 2*in->sx+1, 2*in->sy+1, 15);

floatrgba2color(in->sl, outframe, in->w , in->h);
}

