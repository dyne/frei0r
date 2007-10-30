#include "frei0r.h"
#include "frei0r_math.h"
#include <stdlib.h>
#include <assert.h>

typedef struct transparency_instance
{
  unsigned int width;
  unsigned int height;
  double transparency;
} transparency_instance_t;

int f0r_init()
{
  return 1;
}

void f0r_deinit()
{ /* no initialization required */ }

void f0r_get_plugin_info(f0r_plugin_info_t* transparencyInfo)
{
  transparencyInfo->name = "Transparency";
  transparencyInfo->author = "Richard Spindler";
  transparencyInfo->plugin_type = F0R_PLUGIN_TYPE_FILTER;
  transparencyInfo->color_model = F0R_COLOR_MODEL_RGBA8888;
  transparencyInfo->frei0r_version = FREI0R_MAJOR_VERSION;
  transparencyInfo->major_version = 0; 
  transparencyInfo->minor_version = 9; 
  transparencyInfo->num_params =  1; 
  transparencyInfo->explanation = "Tunes the alpha channel.";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
	switch(param_index)
	{
		case 0:
			info->name = "Transparency";
			info->type = F0R_PARAM_DOUBLE;
			info->explanation = "The transparency value";
			break;
	}
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
  transparency_instance_t* inst = 
    (transparency_instance_t*)malloc(sizeof(transparency_instance_t));
  inst->width = width; inst->height = height;
  inst->transparency = 0.0;
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
	transparency_instance_t* inst = (transparency_instance_t*)instance;

	switch(param_index)
	{
		double val;
		case 0:
		/* transparency */
		val = *((double*)param);
		if (val != inst->transparency)
		{
			inst->transparency = val;
		}
		break;
	}

}

void f0r_get_param_value(f0r_instance_t instance,
			 f0r_param_t param, int param_index)
{
	assert(instance);
	transparency_instance_t* inst = (transparency_instance_t*)instance;
	switch(param_index)
	{
		case 0:
			*((double*)param) = inst->transparency;
			break;
	}
}

void f0r_update(f0r_instance_t instance, double time,
		const uint32_t* inframe, uint32_t* outframe)
{
  assert(instance);
  transparency_instance_t* inst = (transparency_instance_t*)instance;
  unsigned int w = inst->width;
  unsigned int h = inst->height;
  unsigned int x,y;
  
  uint32_t* dst = outframe;
  const uint32_t* src = inframe;
  uint8_t alpha  = (uint8_t)( inst->transparency * 255 );
  for(y=h;y>0;--y)
      for(x=w;x>0;--x,++src,++dst)
	{
	  uint8_t tmpalpha;
	  uint8_t* tmpc = (uint8_t*)src;
	  tmpalpha = MIN(alpha, tmpc[3]);
	  *dst = (tmpalpha << 24) | (tmpc[2] << 16) | (tmpc[1] << 8) | tmpc[0];
	}
}

