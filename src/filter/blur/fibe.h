//fibe.h	FAST IIR BLUR ENGINE

/*
Copyright (C) 2011  Marko Cebokli    http://lea.hamradio.si/~s57uuu


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

/*  
   NOTE: float_rgba must be declared externally as

 typedef struct
    {
    float r;
    float g;
    float b;
    float a;
    } float_rgba;

    --------------------------------
       CONTENTS OF FIBE.H FILE:
    --------------------------------

calcab_lp1()	auxiliary function to calculate lowpass
        tap coefficients for FIBE-2

young_vliet()	auxiliary function to calculate tap coefs
        for Gauss approximation with FIBE-3

rep()		auxiliary function to calculate wraparound
        values for FIBE-2

fibe1o_8()	one tap quadrilateral IIR filter
        speed optimized C function
        includes 8bit/float conversions

fibe2o_8()	two tap quadrilateral IIR filter
        speed optimized C function
        includes 8bit/float conversions

fibe3_8()	three tap quadrilateral IIR filter
        includes 8bit/float conversions

The functions work internally with floats. I have included
the 8bit/float conversion into the first and last
processing loops, to avoid two additional cache polluting
and therefore time consuming "walks" through memory.

*/



//edge compensation average size
#define EDGEAVG 8

#include <sys/types.h>
#include <string.h>
#include "frei0r/math.h"

//---------------------------------------------------------
//koeficienti za biquad lowpass  iz f in q
// f v Nyquistih    0.0 < f < 0.5
void calcab_lp1(float f, float q, float *a0, float *a1, float *a2, float *b0, float *b1, float *b2)
{
    float a,b;

    a=sinf(PI*f)/2.0/q;
    b=cosf(PI*f);
    *b0=(1.0-b)/2.0;
    *b1=1.0-b;
    *b2=(1.0-b)/2.0;
    *a0=1.0+a;
    *a1=-2.0*b;
    *a2=1.0-a;
}

//---------------------------------------------------------
//3tap iir coefficients for Gauss approximation according to:
//Ian T. Young, Lucas J. van Vliet:
//Recursive implementation of the Gaussian filter
//Signal Processing 44 (1995) 139-151
// s=sigma    0.5 < s < 200.0
void young_vliet(float s, float *a0, float *a1, float *a2, float *a3)
{
    float q;

    q=0.0;
    if (s>2.5)
    {
        q = 0.98711*s - 0.96330;
    }
    else
    {	//to velja za s>0.5 !!!!
        q = 3.97156 - 4.14554*sqrtf(1.0-0.26891*s);
    }

    *a0 = 1.57825 + 2.44413*q + 1.4281*q*q + 0.422205*q*q*q;
    *a1 = 2.44413*q + 2.85619*q*q + 1.26661*q*q*q;
    *a2 = -1.4281*q*q - 1.26661*q*q*q;
    *a3 = 0.422205*q*q*q;
}

//---------------------------------------------------
//kompenzacija na desni
//c=0.0 "odziv na zacetno stanje" (zunaj crno)
//gain ni kompenziran
void rep(float v1, float v2, float c, float *i1, float *i2, int n,  float a1, float a2)
{
    int i;
    float lb[8192];

    lb[0]=v1;lb[1]=v2;
    for (i=2;i<n-2;i++)
    {
        lb[i]=c-a1*lb[i-1]-a2*lb[i-2];
    }

    lb[n-2]=0.0;lb[n-1]=0.0;
    for (i=n-3;i>=0;i--)
    {
        lb[i]=lb[i]-a1*lb[i+1]-a2*lb[i+2];
    }

    *i1=lb[0]; *i2=lb[1];
}

