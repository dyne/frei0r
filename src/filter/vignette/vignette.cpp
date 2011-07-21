/*
 * Copyright (C) 2011 Simon Andreas Eugster (simon.eu@gmail.com)
 * This file is a Frei0r plugin.
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

#include <cmath>
#include <iostream>
#include "frei0r.hpp"
#include "frei0r_math.h"


class Vignette : public frei0r::filter
{

public:

    f0r_param_double m_scaleY;
    f0r_param_double m_cc;
    f0r_param_double m_soft;

    Vignette(unsigned int width, unsigned int height) :
        m_width(width),
        m_height(height)
    {
        std::cout << "Vignette is " << width << "x" << height << std::endl;

        register_param(m_scaleY, "scaleY", "Shapee");
        register_param(m_cc, "clearCenter", "Size of the unaffected center");
        register_param(m_soft, "soft", "Softness");
        m_scaleY = .5;
        m_cc = 0;
        m_soft = 1;

        m_initialized = width*height > 0;
        if (m_initialized) {
            m_vignette = new float[width*height];
            updateVignette();
        } else {
            std::cout << "Not initializing vignette, has size 0";
        }
    }

    ~Vignette()
    {
        std::cout << "~Vignette()" << std::endl;
        if (m_initialized) {
            delete[] m_vignette;
        }
    }

    virtual void update()
    {
        std::cout << "Update called for " << m_width << "x" << m_height << " vignette.\n";
        std::copy(in, in + m_width*m_height, out);

        // Rebuild the vignette matrix if a parameter has changed
        if (m_prev_scaleY != m_scaleY
                || m_prev_cc != m_cc
                || m_prev_soft != m_soft) {
            std::cout << "Vignette needs to be updated." << std::endl;
            updateVignette();
        }

        unsigned char *pixel = (unsigned char *) in;
        unsigned char *dest = (unsigned char *) out;

        float *vignette = m_vignette;
        std::cout << "Applying vignette.\n";
        for (int i = 0; i < size; i++) {
            *dest++ = (char) (*vignette * *pixel++);
            *dest++ = (char) (*vignette * *pixel++);
            *dest++ = (char) (*vignette * *pixel++);
            *dest++ = *pixel++;
            vignette++;
        }
        std::cout << "Vignette applied.\n";

    }

private:
    f0r_param_double m_prev_scaleY;
    f0r_param_double m_prev_cc;
    f0r_param_double m_prev_soft;

    float *m_vignette;
    bool m_initialized;

    unsigned int m_width;
    unsigned int m_height;

    void updateVignette()
    {
        std::cout << "Updating " << m_width << "x" << m_height << " vignette now.";
        std::cout << "New settings: scaleY = " << m_scaleY << ", clear center = " << m_cc << ", soft = " << m_soft << std::endl;
        m_prev_scaleY = m_scaleY;
        m_prev_cc = m_cc;
        m_prev_soft = m_soft;

        float scaleY = 2*m_scaleY;

        int cx = m_width/2;
        int cy = m_height/2;
        float rmax = std::sqrt(std::pow(cx, 2) + std::pow(cy, 2));
        float r;

        for (int y = 0; y < m_height; y++) {
            for (int x = 0; x < m_width; x++) {
//                std::cout << "\tv " << x << "/" << y;
//                std::cout << ".";

                // Euclidian distance to the center, normalized to [0,1]
                r = std::sqrt(std::pow(x-cx, 2) + std::pow(scaleY*(y-cy), 2))/rmax;

                // Subtract the clear center
                r -= m_cc;

                if (r <= 0) {
                    // Clear center: Do not modify the brightness here
                    m_vignette[y*m_width+x] = 1;
                } else {
                    r *= m_soft;
                    if (r > M_PI_2) {
                        m_vignette[y*m_width+x] = 0;
                    } else {
                        m_vignette[y*m_width+x] = std::pow(std::cos(r), 4);
                    }
                }

            }
        }

        std::cout << "\nVignette updated.\n";

    }

};



frei0r::construct<Vignette> plugin("Vignette",
                "Vignetting simulation",
                "Simon A. Eugster (Granjow)",
                0,1,
                F0R_COLOR_MODEL_RGBA8888);
