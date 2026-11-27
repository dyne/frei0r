#include "frei0r.h"
#include <stdlib.h>
#ifdef __SSE2__
#include <emmintrin.h>
#endif
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
  blackwhite_instance_t* inst = (blackwhite_instance_t*)calloc(1, sizeof(*inst));
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
  unsigned int len = inst->width * inst->height;
  unsigned int i = 0;

#ifdef __SSE2__
  __m128i vFF = _mm_set1_epi32(0x000000FF);
  __m128i vDiv3 = _mm_set1_epi16(21846);
  __m128i vAlphaMask = _mm_set1_epi32((int)0xFF000000u);
  for (; i + 3 < len; i += 4)
  {
    __m128i px = _mm_loadu_si128((const __m128i*)(inframe + i));
    __m128i r = _mm_and_si128(px, vFF);
    __m128i g = _mm_and_si128(_mm_srli_epi32(px, 8), vFF);
    __m128i b = _mm_and_si128(_mm_srli_epi32(px, 16), vFF);
    __m128i sum = _mm_add_epi16(_mm_add_epi16(r, g), b);
    __m128i avg = _mm_mulhi_epu16(sum, vDiv3);
    __m128i rgb = _mm_or_si128(avg, _mm_or_si128(_mm_slli_epi32(avg, 8), _mm_slli_epi32(avg, 16)));
    __m128i res = _mm_or_si128(rgb, _mm_and_si128(px, vAlphaMask));
    _mm_storeu_si128((__m128i*)(outframe + i), res);
  }
#endif

  for (; i < len; ++i)
  {
    const unsigned char* tmpc = (const unsigned char*)(inframe + i);
    unsigned int tmpbw = ((unsigned int)tmpc[0] + tmpc[1] + tmpc[2]) / 3u;
    outframe[i] = ((uint32_t)tmpc[3] << 24) | (tmpbw << 16) | (tmpbw << 8) | tmpbw;
  }
}

