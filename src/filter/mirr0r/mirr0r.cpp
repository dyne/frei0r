/*
 * Copyright (C) 2024 Johann JEG (johannjeg1057@gmail.com)
 *
 * This file is a Frei0r plugin.
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "frei0r.hpp"
#include "frei0r/math.h"
#define _USE_MATH_DEFINES
#include <math.h>

class Mirr0r : public frei0r::filter {

private:
    unsigned int width;
    unsigned int height;
    double x_offset;
    double y_offset;
    double zoom;
    double rotation;

public:

    Mirr0r(unsigned int width, unsigned int height){
        register_param(this->x_offset, "x_offset", "Horizontal offset for image positioning.");
        register_param(this->y_offset, "y_offset", "Vertical offset for image positioning.");
        register_param(this->zoom, "zoom", "Zoom level for image scaling.");
        register_param(this->rotation, "rotation", "Rotation angle in degrees.");

        this->width = width;
        this->height = height;
        this->x_offset = 0.5;
        this->y_offset = 0.5;
        this->zoom = 0.5;
        this->rotation = 0.5;
    }

    ~Mirr0r(){

    }

    virtual void update(double time, uint32_t* out, const uint32_t* in){
        const unsigned char* src = (unsigned char*)in;
        unsigned char* dst = (unsigned char*)out;

        int w = this->width;
        int h = this->height;

        if (w <= 0 || h <= 0) {
            return;
        }

        // Zoom limit range from -0.9 to 2.0
        float zoom = 1.0f + CLAMP(this->zoom, -0.9, 2.0);

        int x_offset_px = (int)(this->x_offset * w);
        int y_offset_px = (int)(this->y_offset * h);

        // Calculate the center of the destination image
        float center_x = (float)w / 2.0f;
        float center_y = (float)h / 2.0f;

        // Convert the rotation to radians
        float angle = (float)(this->rotation * M_PI / 180.0f);
        float cos_angle = cosf(angle);
        float sin_angle = sinf(angle);

        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
               // Convert the destination coordinates to source coordinates with zoom
                float src_x = (x - center_x) / zoom + center_x;
                float src_y = (y - center_y) / zoom + center_y;

                // Apply the rotation
                float rotated_x = cos_angle * (src_x - center_x) - sin_angle * (src_y - center_y) + center_x;
                float rotated_y = sin_angle * (src_x - center_x) + cos_angle * (src_y - center_y) + center_y;

                // Apply the offset
                rotated_x += x_offset_px;
                rotated_y += y_offset_px;

                int src_x_int = (int)rotated_x % (w * 2);
                int src_y_int = (int)rotated_y % (h * 2);

                // Reflect the image if coordinates are out of bounds
                // If less than the bound
                if (src_x_int < 0) {
                    src_x_int = 0 - src_x_int - 1;
                }
                if (src_y_int < 0) {
                    src_y_int = 0 - src_y_int - 1;
                }
                // If greater than the bounds
                if (src_x_int >= w) {
                    src_x_int = w - (src_x_int - w) - 1;
                }
                if (src_y_int >= h) {
                    src_y_int = h - (src_y_int - h) - 1;
                }

                // Copy the pixel from the source image to the destination buffer
                ((uint32_t*)dst)[y * w + x] = ((const uint32_t*)src)[src_y_int * w + src_x_int];
            }
        }
    }
};

frei0r::construct<Mirr0r> plugin(
    "Mirr0r", 
    "Repeats and flips the input image when it goes out of bounds, allowing for adjustable offset, zoom and rotation. A versatile tool for creative video effects.",
    "Johann JEG",
    1, 0,
    F0R_COLOR_MODEL_RGBA8888);
