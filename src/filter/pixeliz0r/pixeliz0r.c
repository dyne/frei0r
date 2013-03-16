#include "frei0r.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static uint32_t average(const uint32_t* start,
                       int bxsize,
                       int bysize, int xsize);

static void fill_block(uint32_t* start,
                       int bxsize,
                       int bysize,
                       int xsize, uint32_t col);


typedef struct pixelizer_instance
{
  unsigned int width;
  unsigned int height;
  unsigned int block_size_x;
  unsigned int block_size_y;
} pixelizer_instance_t;

int f0r_init()
{
  return 1;
}

void f0r_deinit()
{ /* no initialization required */ }

void f0r_get_plugin_info(f0r_plugin_info_t* pixelizerInfo)
{
  pixelizerInfo->name = "pixeliz0r";
  pixelizerInfo->author = "Gephex crew";
  pixelizerInfo->plugin_type = F0R_PLUGIN_TYPE_FILTER;
  pixelizerInfo->color_model = F0R_COLOR_MODEL_PACKED32;
  pixelizerInfo->frei0r_version = FREI0R_MAJOR_VERSION;
  pixelizerInfo->major_version = 1; 
  pixelizerInfo->minor_version = 0; 
  pixelizerInfo->num_params =  2; 
  pixelizerInfo->explanation = "Pixelize input image.";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
  switch(param_index)
    {
    case 0:
      info->name = "Block width";
      info->type = F0R_PARAM_DOUBLE;
      info->explanation = "Horizontal size of one \"pixel\"";
      break;
    case 1:
      info->name = "Block height";
      info->type = F0R_PARAM_DOUBLE;
      info->explanation = "Vertical size of one \"pixel\"";
      break;      
    }
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
  pixelizer_instance_t* inst = (pixelizer_instance_t*)calloc(1, sizeof(*inst));
  inst->width = width; inst->height = height;
  inst->block_size_x = 8; inst->block_size_y = 8;
  return (f0r_instance_t)inst;
}

void f0r_destruct(f0r_instance_t instance)
{
  free(instance);
}

void f0r_set_param_value(f0r_instance_t instance, 
			 f0r_param_t param, int param_index)
{ 
  assert(instance);
  pixelizer_instance_t* inst = (pixelizer_instance_t*)instance;
  
  switch(param_index)
    {
    case 0:
      // scale to [1..width]
      inst->block_size_x =  1 + ( *((double*)param) * (inst->width/2)) ;
      break;
    case 1:
      // scale to [1..height]
      inst->block_size_y =  1 + ( *((double*)param) * (inst->height/2)) ;
      break;
    }  
}

void f0r_get_param_value(f0r_instance_t instance,
			 f0r_param_t param, int param_index)
{ 
  assert(instance);
  pixelizer_instance_t* inst = (pixelizer_instance_t*)instance;
  
  switch(param_index)
    {
    case 0:
      // scale back to [0..1]
      *((double*)param) = (double)(inst->block_size_x-1)/(inst->width/2);
      break;
    case 1:
      // scale back to [0..1]
      *((double*)param) = (double)(inst->block_size_y-1)/(inst->height/2);
      break;
    }  
}

void f0r_update(f0r_instance_t instance, double time,
		const uint32_t* inframe, uint32_t* outframe)
{
  assert(instance);
  pixelizer_instance_t* inst = (pixelizer_instance_t*)instance;
  unsigned int xsize = inst->width;
  unsigned int ysize = inst->height;
  unsigned int bsizex = inst->block_size_x;
  unsigned int bsizey = inst->block_size_y;
  unsigned int offset = 0;
  unsigned int blocks_x, blocks_y, xrest, yrest, xi, yi;


  blocks_x = xsize / bsizex;
  blocks_y = ysize / bsizey;
  xrest = xsize - blocks_x*bsizex;
  yrest = ysize - blocks_y*bsizey;

  uint32_t* dst = outframe;
  const uint32_t* src = inframe;


  if (bsizex == 1 && bsizey == 1)
    {
      memcpy(dst, src, xsize*ysize*sizeof(uint32_t));
    }

  
  // now get average for every block and write the average color to the output
  for (yi = 0; yi < blocks_y; ++yi)
    {	  
      offset = yi*bsizey*xsize;
      for (xi = 0; xi < blocks_x; ++xi)
	{
	  uint32_t col = average(src + offset, bsizex, bsizey, xsize);
	  fill_block(dst + offset, bsizex, bsizey, xsize, col);
	  offset += bsizex;
	}
      if (xrest > 0)
	{
	  uint32_t col = average(src + offset, xrest, bsizey, xsize);
	  fill_block(dst + offset, xrest, bsizey, xsize, col);
	}
    }
  // check for last line
  if (yrest > 0)
    {
      offset = blocks_y*bsizey*xsize;
      for (xi = 0; xi < blocks_x; ++xi)
	{
	  uint32_t col = average(src + offset, bsizex, yrest, xsize);
	  fill_block(dst + offset, bsizex, yrest, xsize, col);
	  offset += bsizex;
	}
      if (xrest > 0)
	{
	  uint32_t col = average(src + offset, xrest, yrest, xsize);
	  fill_block(dst + offset, xrest, yrest, xsize, col);
	}
    } 
}


static uint32_t average(const uint32_t* start, 
			int bxsize, int bysize, int xsize)
{
  const uint32_t* p = start;
  uint32_t alpha = 0, red = 0, green = 0, blue = 0;
  uint32_t avg_alpha, avg_red, avg_green, avg_blue;
  int x, y;
  const uint32_t* pp;
  uint32_t c;
  
  for (y = 0; y < bysize; ++y)
    {
      pp = p;
      for (x = 0; x < bxsize; ++x)
	{
	  c = *(pp++);
          alpha += (c & 0xff000000) >> 24;
	  red   += (c & 0x00ff0000) >> 16;
	  green += (c & 0x0000ff00) >> 8;
	  blue  += (c & 0x000000ff);
	}
      p += xsize;
    }
  
  avg_alpha = (alpha / (bxsize*bysize)) & 0xff;
  avg_red   = (red   / (bxsize*bysize)) & 0xff;
  avg_green = (green / (bxsize*bysize)) & 0xff;
  avg_blue  = (blue  / (bxsize*bysize)) & 0xff;
  
  return (avg_alpha << 24) + (avg_red << 16) + (avg_green << 8) + avg_blue;
}

static void fill_block(uint32_t* start, int bxsize, int bysize,
                       int xsize, uint32_t col)
{
	uint32_t* p = start;
	int x, y;
	uint32_t* pp;

	for (y = 0; y < bysize; ++y)
	{
		pp = p;
		for (x = 0; x < bxsize; ++x)
		{
			*(pp++) = col;
		}
		p += xsize;
	}

}

