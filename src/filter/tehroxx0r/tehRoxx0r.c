#include "frei0r.h"
#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <string.h>

/**
 * This is our instance.
 * It has a buffer allocated to place a small version of the incoming 
 * frame into.
 */
typedef struct teh_roxx0r
{
  unsigned int width;      // frame size in x-dimension
  unsigned int height;     // frame size in y-dimension
  unsigned int block_size; // x/y size of one block

  double change_speed;
  double last_time;
  double time_stack;

  uint32_t* small_block;    // buffer to write downscaled frame
  
} tehRoxx0r_instance_t;


// returns greatest common divisor of to int numbers
int gcd(int a, int b);

int f0r_init()
{
  return 1;
}

void f0r_deinit()
{ /* no initialization required */ }

void f0r_get_plugin_info(f0r_plugin_info_t* tehRoxx0rInfo)
{
  tehRoxx0rInfo->name = "TehRoxx0r";
  tehRoxx0rInfo->author = "Coma";
  tehRoxx0rInfo->plugin_type = F0R_PLUGIN_TYPE_FILTER;
  tehRoxx0rInfo->color_model = F0R_COLOR_MODEL_BGRA8888;
  tehRoxx0rInfo->frei0r_version = FREI0R_MAJOR_VERSION;
  tehRoxx0rInfo->major_version = 0; 
  tehRoxx0rInfo->minor_version = 9; 
  tehRoxx0rInfo->num_params =  1; 
  tehRoxx0rInfo->explanation = "Something videowall-ish";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
  
  info->name = "Interval";
  info->type = F0R_PARAM_DOUBLE;
  info->explanation = "Changing speed of small blocks";
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
  int blocksize;
  tehRoxx0r_instance_t* inst = 
    (tehRoxx0r_instance_t*)malloc(sizeof(tehRoxx0r_instance_t));
  inst->width = width; inst->height = height;
  inst->change_speed = 0.01;
  inst->last_time = 0.0;
  inst->time_stack = 0.0;

  // get greatest common divisor
  blocksize = gcd(width, height);
  // this will sometimes be to large, so roughly estimate a check
  if(blocksize >= (width/3) || blocksize >= (height/3))
    blocksize /= 2;

  inst->block_size = blocksize;

  inst->small_block = 
    (uint32_t*)malloc(sizeof(uint32_t)*inst->block_size*inst->block_size);

  return (f0r_instance_t)inst;
}

void f0r_destruct(f0r_instance_t instance)
{
  tehRoxx0r_instance_t* inst = (tehRoxx0r_instance_t*)instance;
  free(inst->small_block);
  free(inst);
}

void f0r_set_param_value(f0r_instance_t instance, 
			 f0r_param_t param, int param_index)
{ 
  tehRoxx0r_instance_t* inst = 
    (tehRoxx0r_instance_t*)malloc(sizeof(tehRoxx0r_instance_t));

  switch(param_index)
    {
    case 0:
      inst->change_speed = *((double*)param);
      break;
    };
}

void f0r_get_param_value(f0r_instance_t instance,
			 f0r_param_t param, int param_index)
{ 
  tehRoxx0r_instance_t* inst = 
    (tehRoxx0r_instance_t*)malloc(sizeof(tehRoxx0r_instance_t));

  switch(param_index)
    {
    case 0:
      *((double*)param) = inst->change_speed;
      break;
    };  
}


void f0r_update(f0r_instance_t instance, double time,
		const uint32_t* inframe, uint32_t* outframe)
{
  assert(instance);
  tehRoxx0r_instance_t* inst = (tehRoxx0r_instance_t*)instance;
  unsigned int w = inst->width;
  unsigned int h = inst->height;
  uint32_t* dst;
  const uint32_t* src;
  uint32_t* small_block = inst->small_block;
  unsigned int x,y;
  unsigned int small_x, small_y;
  unsigned int small_w, small_h;
  double step_x, step_y;
  unsigned int pos_w, pos_h;
  
  // get x/y-size of middle block 
  small_w = w-2*inst->block_size;
  small_h = h-2*inst->block_size;
  
  // get interpolation step for that
  step_x = (double)w / (double)small_w;
  step_y = (double)h / (double)small_h;
  
  
  // copy a downscaled version into the middle of the result frame 
  // (blocksize to x-blocksize and blocksize to y-blocksize)
  for(y = 0, small_y=inst->block_size;small_y<h-inst->block_size;small_y++,
	y=step_y*(small_y-inst->block_size))
    {
      src = inframe + y*w;
      dst = outframe + small_y*w + inst->block_size;
      for(x=0;x<w-2*inst->block_size;x++)
	{
	  *dst++ = *(src + (int)(x*step_x));
	}
    }

  // add elapsed time to timestack
  inst->time_stack += (time-inst->last_time);  
  

  // get interpolation step size
  step_x = w / inst->block_size;
  step_y = h / inst->block_size;

  // create a small picture
  for(y=0,small_y=0; small_y<inst->block_size; small_y++,y+=step_y)
    {
      src = inframe + y*w;
      dst = small_block + small_y*inst->block_size;
      for(x=0,small_x = 0; small_x<inst->block_size; small_x++)//,x+=step_x)
	{
	  *dst++ = *src;
	  src += (unsigned int)step_x;
	}
    }
  // do we actually changed anything?
  if(inst->time_stack > inst->change_speed)
    {
      // get random position
      pos_w = inst->block_size * 
	(unsigned int)(rand()/(double)RAND_MAX * ((w / inst->block_size)));
      pos_h = inst->block_size * 
	(unsigned int)(rand()/(double)RAND_MAX * ((h / inst->block_size)));
      
      // now copy to some (random) places along the border of
      // the incoming frame.....
      dst = outframe + pos_w;
      src = small_block;
      for(x=0; x<inst->block_size; 
	  x++, dst += w, src += inst->block_size)
	memcpy(dst, src, sizeof(int32_t)*inst->block_size);

      dst = outframe + pos_h * w;
      src = small_block;
      for(x=0; x<inst->block_size; 
	  x++, dst += w, src += inst->block_size)
	memcpy(dst, src, sizeof(int32_t)*inst->block_size);

      dst = outframe + pos_h* w + w - inst->block_size;
      src = small_block;
      for(x=0; x<inst->block_size; 
	  x++, dst += w, src += inst->block_size)
	memcpy(dst, src, sizeof(int32_t)*inst->block_size);


      dst = outframe + (h-inst->block_size) *w + pos_w;
      src = small_block;
      for(x=0; x<inst->block_size; 
	  x++, dst += w, src += inst->block_size)
	memcpy(dst, src, sizeof(int32_t)*inst->block_size);

      
      // reset timestack
      inst->time_stack = 0.0;
    }
  
  
  inst->last_time = time;
}

// greatest common divisor. this will never become smaller than 8.
int gcd(int a, int b)
{
  if(b==0) return a;
  else return gcd(b, a%b);
}
