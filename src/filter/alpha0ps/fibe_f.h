//fibe1_f.h

//Skalarna (float) verzija

//	MC maj 2012


#define EDGEAVG 8

double PI=3.14159265358979;

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
//printf("a=%9.6f %9.6f %9.6f   b=%9.6f %9.6f %9.6f\n",*a0,*a1,*a2,*b0,*b1,*b2);
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

//-------------------------------------------------------
// 2-tap IIR v stirih smereh   a only verzija, a0=1.0
//desno kompenzacijo izracuna direktno (rdx,rsx,rcx)
//optimized for speed

// rep za navzgor racuna iz ze procesiranih
// (fibe-2 ga racuna iz deviskih)

void fibe2o_f(float s[], int w, int h, float a1, float a2,  float rd1, float rd2, float rs1, float rs2, float rc1, float rc2, int ec)
{
float cr,g,g4,avg,gavg,avgg,iavg;
float rep1,rep2;
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
	cr=0.0;
	if (ec!=0)
		{	//edge comp (popvprecje prvih)
		for (i=0;i<avg;i++)
			{
			cr=cr+s[jw+i];
			}
		cr=cr*gavg;
		}
	s[jw]=g4*s[jw]-(a1+a2)*g*cr;
	s[jw+1]=g4*s[jw+1]-a1*s[jw]-a2*g*cr;

	if (ec!=0)
		{	//edge comp za nazaj
		cr=0.0;
		for (i=w-avg;i<w;i++)
			{
			cr=cr+s[jw+i];
			}
		cr=cr*gavg;
		}

	for (i=2;i<w;i++)		//tja
		{
		s[jw+i]=g4*s[jw+i]-a1*s[jw+i-1]-a2*s[jw+i-2];
		}

	rep1=(s[jww-1]+s[jww-2])*0.5*rs1+(s[jww-1]-s[jww-2])*rd1;
	rep2=(s[jww-1]+s[jww-2])*0.5*rs2+(s[jww-1]-s[jww-2])*rd2;

	if (ec!=0)
		{
		rep1=rep1+rc1*cr;
		rep2=rep2+rc2*cr;
		}

	s[jww-1]=s[jww-1]-a1*rep1-a2*rep2;
	s[jww-2]=s[jww-2]-a1*s[jww-1]-a2*rep1;

	for (i=w-3;i>=0;i--)		//nazaj
		{
		s[jw+i]=s[jw+i]-a1*s[jw+i+1]-a2*s[jw+i+2];
		}
	}	//prvih avg vrstic

//printaj(s,w,h,1,1,0);

//edge comp zgoraj za navzdol
for (j=0;j<w;j++)	//po stolpcih
	{
	cr=0.0;
	if (ec!=0)
		{	//edge comp (popvprecje prvih)
		for (i=0;i<avg;i++)
			{
			cr=cr+s[j+w*i];
			}
		cr=cr*iavg;
		}

//zgornji vrstici
	s[j]=s[j]-(a1+a2)*g*cr;
	s[j+w]=s[j+w]-a1*s[j]-a2*g*cr;
	}

//tretja do avg, samo navzdol (nazaj so ze)
for (i=2;i<avg;i++)
	{
	iw=i*w; i1w=iw-w;
	for (j=0;j<w;j++)	//po stolpcih
		{
		s[j+iw]=s[j+iw]-a1*s[j+i1w]-a2*s[j+i1w-w];
		}
	}

for (j=avg;j<h;j++)	//po vrsticah tja, nazaj in dol
	{
	jw=j*w; jww=jw+w;
	cr=0.0;
	if (ec!=0)
		{	//edge comp (popvprecje prvih)
		for (i=0;i<avg;i++)
			{
			cr=cr+s[jw+i];
			}
		cr=cr*gavg;
		}
	s[jw]=g4*s[jw]-(a1+a2)*g*cr;
	s[jw+1]=g4*s[jw+1]-a1*s[jw]-a2*g*cr;

	if (ec!=0)
		{	//edge comp za nazaj
		cr=0.0;
		for (i=w-avg;i<w;i++)
			{
			cr=cr+s[jw+i];
			}
		cr=cr*gavg;
		}

	for (i=2;i<w;i++)		//tja
		{
		s[jw+i]=g4*s[jw+i]-a1*s[jw+i-1]-a2*s[jw+i-2];
		}

	rep1=(s[jww-1]+s[jww-2])*0.5*rs1+(s[jww-1]-s[jww-2])*rd1;
	rep2=(s[jww-1]+s[jww-2])*0.5*rs2+(s[jww-1]-s[jww-2])*rd2;

	if (ec!=0)
		{
		rep1=rep1+rc1*cr;
		rep2=rep2+rc2*cr;
		}

	s[jww-1]=s[jww-1]-a1*rep1-a2*rep2;
	s[jww-2]=s[jww-2]-a1*s[jww-1]-a2*rep1;

	for (i=w-3;i>=0;i--)	//po stolpcih
		{ //nazaj
		s[jw+i]=s[jw+i]-a1*s[jw+i+1]-a2*s[jw+i+2];
//dol
		s[jw+i+2]=s[jw+i+2]-a1*s[jw-w+i+2]-a2*s[jw-w-w+i+2];
		}

//se leva stolpca dol
	s[jw+1]=s[jw+1]-a1*s[jw-w+1]-a2*s[jw-w-w+1];
	s[jw]=s[jw]-a1*s[jw-w]-a2*s[jw-w-w];

	}	//po vrsticah

//printaj(s,w,h,1,1,0);
//printf("\n\n");
//printaj(s,w,h,100,100,0);

//pa se navzgor
//spodnji dve vrstici
h1w=(h-1)*w; h2w=(h-2)*w;
for (j=0;j<w;j++)	//po stolpcih
	{
	if (ec!=0)
		{	//edge comp za gor
		cr=0.0;
		for (i=h-avg;i<h;i++)
			{
			cr=cr+s[j+w*i];
			}
		cr=cr*avgg;
		}

	rep1=(s[j+h1w]+s[j+h2w])*0.5*rs1+(s[j+h1w]-s[j+h2w])*rd1;
	rep2=(s[j+h1w]+s[j+h2w])*0.5*rs2+(s[j+h1w]-s[j+h2w])*rd2;

	if (ec!=0)
		{	//edge comp
		rep1=rep1+rc1*cr;
		rep2=rep2+rc2*cr;
		}

	s[j+h1w]=s[j+h1w]-a1*rep1-a2*rep2;
	s[j+h2w]=s[j+h2w]-a1*s[j+h1w]-a2*rep1;
	}

//ostale vrstice
for (i=h-3;i>=0;i--)		//gor
	{
	iw=i*w; i1w=iw+w; i2w=i1w+w;
	for (j=0;j<w;j++)
		{
		s[j+iw]=s[j+iw]-a1*s[j+i1w]-a2*s[j+i2w];
		}
	}

//printaj(s,w,h,1,1,1);
//printf("\n\n");
//printaj(s,w,h,100,100,1);
//printaj(s,w,h,300,300,0);
//printaj(s,w,h,300,574,0);

}