//---------------------------------------------------------
// 1-tap IIR v 4 smereh
//optimized for speed
//loops rearanged for more locality (better cache hit ratio)
//outer (vertical) loop 2x unroll to break dependency chain
//simplified indexes
void fibe1o_8(const uint32_t* inframe, uint32_t* outframe, float_rgba *s, int w, int h, float a, int ec)
{
    int i,j;
    float b,g,g4,avg,avg1,cr,cg,cb,g4a,g4b;
    int p,pw,pj,pwj,pww,pmw;

    avg=EDGEAVG;	//koliko vzorcev za povprecje pri edge comp
    avg1=1.0/avg;

    g=1.0/(1.0-a);
    g4=1.0/g/g/g/g;

    //predpostavimo, da je "zunaj" crnina (nicle)
    b=1.0/(1.0-a)/(1.0+a);

    //prvih avg vrstic
    for (i=0;i<avg;i++)
    {
        p=i*w;pw=p+w;
        if (ec!=0)
        {
            cr=0.0;cg=0.0;cb=0.0;
            for (j=0;j<avg;j++)
            {
                s[p+j].r=(float)(inframe[p+j]&0xFF);
                s[p+j].g=(float)((inframe[p+j]&0xFF00)>>8);
                s[p+j].b=(float)((inframe[p+j]&0xFF0000)>>16);
                cr=cr+s[p+j].r;
                cg=cg+s[p+j].g;
                cb=cb+s[p+j].b;
            }
            cr=cr*avg1; cg=cg*avg1; cb=cb*avg1;
            s[p].r=cr*g+b*(s[p].r-cr);
            s[p].g=cg*g+b*(s[p].g-cg);
            s[p].b=cb*g+b*(s[p].b-cb);
        }
        else
            for (j=0;j<avg;j++)
            {
                s[p+j].r=(float)(inframe[p+j]&0xFF);
                s[p+j].g=(float)((inframe[p+j]&0xFF00)>>8);
                s[p+j].b=(float)((inframe[p+j]&0xFF0000)>>16);
            }

        for (j=1;j<avg;j++)	//tja  (ze pretvorjeni)
        {
            s[p+j].r=s[p+j].r+a*s[p+j-1].r;
            s[p+j].g=s[p+j].g+a*s[p+j-1].g;
            s[p+j].b=s[p+j].b+a*s[p+j-1].b;
        }
        for (j=avg;j<w;j++)	//tja  (s pretvorbo)
        {
            s[p+j].r=(float)(inframe[p+j]&0xFF);
            s[p+j].g=(float)((inframe[p+j]&0xFF00)>>8);
            s[p+j].b=(float)((inframe[p+j]&0xFF0000)>>16);
            s[p+j].r=s[p+j].r+a*s[p+j-1].r;
            s[p+j].g=s[p+j].g+a*s[p+j-1].g;
            s[p+j].b=s[p+j].b+a*s[p+j-1].b;
        }

        if (ec!=0)
        {
            cr=0.0;cg=0.0;cb=0.0;
            for (j=w-avg;j<w;j++)
            {
                cr=cr+s[p+j].r;
                cg=cg+s[p+j].g;
                cb=cb+s[p+j].b;
            }
            cr=cr*avg1; cg=cg*avg1; cb=cb*avg1;
            s[pw-1].r=cr*g+b*(s[pw-1].r-cr);
            s[pw-1].g=cg*g+b*(s[pw-1].g-cg);
            s[pw-1].b=cb*g+b*(s[pw-1].b-cb);
        }
        else
        {
            s[pw-1].r=b*s[pw-1].r;
            s[pw-1].g=b*s[pw-1].g;
            s[pw-1].b=b*s[pw-1].b;
        }
        for (j=w-2;j>=0;j--)	//nazaj
        {
            s[p+j].r=a*s[p+j+1].r+s[p+j].r;
            s[p+j].g=a*s[p+j+1].g+s[p+j].g;
            s[p+j].b=a*s[p+j+1].b+s[p+j].b;
        }
    }

    //prvih avg vrstic samo navzdol (nazaj so ze)
    for (i=0;i<w;i++)
    {
        if (ec!=0)
        {
            cr=0.0;cg=0.0;cb=0.0;
            for (j=0;j<avg;j++)
            {
                cr=cr+s[i+w*j].r;
                cg=cg+s[i+w*j].g;
                cb=cb+s[i+w*j].b;
            }
            cr=cr*avg1; cg=cg*avg1; cb=cb*avg1;
            s[i].r=cr*g+b*(s[i].r-cr);
            s[i].g=cg*g+b*(s[i].g-cg);
            s[i].b=cb*g+b*(s[i].b-cb);
        }
        for (j=1;j<avg;j++)	//dol
        {
            s[i+j*w].r=s[i+j*w].r+a*s[i+w*(j-1)].r;
            s[i+j*w].g=s[i+j*w].g+a*s[i+w*(j-1)].g;
            s[i+j*w].b=s[i+j*w].b+a*s[i+w*(j-1)].b;
        }
    }

    for (i=avg;i<h-1;i=i+2)	//po vrsticah navzdol
    {
        p=i*w; pw=p+w; pww=pw+w; pmw=p-w;
        if (ec!=0)
        {
            cr=0.0;cg=0.0;cb=0.0;
            for (j=0;j<avg;j++)
            {
                s[p+j].r=(float)(inframe[p+j]&0xFF);
                s[p+j].g=(float)((inframe[p+j]&0xFF00)>>8);
                s[p+j].b=(float)((inframe[p+j]&0xFF0000)>>16);
                cr=cr+s[p+j].r;
                cg=cg+s[p+j].g;
                cb=cb+s[p+j].b;
            }
            cr=cr*avg1; cg=cg*avg1; cb=cb*avg1;
            s[p].r=cr*g+b*(s[p].r-cr);
            s[p].g=cg*g+b*(s[p].g-cg);
            s[p].b=cb*g+b*(s[p].b-cb);
            cr=0.0;cg=0.0;cb=0.0;
            for (j=0;j<avg;j++)
            {
                s[pw+j].r=(float)(inframe[pw+j]&0xFF);
                s[pw+j].g=(float)((inframe[pw+j]&0xFF00)>>8);
                s[pw+j].b=(float)((inframe[pw+j]&0xFF0000)>>16);
                cr=cr+s[pw+j].r;
                cg=cg+s[pw+j].g;
                cb=cb+s[pw+j].b;
            }
            cr=cr*avg1; cg=cg*avg1; cb=cb*avg1;
            s[pw].r=cr*g+b*(s[pw].r-cr);
            s[pw].g=cg*g+b*(s[pw].g-cg);
            s[pw].b=cb*g+b*(s[pw].b-cb);
        }
        else
        {
            for (j=0;j<avg;j++)
            {
                s[p+j].r=(float)(inframe[p+j]&0xFF);
                s[p+j].g=(float)((inframe[p+j]&0xFF00)>>8);
                s[p+j].b=(float)((inframe[p+j]&0xFF0000)>>16);
            }
            for (j=0;j<avg;j++)
            {
                s[pw+j].r=(float)(inframe[pw+j]&0xFF);
                s[pw+j].g=(float)((inframe[pw+j]&0xFF00)>>8);
                s[pw+j].b=(float)((inframe[pw+j]&0xFF0000)>>16);
            }
        }
        for (j=1;j<avg;j++)	//tja  (ze pretvojeni)
        {
            pj=p+j;pwj=pw+j;
            s[pj].r=s[pj].r+a*s[pj-1].r;
            s[pj].g=s[pj].g+a*s[pj-1].g;
            s[pj].b=s[pj].b+a*s[pj-1].b;
            s[pwj].r=s[pwj].r+a*s[pwj-1].r;
            s[pwj].g=s[pwj].g+a*s[pwj-1].g;
            s[pwj].b=s[pwj].b+a*s[pwj-1].b;
        }
        for (j=avg;j<w;j++)	//tja  (s pretvorbo)
        {
            pj=p+j;pwj=pw+j;
            s[pj].r=(float)(inframe[pj]&0xFF);
            s[pj].g=(float)((inframe[pj]&0xFF00)>>8);
            s[pj].b=(float)((inframe[pj]&0xFF0000)>>16);
            s[pj].r=s[pj].r+a*s[pj-1].r;
            s[pj].g=s[pj].g+a*s[pj-1].g;
            s[pj].b=s[pj].b+a*s[pj-1].b;
            s[pwj].r=(float)(inframe[pwj]&0xFF);
            s[pwj].g=(float)((inframe[pwj]&0xFF00)>>8);
            s[pwj].b=(float)((inframe[pwj]&0xFF0000)>>16);
            s[pwj].r=s[pwj].r+a*s[pwj-1].r;
            s[pwj].g=s[pwj].g+a*s[pwj-1].g;
            s[pwj].b=s[pwj].b+a*s[pwj-1].b;
        }

        if (ec!=0)
        {
            cr=0.0;cg=0.0;cb=0.0;
            for (j=w-avg;j<w;j++)
            {
                cr=cr+s[p+j].r;
                cg=cg+s[p+j].g;
                cb=cb+s[p+j].b;
            }
            cr=cr*avg1; cg=cg*avg1; cb=cb*avg1;
            s[pw-1].r=cr*g+b*(s[pw-1].r-cr);
            s[pw-1].g=cg*g+b*(s[pw-1].g-cg);
            s[pw-1].b=cb*g+b*(s[pw-1].b-cb);
            cr=0.0;cg=0.0;cb=0.0;
            for (j=w-avg;j<w;j++)
            {
                cr=cr+s[pw+j].r;
                cg=cg+s[pw+j].g;
                cb=cb+s[pw+j].b;
            }
            cr=cr*avg1; cg=cg*avg1; cb=cb*avg1;
            s[pww-1].r=cr*g+b*(s[pww-1].r-cr);
            s[pww-1].g=cg*g+b*(s[pww-1].g-cg);
            s[pww-1].b=cb*g+b*(s[pww-1].b-cb);
        }
        else
        {
            s[pw-1].r=b*s[pw-1].r;	//rep H
            s[pw-1].g=b*s[pw-1].g;
            s[pw-1].b=b*s[pw-1].b;
            s[pww-1].r=b*s[pww-1].r;
            s[pww-1].g=b*s[pww-1].g;
            s[pww-1].b=b*s[pww-1].b;
        }

        //zacetek na desni
        s[pw-2].r=s[pw-2].r+a*s[pw-1].r;	//nazaj
        s[pw-2].g=s[pw-2].g+a*s[pw-1].g;
        s[pw-2].b=s[pw-2].b+a*s[pw-1].b;
        s[pw-1].r=s[pw-1].r+a*s[p-1].r;		//dol
        s[pw-1].g=s[pw-1].g+a*s[p-1].g;
        s[pw-1].b=s[pw-1].b+a*s[p-1].b;

        for (j=w-2;j>=1;j--)	//nazaj
        {
            pj=p+j;pwj=pw+j;
            s[pj-1].r=a*s[pj].r+s[pj-1].r;
            s[pj-1].g=a*s[pj].g+s[pj-1].g;
            s[pj-1].b=a*s[pj].b+s[pj-1].b;
            s[pwj].r=a*s[pwj+1].r+s[pwj].r;
            s[pwj].g=a*s[pwj+1].g+s[pwj].g;
            s[pwj].b=a*s[pwj+1].b+s[pwj].b;
            //zdaj naredi se en piksel vertikalno dol, za vse stolpce
            //dva nazaj, da ne vpliva na H nazaj
            s[pj].r=s[pj].r+a*s[pmw+j].r;
            s[pj].g=s[pj].g+a*s[pmw+j].g;
            s[pj].b=s[pj].b+a*s[pmw+j].b;
            s[pwj+1].r=s[pwj+1].r+a*s[pj+1].r;
            s[pwj+1].g=s[pwj+1].g+a*s[pj+1].g;
            s[pwj+1].b=s[pwj+1].b+a*s[pj+1].b;
        }
        //konec levo
        s[pw].r=s[pw].r+a*s[pw+1].r;	//nazaj
        s[pw].g=s[pw].g+a*s[pw+1].g;
        s[pw].b=s[pw].b+a*s[pw+1].b;
        s[p].r=s[p].r+a*s[pmw].r;	//dol
        s[p].g=s[p].g+a*s[pmw].g;
        s[p].b=s[p].b+a*s[pmw].b;
        s[pw+1].r=s[pw+1].r+a*s[p+1].r;	//dol
        s[pw+1].g=s[pw+1].g+a*s[p+1].g;
        s[pw+1].b=s[pw+1].b+a*s[p+1].b;
        s[pw].r=s[pw].r+a*s[p].r;	//dol
        s[pw].g=s[pw].g+a*s[p].g;
        s[pw].b=s[pw].b+a*s[p].b;
    }

    //ce je sodo stevilo vrstic, moras zadnjo posebej
    if (i!=h)
    {
        p=i*w; pw=p+w;
        for (j=1;j<w;j++)	//tja
        {
            s[p+j].r=s[p+j].r+a*s[p+j-1].r;
            s[p+j].g=s[p+j].g+a*s[p+j-1].g;
            s[p+j].b=s[p+j].b+a*s[p+j-1].b;
        }

        s[pw-1].r=b*s[pw-1].r;	//rep H
        s[pw-1].g=b*s[pw-1].g;
        s[pw-1].b=b*s[pw-1].b;

        for (j=w-2;j>=0;j--)	//nazaj in dol
        {
            s[p+j].r=a*s[p+j+1].r+s[p+j].r;
            s[p+j].g=a*s[p+j+1].g+s[p+j].g;
            s[p+j].b=a*s[p+j+1].b+s[p+j].b;

            //zdaj naredi se en piksel vertikalno dol, za vse stolpce
            //dva nazaj, da ne vpliva na H nazaj
            s[p+j+1].r=s[p+j+1].r+a*s[p-w+j+1].r;
            s[p+j+1].g=s[p+j+1].g+a*s[p-w+j+1].g;
            s[p+j+1].b=s[p+j+1].b+a*s[p-w+j+1].b;
        }
        //levi piksel vert
        s[p].r=s[p].r+a*s[p-w].r;
        s[p].g=s[p].g+a*s[p-w].g;
        s[p].b=s[p].b+a*s[p-w].b;
    }

    //zadnja vrstica (h-1)
    g4b=g4*b;
    g4a=g4/(1.0-a);
    p=(h-1)*w;
    if (ec!=0)
    {
        for (i=0;i<w;i++)	//po stolpcih
        {
            cr=0.0;cg=0.0;cb=0.0;
            for (j=h-avg;j<h;j++)
            {
                cr=cr+s[i+w*j].r;
                cg=cg+s[i+w*j].g;
                cb=cb+s[i+w*j].b;
            }
            cr=cr*avg1; cg=cg*avg1; cb=cb*avg1;
            s[i+p].r=g4a*cr+g4b*(s[i+p].r-cr);
            s[i+p].g=g4a*cg+g4b*(s[i+p].g-cg);
            s[i+p].b=g4a*cb+g4b*(s[i+p].b-cb);
            outframe[p+i]=((uint32_t)s[p+i].r&0xFF) + (((uint32_t)s[p+i].g&0xFF)<<8) + (((uint32_t)s[p+i].b&0xFF)<<16);
        }
    }
    else
    {
        for (j=0;j<w;j++)	//po stolpcih
        {
            s[j+p].r=g4b*s[j+p].r;	//rep V
            s[j+p].g=g4b*s[j+p].g;
            s[j+p].b=g4b*s[j+p].b;
            outframe[p+j]=((uint32_t)s[p+j].r&0xFF) + (((uint32_t)s[p+j].g&0xFF)<<8) + (((uint32_t)s[p+j].b&0xFF)<<16);
        }
    }

    for (i=h-2;i>=0;i--)	//po vrsticah navzgor
    {
        p=i*w; pw=p+w;
        for (j=0;j<w;j++)	//po stolpcih
        {
            s[p+j].r=a*s[pw+j].r+g4*s[p+j].r;
            s[p+j].g=a*s[pw+j].g+g4*s[p+j].g;
            s[p+j].b=a*s[pw+j].b+g4*s[p+j].b;
            outframe[p+j]=((uint32_t)s[p+j].r&0xFF) + (((uint32_t)s[p+j].g&0xFF)<<8) + (((uint32_t)s[p+j].b&0xFF)<<16);
        }
    }

}

