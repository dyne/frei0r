#include <stdlib.h>
#include <string.h>
#ifdef __SSE2__
#include <emmintrin.h>
#endif
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
  /* Endian-independent RGBA byte mask: invert R, G, B (255 - x == x ^ 0xFF), preserve A. */
  static const unsigned char mask_bytes[4] = { 0xFF, 0xFF, 0xFF, 0x00 };
  uint32_t mask;
  memcpy(&mask, mask_bytes, sizeof(mask));

  unsigned int i = 0;
#ifdef __SSE2__
  __m128i vMask = _mm_set1_epi32((int)mask);
  for (; i + 3 < pixels; i += 4) {
    __m128i px = _mm_loadu_si128((const __m128i*)(inframe + i));
    _mm_storeu_si128((__m128i*)(outframe + i), _mm_xor_si128(px, vMask));
  }
#endif
  for (; i < pixels; ++i) {
    outframe[i] = inframe[i] ^ mask;
  }
}

