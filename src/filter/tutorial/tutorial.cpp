/*
 * Copyright (C) 2010-2011 Simon Andreas Eugster (simon.eu@gmail.com)
 * This file is not a Frei0r plugin but a collection of ideas.
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

// Other includes used for the examples below.
// Can be removed on copy/paste.

// Limits (min/max values) of various data types
#include <limits>
// For the CHAR_BIT constant
#include <climits>
// pow() and other mathematical functions
#include <cmath>

/**
This is a sample filter for easy copy/pasting.

The CMakeLists.txt needs to be adjusted as well (both in this directory and in the parent directory).
Also, don't forget src/Makefile.am.
*/

class Tutorial : public frei0r::filter
{

public:

    Tutorial(unsigned int width, unsigned int height)

    {
        // Everything here is example code and can be removed on copy/paste.


        register_param(m_barSize, "barSize", "Size of the black bar");
        register_param(m_pointerMethod, "pointerMethod", "Pointer Method (internal)");
        m_barSize = 0.1;



        // Create the lookup table (see update()). Use std::vector instead of an array here, since:
        // http://stackoverflow.com/questions/381621/using-arrays-or-stdvectors-in-c-whats-the-performance-gap
        // Using std::numeric_limits just for fun here.
        lookupTable = std::vector<uint8_t>(std::numeric_limits<uint8_t>::max()+1, 0);
        std::cout << lookupTable.size() << " elements in the lookup table." << std::endl;

        // Calculate the entries in the lookup table. Applied on the R value in this example.
        // If the R value of an input pixel is r, then the output R value will be mapped to lookupTable[r].
        // We'll use a calculation that looks expensive here.
        float f, h;
        int tempVal;
        for (int i = 0; i < lookupTable.size(); i++) {
            f = i / (float)std::numeric_limits<uint8_t>::max(); // Normalize to [0,1]
            h = f/5;
            f = 2*f - 1; // Stretch to [-1,1]
            f = pow(f, 2); // Parabola
            f = f*.8 + h; // Modification to the parabola

            // Since we might get negative values above, directly putting f into an unsigned data type
            // may lead to interesting effects. To avoid this, use a signed integer and clamp it to valid ranges.
            tempVal = f * std::numeric_limits<uint8_t>::max();
            if (tempVal < 0) {
                tempVal = 0;
            }
            if (tempVal > std::numeric_limits<uint8_t>::max()) {
                tempVal = std::numeric_limits<uint8_t>::max();
            }
            lookupTable[i] = tempVal;
        }


        // This is a second lookup table for addition of two uint8 numbers. The result of {0,255}+{0,255}
        // ranges in {0,511}. The usual way to clamp the values to the {0,255} range is to use if/else
        // (if (k > 255) { k = 255; }), however using a lookup table is slightly faster:
        // http://stackoverflow.com/questions/4783674/lookup-table-vs-if-else
        additionTable = std::vector<uint8_t>( 2*std::numeric_limits<uint8_t>::max() + 1, 0 );
        for (int i = 0; i < 2*std::numeric_limits<uint8_t>::max(); i++) {
            if (i <= std::numeric_limits<uint8_t>::max()) {
                additionTable[i] = i;
            } else {
                additionTable[i] = std::numeric_limits<uint8_t>::max();
            }
        }
    }

    ~Tutorial()
    {
        // Delete member variables if necessary.
    }

    virtual void update()
    {
        // Just copy input to output.
        // This is useful if ony few changes are made to the output.
        // If the whole image is processed, this makes no sense!
        std::copy(in, in + width*height, out);

        // Fill the given amount of the top part with black.
        std::fill(&out[0], &out[(int) (m_barSize*width*height)], 0);

        // Performance is important!
        // One way to improve the performance is to use a LOOKUP TABLE
        // instead of doing the same calculation several times. For example, the
        // SOP/Sat effect uses pow(), division, and multiplication for the Power parameter.
        // Since this only depends on the R/G/B value and not on previous frames, the target value
        // can be pre-computed; when applying the filter, only thing left to do is reading the value
        // in the lookup table.

        // This parameter allows to do simple benchmarking: Rendering a video with uint8_t pointers and with uint32_t pointers.
        // (Don't forget to substract the rendering time without this effect applied to avoid counting
        // encoding and decoding as well!)
        if (m_pointerMethod == 0) {
            uint8_t *in_pointer = (uint8_t *) in;
            uint8_t *out_pointer = (uint8_t *) out;
            for (int x = 0; x < width; x++) {
                for (int y = 0; y < height; y++) {

                    // Apply the parabola to the R channel with a single lookup
                    *out_pointer++ = lookupTable[*in_pointer++];

                    // Add g+b and clamp with the second lookup table
                    *out_pointer = additionTable[*in_pointer + *(in_pointer+1)];
                    out_pointer++;  in_pointer++;

                    // Copy the other channels
                    *out_pointer++ = *in_pointer++;
                    *out_pointer++ = *in_pointer++;
                }
            }
        } else {
            // This method takes only 80% of the time if only processing the R channel,
            // and 90% of the time with additionally the G channel,
            // compared to the above solution using uint8_t pointers.
            for (int px = 0; px < width*height; px++) {
                out[px] =
                            // Parabola to Red channel
                            lookupTable[(in[px] & 0xFF)]
                            // g+b to Green channel
                          | (additionTable[((in[px] & 0xFF00) >> CHAR_BIT) + ((in[px] & 0xFF0000) >> 2*CHAR_BIT)] << CHAR_BIT)
                            // copy blue and alpha
                          | (in[px] & 0xFFFF0000);
            }
        }
    }

private:
    // The various f0r_params are adjustable parameters.
    // This one determines the size of the black bar in this example.
    f0r_param_double m_barSize;
    f0r_param_bool m_pointerMethod;
    std::vector<uint8_t> lookupTable;
    std::vector<uint8_t> additionTable;

};



frei0r::construct<Tutorial> plugin("Tutorial filter",
                "This is an example filter, kind of a quick howto showing how to add a frei0r filter.",
                "Your Name",
                0,1,
                F0R_COLOR_MODEL_RGBA8888);
