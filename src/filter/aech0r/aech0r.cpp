/*
 * aech0r.cpp
 *
 * This frei0r plugin aims to simulate a video echo with some colors tweaks.
 * Version 0.1	sept 2020
 *
 * Copyright (C) 2018-2020 d-j-a-y & vloop
 *
 * This source code  is free software; you can  redistribute it and/or
 * modify it under the terms of the GNU Public License as published by
 * the Free Software  Foundation; either version 3 of  the License, or
 * (at your option) any later version.
 *
 * This source code is distributed in the hope that it will be useful,
 * but  WITHOUT ANY  WARRANTY; without  even the  implied  warranty of
 * MERCHANTABILITY or FITNESS FOR  A PARTICULAR PURPOSE.  Please refer
 * to the GNU Public License for more details.
 *
 * You should  have received  a copy of  the GNU Public  License along
 * with this source code; if  not, write to: Free Software Foundation,
 * Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
#include "frei0r.hpp"
#include "frei0r/math.h"

#include <string.h>
#include <climits>

////// Uncomment to force non optimisation version
//~ #undef __SSE2__

////// TODO / IDEAS / ... //////
// IDEA - directionnal echo ? (X/Y like parameter )
// TODO RGB gradient by fading influence need more love (See '//Fade by color layers')!
// FIXME SSE2 version doesn't support RGB fading influence !

// FIXME (Veejay specifics?) on activate/deactivate/activate/..., some buffers must be cleared!

// EXPLORE ME -
//~ if((skip_count++)>m_skip) {
//~ -      skip_count = 0;
//~ -      m_factor += (factor * 64);
//~ [...]
//~ }

// EXPLORE ME a very high m_factor value give some interesting color result (      m_factor += (factor * 64) * m_skip;)


/* Intrinsic declarations */
#if defined(__SSE2__)
#include <emmintrin.h>
// TODO mmx support and others
//  #elif defined(__MMX__)
// #include <mmintrin.h>
#endif

#define SKIP_MAX_IMAGES 8
#define M_FACTOR_MAV    127

union px_t {
  uint32_t u;
  unsigned char c[4]; // 0=B, 1=G,2=R,3=A ? i think :P
};

class aech0r : public frei0r::filter {
private:
  f0r_param_double factor;
  //~ f0r_param_double factor_r;  //Fade by color layers
  //~ f0r_param_double factor_g;
  //~ f0r_param_double factor_b;
  f0r_param_double strobe_period;
  bool bright;
  bool flag_r;
  bool flag_g;
  bool flag_b;


  //~ f0r_param_double fade_rgb;  //Fade by color layers
  //~ f0r_param_double flag_rgb;

  unsigned int m_factor;

  //~ unsigned int m_factor_r;  //Fade by color layers
  //~ unsigned int m_factor_g;
  //~ unsigned int m_factor_b;

  //~ unsigned int m_flag_rgb;  //Fade by color layers
  //~ unsigned int m_flag_r;
  //~ unsigned int m_flag_g;
  //~ unsigned int m_flag_b;

  unsigned int m_skip;
  unsigned int m_skip_count;

  bool firsttime;

  //~ unsigned int m_rgb; //Fade by color layers

#ifdef __SSE2__
  long long int m_factor_sse2;

  inline void tracesse2_add(uint32_t* out, const uint32_t* in);
  inline void tracesse2_sub(uint32_t* out, const uint32_t* in);
#else
  unsigned int m_factor_r;
  unsigned int m_factor_g;
  unsigned int m_factor_b;

  inline void trace_add(uint32_t* out, const uint32_t* in);
  inline void trace_sub(uint32_t* out, const uint32_t* in);
#endif
public:

  aech0r(unsigned int width, unsigned int height) {

    factor = 0.15; // Quasi full echo has default
    bright = false; // Dark mode has default
    
    flag_r = false; // No RGB flag has default
    flag_g = false;
    flag_b = false;

    strobe_period = 0; // No strobe has default

    //~ fade_rgb = ?  //Fade by color layers

    firsttime = true;
    m_skip_count = 0;

    register_param(factor, "Fade Factor", "Disappearance rate of the echo"); // 0 No fade, 1 No Trace
    register_param(bright, "Direction", "Darker or Brighter echo"); // Add or Subtract data
    register_param(flag_r, "Keep RED", "Influence on Red channel"); // 0 Fade canal, 1 Keep canal data
    register_param(flag_g, "Keep GREEN", "Influence on Green channel"); // 0 Fade canal, 1 Keep canal data
    register_param(flag_b, "Keep BLUE", "Influence on Blue channel"); // 0 Fade canal, 1 Keep canal data
    register_param(strobe_period, "Strobe period", "Rate of the stroboscope: from 0 to 8 frames");

    //~ register_param(fade_rgb, "Plans fade", "RGB");  //Fade by color layers
    //~ register_param(factor_r, "Fade R", "influence"); // 0 No fade, 1 No Trace
    //~ register_param(factor_g, "Fade G", "influence"); // 0 No fade, 1 No Trace
    //~ register_param(factor_b, "Fade B", "influence"); // 0 No fade, 1 No Trace
    //~ register_param(flag_rgb, "Plans comparison", "RGB");

  }
  ~aech0r() {
  }

  virtual void update(double time,
                      uint32_t* out,
                      const uint32_t* in) {

    if (firsttime) {
      memcpy(out, in, size * sizeof(uint32_t)  ); // assuming we are RGBA only
      firsttime = false;
      return;
    }

    m_skip = (strobe_period * SKIP_MAX_IMAGES);
    if(m_skip_count++ < m_skip) {
      return;
    }
    m_skip_count = 0;

    //~ m_factor = m_factor_sse2 = 0; //blink

    unsigned int bright_factor = (bright)? 0:UINT_MAX;

    //~ m_rgb = (fade_rgb * 8); //Fade by color layers
    m_factor = (factor * M_FACTOR_MAV);  //MAgic Value ;-)

#ifdef __SSE2__
    // sse2 mask for fade operation
    m_factor_sse2 = 0;
    m_factor_sse2 = (flag_r==true)?(bright_factor << 24):(m_factor << 16);
    m_factor_sse2 += (flag_g==true)?(bright_factor << 16):(m_factor << 8);
    m_factor_sse2 += (flag_b==true)?(bright_factor << 8):(m_factor << 0);
#else
    m_factor_r = (flag_r==true)?(bright_factor):(m_factor);
    m_factor_g = (flag_g==true)?(bright_factor):(m_factor);
    m_factor_b = (flag_b==true)?(bright_factor):(m_factor);
#endif

    //~ m_factor_r = m_factor * factor_r;  //Fade by color layers
    //~ m_factor_g = m_factor * factor_g;
    //~ m_factor_b = m_factor * factor_b;
    //~ m_flag_rgb=1+flag_rgb*6;
    //~ m_flag_b = (m_flag_rgb & 1) == 1;
    //~ m_flag_g = (m_flag_rgb & 2) == 2;
    //~ m_flag_r = (m_flag_rgb & 4) == 4;
    //~ m_factor_sse2 = (m_factor << 16) + (m_factor << 8) + m_factor ;

    if(bright) {
      for(unsigned int i = 0 ; i < size ; i+=4) {
#ifdef __SSE2__
        tracesse2_sub(out+i, in+i);
#else
        trace_sub(out+i, in+i);
        trace_sub(out+i+1, in+i+1);
        trace_sub(out+i+2, in+i+2);
        trace_sub(out+i+3, in+i+3);
#endif
      }
    } else {
      for(unsigned int i = 0 ; i < size ; i+=4) {
#ifdef __SSE2__
        tracesse2_add(out+i, in+i);
#else
        trace_add(out+i, in+i);
        trace_add(out+i+1, in+i+1);
        trace_add(out+i+2, in+i+2);
        trace_add(out+i+3, in+i+3);
#endif
      }
    }

  }
};

