/*
This frei0r plugin generates isingnoise images

Copyright (C) 2004 Georg Seidel <georg@gephex.org>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "frei0r.h"

//-------------------------------------------------------------------------

struct IsingField {
  char* s;
  int xsize;
  int ysize;
};

static void set_bf(uint32_t bf[3], double t, double b, double s);
static void init_field(struct IsingField* f, int xsize, int ysize);
static void destroy_field(struct IsingField* f);
static void do_step(struct IsingField* f, uint32_t bf[3]);
static void copy_field(const struct IsingField* f, uint32_t* framebuffer);

//-------------------------------------------------------------------------

#define MY_RAND_MAX UINT32_MAX

static uint32_t rnd_lcg1_xn = 1;

__inline static uint32_t rnd_lcg1()
{
  rnd_lcg1_xn *= 3039177861U;

  return rnd_lcg1_xn;
}

#define my_rand() rnd_lcg1()


typedef struct ising0r_instance
{
  unsigned int width;
  unsigned int height;

  double temp;
  double border_growth;
  double spont_growth;

  struct IsingField f;
  uint32_t bf[3];
} ising0r_instance_t;

int f0r_init()
{
  return 1;
}

void f0r_deinit()
{ /* no initialization required */ }

void f0r_get_plugin_info(f0r_plugin_info_t* nois0rInfo)
{
  nois0rInfo->name           = "Ising0r";
  nois0rInfo->author         = "Gephex crew";
  nois0rInfo->plugin_type    = F0R_PLUGIN_TYPE_SOURCE;
  nois0rInfo->color_model    = F0R_COLOR_MODEL_BGRA8888;
  nois0rInfo->frei0r_version = FREI0R_MAJOR_VERSION;
  nois0rInfo->major_version  = 0;
  nois0rInfo->minor_version  = 9;
  nois0rInfo->num_params     = 3;
  nois0rInfo->explanation    = "Generates ising noise";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
  switch (param_index)
    {
    case 0:
      info->name        = "Temperature";
      info->type        = F0R_PARAM_DOUBLE;
      info->explanation = "Noise Temperature"; break;
    case 1:
      info->name        = "Border Growth";
      info->type        = F0R_PARAM_DOUBLE;
      info->explanation = "Border Growth"; break;
    case 2:
      info->name        = "Spontaneous Growth";
      info->type        = F0R_PARAM_DOUBLE;
      info->explanation = "Spontaneous Growth"; break;
    }
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
  ising0r_instance_t* inst = 
    (ising0r_instance_t*)malloc(sizeof(ising0r_instance_t));
  inst->width  = width; 
  inst->height = height;

  init_field(&inst->f, width, height);

  return (f0r_instance_t)inst;
}

void f0r_destruct(f0r_instance_t instance)
{
  ising0r_instance_t* inst = (ising0r_instance_t*)instance;
  destroy_field(&inst->f);
  free(inst);
}

void f0r_set_param_value(f0r_instance_t instance, 
			 f0r_param_t param, int param_index)
{
  ising0r_instance_t* inst = (ising0r_instance_t*)instance;

  f0r_param_double* p = (f0r_param_double*) param;

  switch (param_index)
    {
    case 0:
      inst->temp          = *p *6; break;
    case 1:
      inst->border_growth = (1.0 - *p)*100; break;
    case 2:
      inst->spont_growth  = (1.0 - *p)*100; break;
    }
}

void f0r_get_param_value(f0r_instance_t instance,
			 f0r_param_t param, int param_index)
{
  ising0r_instance_t* inst = (ising0r_instance_t*)instance;

  f0r_param_double* p = (f0r_param_double*) param;

  switch (param_index)
    {
    case 0:
      *p = inst->temp / 6; break;
    case 1:
      *p = 1.0 - inst->border_growth / 100; break;
    case 2:
      *p = 1.0 - inst->spont_growth / 100; break;
    }
}


void f0r_update(f0r_instance_t instance, double time,
		const uint32_t* inframe, uint32_t* outframe)
{
  assert(instance);
  ising0r_instance_t* inst = (ising0r_instance_t*)instance;
  unsigned int w = inst->width;
  unsigned int h = inst->height;
  unsigned int x,y;
  
  set_bf(inst->bf, inst->temp, inst->border_growth, inst->spont_growth);
  
  do_step(&inst->f, inst->bf);

  copy_field(&inst->f, outframe);  
}

//-------------------------------------------------------------------------

static void set_bf(uint32_t bf[3], double t, double b, double s)
{
  /*  {
    char buffer[128];

    snprintf(buffer, sizeof(buffer), "Changing bf: (t,b,s)=(%f,%f,%f)\n",
	     t, b, s);

    s_log(2, buffer);
    }*/

  bf[0] = (uint32_t) (0.5 * MY_RAND_MAX);

  if (t > 0)
    {
      bf[1] = (uint32_t) (exp(-b/t)*MY_RAND_MAX);
      bf[2] = (uint32_t) (exp(-s/t)*MY_RAND_MAX);
    }
  else
    {
      bf[1] = bf[2] = 0;
    }

}

static void init_field(struct IsingField* f, int xsize, int ysize)
{
  int x, y;
  f->s = (char*) malloc(xsize*ysize);

  f->xsize = xsize;
  f->ysize = ysize;

  //  memset(

  for (y = 1; y < ysize-1; ++y)
    {
      int y_base = y*xsize;
      for (x = 1; x < xsize-1; ++x) 
	{
	  f->s[x + y_base] = (my_rand() < MY_RAND_MAX/2) ? -1 : 1;
	}
      f->s[y_base] = f->s[xsize-1 + y_base] = 1;
    }

  // set first and last line to black
  memset(f->s, 1, xsize);
  memset(f->s + (ysize-1)*xsize, 1, xsize);
}

static void destroy_field(struct IsingField* f)
{
  if (f->s != 0)
    {
      free(f->s);
      f->s = 0;
      // plain paranoia...
      f->xsize = 0;
      f->ysize = 0;
    }
}

static void do_step(struct IsingField* f, uint32_t bf[3])
{
  int x, y;
  int xsize = f->xsize;
  int ysize = f->ysize;

  // start on second pixel of the second line (f->s[1][1]):
  char* current = f->s + xsize + 1;

  for (y = ysize-2; y > 0; --y)
    {
      for (x = xsize-2; x > 0; --x)
	{
	  int sum =
	    current[-xsize] + current[xsize] +
	    current[-1] + current[1];
	  
	  int e = *current * sum;

	  if (e < 0 || my_rand() < bf[e>>1])
	    {
	      *current *= -1;
	    }

	  ++current;
	}
      // skip last pixel of this line and first pixel of next line:
      current += 2;
    }
}

static void copy_field(const struct IsingField* f, uint32_t* framebuffer)
{
  int i;
  char* s = f->s;
  uint32_t* fr = framebuffer;

  for (i = (f->xsize)*(f->ysize)-1; i >= 0; --i)
    {
      *(fr++) = *(s++);	  
    }
}

//-------------------------------------------------------------------------

