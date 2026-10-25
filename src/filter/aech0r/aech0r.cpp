/*
 * aech0r.cpp
 *
 * This frei0r plugin aims to simulate a video echo with some colors tweaks.
 * Version 0.1	sept 2020
 *
 * Copyright (C) 2018-2020 d-j-a-y & vloop
 * 
 * --- Comment from shadowfax65: --------------------------------------
 
 * The original implementation doesn't work (at least in Kdenlive where
 * I tested it). It generated flashing frames with complementary colors
 * (at least for the blues) and didn't  produce any  trailing echoes or
 * ghost images like  one would expect based  on the original  filter's
 * description. I asked claude.ai  what's wrong with it and how  to fix
 * it. This is the explanation:
 * 1) The echo state lives in the host’s out buffer. The filter assumes
 *    out  still  holds  last  frame’s  result,  so  out  is  both  the 
 *    accumulator  and  the output.  Many hosts  (MLT/Kdenlive/Shotcut,
 *    ping-pong buffering,in-place processing)  hand you a different or
 *    stale buffer  each frame.  You then fade a  frame from two frames
 *    ago, or garbage,  and get flashing.  The strobe early-return  has
 *    the same problem, because it never writesout at all.
 * 2) Keep flags are built  wrongly in the SSE2 path.  bright_factor is
 *    UINT_MAX (all 32 bits set), so bright_factor <<16 sets two bytes,
 *    not one.  The shifts are  also off by 8  (red uses <<24 here  but
 *    <<16 in the fade branch).   Since the mask is built with +=,  you
 *    also get c arries into  neighbouring channels.  Enabling any Keep
 *    flag therefore pushes  saturated values into the  wrong channels,
 *    which looks like complementary colour flashes.
 * 3) The decision is per pixel  while the fade is per channel.  If any
 *    one channel crosses  the threshold  (noise is enough),  the whole
 *    pixel snaps to  the input.  Combined with  the inconsistent  Keep
 *    semantics (255 in one mode, 0 in the other), the trails break up.
 *
 * Since I am sure  no one  wants to use the  non optimized  version, I
 * deleted all #ifdef __SSE2__ branches.
 *
 * All credit belongs to the  original creative minds  behind this  and
 * the original developers.
 * --- end of comment -------------------------------------------------
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
#include <vector>

#define SKIP_MAX_IMAGES 8
#define M_FACTOR_MAV    127

union px_t {
  uint32_t u;
  unsigned char c[4]; // 0=B, 1=G,2=R,3=A ? i think :P
};

class aech0r : public frei0r::filter {
private:
  f0r_param_double factor;
  f0r_param_double strobe_period;
  bool bright, flag_r, flag_g, flag_b;

  unsigned int m_factor;
  unsigned int m_skip;
  unsigned int m_skip_count;
  bool firsttime;
  std::vector<uint32_t> echo;   // persistent state, independent of host buffers

public:
  aech0r(unsigned int width, unsigned int height) {
    factor = 0.15;
    bright = false;
    flag_r = flag_g = flag_b = false;
    strobe_period = 0;
    firsttime = true;
    m_skip_count = 0;

    register_param(factor, "Fade Factor", "Disappearance rate of the echo");
    register_param(bright, "Direction", "Darker or Brighter echo");
    register_param(flag_r, "Keep RED", "Influence on Red channel");
    register_param(flag_g, "Keep GREEN", "Influence on Green channel");
    register_param(flag_b, "Keep BLUE", "Influence on Blue channel");
    register_param(strobe_period, "Strobe period", "Rate of the stroboscope: from 0 to 8 frames");
  }

  virtual void update(double time, uint32_t* out, const uint32_t* in) {
    if (firsttime || echo.size() != size) {
      echo.assign(in, in + size);
      memcpy(out, in, size * sizeof(uint32_t));
      firsttime = false;
      m_skip_count = 0;
      return;
    }

    m_skip = (unsigned int)(strobe_period * SKIP_MAX_IMAGES);
    if (m_skip_count++ >= m_skip) {
      m_skip_count = 0;
      m_factor = (unsigned int)(factor * M_FACTOR_MAV);

      // per-byte decay; byte order follows the original (B,G,R,A)
      uint8_t d[4] = {
        (uint8_t)(flag_b ? 255 : m_factor),
        (uint8_t)(flag_g ? 255 : m_factor),
        (uint8_t)(flag_r ? 255 : m_factor),
        255                                   // alpha: pass input through
      };

      uint8_t*       e = reinterpret_cast<uint8_t*>(echo.data());
      const uint8_t* s = reinterpret_cast<const uint8_t*>(in);
      const size_t   n = size * 4;
      size_t i = 0;

      for (; i < n; ++i) {                    // scalar fallback / tail
        int v = bright ? e[i] - d[i & 3] : e[i] + d[i & 3];
        v = v < 0 ? 0 : (v > 255 ? 255 : v);
        e[i] = bright ? (s[i] > v ? s[i] : v)
                      : (s[i] < v ? s[i] : v);
      }
    }

    memcpy(out, echo.data(), size * sizeof(uint32_t));
  }
};

frei0r::construct<aech0r> plugin("aech0r",
									"analog video echo",
									"d-j-a-y & vloop",
									0,1);