#ifdef __SSE2__
inline void aech0r::tracesse2_sub(uint32_t* out, const uint32_t* in) {
  __m128i aa = _mm_load_si128((__m128i*)in);
  __m128i bb = _mm_load_si128((__m128i*)out);
  // set a fade (rgb) computation
  __m128i ff = _mm_set1_epi32(m_factor_sse2);
  bb = _mm_subs_epu8(bb, ff);

  // unsigned a < b
  __m128i tmp = _mm_cmpeq_epi8( aa, _mm_min_epu8(aa, bb));

  // create a bit mask
  ff  = _mm_cmpeq_epi32 (tmp, _mm_set1_epi8(0xff));

  bb = _mm_or_si128 (_mm_andnot_si128 (ff, aa),_mm_and_si128 (bb,ff));
  _mm_store_si128((__m128i*)&out[0], bb);
}

inline void aech0r::tracesse2_add(uint32_t* out, const uint32_t* in) {
  __m128i aa = _mm_load_si128((__m128i*)in);
  __m128i bb = _mm_load_si128((__m128i*)out);

  // set a fade (rgb) value
  __m128i ff = _mm_set1_epi32(m_factor_sse2);
  bb = _mm_adds_epu8(bb, ff);

  // unsigned a >= b
  __m128i tmp = _mm_cmpeq_epi8( aa, _mm_max_epu8(aa, bb));

  // create a bit mask
  ff  = _mm_cmpeq_epi32 (tmp, _mm_set1_epi8(0xff));

  bb = _mm_or_si128 (_mm_andnot_si128 (ff, aa),_mm_and_si128 (bb,ff));
  _mm_store_si128((__m128i*)&out[0], bb);
}

#else

inline void aech0r::trace_sub(uint32_t* out, const uint32_t* in) {

  px_t po, pi;
  po.u = *out;
  pi.u = *in;

  //~ po.c[0]=(m_rgb & 4)?pi.c[0]:CLAMP0255(po.c[0] - m_factor_b);//Fade by color layers
  //~ po.c[1]=(m_rgb & 2)?pi.c[1]:CLAMP0255(po.c[1] - m_factor_g);
  //~ po.c[2]=(m_rgb & 1)?pi.c[2]:CLAMP0255(po.c[2] - m_factor_r);
  //NOTA : BGR order come from Frei0r spec
  po.c[0]=(CLAMP0255(po.c[0] - m_factor_b));
  po.c[1]=(CLAMP0255(po.c[1] - m_factor_g));
  po.c[2]=(CLAMP0255(po.c[2] - m_factor_r));
  *out = po.u;
  if( (po.c[0]<=pi.c[0]) |
      (po.c[1]<=pi.c[1]) |
      (po.c[2]<=pi.c[2]) ) {
    *out = pi.u;
  }
}

inline void aech0r::trace_add(uint32_t* out, const uint32_t* in) {

  px_t po, pi;
  po.u = *out;
  pi.u = *in;

  //NOTA : BGR order come from Frei0r spec
  po.c[0] = CLAMP0255(po.c[0] + m_factor_b);
  po.c[1] = CLAMP0255(po.c[1] + m_factor_g);
  po.c[2] = CLAMP0255(po.c[2] + m_factor_r);
  //~ po.c[0]=(m_rgb & 4)?pi.c[0]:CLAMP0255(po.c[0] + m_factor_b); //Fade by color layers
  //~ po.c[1]=(m_rgb & 2)?pi.c[1]:CLAMP0255(po.c[1] + m_factor_g);
  //~ po.c[2]=(m_rgb & 1)?pi.c[2]:CLAMP0255(po.c[2] + m_factor_r);
  *out = po.u;
  //~ if( ((po.c[0]>pi.c[0])&m_flag_b) | //Fade by color layers (why this test is here and not in trace_sub?)
      //~ ((po.c[1]>pi.c[1])&m_flag_g) |
      //~ ((po.c[2]>pi.c[2])&m_flag_r) ) {
  if( ((po.c[0]>pi.c[0])) |
      ((po.c[1]>pi.c[1])) |
      ((po.c[2]>pi.c[2])) ) {
    *out = pi.u;
  }
}

#endif // __SSE2__

frei0r::construct<aech0r> plugin("aech0r",
									"analog video echo",
									"d-j-a-y & vloop",
									0,1);
