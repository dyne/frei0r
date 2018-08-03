/*
 * Copyright (C) 2018 Dan Dennedy <dan@dennedy.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "frei0r.hpp"
#include "frei0r_math.h"

class Premultiply : public frei0r::filter
{

public:

    Premultiply(unsigned int width, unsigned int height)
        : m_unpremultiply(0)
    {
        register_param(m_unpremultiply, "unpremultiply", "Whether to unpremultiply instead");
    }

    ~Premultiply()
    {
    }

    virtual void update(double,
                        uint32_t* out,
                        const uint32_t* in)
    {
        uint8_t *src = (uint8_t*) in;
        uint8_t *dst = (uint8_t*) out;
        unsigned int n = width * height + 1;
        if (m_unpremultiply == 0.0) {
            // premultiply
            while (--n) {
                uint8_t a = src[3];
                dst[0] = (src[0] * a) >> 8;
                dst[1] = (src[1] * a) >> 8;
                dst[2] = (src[2] * a) >> 8;
                dst[3] =  a;
                src += 4;
                dst += 4;
            }
        } else {
            // unpremultiply
            while (--n) {
                uint8_t a = src[3];
                if (a > 0 && a < 255) {
                    dst[0] = MIN((src[0] << 8) / a, 255);
                    dst[1] = MIN((src[1] << 8) / a, 255);
                    dst[2] = MIN((src[2] << 8) / a, 255);
                } else {
                    dst[0] = src[0];
                    dst[1] = src[1];
                    dst[2] = src[2];
                }
                dst[3] = a;
                src += 4;
                dst += 4;
            }
        }
    }

private:
    f0r_param_bool m_unpremultiply;
};

frei0r::construct<Premultiply> plugin("Premultiply or Unpremultiply",
                "Multiply (or divide) each color component by the pixel's alpha value",
                "Dan Dennedy",
                0, 2,
                F0R_COLOR_MODEL_RGBA8888);
