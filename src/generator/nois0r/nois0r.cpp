/*
 * This frei0r plugin generates white noise images
 *
 * Copyright (C) 2004, 2005 Martin Bayer <martin@gephex.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "frei0r.hpp"

#include <algorithm>

struct wnoise
{
  wnoise(unsigned int s)
    : seed(s)
  {
  }

  unsigned int seed;

  unsigned int operator()()
  {
    seed *= 3039177861U; // parameter for LCG
    unsigned char rd = seed >> 24;
    return (rd | rd << 8 | rd << 16 | 0xff000000);
  }
};

class nois0r : public frei0r::source
{
public:
  nois0r(unsigned int width, unsigned int height)
  {
  }

  virtual void update(double time,
                      uint32_t *out)
  {
    wnoise wn(0x0f0f0f0f ^ (unsigned int)(time * 100000.0));

    std::generate(out, out + width * height, wn);
  }
};


frei0r::construct <nois0r> plugin("Nois0r",
                                  "Generates white noise images",
                                  "Martin Bayer",
                                  0, 3);