//-------------------------------------------------------
// 2-tap IIR v stirih smereh   a only verzija, a0=1.0
//desno kompenzacijo izracuna direktno (rdx,rsx,rcx)
//optimized for speed
void fibe2o_8(const uint32_t* inframe, uint32_t* outframe, float_rgba s[], int w, int h, float a1, float a2,  float rd1, float rd2, float rs1, float rs2, float rc1, float rc2, int ec)
{
    float cr,cg,cb,g,g4,avg,gavg,avgg,iavg;
    float_rgba rep1,rep2;
    int i,j;
    int jw,jww,h1w,h2w,iw,i1w,i2w;

    g=1.0/(1.0+a1+a2);
    g4=1.0/g/g/g/g;
    avg=EDGEAVG;	//koliko vzorcev za povprecje pri edge comp
    gavg=g4/avg;
    avgg=1.0/g/avg;
    iavg=1.0/avg;

    for (j=0;j<avg;j++)	//prvih avg vrstic tja in nazaj
    {
        jw=j*w; jww=jw+w;
        cr=0.0;cg=0.0;cb=0.0;
        if (ec!=0)
        {	//edge comp (popvprecje prvih)
            for (i=0;i<avg;i++)
            {
                s[jw+i].r=(float)(inframe[jw+i]&0xFF);
                s[jw+i].g=(float)((inframe[jw+i]&0xFF00)>>8);
                s[jw+i].b=(float)((inframe[jw+i]&0xFF0000)>>16);
                cr=cr+s[jw+i].r;
                cg=cg+s[jw+i].g;
                cb=cb+s[jw+i].b;
            }
            cr=cr*gavg; cg=cg*gavg; cb=cb*gavg;
        }
        else
            for (i=0;i<avg;i++)
            {
                s[jw+i].r=(float)(inframe[jw+i]&0xFF);
                s[jw+i].g=(float)((inframe[jw+i]&0xFF00)>>8);
                s[jw+i].b=(float)((inframe[jw+i]&0xFF0000)>>16);
            }

        s[jw].r=g4*s[jw].r-(a1+a2)*g*cr;
        s[jw].g=g4*s[jw].g-(a1+a2)*g*cg;
        s[jw].b=g4*s[jw].b-(a1+a2)*g*cb;
        s[jw+1].r=g4*s[jw+1].r-a1*s[jw].r-a2*g*cr;
        s[jw+1].g=g4*s[jw+1].g-a1*s[jw].g-a2*g*cg;
        s[jw+1].b=g4*s[jw+1].b-a1*s[jw].b-a2*g*cb;

        if (ec!=0)
        {	//edge comp za nazaj
            cr=0.0;cg=0.0;cb=0.0;
            for (i=w-avg;i<w;i++)
            {
                s[jw+i].r=(float)(inframe[jw+i]&0xFF);
                s[jw+i].g=(float)((inframe[jw+i]&0xFF00)>>8);
                s[jw+i].b=(float)((inframe[jw+i]&0xFF0000)>>16);
                cr=cr+s[jw+i].r;
                cg=cg+s[jw+i].g;
                cb=cb+s[jw+i].b;
            }
            cr=cr*gavg; cg=cg*gavg; cb=cb*gavg;
        }
        else
            for (i=w-avg;i<w;i++)
            {
                s[jw+i].r=(float)(inframe[jw+i]&0xFF);
                s[jw+i].g=(float)((inframe[jw+i]&0xFF00)>>8);
                s[jw+i].b=(float)((inframe[jw+i]&0xFF0000)>>16);
            }

        for (i=2;i<avg;i++)	//tja (ze pretv. levo)
        {
            s[jw+i].r=g4*s[jw+i].r-a1*s[jw+i-1].r-a2*s[jw+i-2].r;
            s[jw+i].g=g4*s[jw+i].g-a1*s[jw+i-1].g-a2*s[jw+i-2].g;
            s[jw+i].b=g4*s[jw+i].b-a1*s[jw+i-1].b-a2*s[jw+i-2].b;
        }

        for (i=avg;i<w-avg;i++)	//tja (s pretvorbo)
        {
            s[jw+i].r=(float)(inframe[jw+i]&0xFF);
            s[jw+i].g=(float)((inframe[jw+i]&0xFF00)>>8);
            s[jw+i].b=(float)((inframe[jw+i]&0xFF0000)>>16);
            s[jw+i].r=g4*s[jw+i].r-a1*s[jw+i-1].r-a2*s[jw+i-2].r;
            s[jw+i].g=g4*s[jw+i].g-a1*s[jw+i-1].g-a2*s[jw+i-2].g;
            s[jw+i].b=g4*s[jw+i].b-a1*s[jw+i-1].b-a2*s[jw+i-2].b;
        }

        for (i=w-avg;i<w;i++)	//tja (ze pretv. desno)
        {
            s[jw+i].r=g4*s[jw+i].r-a1*s[jw+i-1].r-a2*s[jw+i-2].r;
            s[jw+i].g=g4*s[jw+i].g-a1*s[jw+i-1].g-a2*s[jw+i-2].g;
            s[jw+i].b=g4*s[jw+i].b-a1*s[jw+i-1].b-a2*s[jw+i-2].b;
        }

        rep1.r=(s[jww-1].r+s[jww-2].r)*0.5*rs1+(s[jww-1].r-s[jww-2].r)*rd1;
        rep1.g=(s[jww-1].g+s[jww-2].g)*0.5*rs1+(s[jww-1].g-s[jww-2].g)*rd1;
        rep1.b=(s[jww-1].b+s[jww-2].b)*0.5*rs1+(s[jww-1].b-s[jww-2].b)*rd1;
        rep2.r=(s[jww-1].r+s[jww-2].r)*0.5*rs2+(s[jww-1].r-s[jww-2].r)*rd2;
        rep2.g=(s[jww-1].g+s[jww-2].g)*0.5*rs2+(s[jww-1].g-s[jww-2].g)*rd2;
        rep2.b=(s[jww-1].b+s[jww-2].b)*0.5*rs2+(s[jww-1].b-s[jww-2].b)*rd2;

        if (ec!=0)
        {
            rep1.r=rep1.r+rc1*cr;
            rep1.g=rep1.g+rc1*cg;
            rep1.b=rep1.b+rc1*cb;
            rep2.r=rep2.r+rc2*cr;
            rep2.g=rep2.g+rc2*cg;
            rep2.b=rep2.b+rc2*cb;
        }

        s[jww-1].r=s[jww-1].r-a1*rep1.r-a2*rep2.r;
        s[jww-1].g=s[jww-1].g-a1*rep1.g-a2*rep2.g;
        s[jww-1].b=s[jww-1].b-a1*rep1.b-a2*rep2.b;
        s[jww-2].r=s[jww-2].r-a1*s[jww-1].r-a2*rep1.r;
        s[jww-2].g=s[jww-2].g-a1*s[jww-1].g-a2*rep1.g;
        s[jww-2].b=s[jww-2].b-a1*s[jww-1].b-a2*rep1.b;

        for (i=w-3;i>=0;i--)		//nazaj
        {
            s[jw+i].r=s[jw+i].r-a1*s[jw+i+1].r-a2*s[jw+i+2].r;
            s[jw+i].g=s[jw+i].g-a1*s[jw+i+1].g-a2*s[jw+i+2].g;
            s[jw+i].b=s[jw+i].b-a1*s[jw+i+1].b-a2*s[jw+i+2].b;
        }
    }	//prvih avg vrstic

    //edge comp zgoraj za navzdol
    for (j=0;j<w;j++)	//po stolpcih
    {
        cr=0.0;cg=0.0;cb=0.0;
        if (ec!=0)
        {	//edge comp (popvprecje prvih)
            for (i=0;i<avg;i++)
            {
                cr=cr+s[j+w*i].r;
                cg=cg+s[j+w*i].g;
                cb=cb+s[j+w*i].b;
            }
            cr=cr*iavg; cg=cg*iavg; cb=cb*iavg;
        }

        //zgornji vrstici
        s[j].r=s[j].r-(a1+a2)*g*cr;
        s[j].g=s[j].g-(a1+a2)*g*cg;
        s[j].b=s[j].b-(a1+a2)*g*cb;
        s[j+w].r=s[j+w].r-a1*s[j].r-a2*g*cr;
        s[j+w].g=s[j+w].g-a1*s[j].g-a2*g*cg;
        s[j+w].b=s[j+w].b-a1*s[j].b-a2*g*cb;
    }

    //tretja do avg, samo navzdol (nazaj so ze)
    for (i=2;i<avg;i++)
    {
        iw=i*w; i1w=iw-w;
        for (j=0;j<w;j++)	//po stolpcih
        {
            s[j+iw].r=s[j+iw].r-a1*s[j+i1w].r-a2*s[j+i1w-w].r;
            s[j+iw].g=s[j+iw].g-a1*s[j+i1w].g-a2*s[j+i1w-w].g;
            s[j+iw].b=s[j+iw].b-a1*s[j+i1w].b-a2*s[j+i1w-w].b;
        }
    }

    for (j=avg;j<h;j++)	//po vrsticah tja, nazaj in dol
    {
        jw=j*w; jww=jw+w;
        cr=0.0;cg=0.0;cb=0.0;
        if (ec!=0)
        {	//edge comp (popvprecje prvih)
            for (i=0;i<avg;i++)
            {
                s[jw+i].r=(float)(inframe[jw+i]&0xFF);
                s[jw+i].g=(float)((inframe[jw+i]&0xFF00)>>8);
                s[jw+i].b=(float)((inframe[jw+i]&0xFF0000)>>16);
                cr=cr+s[jw+i].r;
                cg=cg+s[jw+i].g;
                cb=cb+s[jw+i].b;
            }
            cr=cr*gavg; cg=cg*gavg; cb=cb*gavg;
        }
        else
            for (i=0;i<avg;i++)
            {
                s[jw+i].r=(float)(inframe[jw+i]&0xFF);
                s[jw+i].g=(float)((inframe[jw+i]&0xFF00)>>8);
                s[jw+i].b=(float)((inframe[jw+i]&0xFF0000)>>16);
            }
        s[jw].r=g4*s[jw].r-(a1+a2)*g*cr;
        s[jw].g=g4*s[jw].g-(a1+a2)*g*cg;
        s[jw].b=g4*s[jw].b-(a1+a2)*g*cb;
        s[jw+1].r=g4*s[jw+1].r-a1*s[jw].r-a2*g*cr;
        s[jw+1].g=g4*s[jw+1].g-a1*s[jw].g-a2*g*cg;
        s[jw+1].b=g4*s[jw+1].b-a1*s[jw].b-a2*g*cb;

        if (ec!=0)
        {	//edge comp za nazaj
            cr=0.0;cg=0.0;cb=0.0;
            for (i=w-avg;i<w;i++)
            {
                s[jw+i].r=(float)(inframe[jw+i]&0xFF);
                s[jw+i].g=(float)((inframe[jw+i]&0xFF00)>>8);
                s[jw+i].b=(float)((inframe[jw+i]&0xFF0000)>>16);
                cr=cr+s[jw+i].r;
                cg=cg+s[jw+i].g;
                cb=cb+s[jw+i].b;
            }
            cr=cr*gavg; cg=cg*gavg; cb=cb*gavg;
        }
        else
            for (i=w-avg;i<w;i++)
            {
                s[jw+i].r=(float)(inframe[jw+i]&0xFF);
                s[jw+i].g=(float)((inframe[jw+i]&0xFF00)>>8);
                s[jw+i].b=(float)((inframe[jw+i]&0xFF0000)>>16);
            }

        for (i=2;i<avg;i++)	//tja  (ze pretv. levo)
        {
            s[jw+i].r=g4*s[jw+i].r-a1*s[jw+i-1].r-a2*s[jw+i-2].r;
            s[jw+i].g=g4*s[jw+i].g-a1*s[jw+i-1].g-a2*s[jw+i-2].g;
            s[jw+i].b=g4*s[jw+i].b-a1*s[jw+i-1].b-a2*s[jw+i-2].b;
        }

        for (i=avg;i<w-avg;i++)	//tja  (s retvorbo)
        {
            s[jw+i].r=(float)(inframe[jw+i]&0xFF);
            s[jw+i].g=(float)((inframe[jw+i]&0xFF00)>>8);
            s[jw+i].b=(float)((inframe[jw+i]&0xFF0000)>>16);
            s[jw+i].r=g4*s[jw+i].r-a1*s[jw+i-1].r-a2*s[jw+i-2].r;
            s[jw+i].g=g4*s[jw+i].g-a1*s[jw+i-1].g-a2*s[jw+i-2].g;
            s[jw+i].b=g4*s[jw+i].b-a1*s[jw+i-1].b-a2*s[jw+i-2].b;
        }

        for (i=w-avg;i<w;i++)	//tja  (ze pretv. desno)
        {
            s[jw+i].r=g4*s[jw+i].r-a1*s[jw+i-1].r-a2*s[jw+i-2].r;
            s[jw+i].g=g4*s[jw+i].g-a1*s[jw+i-1].g-a2*s[jw+i-2].g;
            s[jw+i].b=g4*s[jw+i].b-a1*s[jw+i-1].b-a2*s[jw+i-2].b;
        }

        rep1.r=(s[jww-1].r+s[jww-2].r)*0.5*rs1+(s[jww-1].r-s[jww-2].r)*rd1;
        rep1.g=(s[jww-1].g+s[jww-2].g)*0.5*rs1+(s[jww-1].g-s[jww-2].g)*rd1;
        rep1.b=(s[jww-1].b+s[jww-2].b)*0.5*rs1+(s[jww-1].b-s[jww-2].b)*rd1;
        rep2.r=(s[jww-1].r+s[jww-2].r)*0.5*rs2+(s[jww-1].r-s[jww-2].r)*rd2;
        rep2.g=(s[jww-1].g+s[jww-2].g)*0.5*rs2+(s[jww-1].g-s[jww-2].g)*rd2;
        rep2.b=(s[jww-1].b+s[jww-2].b)*0.5*rs2+(s[jww-1].b-s[jww-2].b)*rd2;

        if (ec!=0)
        {
            rep1.r=rep1.r+rc1*cr;
            rep1.g=rep1.g+rc1*cg;
            rep1.b=rep1.b+rc1*cb;
            rep2.r=rep2.r+rc2*cr;
            rep2.g=rep2.g+rc2*cg;
            rep2.b=rep2.b+rc2*cb;
        }

        s[jww-1].r=s[jww-1].r-a1*rep1.r-a2*rep2.r;
        s[jww-1].g=s[jww-1].g-a1*rep1.g-a2*rep2.g;
        s[jww-1].b=s[jww-1].b-a1*rep1.b-a2*rep2.b;
        s[jww-2].r=s[jww-2].r-a1*s[jww-1].r-a2*rep1.r;
        s[jww-2].g=s[jww-2].g-a1*s[jww-1].g-a2*rep1.g;
        s[jww-2].b=s[jww-2].b-a1*s[jww-1].b-a2*rep1.b;

        for (i=w-3;i>=0;i--)	//po stolpcih
        { //nazaj
            s[jw+i].r=s[jw+i].r-a1*s[jw+i+1].r-a2*s[jw+i+2].r;
            s[jw+i].g=s[jw+i].g-a1*s[jw+i+1].g-a2*s[jw+i+2].g;
            s[jw+i].b=s[jw+i].b-a1*s[jw+i+1].b-a2*s[jw+i+2].b;
            //dol
            s[jw+i+2].r=s[jw+i+2].r-a1*s[jw-w+i+2].r-a2*s[jw-w-w+i+2].r;
            s[jw+i+2].g=s[jw+i+2].g-a1*s[jw-w+i+2].g-a2*s[jw-w-w+i+2].g;
            s[jw+i+2].b=s[jw+i+2].b-a1*s[jw-w+i+2].b-a2*s[jw-w-w+i+2].b;
        }

        //se leva stolpca dol
        s[jw+1].r=s[jw+1].r-a1*s[jw-w+1].r-a2*s[jw-w-w+1].r;
        s[jw+1].g=s[jw+1].g-a1*s[jw-w+1].g-a2*s[jw-w-w+1].g;
        s[jw+1].b=s[jw+1].b-a1*s[jw-w+1].b-a2*s[jw-w-w+1].b;
        s[jw].r=s[jw].r-a1*s[jw-w].r-a2*s[jw-w-w].r;
        s[jw].g=s[jw].g-a1*s[jw-w].g-a2*s[jw-w-w].g;
        s[jw].b=s[jw].b-a1*s[jw-w].b-a2*s[jw-w-w].b;

    }	//po vrsticah

    //pa se navzgor
    //spodnji dve vrstici
    h1w=(h-1)*w; h2w=(h-2)*w;
    for (j=0;j<w;j++)	//po stolpcih
    {
        if (ec!=0)
        {	//edge comp za gor
            cr=0.0;cg=0.0;cb=0.0;
            for (i=h-avg;i<h;i++)
            {
                cr=cr+s[j+w*i].r;
                cg=cg+s[j+w*i].g;
                cb=cb+s[j+w*i].b;
            }
            cr=cr*avgg; cg=cg*avgg; cb=cb*avgg;
        }

        rep1.r=(s[j+h1w].r+s[j+h2w].r)*0.5*rs1+(s[j+h1w].r-s[j+h2w].r)*rd1;
        rep1.g=(s[j+h1w].g+s[j+h2w].g)*0.5*rs1+(s[j+h1w].g-s[j+h2w].g)*rd1;
        rep1.b=(s[j+h1w].b+s[j+h2w].b)*0.5*rs1+(s[j+h1w].b-s[j+h2w].b)*rd1;
        rep2.r=(s[j+h1w].r+s[j+h2w].r)*0.5*rs2+(s[j+h1w].r-s[j+h2w].r)*rd2;
        rep2.g=(s[j+h1w].g+s[j+h2w].g)*0.5*rs2+(s[j+h1w].g-s[j+h2w].g)*rd2;
        rep2.b=(s[j+h1w].b+s[j+h2w].b)*0.5*rs2+(s[j+h1w].b-s[j+h2w].b)*rd2;

        if (ec!=0)
        {	//edge comp
            rep1.r=rep1.r+rc1*cr;
            rep1.g=rep1.g+rc1*cg;
            rep1.b=rep1.b+rc1*cb;
            rep2.r=rep2.r+rc2*cr;
            rep2.g=rep2.g+rc2*cg;
            rep2.b=rep2.b+rc2*cb;
        }

        s[j+h1w].r=s[j+h1w].r-a1*rep1.r-a2*rep2.r;
        s[j+h1w].g=s[j+h1w].g-a1*rep1.g-a2*rep2.g;
        s[j+h1w].b=s[j+h1w].b-a1*rep1.b-a2*rep2.b;
        if (s[j+h1w].r>255) s[j+h1w].r=255.0;
        if (s[j+h1w].r<0.0) s[j+h1w].r=0.0;
        if (s[j+h1w].g>255) s[j+h1w].g=255.0;
        if (s[j+h1w].g<0.0) s[j+h1w].g=0.0;
        if (s[j+h1w].b>255) s[j+h1w].b=255.0;
        if (s[j+h1w].b<0.0) s[j+h1w].b=0.0;
        outframe[j+h1w]=((uint32_t)s[j+h1w].r&0xFF) + (((uint32_t)s[j+h1w].g&0xFF)<<8) + (((uint32_t)s[j+h1w].b&0xFF)<<16);
        s[j+h2w].r=s[j+h2w].r-a1*s[j+h1w].r-a2*rep1.r;
        s[j+h2w].g=s[j+h2w].g-a1*s[j+h1w].g-a2*rep1.g;
        s[j+h2w].b=s[j+h2w].b-a1*s[j+h1w].b-a2*rep1.b;
        if (s[j+h2w].r>255) s[j+h2w].r=255.0;
        if (s[j+h2w].r<0.0) s[j+h2w].r=0.0;
        if (s[j+h2w].g>255) s[j+h2w].g=255.0;
        if (s[j+h2w].g<0.0) s[j+h2w].g=0.0;
        if (s[j+h2w].b>255) s[j+h2w].b=255.0;
        if (s[j+h2w].b<0.0) s[j+h2w].b=0.0;
        outframe[j+h2w]=((uint32_t)s[j+h2w].r&0xFF) + (((uint32_t)s[j+h2w].g&0xFF)<<8) + (((uint32_t)s[j+h2w].b&0xFF)<<16);
    }

    //ostale vrstice
    for (i=h-3;i>=0;i--)		//gor
    {
        iw=i*w; i1w=iw+w; i2w=i1w+w;
        for (j=0;j<w;j++)
        {
            s[j+iw].r=s[j+iw].r-a1*s[j+i1w].r-a2*s[j+i2w].r;
            s[j+iw].g=s[j+iw].g-a1*s[j+i1w].g-a2*s[j+i2w].g;
            s[j+iw].b=s[j+iw].b-a1*s[j+i1w].b-a2*s[j+i2w].b;
            if (s[j+iw].r>255) s[j+iw].r=255.0;
            if (s[j+iw].r<0.0) s[j+iw].r=0.0;
            if (s[j+iw].g>255) s[j+iw].g=255.0;
            if (s[j+iw].g<0.0) s[j+iw].g=0.0;
            if (s[j+iw].b>255) s[j+iw].b=255.0;
            if (s[j+iw].b<0.0) s[j+iw].b=0.0;
            outframe[j+iw]=((uint32_t)s[j+iw].r&0xFF) + (((uint32_t)s[j+iw].g&0xFF)<<8) + (((uint32_t)s[j+iw].b&0xFF)<<16);
        }
    }

}

