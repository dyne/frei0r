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
#include <cairo.h>
#define _USE_MATH_DEFINES
#include <math.h>

class ShakeOscillate : public frei0r::filter {

private:

    unsigned int width;
    unsigned int height;
    double movement_amount_x;
    double movement_speed_x;
    double movement_amount_y;
    double movement_speed_y;
    double rotation_amount;
    double rotation_speed;
    double scale;
    double phase;
    bool mirrored;

    const float MOVEMENT_AMOUNT_MULTIPLIER = 0.3f;
    const float ROTATION_AMOUNT_MULTIPLIER = 0.2f;
    const float SPEED_MULTIPLIER = 10.0f;

public:

    ShakeOscillate(unsigned int width, unsigned int height) { 
        register_param(this->movement_amount_x, "movement_amount_x", "Amount of the X movement.");
        register_param(this->movement_speed_x, "movement_speed_x", "X Movement Speed.");
        register_param(this->movement_amount_y, "movement_amount_y", "Amount of the Y movement.");
        register_param(this->movement_speed_y, "movement_speed_y", "Y Movement Speed.");
        register_param(this->rotation_amount, "rotation_amount", "Rotation amount.");
        register_param(this->rotation_speed, "rotation_speed", "Rotation Speed.");
        register_param(this->scale, "scale", "Scale level.");
        register_param(this->phase, "phase", "The phase of the sin and cos functions of this effect.");
        register_param(this->mirrored, "mirrored", "On/Off Image Mirror Extend");

        this->width = width;
        this->height = height;
        this->movement_amount_x = 0.5; // Movement amount off is 0.5
        this->movement_speed_x = 0.2;
        this->movement_amount_y = 0.5; // Movement amount off is 0.5
        this->movement_speed_y = 0.1;
        this->rotation_amount = 0.5; // Rotation amount off is 0.5
        this->rotation_speed = 0.2;
        this->scale = 0.5; // Original image size is 0.5
        this->phase = 0.0;
        this->mirrored = true;
    }

    ~ShakeOscillate() {

    }

    void clearScreen(cairo_t* cr, int width, int height) {
        cairo_save (cr);
        cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.0);
        cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
        cairo_paint (cr);
        cairo_restore (cr);
    }

    void oscillate(cairo_matrix_t* matrix, double time) {
        float x = 0.0f;
        float y = 0.0f; 
        
        // Operations for the translation amount do not depend on the screen size
        float movement_range_x = MOVEMENT_AMOUNT_MULTIPLIER * this->height;
        float movement_range_y = MOVEMENT_AMOUNT_MULTIPLIER * this->width;

        // Translate X
        if(this->movement_amount_x != 0.5){
            float movement_x = (this->movement_amount_x - 0.5f) * 2.0f;
            float movement_speed_value_x = this->movement_speed_x * SPEED_MULTIPLIER * time;
            x = movement_x * movement_range_x * std::sin(movement_speed_value_x + this->phase);
        }

        // Translate Y
        if(this->movement_amount_y != 0.5){
            float movement_y = (this->movement_amount_y - 0.5f) * 2.0f;
            float movement_speed_value_y = this->movement_speed_y * SPEED_MULTIPLIER * time;
            y = movement_y * movement_range_y * std::cos(movement_speed_value_y + this->phase);
        }

        // Translate by adding the half screen coordinates to scale and rotate from center
        float center_x = this->width / 2.0f;
        float center_y = this->height / 2.0f;
        cairo_matrix_translate(matrix, center_x + x, center_y + y);
        
        // Scale
        if(this->scale != 0.5){
            float scale_factor = 1.5f - this->scale;
            cairo_matrix_scale(matrix, scale_factor, scale_factor);
        }
        
        // Rotation
        if(this->rotation_amount != 0.5){
            float rotation_speed_value = (this->rotation_speed * SPEED_MULTIPLIER * time) + this->phase;
            float rotation = ((this->rotation_amount - 0.5f) * ROTATION_AMOUNT_MULTIPLIER) * std::sin(rotation_speed_value) * M_PI;
            cairo_matrix_rotate(matrix, rotation);
        }
        
        // Restore translate coordinates
        cairo_matrix_translate(matrix, -center_x, -center_y);
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

        if(this->mirrored){
            // Set the pattern extend mode to reflect (mirror) when the pattern is out of bounds
            cairo_pattern_set_extend (pattern, CAIRO_EXTEND_REFLECT);
        } else {
            // Set the pattern extend mode to none (mirroring does not apply)
            cairo_pattern_set_extend (pattern, CAIRO_EXTEND_NONE);   
        }
        
        // Initialize the transformation matrix
        cairo_matrix_t matrix;
        cairo_matrix_init_identity(&matrix);

        // Call the function that applies the movement
        oscillate(&matrix, time);

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

frei0r::construct<ShakeOscillate> plugin(
    "ShakeOscillate", 
    "Animate the input image with adjustable parameters such as amount, speed, rotation, scale and option to mirror the image if it goes outside the screen bounds.",
    "Johann JEG",
    1, 0,
    F0R_COLOR_MODEL_RGBA8888);
