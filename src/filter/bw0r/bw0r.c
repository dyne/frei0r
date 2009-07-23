#include "frei0r.h"
#include <stdlib.h>
#include <assert.h>

typedef struct blackwhite_instance
{
  unsigned int width;
  unsigned int height;
} blackwhite_instance_t;

int f0r_init()
{
  return 1;
}

void f0r_deinit()
{ /* no initialization required */ }

void f0r_get_plugin_info(f0r_plugin_info_t* blackwhiteInfo)
{
  blackwhiteInfo->name = "bw0r";
  blackwhiteInfo->author = "coma@gephex.org";
  blackwhiteInfo->plugin_type = F0R_PLUGIN_TYPE_FILTER;
  blackwhiteInfo->color_model = F0R_COLOR_MODEL_RGBA8888;
  blackwhiteInfo->frei0r_version = FREI0R_MAJOR_VERSION;
  blackwhiteInfo->major_version = 0; 
  blackwhiteInfo->minor_version = 9; 
  blackwhiteInfo->num_params =  0; 
  blackwhiteInfo->explanation = "Turns image black/white.";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
  /* no params */
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
  blackwhite_instance_t* inst = calloc(1, sizeof(*inst));
  inst->width = width; inst->height = height;
  return (f0r_instance_t)inst;
}

void f0r_destruct(f0r_instance_t instance)
{
  free(instance);
}

void f0r_set_param_value(f0r_instance_t instance, 
			 f0r_param_t param, int param_index)
{ /* no params */ }

void f0r_get_param_value(f0r_instance_t instance,
			 f0r_param_t param, int param_index)
{ /* no params */ }

void f0r_update(f0r_instance_t instance, double time,
		const uint32_t* inframe, uint32_t* outframe)
{
  assert(instance);
  blackwhite_instance_t* inst = (blackwhite_instance_t*)instance;
  unsigned int w = inst->width;
  unsigned int h = inst->height;
  unsigned int x,y;
  
  uint32_t* dst = outframe;
  const uint32_t* src = inframe;
  for(y=h;y>0;--y)
      for(x=w;x>0;--x,++src,++dst)
	{
	  int tmpbw;
	  unsigned char* tmpc = (unsigned char*)src;
	  tmpbw = (tmpc[0] + tmpc[1] + tmpc[2]) / 3;
	  *dst = (tmpc[3] << 24) | (tmpbw << 16) | (tmpbw << 8) | tmpbw;
	}
}

