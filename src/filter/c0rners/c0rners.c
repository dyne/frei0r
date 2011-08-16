/*c0rners.c
 four corners geometry engine


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

//compile: gcc -Wall -std=c99 -c -fPIC c0rners.c -o c0rners.o
//link: gcc -shared -lm -o c0rners.so c0rners.o

#include <stdlib.h>
#include <stdio.h>
#include <frei0r.h>
#include <string.h>
#include <math.h>
#include "frei0r_math.h"
#include "interp.h"

//----------------------------------------
//structure for Frei0r instance
typedef struct
{
	int h;
	int w;
	float x1;
	float y1;
	float x2;
	float y2;
	float x3;
	float y3;
	float x4;
	float y4;
	int stretchON;
	float stretchx;
	float stretchy;
	int intp;
	int transb;
	float feath;
        int op;

	interpp interp;
	float *map;
	unsigned char *amap;
	int mapIsDirty;
} inst;


//2D point
typedef struct		//tocka v ravnini
{
	float x;
	float y;
} tocka2;

//2D line
typedef struct		//premica v ravnini
{
	float a;
	float b;
	float c;
	float sa;	//se normalna oblika
	float ca;
	float p;
} premica2;

//------------------------------------------------------
//premica skozi dve tocki v ravnini (2D)
//izracuna a,b,c za enacbo premice:   ax + by + c = 0
//ps se sin a, cos a in p za normalno obliko
//vrne: (return value)
//-10	ce ni mozno dolociti p. (tocki sovpadata) coincident points
//0	splosna p.			general case
//+1	navpicna p.	x v *a		vertical
//+2	vodoravna p.	y v *b		horizontal
//a line through two points
int premica2d(tocka2 t1, tocka2 t2, premica2 *p)
{
	float dx,dy,m;

	dx=t2.x-t1.x;
	dy=t2.y-t1.y;
	if (dx==0.0)	//navpicna
	{
		if (dy==0.0) return -10;
		p->a=1.0;
		p->b=0.0;
		p->c=-t1.x;
		m=1.0/p->a; if (p->c>=0) m=-m;
		p->sa=m;
		p->ca=0.0;
		p->p=m*p->c;
		return 1;
	}

	if (dy==0.0)	//vodoravna
	{
		if (dx==0.0) return -10;
		p->a=0.0;
		p->b=1.0;
		p->c=-t1.y;
		m=1.0/p->b; if (p->c>=0) m=-m;
		p->sa=0.0;
		p->ca=m;
		p->p=m*p->c;
		return 2;
	}

	//posevna
	p->a=1.0/dx;
	p->b=-1.0/dy;
	p->c=t1.y/dy-t1.x/dx;
	m=1.0/sqrtf(p->a*p->a+p->b*p->b); if (p->c>=0) m=-m;
	p->sa=m*p->a;
	p->ca=m*p->b;
	p->p=m*p->c;
	return 0;
}

//-----------------------------------------------------
//razdalja tocke od premice (za alpha feather)
//distance between a point and a line
//needed only for alpha feathering
float razd_t_p(tocka2 t, premica2 p)
{
	float r;

	r = t.x*p.ca + t.y*p.sa + p.p;

	return r;
}

//-----------------------------------------------------
//presecisce dveh premic v ravnini (2D)
//vrne:
//0  ce je vse OK
//-1 ce sta premici vzporedni
//intersection of two lines
int presecisce2(premica2 p1, premica2 p2, tocka2 *t)
{
	float d1,d2,d3;

	d1=p1.a*p2.b-p2.a*p1.b;
	if (d1==0.0)	//vzporedni
	{
		return -1;
	}
	d2=p1.b*p2.c-p2.b*p1.c;
	d3=p1.c*p2.a-p2.c*p1.a;
	t->x=d2/d1;
	t->y=d3/d1;
	return 0;
}

//---------------------------------------------------------------
//generate mapping for a general quadrangle
//wi,hi = input image size
//wo,ho = output image size
//vog[] = the four corners
//str: 0=no stretch  1=do stretch
//strx,stry:	stretch values   [0...1]   0.5 = no stretch
void cetverokotnik4(int wi, int hi, int wo, int ho, tocka2 vog[], int str, float strx, float stry, float *map)
{
	double a,b,c,d,e,f,g,h,a2,b2,c2,u,v,aa,bb,de,sde,v1,v2,u1,u2;
	tocka2 T;
	int x,y;
	float kx,ky,k1,k2;

	de=0.0;v1=1000.0;v2=1000.0;	//da compiler ne jamra

	kx=4.0*2.0*fabsl(strx-0.5)+0.00005;
	k1=1.0-1.0/(kx+1.0);
	ky=4.0*2.0*fabsl(stry-0.5)+0.00005;
	k2=1.0-1.0/(ky+1.0);

	for (y=0;y<ho;y++)
	{
		for (x=0;x<wo;x++)
		{
			T.x=(float)x+0.5;T.y=(float)y+0.5;

			//enacba za Xt, prva moznost
			a=vog[0].x-T.x; b=vog[1].x-vog[0].x;
			c=vog[3].x-vog[0].x;
			d=vog[2].x-vog[1].x-(vog[3].x-vog[0].x);
			//enacba za Xt, druga moznost
			//		a=vog[0].x-T.x; b=vog[1].x-vog[0].x;
			//		c=vog[2].x-vog[0].x;
			//		d=vog[3].x-vog[2].x-(vog[1].x-vog[0].x);

			//enacba za Yt, prva moznost
			e=vog[0].y-T.y; f=vog[1].y-vog[0].y;
			g=vog[3].y-vog[0].y;
			h=vog[2].y-vog[1].y-(vog[3].y-vog[0].y);
			//enacba za Yt, druga moznost
			//		e=vog[0].y-T.y; f=vog[1].y-vog[0].y;
			//		g=vog[2].y-vog[0].y;
			//		h=vog[3].y-vog[2].y-(vog[1].y-vog[0].y);

			//resitev za v in u
			a2=g*d-h*c; b2=e*d-f*c-h*a+g*b; c2=e*b-f*a;
			//linearni priblizek uporabim, ce je napaka < 1/10 piksla
			//		if (fabsl(a2*c2*c2/(b2*b2*b2))< 0.1/wi)
			//dodaten pogoj za a2, da ni spranje v konkavnih
			if ((fabsl(a2*c2*c2/(b2*b2*b2))< 0.1/wi) && (fabsl(a2)<1.0))
			{
				v1 = (b2!=0.0) ? -c2/b2 : 1000.0;
				v2=1000.0;
			}
			else
			{
				de=b2*b2-4.0*a2*c2;
				if (de>=0.0)
				{
					sde=sqrt(de);
					v1=(-b2+sde)/2.0/a2;
					v2=(-b2-sde)/2.0/a2;
				}
				else
				{
					v1=1001.0;  //krneki zunaj
					v2=1001.0;  //krneki zunaj
				}
			}
			aa=b+d*v1; bb=f+h*v1;
			if (fabsf(aa)>fabsf(bb))
				u1 = (aa!=0.0) ? -(a+c*v1)/aa : 1000.0;
			else
				u1 = (bb!=0.0) ? -(e+g*v1)/bb : 1000.0;
			aa=b+d*v2; bb=f+h*v2;
			if (fabsf(aa)>fabsf(bb))
				u2 = (aa!=0.0) ? -(a+c*v2)/aa : 1000.0;
			else
				u2 = (bb!=0.0) ? -(e+g*v2)/bb : 1000.0;

			if ((u1>0.0)&&(u1<1.0)&&(v1>0.0)&&(v1<1.0))
			{
				u=u1;
				v=v1;
			}
			else
			{
				if ((u2>0.0)&&(u2<1.0)&&(v2>0.0)&&(v2<1.0))
				{
					u=u2;
					v=v2;
				}
				else
				{
					u=1002.0;
					v=1002.0;
				}
			}

			//if requested, apply stretching
			if (str!=0)
			{
				if (strx>0.5)
					u=(1.0-1.0/(kx*u+1.0))/k1;
				else
					u=1.0-(1.0-1.0/(kx*(1.0-u)+1.0))/k1;
				if (stry>0.5)
					v=(1.0-1.0/(ky*v+1.0))/k2;
				else
					v=1.0-(1.0-1.0/(ky*(1.0-v)+1.0))/k2;
			}

			//zdaj samo se vpise izracunana (u,v) v map[]
			if ((u>=0.0)&&(u<=1.0)&&(v>=0.0)&&(v<=1.0))
			{	//ce smo znotraj orig slike
				map[2*(y*wo+x)]=u*(wi-1);
				map[2*(y*wo+x)+1]=v*(hi-1);
			}
			else
			{
				map[2*(y*wo+x)]=-1;
				map[2*(y*wo+x)+1]=-1;
			}

		}
	}

}

//---------------------------------------------------------------
//generate mapping for a triangle
void trikotnik1(int wi, int hi, int wo, int ho, tocka2 vog[], tocka2 R, tocka2 S, premica2 p12, premica2 p23, premica2 p34, premica2 p41, int t12, int t23, int str, float strx, float stry, float *map)
{
	int x,y;
	tocka2 T,A,B;
	premica2 p5,p6;
	float u,v;
	float kx,ky,k1,k2;

	kx=4.0*2.0*fabsl(strx-0.5)+0.00005;
	k1=1.0-1.0/(kx+1.0);
	ky=4.0*2.0*fabsl(stry-0.5)+0.00005;
	k2=1.0-1.0/(ky+1.0);

	for (y=0;y<ho;y++)
	{
		for (x=0;x<wo;x++)
		{
			T.x=(float)x+0.5;T.y=(float)y+0.5;
			premica2d(T,R,&p5);
			presecisce2(p5,p12,&A);
			if (t12!=-10)	//razlicno od cetverokotnika
			{ //tocka je med preseciscem in r
				if (fabsf(p12.a)>fabsf(p12.b))  //bolj pokonci
					u=(A.y-vog[0].y)/(vog[1].y-vog[0].y);
				else
					u=(A.x-vog[0].x)/(vog[1].x-vog[0].x);
			}
			else
			{
				presecisce2(p5,p34,&A);
				if (fabsf(p34.a)>fabsf(p34.b))  //bolj pokonci
					u=(A.y-vog[3].y)/(vog[2].y-vog[3].y);
				else
					u=(A.x-vog[3].x)/(vog[2].x-vog[3].x);
			}

			premica2d(T,S,&p6);
			presecisce2(p6,p23,&B);
			if (t23!=-10)	//razlicno od cetverokotnika
			{
				if (fabsf(p23.a)>fabsf(p23.b))  //bolj pokonci
					v=(B.y-vog[1].y)/(vog[2].y-vog[1].y);
				else
					v=(B.x-vog[1].x)/(vog[2].x-vog[1].x);
			}
			else
			{
				presecisce2(p6,p41,&B);
				if (fabsf(p41.a)>fabsf(p41.b))  //bolj pokonci
					v=(B.y-vog[0].y)/(vog[3].y-vog[0].y);
				else
					v=(B.x-vog[0].x)/(vog[3].x-vog[0].x);
			}

			//if requested, apply stretching
			if (str!=0)
			{
				if (strx>0.5)
					u=(1.0-1.0/(kx*u+1.0))/k1;
				else
					u=1.0-(1.0-1.0/(kx*(1.0-u)+1.0))/k1;
				if (stry>0.5)
					v=(1.0-1.0/(ky*v+1.0))/k2;
				else
					v=1.0-(1.0-1.0/(ky*(1.0-v)+1.0))/k2;
			}

			//zdaj samo se vpise izracunana (u,v) v map[]
			if ((u>=0.0)&&(u<=1.0)&&(v>=0.0)&&(v<=1.0))
			{	//ce smo znotraj orig slike
				map[2*(y*wo+x)]=u*(wi-1);
				map[2*(y*wo+x)+1]=v*(hi-1);
			}
			else
			{
				map[2*(y*wo+x)]=-1;
				map[2*(y*wo+x)+1]=-1;
			}
		}
	}
}

//-------------------------------------------------------
//generates a map of alpha values for transparent background
//with feathered (soft) border
//feath = soft border width  0..max in pixels
//amap = generated alpha map
//map = map generated by geom4c_b()
//nots[] = flags for inner sides
//for now it does not feather caustics on concaves an crossed sides
void make_alphamap(unsigned char *amap, tocka2 vog[], int wo, int ho, float *map, float feath, int nots[])
{
	float r12, r23, r34, r41, rmin;
	tocka2 t;
	int i,j;
	premica2 p12,p23,p34,p41;

	premica2d(vog[0],vog[1],&p12);	//  1-2
	premica2d(vog[2],vog[3],&p34);	//  3-4
	premica2d(vog[3],vog[0],&p41);	//  4-1
	premica2d(vog[1],vog[2],&p23);	//  2-3

	for (i=0;i<ho;i++)
		for (j=0;j<wo;j++)
		{
		t.x=(float)i+0.5; t.y=(float)j+0.5;
		r12=fabsf(razd_t_p(t,p12));
		r23=fabsf(razd_t_p(t,p23));
		r34=fabsf(razd_t_p(t,p34));
		r41=fabsf(razd_t_p(t,p41));
		rmin=1.0E22;
		if ((r12<rmin) && (nots[0]!=1)) rmin=r12;
		if ((r23<rmin) && (nots[1]!=1)) rmin=r23;
		if ((r34<rmin) && (nots[2]!=1)) rmin=r34;
		if ((r41<rmin) && (nots[3]!=1)) rmin=r41;
		if ((map[2*(i*wo+j)]>=0.0)&&(map[2*(i*wo+j)+1]>=0.0))
		{	//inside
			if (rmin<=feath) //border area
				amap[i*wo+j]=255*(rmin/feath);
			else
				amap[i*wo+j]=255;
		}
		else		//outside
			amap[i*wo+j]=0;
	}
}

//-------------------------------------------------------
void apply_alphamap(uint32_t* frame, int w, int h, unsigned char *amap, int operation)
{
	int i,j, length;
	uint32_t t;
        length = w * h;

        switch (operation)
        {
        case 0:         //write on clear
            for (i=0;i<length;i++)
            {
                t=((uint32_t)amap[i])<<24;
                frame[i] = (frame[i]&0x00FFFFFF) | t;
            }
            break;
        case 1:         //max
            for (i=0;i<length;i++)
            {
                t=((uint32_t)amap[i])<<24;
                frame[i] = (frame[i]&0x00FFFFFF) | MAX(frame[i]&0xFF000000, t);
            }
            break;
        case 2:         //min
            for (i=0;i<length;i++)
            {
                t=((uint32_t)amap[i])<<24;
                frame[i] = (frame[i]&0x00FFFFFF) | MIN(frame[i]&0xFF000000, t);
            }
            break;
        case 3:         //add
            for (i=0;i<length;i++)
            {
                t=((uint32_t)amap[i])<<24;
                t=((frame[i]&0xFF000000)>>1)+(t>>1);
                t = (t>0x7F800000) ? 0xFF000000 : t<<1;
                frame[i] = (frame[i]&0x00FFFFFF) | t;
            }
            break;
        case 4:         //subtract
            for (i=0;i<length;i++)
            {
                t=((uint32_t)amap[i])<<24;
                t= ((frame[i]&0xFF000000)>t) ? (frame[i]&0xFF000000)-t : 0;
                frame[i] = (frame[i]&0x00FFFFFF) | t;
            }
            break;
        default:
            break;
        }
}

//---------------------------------------------------------------
//funkcija za byte polja (char)
//generate map from the four corners
//first checks for different types of degenerate geometrty...
//wi,hi		input image size
//wo,ho		output image size
//vog[]		the four corners
//nots[]		"inner" sides (for alpha feathering)
int geom4c_b(int wi, int hi, int wo, int ho, tocka2 vog[], int str, float strx, float stry, float *map, int nots[])
{
	premica2 p12,p23,p34,p41;
	tocka2 R,S;
	int r41,r23,s12,s34;		//tocki R in S
	int p1,p2;			//paralelnost stranic
	int t12,t23,t34,t41;		//sovpadanje tock
	int tip;	//1=degen trik 2=paral 3=splosni 4=twist 5=konkavni
	int i;

	for (i=0;i<4;i++)	//convert indexes to positions (pixel)
	{
		vog[i].x=vog[i].x+0.5;
		vog[i].y=vog[i].y+0.5;
	}

	tip=3;

	t12=premica2d(vog[0],vog[1],&p12);	//  1-2
	t34=premica2d(vog[2],vog[3],&p34);	//  3-4
	t41=premica2d(vog[3],vog[0],&p41);	//  4-1
	t23=premica2d(vog[1],vog[2],&p23);	//  2-3

	//preveri degeneracijo v crto ali piko
	//check for degeneration into a line or point
	if ((t12+t34+t41+t23)<-19)
	{  //vec kot dve sovpadata
		//daj tu fill with background??
		return 0;
	}
	if (((vog[0].x==vog[2].x)&&(vog[0].y==vog[2].y)) ||          ((vog[1].x==vog[3].x)&&(vog[1].y==vog[3].y)))
	{  //sovpadata dve diagonalni tocki
		//daj tu fill with background??
		return 0;
	}

	p1=presecisce2(p12,p34,&S);		//tocka S
	p2=presecisce2(p41,p23,&R);		//tocka R

	//preveri degeneracijo v trikotnik (sovpadanje nediagonalnih tock)
	//check for degeneration into triangle (coincident non-diagonal c.)
	if (t12==-10) {R=vog[0];p12=p34;p1=-1;tip=1;}
	if (t34==-10) {R=vog[2];p34=p12;p1=-1;tip=1;}
	if (t41==-10) {S=vog[0];p41=p23;p2=-1;tip=1;}
	if (t23==-10) {S=vog[2];p23=p41;p2=-1;tip=1;}

	//preveri vzporednost
	//check parallelity
	if (p1==-1)	//vzporedni   1-2 in 3-4
	{
		if (fabsf(p12.a)>fabsf(p12.b))	//bolj pokonci
		{S.y=1.0E9;S.x=-(p12.b*S.y+p12.c)/p12.a;}
		else
		{S.x=1.0E9;S.y=-(p12.a*S.x+p12.c)/p12.b;}
	}
	if (p2==-1)	//vzporedni   2-3 in 4-1
	{
		if (fabsf(p41.a)>fabsf(p41.b))	//bolj pokonci
		{R.y=1.0E9;R.x=-(p41.b*R.y+p41.c)/p41.a;}
		else
		{R.x=1.0E9;R.y=-(p41.a*R.x+p41.c)/p41.b;}
	}

	//pogleda, ce je priblizno paralelogram
	//check if approximately parallelogram
	if (((fabsf(R.x)>1000000.0)||(fabsf(R.y)>1000000.0)) &&
		((fabsf(S.x)>1000000.0)||(fabsf(S.y)>1000000.0)))
		tip=2;

	//preveri, ce je prekrizan ali konkaven  (R ali S med ogljici)
	//check for concave or crossed sides
	r41=0;r23=0;s12=0;s34=0;

	if (fabsf(p41.a)>fabsf(p41.b))	//bolj pokonci
	{if (((R.y-vog[3].y)*(R.y-vog[0].y))<0.0) r41=1;}//R na 4-1
	else
	{if (((R.x-vog[3].x)*(R.x-vog[0].x))<0.0) r41=1;}//R na 4-1

	if (fabsf(p23.a)>fabsf(p23.b))	//bolj pokonci
	{if (((R.y-vog[1].y)*(R.y-vog[2].y))<0.0) r23=1;}//R na 2-3
	else
	{if (((R.x-vog[1].x)*(R.x-vog[2].x))<0.0) r23=1;}//R na 2-3

	if (fabsf(p12.a)>fabsf(p12.b))	//bolj pokonci
	{if (((S.y-vog[0].y)*(S.y-vog[1].y))<0.0) s12=1;}//S na 1-2
	else
	{if (((S.x-vog[0].x)*(S.x-vog[1].x))<0.0) s12=1;}//S na 1-2

	if (fabsf(p34.a)>fabsf(p34.b))	//bolj pokonci
	{if (((S.y-vog[2].y)*(S.y-vog[3].y))<0.0) s34=1;}//S na 3-4
	else
	{if (((S.x-vog[2].x)*(S.x-vog[3].x))<0.0) s34=1;}//S na 3-4

	if (((r41+r23+s12+s34)>0)&&(tip==3))
	{
		if ((r41*r23+s12*s34)==0)	//konkaven
			tip=5;
		else	//prekrizan
			tip=4;
	}

	//prepare nots[] flags
	nots[0]=nots[1]=nots[2]=nots[3]=0;
	if (tip==4)
	{
		nots[0] = (s12==0) ? 0 : 1;
		nots[1] = (r23==0) ? 0 : 1;
		nots[2] = (s34==0) ? 0 : 1;
		nots[3] = (r41==0) ? 0 : 1;
	}
	if (tip==5)
	{
		nots[2] = (s12==0) ? 0 : 1;
		nots[3] = (r23==0) ? 0 : 1;
		nots[0] = (s34==0) ? 0 : 1;
		nots[1] = (r41==0) ? 0 : 1;
	}

	//OK, zdaj gremo risat...
	switch (tip)
	{
	case 0:		//should never come to here...
		break;
	case 1:		//triangle
		trikotnik1(wi, hi, wo, ho, vog, R, S, p12, p23, p34, p41, t12, t23, str, strx, stry, map);
		break;
	case 2:		//paralelogram
		//a faster algorithm could be used here...
	case 3:		//general quadrangle
	case 4:		//crossed sides
	case 5:		//concave quadrangle
		cetverokotnik4(wi, hi, wo, ho, vog, str, strx, stry, map);
		break;
	}

	return 0;
}

//-------------------------------------------------------
interpp set_intp(inst p)
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

	info->name="c0rners";
	info->author="Marko Cebokli";
	info->plugin_type=F0R_PLUGIN_TYPE_FILTER;
	info->color_model=F0R_COLOR_MODEL_RGBA8888;
	info->frei0r_version=FREI0R_MAJOR_VERSION;
	info->major_version=0;
	info->minor_version=1;
	info->num_params=15;
	info->explanation="Four corners geometry engine";
}

//--------------------------------------------------
void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{

	switch(param_index)
	{
	case 0:
		info->name = "Corner 1 X";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "X coordinate of corner 1";
		break;
	case 1:
		info->name = "Corner 1 Y";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "Y coordinate of corner 1";
		break;
	case 2:
		info->name = "Corner 2 X";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "X coordinate of corner 2";
		break;
	case 3:
		info->name = "Corner 2 Y";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "Y coordinate of corner 2";
		break;
	case 4:
		info->name = "Corner 3 X";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "X coordinate of corner 3";
		break;
	case 5:
		info->name = "Corner 3 Y";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "Y coordinate of corner 3";
		break;
	case 6:
		info->name = "Corner 4 X";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "X coordinate of corner 4";
		break;
	case 7:
		info->name = "Corner 4 Y";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "Y coordinate of corner 4";
		break;
	case 8:
		info->name = "Enable Stretch";
		info->type = F0R_PARAM_BOOL;
		info->explanation = "Enable stretching";
		break;
	case 9:
		info->name = "Stretch X";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "Amount of stretching in X direction";
		break;
	case 10:
		info->name = "Stretch Y";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "Amount of stretching in Y direction";
		break;
	case 11:
		info->name = "Interpolator";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "Quality of interpolation";
		break;
	case 12:
		info->name = "Transparent Background";
		info->type = F0R_PARAM_BOOL;
		info->explanation = "Makes background transparent";
		break;
	case 13:
		info->name = "Feather Alpha";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "Makes smooth transition into transparent";
		break;
        case 14:
            info->name = "Alpha operation";
            info->type = F0R_PARAM_DOUBLE;
            info->explanation = "";
	}
}

//----------------------------------------------
f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
	inst *in;

	in=(inst*)calloc(1, sizeof(inst));
	in->w=width;
	in->h=height;
	in->x1=0.333333;
	in->y1=0.333333;
	in->x2=0.666666;
	in->y2=0.333333;
	in->x3=0.666666;
	in->y3=0.666666;
	in->x4=0.333333;
	in->y4=0.666666;
	in->stretchON=0;
	in->stretchx=0.5;
	in->stretchy=0.5;
	in->intp=1;
	in->transb=0;
	in->feath=1.0;
        in->op=0;

	in->map=(float*)calloc(1, sizeof(float)*(in->w*in->h*2+2));
	in->amap=(unsigned char*)calloc(1, sizeof(char)*(in->w*in->h*2+2));
	in->interp=set_intp(*in);
	in->mapIsDirty=1;

	return (f0r_instance_t)in;
}

//---------------------------------------------------
void f0r_destruct(f0r_instance_t instance)
{
	inst *p;

	p=(inst*)instance;

	free(p->map);
	free(p->amap);
	free(instance);
}

//-----------------------------------------------------
void f0r_set_param_value(f0r_instance_t instance, f0r_param_t parm, int param_index)
{
	inst *p;
	double tmpf;
	int chg;

	p=(inst*)instance;

	chg=0;
	switch(param_index)
	{
	case 0:		//X coordinate of corner 1
		tmpf=*(double*)parm;
		if (tmpf!=p->x1) chg=1;
		p->x1=tmpf;
		break;
	case 1:		//Y coordinate of corner 1
		tmpf=*(double*)parm;
		if (tmpf!=p->y1) chg=1;
		p->y1=tmpf;
		break;
	case 2:		//X coordinate of corner 2
		tmpf=*(double*)parm;
		if (tmpf!=p->x2) chg=1;
		p->x2=tmpf;
		break;
	case 3:		//Y coordinate of corner 2
		tmpf=*(double*)parm;
		if (tmpf!=p->y2) chg=1;
		p->y2=tmpf;
		break;
	case 4:		//X coordinate of corner 3
		tmpf=*(double*)parm;
		if (tmpf!=p->x3) chg=1;
		p->x3=tmpf;
		break;
	case 5:		//Y coordinate of corner 3
		tmpf=*(double*)parm;
		if (tmpf!=p->y3) chg=1;
		p->y3=tmpf;
		break;
	case 6:		//X coordinate of corner 4
		tmpf=*(double*)parm;
		if (tmpf!=p->x4) chg=1;
		p->x4=tmpf;
		break;
	case 7:		//Y coordinate of corner 4
		tmpf=*(double*)parm;
		if (tmpf!=p->y4) chg=1;
		p->y4=tmpf;
		break;
	case 8:		//Enable stretching
		tmpf=map_value_forward(*((double*)parm), 0.0, 1.0);//BOOL!!
		if (p->stretchON != tmpf) chg=1;
		p->stretchON = tmpf;
		break;
	case 9:		//Stretch X
		tmpf=*(double*)parm;
		if (tmpf!=p->stretchx) chg=1;
		p->stretchx=tmpf;
		break;
	case 10:		//Stretch Y
		tmpf=*(double*)parm;
		if (tmpf!=p->stretchy) chg=1;
		p->stretchy=tmpf;
		break;
	case 11:		//Interpolation
		tmpf=map_value_forward(*((double*)parm), 0.0, 6.999);
		if (p->intp != tmpf) chg=1;
		p->intp=tmpf;
		break;
	case 12:		//Transparent Background
		tmpf=map_value_forward(*((double*)parm), 0.0, 1.0);//BOOL!!
		//		if (p->transb != tmpf) chg=1;
		p->transb = tmpf;
		break;
	case 13:		//Feather Alpha
		tmpf=map_value_forward(*((double*)parm), 0.0, 100.0);
		if (tmpf!=p->feath) chg=1;
		p->feath=tmpf;
		break;
        case 14:                //Alpha operation
            p->op=map_value_forward(*((double*)parm), 0.0, 4.9999);
            break;
	}

	if (chg!=0)
	{
		p->interp=set_intp(*p);
		p->mapIsDirty = 1;
	}

}

//--------------------------------------------------
void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index)
{
	inst *p;
	double tmpf;

	p=(inst*)instance;

	switch(param_index)
	{
	case 0:		//X coordinate of corner 1
		tmpf=(float)p->x1;
		*((double*)param)=tmpf;
		break;
	case 1:		//Y coordinate of corner 1
		tmpf=(float)p->y1;
		*((double*)param)=tmpf;
		break;
	case 2:		//X coordinate of corner 2
		tmpf=(float)p->x2;
		*((double*)param)=tmpf;
		break;
	case 3:		//Y coordinate of corner 2
		tmpf=(float)p->y2;
		*((double*)param)=tmpf;
		break;
	case 4:		//X coordinate of corner 3
		tmpf=(float)p->x3;
		*((double*)param)=tmpf;
		break;
	case 5:		//Y coordinate of corner 3
		tmpf=(float)p->y3;
		*((double*)param)=tmpf;
		break;
	case 6:		//X coordinate of corner 4
		tmpf=(float)p->x4;
		*((double*)param)=tmpf;
		break;
	case 7:		//Y coordinate of corner 4
		tmpf=(float)p->y4;
		*((double*)param)=tmpf;
		break;
	case 8:		//Enable stretching
		*((double*)param)=map_value_backward(p->stretchON, 0.0, 1.0); //BOOL!!
		break;
	case 9:		//Stretch X
		tmpf=(float)p->stretchx;
		*((double*)param)=tmpf;
		break;
	case 10:		//Stretch Y
		tmpf=(float)p->stretchy;
		*((double*)param)=tmpf;
		break;
	case 11:		//Interpolation
		*((double*)param)=map_value_backward(p->intp, 0.0, 6.0);	//!!!!!! 6.999 ???? tudi v defish!!!!
		break;
	case 12:		//Transparent Background
		*((double*)param)=map_value_backward(p->transb, 0.0, 1.0); //BOOL!!
		break;
	case 13:		//Feather Alpha
		*((double*)param)=map_value_backward(p->feath, 0.0, 100.0);
		break;
        case 14:                //Alpha operation
                *((double*)param)=map_value_backward(p->op, 0.0, 4.9999);
                break;
	}
}


//-------------------------------------------------
void f0r_update(f0r_instance_t instance, double time, const uint32_t* inframe, uint32_t* outframe)
{
	inst *p;
	int bkgr;

	p=(inst*)instance;

	if (p->mapIsDirty) {
		tocka2 vog[4];
		int nots[4];
		vog[0].x=(p->x1*3-1)*p->w;
		vog[0].y=(p->y1*3-1)*p->h;
		vog[1].x=(p->x2*3-1)*p->w;
		vog[1].y=(p->y2*3-1)*p->h;
		vog[2].x=(p->x3*3-1)*p->w;
		vog[2].y=(p->y3*3-1)*p->h;
		vog[3].x=(p->x4*3-1)*p->w;
		vog[3].y=(p->y4*3-1)*p->h;
		geom4c_b(p->w, p->h, p->w, p->h, vog, p->stretchON, p->stretchx, p->stretchy, p->map, nots);
		make_alphamap(p->amap, vog, p->w, p->h, p->map, p->feath, nots);
		p->mapIsDirty = 0;
	}

	//if (p->transb==0) bkgr=0xFF000000; else bkgr=0;
	bkgr=0xFF000000;

	remap32(p->w, p->h, p->w, p->h, (unsigned char*) inframe, (unsigned char *) outframe, p->map, bkgr, p->interp);

	if (p->transb!=0)
		apply_alphamap(outframe, p->w, p->h, p->amap, p->op);

}