//-------------------------------------------------------
// 3-tap IIR v stirih smereh
//a only verzija, a0=1.0
//edge efekt na desni kompenzira tako, da racuna 256 vzorcev
//cez rob in in gre potem nazaj
void fibe3_8(const uint32_t* inframe, uint32_t* outframe, float_rgba s[], int w, int h, float a1, float a2, float a3, int ec)
{
    float cr,cg,cb,g,g4;
    int i,j;
    const float avg = EDGEAVG; // how many samples for average at edge comp
    const int cez = 256; // how many samples go right
    float_rgba *lb = malloc((MAX(w, h) + cez) * sizeof(*lb));

    g=1.0/(1.0+a1+a2+a3); g4=1.0/g/g/g/g;

    for (j=0;j<h;j++)	//po vrsticah
    {
        cr=0.0;cg=0.0;cb=0.0;
        if (ec!=0)
        {	//edge comp (popvprecje prvih)
            for (i=0;i<avg;i++)
            {
                s[j*w+i].r=(float)(inframe[j*w+i]&0xFF);
                s[j*w+i].g=(float)((inframe[j*w+i]&0xFF00)>>8);
                s[j*w+i].b=(float)((inframe[j*w+i]&0xFF0000)>>16);
                cr=cr+s[j*w+i].r;
                cg=cg+s[j*w+i].g;
                cb=cb+s[j*w+i].b;
            }
            cr=g4*cr/avg; cg=g4*cg/avg; cb=g4*cb/avg;
        }
        else
            for (i=0;i<avg;i++)
            {
                s[j*w+i].r=(float)(inframe[j*w+i]&0xFF);
                s[j*w+i].g=(float)((inframe[j*w+i]&0xFF00)>>8);
                s[j*w+i].b=(float)((inframe[j*w+i]&0xFF0000)>>16);
            }
        lb[0].r=g4*s[j*w].r-(a1+a2+a3)*g*cr;
        lb[0].g=g4*s[j*w].g-(a1+a2+a3)*g*cg;
        lb[0].b=g4*s[j*w].b-(a1+a2+a3)*g*cb;
        lb[1].r=g4*s[j*w+1].r-a1*lb[0].r-(a2+a3)*g*cr;
        lb[1].g=g4*s[j*w+1].g-a1*lb[0].g-(a2+a3)*g*cg;
        lb[1].b=g4*s[j*w+1].b-a1*lb[0].b-(a2+a3)*g*cb;
        lb[2].r=g4*s[j*w+2].r-a1*lb[1].r-a2*lb[0].r-a3*g*cr;
        lb[2].g=g4*s[j*w+2].g-a1*lb[1].g-a2*lb[0].g-a3*g*cg;
        lb[2].b=g4*s[j*w+2].b-a1*lb[1].b-a2*lb[0].b-a3*g*cb;

        for (i=3;i<avg;i++)	//tja  (ze pretvorjeni)
        {
            lb[i].r=g4*s[j*w+i].r-a1*lb[i-1].r-a2*lb[i-2].r-a3*lb[i-3].r;
            lb[i].g=g4*s[j*w+i].g-a1*lb[i-1].g-a2*lb[i-2].g-a3*lb[i-3].g;
            lb[i].b=g4*s[j*w+i].b-a1*lb[i-1].b-a2*lb[i-2].b-a3*lb[i-3].b;
        }

        for (i=avg;i<w;i++)	//tja  (s pretvorbo)
        {
            s[j*w+i].r=(float)(inframe[j*w+i]&0xFF);
            s[j*w+i].g=(float)((inframe[j*w+i]&0xFF00)>>8);
            s[j*w+i].b=(float)((inframe[j*w+i]&0xFF0000)>>16);
            lb[i].r=g4*s[j*w+i].r-a1*lb[i-1].r-a2*lb[i-2].r-a3*lb[i-3].r;
            lb[i].g=g4*s[j*w+i].g-a1*lb[i-1].g-a2*lb[i-2].g-a3*lb[i-3].g;
            lb[i].b=g4*s[j*w+i].b-a1*lb[i-1].b-a2*lb[i-2].b-a3*lb[i-3].b;
        }

        cr=0.0;cg=0.0;cb=0.0;
        if (ec!=0)
        {	//edge comp
            for (i=w-avg;i<w;i++)
            {
                cr=cr+s[j*w+i].r;
                cg=cg+s[j*w+i].g;
                cb=cb+s[j*w+i].b;
            }
            cr=g4*cr/avg; cg=g4*cg/avg; cb=g4*cb/avg;
        }

        for (i=w;i<(w+cez);i++)	//naprej cez rob
        {
            lb[i].r=cr-a1*lb[i-1].r-a2*lb[i-2].r-a3*lb[i-3].r;
            lb[i].g=cr-a1*lb[i-1].g-a2*lb[i-2].g-a3*lb[i-3].g;
            lb[i].b=cr-a1*lb[i-1].b-a2*lb[i-2].b-a3*lb[i-3].b;
        }
        //nazaj do roba
        lb[w+cez-2].r=lb[w+cez-2].r-a1*lb[w+cez-1].r;
        lb[w+cez-2].g=lb[w+cez-2].g-a1*lb[w+cez-1].g;
        lb[w+cez-2].b=lb[w+cez-2].b-a1*lb[w+cez-1].b;
        lb[w+cez-3].r=lb[w+cez-3].r-a1*lb[w+cez-2].r-a2*lb[w+cez-1].r;
        lb[w+cez-3].g=lb[w+cez-3].g-a1*lb[w+cez-2].g-a2*lb[w+cez-1].g;
        lb[w+cez-3].b=lb[w+cez-3].b-a1*lb[w+cez-2].b-a2*lb[w+cez-1].b;
        for (i=(w+cez-4);i>=w;i--)
        {
            lb[i].r=lb[i].r-a1*lb[i+1].r-a2*lb[i+2].r-a3*lb[i+3].r;
            lb[i].g=lb[i].g-a1*lb[i+1].g-a2*lb[i+2].g-a3*lb[i+3].g;
            lb[i].b=lb[i].b-a1*lb[i+1].b-a2*lb[i+2].b-a3*lb[i+3].b;
        }

        s[j*w+w-1].r=lb[w-1].r-a1*lb[w].r-a2*lb[w+1].r-a3*lb[w+2].r;
        s[j*w+w-1].g=lb[w-1].g-a1*lb[w].g-a2*lb[w+1].g-a3*lb[w+2].g;
        s[j*w+w-1].b=lb[w-1].b-a1*lb[w].b-a2*lb[w+1].b-a3*lb[w+2].b;
        s[j*w+w-2].r=lb[w-2].r-a1*s[j*w+w-1].r-a2*lb[w].r-a3*lb[w+1].r;
        s[j*w+w-2].g=lb[w-2].g-a1*s[j*w+w-1].g-a2*lb[w].g-a3*lb[w+1].g;
        s[j*w+w-2].b=lb[w-2].b-a1*s[j*w+w-1].b-a2*lb[w].b-a3*lb[w+1].b;
        s[j*w+w-3].r=lb[w-3].r-a1*s[j*w+w-2].r-a2*s[j*w+w-1].r-a3*lb[w].r;
        s[j*w+w-3].g=lb[w-3].g-a1*s[j*w+w-2].g-a2*s[j*w+w-1].g-a3*lb[w].g;
        s[j*w+w-3].b=lb[w-3].b-a1*s[j*w+w-2].b-a2*s[j*w+w-1].b-a3*lb[w].b;

        for (i=w-4;i>=0;i--)		//nazaj
        {
            s[j*w+i].r=lb[i].r-a1*s[j*w+i+1].r-a2*s[j*w+i+2].r-a3*s[j*w+i+3].r;
            s[j*w+i].g=lb[i].g-a1*s[j*w+i+1].g-a2*s[j*w+i+2].g-a3*s[j*w+i+3].g;
            s[j*w+i].b=lb[i].b-a1*s[j*w+i+1].b-a2*s[j*w+i+2].b-a3*s[j*w+i+3].b;
        }
    }	//po vrsticah

    for (j=0;j<w;j++)	//po stolpcih
    {
        cr=0.0;cg=0.0;cb=0.0;
        if (ec!=0)
        {	//edge comp (popvprecje prvih)
            for (i=0;i<avg;i++)
            {
                cr=cr+s[j+w*i].r;
                cg=cg+s[j+w*i].g;
                cb=cb+s[j+w*i].b;
            }
            cr=cr/avg; cg=cg/avg; cb=cb/avg;
        }
        lb[0].r=s[j].r-(a1+a2+a3)*g*cr;
        lb[0].g=s[j].g-(a1+a2+a3)*g*cg;
        lb[0].b=s[j].b-(a1+a2+a3)*g*cb;
        lb[1].r=s[j+w].r-a1*lb[0].r-(a2+a3)*g*cr;
        lb[1].g=s[j+w].g-a1*lb[0].g-(a2+a3)*g*cg;
        lb[1].b=s[j+w].b-a1*lb[0].b-(a2+a3)*g*cb;
        lb[2].r=s[j+2*w].r-a1*lb[1].r-a2*lb[0].r-a3*g*cr;
        lb[2].g=s[j+2*w].g-a1*lb[1].g-a2*lb[0].g-a3*g*cg;
        lb[2].b=s[j+2*w].b-a1*lb[1].b-a2*lb[0].b-a3*g*cb;

        for (i=3;i<h;i++)		//dol
        {
            lb[i].r=s[j+w*i].r-a1*lb[i-1].r-a2*lb[i-2].r-a3*lb[i-3].r;
            lb[i].g=s[j+w*i].g-a1*lb[i-1].g-a2*lb[i-2].g-a3*lb[i-3].g;
            lb[i].b=s[j+w*i].b-a1*lb[i-1].b-a2*lb[i-2].b-a3*lb[i-3].b;
        }

        cr=0.0;cg=0.0;cb=0.0;
        if (ec!=0)
        {	//edge comp
            for (i=h-avg;i<h;i++)
            {
                cr=cr+s[j+w*i].r;
                cg=cg+s[j+w*i].g;
                cb=cb+s[j+w*i].b;
            }
            cr=cr/avg; cg=cg/avg; cb=cb/avg;
        }

        for (i=h;i<(h+cez);i++)	//naprej cez rob
        {
            lb[i].r=cr-a1*lb[i-1].r-a2*lb[i-2].r-a3*lb[i-3].r;
            lb[i].g=cg-a1*lb[i-1].g-a2*lb[i-2].g-a3*lb[i-3].g;
            lb[i].b=cb-a1*lb[i-1].b-a2*lb[i-2].b-a3*lb[i-3].b;
        }
        //nazaj do roba
        lb[h+cez-2].r=lb[h+cez-2].r-a1*lb[h+cez-1].r;
        lb[h+cez-2].g=lb[h+cez-2].g-a1*lb[h+cez-1].g;
        lb[h+cez-2].b=lb[h+cez-2].b-a1*lb[h+cez-1].b;
        lb[h+cez-3].r=lb[h+cez-3].r-a1*lb[h+cez-2].r-a2*lb[h+cez-1].r;
        lb[h+cez-3].g=lb[h+cez-3].g-a1*lb[h+cez-2].g-a2*lb[h+cez-1].g;
        lb[h+cez-3].b=lb[h+cez-3].b-a1*lb[h+cez-2].b-a2*lb[h+cez-1].b;
        for (i=(h+cez-4);i>=h;i--)
        {
            lb[i].r=lb[i].r-a1*lb[i+1].r-a2*lb[i+2].r-a3*lb[i+3].r;
            lb[i].g=lb[i].g-a1*lb[i+1].g-a2*lb[i+2].g-a3*lb[i+3].g;
            lb[i].b=lb[i].b-a1*lb[i+1].b-a2*lb[i+2].b-a3*lb[i+3].b;
        }

        s[j+(h-1)*w].r=lb[h-1].r-a1*lb[h].r-a2*lb[h+1].r-a3*lb[h+2].r;
        s[j+(h-1)*w].g=lb[h-1].g-a1*lb[h].g-a2*lb[h+1].g-a3*lb[h+2].g;
        s[j+(h-1)*w].b=lb[h-1].b-a1*lb[h].b-a2*lb[h+1].b-a3*lb[h+2].b;
        s[j+(h-2)*w].r=lb[h-2].r-a1*s[j+(h-1)*w].r-a2*lb[h].r-a3*lb[h+1].r;
        s[j+(h-2)*w].g=lb[h-2].g-a1*s[j+(h-1)*w].g-a2*lb[h].g-a3*lb[h+1].g;
        s[j+(h-2)*w].b=lb[h-2].b-a1*s[j+(h-1)*w].b-a2*lb[h].b-a3*lb[h+1].b;
        s[j+(h-3)*w].r=lb[h-3].r-a1*s[j+(h-2)*w].r-a2*s[j+(h-1)*w].r-a3*lb[h].r;
        s[j+(h-3)*w].g=lb[h-3].g-a1*s[j+(h-2)*w].g-a2*s[j+(h-1)*w].g-a3*lb[h].g;
        s[j+(h-3)*w].b=lb[h-3].b-a1*s[j+(h-2)*w].b-a2*s[j+(h-1)*w].b-a3*lb[h].b;

        for (i=h-4;i>=0;i--)		//gor
        {
            s[j+w*i].r=lb[i].r-a1*s[j+w*(i+1)].r-a2*s[j+w*(i+2)].r-a3*s[j+w*(i+3)].r;
            s[j+w*i].g=lb[i].g-a1*s[j+w*(i+1)].g-a2*s[j+w*(i+2)].g-a3*s[j+w*(i+3)].g;
            s[j+w*i].b=lb[i].b-a1*s[j+w*(i+1)].b-a2*s[j+w*(i+2)].b-a3*s[j+w*(i+3)].b;
            outframe[j+w*i]=((uint32_t)s[j+w*i].r&0xFF) + (((uint32_t)s[j+w*i].g&0xFF)<<8) + (((uint32_t)s[j+w*i].b&0xFF)<<16);
        }
    }	//po stolpcih

    free(lb);
}
