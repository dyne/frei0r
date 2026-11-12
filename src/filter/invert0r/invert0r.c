#include <stdlib.h>
#include <assert.h>

#include "frei0r.h"

typedef struct inverter_instance
{
  unsigned int width;
  unsigned int height;
} inverter_instance_t;

int f0r_init()
{
  return 1;
}

void f0r_deinit()
{ /* no initialization required */ }

void f0r_get_plugin_info(f0r_plugin_info_t* inverterInfo)
{
  inverterInfo->name = "Invert0r";
  inverterInfo->author = "Gephex crew";
  inverterInfo->plugin_type = F0R_PLUGIN_TYPE_FILTER;
  inverterInfo->color_model = F0R_COLOR_MODEL_RGBA8888;
  inverterInfo->frei0r_version = FREI0R_MAJOR_VERSION;
  inverterInfo->major_version = 0; 
  inverterInfo->minor_version = 9; 
  inverterInfo->num_params =  0; 
  inverterInfo->explanation = "Inverts all colors of a source image";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
  /* no params */
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
  inverter_instance_t* inst = (inverter_instance_t*)calloc(1, sizeof(*inst));
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
  inverter_instance_t* inst = (inverter_instance_t*)instance;
  unsigned int pixels = inst->width * inst->height;
  unsigned char* dst = (unsigned char*)outframe;
  const unsigned char* src = (const unsigned char*)inframe;

  /* RGBA channels are ordered bytes, not uint32_t bit fields. */
  while (pixels--) {
    dst[0] = 255 - src[0];
    dst[1] = 255 - src[1];
    dst[2] = 255 - src[2];
    dst[3] = src[3];
    src += 4;
    dst += 4;
  }
}

