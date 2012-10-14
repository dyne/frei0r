/*
select0r.c

This frei0r plugin   makes a color based alpha selection
Version 0.4	apr 2012

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

//	apr 2012	added slope parameter

//compile: gcc -c -fPIC -Wall select0r.c -o select0r.o
//link: gcc -shared -o select0r.so select0r.o

//#include <stdio.h>	/* for debug printf only +/
#include <frei0r.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

typedef struct
	{
	float r;
	float g;
	float b;
	float a;
	} float_rgba;

typedef struct
	{
	float x;
	float y;
	float z;
	} triplet;

double PI=3.14159265358979;

//-----------------------------------------------------------
//inline functions for subspace metrics
//distance from center point for different shapes
//for cartesian and cylindric (wraparound hue) spaces
//cx,cy,cz:  center of subspace
//dx,dy,dz:  size of subspace
//dx, dy and dz must be inverse (1/x) values!   (avoid division)
//x,y,z:  point from which distance is determined
//  returns square of distance
//  r==1 is edge of subspace
//box shape
static inline float dist_box(float cx, float cy, float cz, float dx, float dy, float dz, float x, float y, float z)
{
float ax,ay,az,r;

ax=fabsf(x-cx)*dx;
ay=fabsf(y-cy)*dy;
az=fabsf(z-cz)*dz;
r=ax;
if (ay>r) r=ay;
if (az>r) r=az;
r=r*r;
return r;
}
//ellipsoid shape
static inline float dist_eli(float cx, float cy, float cz, float dx, float dy, float dz, float x, float y, float z)
{
float ax,ay,az,r;

ax=(x-cx)*dx;
ay=(y-cy)*dy;
az=(z-cz)*dz;
r=ax*ax+ay*ay+az*az;
return r;
}
//octahedron shape
static inline float dist_oct(float cx, float cy, float cz, float dx, float dy, float dz, float x, float y, float z)
{
float ax,ay,az,r;

ax=fabsf(x-cx)*dx;
ay=fabsf(y-cy)*dy;
az=fabsf(z-cz)*dz;
r=ax+ay+az;
r=r*r;
return r;
}
//box shape, cylindrical space
static inline float dist_box_c(float chue, float cy, float cz, float dhue, float dy, float dz, float hue, float y, float z)
{
float ax,ay,az,r;

ax=fabsf(hue-chue);
//if (ax>PI) ax=ax-PI;
if (ax>0.5) ax=ax-0.5;
ax=ax*dhue;
ay=fabsf(y-cy)*dy;
az=fabsf(z-cz)*dz;
r=ax;
if (ay>r) r=ay;
if (az>r) r=az;
r=r*r;
return r;
}
//ellipsoid shape, cylindrical space
static inline float dist_eli_c(float chue, float cy, float cz, float dhue, float dy, float dz, float hue, float y, float z)
{
float ax,ay,az,r;

ax=(hue-chue);
//if (ax>PI) ax=ax-PI;
if (ax>0.5) ax=ax-0.5;
ax=ax*dhue;
ay=(y-cy)*dy;
az=(z-cz)*dz;
r=ax*ax+ay*ay+az*az;
return r;
}
//octahedron shape, cylindrical space
static inline float dist_oct_c(float chue, float cy, float cz, float dhue, float dy, float dz, float hue, float y, float z)
{
float ax,ay,az,r;

ax=fabsf(hue-chue);
//if (ax>PI) ax=ax-PI;
if (ax>0.5) ax=ax-0.5;
ax=ax*dhue;
ay=fabsf(y-cy)*dy;
az=fabsf(z-cz)*dz;
r=ax+ay+az;
r=r*r;
return r;
}

//----------------------------------------------------------
//inline RGB to ABI conversion function
static inline void rgb2abi(float k32, float r, float g, float b, float *a, float *bb, float *i)
{
*a=r-0.5*g-0.5*b;
*bb=k32*(g-b);
*i=0.3333*(r+g+b);
}

//----------------------------------------------------------
//inline RGB to HCI conversion function
static inline void rgb2hci(float ipi2, float k32, float r, float g, float b, float *h, float *c, float *i)
{
float a,bb;
a=r-0.5*g-0.5*b;
bb=k32*(g-b);
*h=atan2(bb,a)*ipi2;
*c=hypotf(a,bb);
*i=0.3333*(r+g+b);
}

//------------------------------------------------------
//thresholding inline functions  (hard and soft)
static inline float thres(float a)
{
return (a<1.0) ? 1.0 : 0.0;
}

static inline float fat(float a)
{
a=a*a*a*a;
return (a<1.0) ? 1.0-a : 0.0;
}

static inline float norm(float a)
{
a=a*a;
return (a<1.0) ? 1.0-a : 0.0;
}

static inline float skiny(float a)
{
return (a<1.0) ? 1.0-a : 0.0;
}

static inline float slope(float a, float is)
{
a = (a<1.0) ? 1.0 : 1.0-is*(a-1);
return (a>=0) ? a : 0.0;
}

//----------------------------------------------------------
//RGB selection
//d = deltas (size of subspace)
//n = nudges
//ss = subspace shape [0..2] box, ellipsoid, octahedron
//thr:  0=thresholded  1=linear fat  2=lin norm  3=lin skiny
//avoids switch () inside inner loop for speed - this means
//a big, repetitive switch statement outside....
void sel_rgb(float_rgba *slika, int w, int h, float_rgba key, triplet d, triplet n, float slp, int ss, int thr)
{
float kr,kg,kb,dd;
int i,s;
float ddx,ddy,ddz;
float islp;

//add nudge
kr=key.r+n.x;
kg=key.g+n.y;
kb=key.b+n.z;
ddx = (d.x!=0) ? 1.0/d.x : 1.0E6; 
ddy = (d.y!=0) ? 1.0/d.y : 1.0E6;
ddz = (d.z!=0) ? 1.0/d.z : 1.0E6;

islp = (slp>0.000001) ? 0.2/slp : 200000.0;

s=10*ss+thr;	//to avoid nested switch statements

switch (s)
	{
	case 0:		//box, thresholded
		for (i=0;i<w*h;i++)
			{
			dd = dist_box(kr, kg, kb, ddx, ddy, ddz, slika[i].r, slika[i].g, slika[i].b);
			slika[i].a = thres(dd);
			}
		break;
	case 1:		//box, linear fat
		for (i=0;i<w*h;i++)
			{
			dd = dist_box(kr, kg, kb, ddx, ddy, ddz, slika[i].r, slika[i].g, slika[i].b);
			slika[i].a = fat(dd);
			}
		break;
	case 2:		//box, linear normal
		for (i=0;i<w*h;i++)
			{
			dd = dist_box(kr, kg, kb, ddx, ddy, ddz, slika[i].r, slika[i].g, slika[i].b);
			slika[i].a = norm(dd);
			}
		break;
	case 3:		//box, linear skiny
		for (i=0;i<w*h;i++)
			{
			dd = dist_box(kr, kg, kb, ddx, ddy, ddz, slika[i].r, slika[i].g, slika[i].b);
			slika[i].a = skiny(dd);
			}
		break;
	case 4:		//box, linear slope
		for (i=0;i<w*h;i++)
			{
			dd = dist_box(kr, kg, kb, ddx, ddy, ddz, slika[i].r, slika[i].g, slika[i].b);
			slika[i].a = slope(dd, islp);
			}
		break;

	case 10:	//ellipsoid, thresholded
		for (i=0;i<w*h;i++)
			{
			dd = dist_eli(kr, kg, kb, ddx, ddy, ddz, slika[i].r, slika[i].g, slika[i].b);
			slika[i].a = thres(dd);
			}
		break;
	case 11:	//ellipsoid, linear fat
		for (i=0;i<w*h;i++)
			{
			dd = dist_eli(kr, kg, kb, ddx, ddy, ddz, slika[i].r, slika[i].g, slika[i].b);
			slika[i].a = fat(dd);
			}
		break;
	case 12:	//ellipsoid, linear normal
		for (i=0;i<w*h;i++)
			{
			dd = dist_eli(kr, kg, kb, ddx, ddy, ddz, slika[i].r, slika[i].g, slika[i].b);
			slika[i].a = norm(dd);
			}
		break;
	case 13:	//ellipsoid, linear skiny
		for (i=0;i<w*h;i++)
			{
			dd = dist_eli(kr, kg, kb, ddx, ddy, ddz, slika[i].r, slika[i].g, slika[i].b);
			slika[i].a = skiny(dd);
			}
		break;
	case 14:	//ellipsoid, linear slope
		for (i=0;i<w*h;i++)
			{
			dd = dist_eli(kr, kg, kb, ddx, ddy, ddz, slika[i].r, slika[i].g, slika[i].b);
			slika[i].a = slope(dd, islp);
			}
		break;

	case 20:	//octahedron, thresholded
		for (i=0;i<w*h;i++)
			{
			dd = dist_oct(kr, kg, kb, ddx, ddy, ddz, slika[i].r, slika[i].g, slika[i].b);
			slika[i].a = thres(dd);
			}
		break;
	case 21:	//octahedron, linear fat
		for (i=0;i<w*h;i++)
			{
			dd = dist_oct(kr, kg, kb, ddx, ddy, ddz, slika[i].r, slika[i].g, slika[i].b);
			slika[i].a = fat(dd);
			}
		break;
	case 22:	//octahedron, linear normal
		for (i=0;i<w*h;i++)
			{
			dd = dist_oct(kr, kg, kb, ddx, ddy, ddz, slika[i].r, slika[i].g, slika[i].b);
			slika[i].a = norm(dd);
			}
		break;
	case 23:	//octahedron, linear skiny
		for (i=0;i<w*h;i++)
			{
			dd = dist_oct(kr, kg, kb, ddx, ddy, ddz, slika[i].r, slika[i].g, slika[i].b);
			slika[i].a = skiny(dd);
			}
		break;
	case 24:	//octahedron, linear slope
		for (i=0;i<w*h;i++)
			{
			dd = dist_oct(kr, kg, kb, ddx, ddy, ddz, slika[i].r, slika[i].g, slika[i].b);
			slika[i].a = slope(dd, islp);
			}
		break;
	default:
		break;
	}
}

//----------------------------------------------------------
//ABI selection
//d = deltas (size of subspace)
//n = nudges
//ss = subspace shape [0..2] box, ellipsoid, octahedron
//thr:  0=thresholded  1=linear fat  2=lin norm  3=lin skiny
//avoids switch () inside inner loop for speed - this means
//a big, repetitive switch statement outside....
void sel_abi(float_rgba *slika, int w, int h, float_rgba key, triplet d, triplet n, float slp, int ss, int thr)
{
float ka,kb,ki,k32,dd;
int i,s;
float dda,ddb,ddi,sa,sb,si;
float islp;

dda = (d.x!=0) ? 1.0/d.x : 1.0E6; 
ddb = (d.y!=0) ? 1.0/d.y : 1.0E6;
ddi = (d.z!=0) ? 1.0/d.z : 1.0E6;

//convert key to ABI and add nudge
k32=sqrtf(3.0)/2.0;
ka=key.r-0.5*key.g-0.5*key.b+n.x;
kb=k32*(key.g-key.b)+n.y;
ki=0.3333*(key.r+key.g+key.b)+n.z;

islp = (slp>0.000001) ? 0.2/slp : 200000.0;

s=10*ss+thr;	//to avoid nested switch statements

switch (s)
	{
	case 0:		//box, thresholded
		for (i=0;i<w*h;i++)
			{
			rgb2abi(k32,slika[i].r,slika[i].g,slika[i].b,&sa,&sb,&si);
			dd = dist_box(ka, kb, ki, dda, ddb, ddi, sa, sb, si);
			slika[i].a = thres(dd);
			}
		break;
	case 1:		//box, linear fat
		for (i=0;i<w*h;i++)
			{
			rgb2abi(k32,slika[i].r,slika[i].g,slika[i].b,&sa,&sb,&si);
			dd = dist_box(ka, kb, ki, dda, ddb, ddi, sa, sb, si);
			slika[i].a = fat(dd);
			}
		break;
	case 2:		//box, linear normal
		for (i=0;i<w*h;i++)
			{
			rgb2abi(k32,slika[i].r,slika[i].g,slika[i].b,&sa,&sb,&si);
			dd = dist_box(ka, kb, ki, dda, ddb, ddi, sa, sb, si);
			slika[i].a = norm(dd);
			}
		break;
	case 3:		//box, linear skiny
		for (i=0;i<w*h;i++)
			{
			rgb2abi(k32,slika[i].r,slika[i].g,slika[i].b,&sa,&sb,&si);
			dd = dist_box(ka, kb, ki, dda, ddb, ddi, sa, sb, si);
			slika[i].a = skiny(dd);
			}
		break;
	case 4:		//box, linear slope
		for (i=0;i<w*h;i++)
			{
			rgb2abi(k32,slika[i].r,slika[i].g,slika[i].b,&sa,&sb,&si);
			dd = dist_box(ka, kb, ki, dda, ddb, ddi, sa, sb, si);
			slika[i].a = slope(dd, islp);
			}
		break;

	case 10:	//ellipsoid, thresholded
		for (i=0;i<w*h;i++)
			{
			rgb2abi(k32,slika[i].r,slika[i].g,slika[i].b,&sa,&sb,&si);
			dd = dist_eli(ka, kb, ki, dda, ddb, ddi, sa, sb, si);
			slika[i].a = thres(dd);
			}
		break;
	case 11:	//ellipsoid, linear fat
		for (i=0;i<w*h;i++)
			{
			rgb2abi(k32,slika[i].r,slika[i].g,slika[i].b,&sa,&sb,&si);
			dd = dist_eli(ka, kb, ki, dda, ddb, ddi, sa, sb, si);
			slika[i].a = fat(dd);
			}
		break;
	case 12:	//ellipsoid, linear normal
		for (i=0;i<w*h;i++)
			{
			rgb2abi(k32,slika[i].r,slika[i].g,slika[i].b,&sa,&sb,&si);
			dd = dist_eli(ka, kb, ki, dda, ddb, ddi, sa, sb, si);
			slika[i].a = norm(dd);
			}
		break;
	case 13:	//ellipsoid, linear skiny
		for (i=0;i<w*h;i++)
			{
			rgb2abi(k32,slika[i].r,slika[i].g,slika[i].b,&sa,&sb,&si);
			dd = dist_eli(ka, kb, ki, dda, ddb, ddi, sa, sb, si);
			slika[i].a = skiny(dd);
			}
		break;
	case 14:	//ellipsoid, linear slope
		for (i=0;i<w*h;i++)
			{
			rgb2abi(k32,slika[i].r,slika[i].g,slika[i].b,&sa,&sb,&si);
			dd = dist_eli(ka, kb, ki, dda, ddb, ddi, sa, sb, si);
			slika[i].a = slope(dd, islp);
			}
		break;

	case 20:	//octahedron, thresholded
		for (i=0;i<w*h;i++)
			{
			rgb2abi(k32,slika[i].r,slika[i].g,slika[i].b,&sa,&sb,&si);
			dd = dist_oct(ka, kb, ki, dda, ddb, ddi, sa, sb, si);
			slika[i].a = thres(dd);
			}
		break;
	case 21:	//octahedron, linear fat
		for (i=0;i<w*h;i++)
			{
			rgb2abi(k32,slika[i].r,slika[i].g,slika[i].b,&sa,&sb,&si);
			dd = dist_oct(ka, kb, ki, dda, ddb, ddi, sa, sb, si);
			slika[i].a = fat(dd);
			}
		break;
	case 22:	//octahedron, linear normal
		for (i=0;i<w*h;i++)
			{
			rgb2abi(k32,slika[i].r,slika[i].g,slika[i].b,&sa,&sb,&si);
			dd = dist_oct(ka, kb, ki, dda, ddb, ddi, sa, sb, si);
			slika[i].a = norm(dd);
			}
		break;
	case 23:	//octahedron, linear skiny
		for (i=0;i<w*h;i++)
			{
			rgb2abi(k32,slika[i].r,slika[i].g,slika[i].b,&sa,&sb,&si);
			dd = dist_oct(ka, kb, ki, dda, ddb, ddi, sa, sb, si);
			slika[i].a = skiny(dd);
			}
		break;
	case 24:	//octahedron, linear slope
		for (i=0;i<w*h;i++)
			{
			rgb2abi(k32,slika[i].r,slika[i].g,slika[i].b,&sa,&sb,&si);
			dd = dist_oct(ka, kb, ki, dda, ddb, ddi, sa, sb, si);
			slika[i].a = slope(dd, islp);
			}
		break;
	default:
		break;
	}
}

//----------------------------------------------------------
//HCI selection
//d = deltas (size of subspace)
//n = nudges
//ss = subspace shape [0..2] box, ellipsoid, octahedron
//thr:  0=thresholded  1=linear fat  2=lin norm  3=lin skiny
//avoids switch () inside inner loop for speed - this means
//a big, repetitive switch statement outside....
void sel_hci(float_rgba *slika, int w, int h, float_rgba key, triplet d, triplet n, float slp, int ss, int thr)
{
float ka,kb,ki,kh,kc,k32,dd;
int i,s;
float ddh,ddc,ddi,sh,sc,si,ipi2;
float islp;

ipi2=0.5/PI;
ddh = (d.x!=0) ? 1.0/d.x : 1.0E6; 
ddc = (d.y!=0) ? 1.0/d.y : 1.0E6;
ddi = (d.z!=0) ? 1.0/d.z : 1.0E6;

//convert key to HCI and add nudge
k32=sqrtf(3.0)/2.0;
ka=key.r-0.5*key.g-0.5*key.b;
kb=k32*(key.g-key.b);
ki=0.3333*(key.r+key.g+key.b)+n.z;
kh=atan2f(kb,ka)*ipi2+n.x;
kc=hypotf(ka,kb)+n.y;

islp = (slp>0.000001) ? 0.2/slp : 200000.0;

s=10*ss+thr;	//to avoid nested switch statements

switch (s)
	{
	case 0:		//box, thresholded
		for (i=0;i<w*h;i++)
			{
			rgb2hci(ipi2,k32,slika[i].r,slika[i].g,slika[i].b,&sh,&sc,&si);
			dd = dist_box_c(kh, kc, ki, ddh, ddc, ddi, sh, sc, si);
			slika[i].a = thres(dd);
			}
		break;
	case 1:		//box, linear fat
		for (i=0;i<w*h;i++)
			{
			rgb2hci(ipi2,k32,slika[i].r,slika[i].g,slika[i].b,&sh,&sc,&si);
			dd = dist_box_c(kh, kc, ki, ddh, ddc, ddi, sh, sc, si);
			slika[i].a = fat(dd);
			}
		break;
	case 2:		//box, linear normal
		for (i=0;i<w*h;i++)
			{
			rgb2hci(ipi2,k32,slika[i].r,slika[i].g,slika[i].b,&sh,&sc,&si);
			dd = dist_box_c(kh, kc, ki, ddh, ddc, ddi, sh, sc, si);
			slika[i].a = norm(dd);
			}
		break;
	case 3:		//box, linear skiny
		for (i=0;i<w*h;i++)
			{
			rgb2hci(ipi2,k32,slika[i].r,slika[i].g,slika[i].b,&sh,&sc,&si);
			dd = dist_box_c(kh, kc, ki, ddh, ddc, ddi, sh, sc, si);
			slika[i].a = skiny(dd);
			}
		break;
	case 4:		//box, linear slope
		for (i=0;i<w*h;i++)
			{
			rgb2hci(ipi2,k32,slika[i].r,slika[i].g,slika[i].b,&sh,&sc,&si);
			dd = dist_box_c(kh, kc, ki, ddh, ddc, ddi, sh, sc, si);
			slika[i].a = slope(dd, islp);
			}
		break;

	case 10:	//ellipsoid, thresholded
		for (i=0;i<w*h;i++)
			{
			rgb2hci(ipi2,k32,slika[i].r,slika[i].g,slika[i].b,&sh,&sc,&si);
			dd = dist_eli_c(kh, kc, ki, ddh, ddc, ddi, sh, sc, si);
			slika[i].a = thres(dd);
			}
		break;
	case 11:	//ellipsoid, linear fat
		for (i=0;i<w*h;i++)
			{
			rgb2hci(ipi2,k32,slika[i].r,slika[i].g,slika[i].b,&sh,&sc,&si);
			dd = dist_eli_c(kh, kc, ki, ddh, ddc, ddi, sh, sc, si);
			slika[i].a = fat(dd);
			}
		break;
	case 12:	//ellipsoid, linear normal
		for (i=0;i<w*h;i++)
			{
			rgb2hci(ipi2,k32,slika[i].r,slika[i].g,slika[i].b,&sh,&sc,&si);
			dd = dist_eli_c(kh, kc, ki, ddh, ddc, ddi, sh, sc, si);
			slika[i].a = norm(dd);
			}
		break;
	case 13:	//ellipsoid, linear skiny
		for (i=0;i<w*h;i++)
			{
			rgb2hci(ipi2,k32,slika[i].r,slika[i].g,slika[i].b,&sh,&sc,&si);
			dd = dist_eli_c(kh, kc, ki, ddh, ddc, ddi, sh, sc, si);
			slika[i].a = skiny(dd);
			}
		break;
	case 14:	//ellipsoid, linear slope
		for (i=0;i<w*h;i++)
			{
			rgb2hci(ipi2,k32,slika[i].r,slika[i].g,slika[i].b,&sh,&sc,&si);
			dd = dist_eli_c(kh, kc, ki, ddh, ddc, ddi, sh, sc, si);
			slika[i].a = slope(dd, islp);
			}
		break;

	case 20:	//octahedron, thresholded
		for (i=0;i<w*h;i++)
			{
			rgb2hci(ipi2,k32,slika[i].r,slika[i].g,slika[i].b,&sh,&sc,&si);
			dd = dist_oct_c(kh, kc, ki, ddh, ddc, ddi, sh, sc, si);
			slika[i].a = thres(dd);
			}
		break;
	case 21:	//octahedron, linear fat
		for (i=0;i<w*h;i++)
			{
			rgb2hci(ipi2,k32,slika[i].r,slika[i].g,slika[i].b,&sh,&sc,&si);
			dd = dist_oct_c(kh, kc, ki, ddh, ddc, ddi, sh, sc, si);
			slika[i].a = fat(dd);
			}
		break;
	case 22:	//octahedron, linear normal
		for (i=0;i<w*h;i++)
			{
			rgb2hci(ipi2,k32,slika[i].r,slika[i].g,slika[i].b,&sh,&sc,&si);
			dd = dist_oct_c(kh, kc, ki, ddh, ddc, ddi, sh, sc, si);
			slika[i].a = norm(dd);
			}
		break;
	case 23:	//octahedron, linear skiny
		for (i=0;i<w*h;i++)
			{
			rgb2hci(ipi2,k32,slika[i].r,slika[i].g,slika[i].b,&sh,&sc,&si);
			dd = dist_oct_c(kh, kc, ki, ddh, ddc, ddi, sh, sc, si);
			slika[i].a = skiny(dd);
			}
		break;
	case 24:	//octahedron, linear slope
		for (i=0;i<w*h;i++)
			{
			rgb2hci(ipi2,k32,slika[i].r,slika[i].g,slika[i].b,&sh,&sc,&si);
			dd = dist_oct_c(kh, kc, ki, ddh, ddc, ddi, sh, sc, si);
			slika[i].a = slope(dd, islp);
			}
		break;
	default:
		break;
	}
}

//----------------------------------------
//struktura za instanco efekta
typedef struct
{
int h;
int w;
f0r_param_color_t col;
int subsp;
int sshape;
float del1,del2,del3;
float slp;
float nud1,nud2,nud3;
int soft;
int inv;
int op;

float_rgba *sl;
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

info->name="select0r";
info->author="Marko Cebokli";
info->plugin_type=F0R_PLUGIN_TYPE_FILTER;
info->color_model=F0R_COLOR_MODEL_RGBA8888;
info->frei0r_version=FREI0R_MAJOR_VERSION;
info->major_version=0;
info->minor_version=4;
info->num_params=10;
info->explanation="Color based alpha selection";
}

//--------------------------------------------------
void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
switch(param_index)
	{
	case 0:
		info->name = "Color to select";
		info->type = F0R_PARAM_COLOR;
		info->explanation = "";
		break;
	case 1:
		info->name = "Invert selection";
		info->type = F0R_PARAM_BOOL;
		info->explanation = "";
		break;
	case 2:
		info->name = "Delta R / A / Hue";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "";
		break;
	case 3:
		info->name = "Delta G / B / Chroma";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "";
		break;
	case 4:
		info->name = "Delta B / I / I";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "";
		break;
	case 5:
		info->name = "Slope";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "";
		break;
	case 6:
		info->name = "Selection subspace";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "";
		break;
	case 7:
		info->name = "Subspace shape";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "";
		break;
	case 8:
		info->name = "Edge mode";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "";
		break;
	case 9:
		info->name = "Operation";
		info->type = F0R_PARAM_DOUBLE;
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

in->col.r=0.0;
in->col.g=0.8;
in->col.b=0.0;
in->subsp=0;
in->sshape=0;
in->del1=0.2; in->del2=0.2; in->del3=0.2;
in->nud1=0.0; in->nud2=0.0; in->nud3=0.0;
in->slp=0.0;
in->soft=0;
in->inv=0;
in->op=0;

in->sl=(float_rgba*)calloc(in->w*in->h,sizeof(float_rgba));

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
double tmpf;
int tmpi,chg;
f0r_param_color_t tmpc;

p=(inst*)instance;

chg=0;
switch(param_index)
	{
	case 0:		//color
		tmpc=*(f0r_param_color_t*)parm;
		if ((tmpc.r!=p->col.r) || (tmpc.g!=p->col.g) || (tmpc.b!=p->col.b))
			chg=1;
		p->col=tmpc;
		break;
	case 1:		//invert
                tmpi=map_value_forward(*((double*)parm), 0.0, 1.0); //BOOL!!
                if (p->inv != tmpi) chg=1;
                p->inv=tmpi;
                break;
	case 2:		//delta 1
		tmpf=*(double*)parm;
		if (tmpf!=p->del1) chg=1;
		p->del1=tmpf;
		break;
	case 3:		//delta 2
		tmpf=*(double*)parm;
		if (tmpf!=p->del2) chg=1;
		p->del2=tmpf;
		break;
	case 4:		//delta 3
		tmpf=*(double*)parm;
		if (tmpf!=p->del3) chg=1;
		p->del3=tmpf;
		break;
	case 5:		//slope 1
		tmpf=*(double*)parm;
		if (tmpf!=p->slp) chg=1;
		p->slp=tmpf;
		break;
	case 6:		//subspace
		tmpi = map_value_forward(*(double*)parm, 0.0, 2.9999); //N-0.0001		if ((tmpi<0)||(tmpi>2.0)) break;
		if (p->subsp != tmpi) chg=1;
		p->subsp = tmpi;
                break;
	case 7:		//shape
		tmpi = map_value_forward(*(double*)parm, 0.0, 2.9999); //N-0.0001		if ((tmpi<0)||(tmpi>2.0)) break;
		if (p->sshape != tmpi) chg=1;
		p->sshape = tmpi;
		break;
	case 8:		//edge mode
		tmpi = map_value_forward(*(double*)parm, 0.0, 4.9999); //N-0.0001		if ((tmpi<0)||(tmpi>2.0)) break;
		if (p->soft != tmpi) chg=1;
		p->soft = tmpi;
		break;
	case 9:		//operation
		tmpi = map_value_forward(*(double*)parm, 0.0, 4.9999); //N-0.0001		if ((tmpi<0)||(tmpi>2.0)) break;
		if (p->op != tmpi) chg=1;
		p->op = tmpi;
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
		*((f0r_param_color_t*)param)=p->col;
		break;
	case 1:
                *((double*)param)=map_value_backward(p->inv, 0.0, 1.0);//BOOL!!
		break;
	case 2:
		*((double*)param)=p->del1;
		break;
	case 3:
		*((double*)param)=p->del2;
		break;
	case 4:
		*((double*)param)=p->del3;
		break;
	case 5:
		*((double*)param)=p->slp;
		break;
	case 6:
                *((double*)param)=map_value_backward(p->subsp, 0.0, 2.9999);
		break;
	case 7:
                *((double*)param)=map_value_backward(p->sshape, 0.0, 2.9999);
		break;
	case 8:
                *((double*)param)=map_value_backward(p->soft, 0.0, 3.9999);
		break;
	case 9:
                *((double*)param)=map_value_backward(p->op, 0.0, 4.9999);
		break;
	}
}

//-------------------------------------------------
//RGBA8888 little endian
void f0r_update(f0r_instance_t instance, double time, const uint32_t* inframe, uint32_t* outframe)
{
inst *in;
float_rgba key;
triplet d,n;
int i;
uint32_t t;
uint8_t *cin, *cout;
float f1=1.0/256.0;
uint8_t a1,a2;

assert(instance);
in=(inst*)instance;

key.r=in->col.r;
key.g=in->col.g;
key.b=in->col.b;
key.a=1.0;
d.x=in->del1;
d.y=in->del2;
d.z=in->del3;
n.x=in->nud1;
n.y=in->nud2;
n.z=in->nud3;

//convert to float
cin=(uint8_t *)inframe;
for (i=0;i<in->h*in->w;i++)
	{
	in->sl[i].r=f1*(float)*cin++;
	in->sl[i].g=f1*(float)*cin++;
	in->sl[i].b=f1*(float)*cin++;
	cin++;
	}

//make the selection
switch (in->subsp)
	{
	case 0:
		sel_rgb(in->sl, in->w, in->h, key, d, n, in->slp, in->sshape, in->soft);
		break;
	case 1:
		sel_abi(in->sl, in->w, in->h, key, d, n, in->slp, in->sshape, in->soft);
		break;
	case 2:
		sel_hci(in->sl, in->w, in->h, key, d, n, in->slp, in->sshape, in->soft);
		break;
	default:
		break;
	}

//invert selection if required
if (in->inv==1)
	for (i=0;i<in->h*in->w;i++)
		in->sl[i].a = 1.0 - in->sl[i].a;

//apply alpha
cin=(uint8_t *)inframe;
cout=(uint8_t *)outframe;
switch (in->op)
	{
	case 0:		//write on clear
		for (i=0;i<in->h*in->w;i++)
			{
			*cout++ = *cin++;	//copy R
			*cout++ = *cin++;	//copy G
			*cout++ = *cin++;	//copy B
			*cout++ = (uint8_t)(in->sl[i].a*255.0);
			cin++;
			}
		break;
	case 1:		//max
		for (i=0;i<in->h*in->w;i++)
			{
			*cout++ = *cin++;	//copy R
			*cout++ = *cin++;	//copy G
			*cout++ = *cin++;	//copy B
			a1 = *cin++;
			a2 = (uint8_t)(in->sl[i].a*255.0);
			*cout++ = (a1>a2) ? a1 : a2;
			}
		break;
	case 2:		//min
		for (i=0;i<in->h*in->w;i++)
			{
			*cout++ = *cin++;	//copy R
			*cout++ = *cin++;	//copy G
			*cout++ = *cin++;	//copy B
			a1 = *cin++;
			a2 = (uint8_t)(in->sl[i].a*255.0);
			*cout++ = (a1<a2) ? a1 : a2;
			}
		break;
	case 3:		//add
		for (i=0;i<in->h*in->w;i++)
			{
			*cout++ = *cin++;	//copy R
			*cout++ = *cin++;	//copy G
			*cout++ = *cin++;	//copy B
			a1 = *cin++;
			a2 = (uint8_t)(in->sl[i].a*255.0);
			t=(uint32_t)a1+(uint32_t)a2;
			*cout++ = (t<=255) ? (uint8_t)t : 255;
			}
		break;
	case 4:		//subtract
		for (i=0;i<in->h*in->w;i++)
			{
			*cout++ = *cin++;	//copy R
			*cout++ = *cin++;	//copy G
			*cout++ = *cin++;	//copy B
			a1 = *cin++;
			a2 = (uint8_t)(in->sl[i].a*255.0);
			*cout++ = (a1>a2) ? a1-a2 : 0;
			}
		break;
	default:
		break;
	}

}

//**********************************************************
