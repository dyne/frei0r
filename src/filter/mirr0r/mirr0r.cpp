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
#include <cairo.h>
#define _USE_MATH_DEFINES
#include <cmath>

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
        this->x_offset = 0.0;
        this->y_offset = 0.0;
        this->zoom = 0.5;
        this->rotation = 0.0;
    }

    ~Mirr0r(){

    }

    void clearScreen(cairo_t* cr, int width, int height) {
        cairo_save (cr);
        cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.0);
        cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
        cairo_paint (cr);
        cairo_restore (cr);
    }

    virtual void update(double time, uint32_t* out, const uint32_t* in) {
        
        int w = this->width;
        int h = this->height;

        // Calculate the stride for the image surface based on width and format
        int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, w);
    
        // Create a Cairo surface for the destination image
        cairo_surface_t* dest_image = cairo_image_surface_create_for_data((unsigned char*)out,
                                                                        CAIRO_FORMAT_ARGB32,
                                                                        w,
                                                                        h,
                                                                        stride);
        
        // Create a Cairo drawing context for the destination surface
        cairo_t *cr = cairo_create(dest_image);

        // Clear the screen
        clearScreen(cr, w, h);

        // Create a Cairo surface for the source image
        cairo_surface_t *image = cairo_image_surface_create_for_data((unsigned char*)in,
                                                                    CAIRO_FORMAT_ARGB32,
                                                                    w,
                                                                    h,
                                                                    stride);
        // Create a pattern from the source image surface
        cairo_pattern_t *pattern = cairo_pattern_create_for_surface(image);
        // Set the pattern extend mode to reflect (mirror) when the pattern is out of bounds
        cairo_pattern_set_extend (pattern, CAIRO_EXTEND_REFLECT);

        // Initialize the transformation matrix
        cairo_matrix_t matrix;
        cairo_matrix_init_identity(&matrix);

        // Calculate the center coordinates of the destination image
        float center_x = (float)w / 2.0f;
        float center_y = (float)h / 2.0f;
        
        float scale_factor = 1.5f - this->zoom;
        
        // Apply transformations 
        cairo_matrix_translate(&matrix, center_x - (this->x_offset) * w, center_y - (this->y_offset) * h);
        cairo_matrix_scale(&matrix, scale_factor, scale_factor);
        cairo_matrix_rotate(&matrix, this->rotation * M_PI);
        cairo_matrix_translate(&matrix, -center_x, -center_y);

        // Set the transformation matrix for the pattern
        cairo_pattern_set_matrix(pattern, &matrix);
        // Set the source pattern to be used for drawing
        cairo_set_source(cr, pattern);
        
        // Draw a rectangle covering the entire image area
        cairo_rectangle(cr, 0, 0, w, h);
        // Fill the rectangle with the source pattern
        cairo_fill(cr);

        // Clean up resources
        cairo_pattern_destroy (pattern);
        cairo_surface_destroy (image);
        cairo_surface_destroy (dest_image);
        cairo_destroy (cr);
    }
};

frei0r::construct<Mirr0r> plugin(
    "Mirr0r", 
    "Repeats and flips the input image when it goes out of bounds, allowing for adjustable offset, zoom and rotation. A versatile tool for creative video effects.",
    "Johann JEG",
    1, 0,
    F0R_COLOR_MODEL_RGBA8888);
    