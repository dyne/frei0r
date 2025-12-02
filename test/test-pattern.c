/* This file is part of frei0r (https://frei0r.dyne.org)
 *
 * Copyright (C) 2024-2025 Dyne.org foundation
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "test-pattern.h"
#include <math.h>
#include <stdio.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static void set_pixel(uint32_t* frame, int width, int height, int x, int y, 
                     uint8_t r, uint8_t g, uint8_t b, uint8_t a, int color_model) {
    if (x >= 0 && x < width && y >= 0 && y < height) {
        if (color_model == 0) {
            frame[y * width + x] = (a << 24) | (r << 16) | (g << 8) | b;
        } else {
            frame[y * width + x] = (a << 24) | (b << 16) | (g << 8) | r;
        }
    }
}

static void draw_circle(uint32_t* frame, int width, int height, 
                       int cx, int cy, int radius,
                       uint8_t r, uint8_t g, uint8_t b, uint8_t a, int color_model) {
    int x_start = cx - radius > 0 ? cx - radius : 0;
    int x_end = cx + radius < width ? cx + radius : width - 1;
    int y_start = cy - radius > 0 ? cy - radius : 0;
    int y_end = cy + radius < height ? cy + radius : height - 1;
    
    for (int y = y_start; y <= y_end; y++) {
        for (int x = x_start; x <= x_end; x++) {
            int dx = x - cx;
            int dy = y - cy;
            if (dx * dx + dy * dy <= radius * radius) {
                set_pixel(frame, width, height, x, y, r, g, b, a, color_model);
            }
        }
    }
}

static void draw_filled_rect(uint32_t* frame, int width, int height,
                             int x1, int y1, int x2, int y2,
                             uint8_t r, uint8_t g, uint8_t b, uint8_t a, int color_model) {
    if (x1 > x2) { int tmp = x1; x1 = x2; x2 = tmp; }
    if (y1 > y2) { int tmp = y1; y1 = y2; y2 = tmp; }
    
    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 >= width) x2 = width - 1;
    if (y2 >= height) y2 = height - 1;
    
    for (int y = y1; y <= y2; y++) {
        for (int x = x1; x <= x2; x++) {
            set_pixel(frame, width, height, x, y, r, g, b, a, color_model);
        }
    }
}

static void draw_rotated_square(uint32_t* frame, int width, int height,
                                int cx, int cy, int size, double angle,
                                uint8_t r, uint8_t g, uint8_t b, uint8_t a, int color_model) {
    double cos_a = cos(angle);
    double sin_a = sin(angle);
    int half = size / 2;
    
    for (int dy = -half; dy <= half; dy++) {
        for (int dx = -half; dx <= half; dx++) {
            int rx = (int)(dx * cos_a - dy * sin_a);
            int ry = (int)(dx * sin_a + dy * cos_a);
            set_pixel(frame, width, height, cx + rx, cy + ry, r, g, b, a, color_model);
        }
    }
}

static void draw_digit(uint32_t* frame, int width, int height,
                      int x, int y, int digit, int scale,
                      uint8_t r, uint8_t g, uint8_t b, uint8_t a, int color_model) {
    // Simple 5x7 bitmap font for digits 0-9
    static const uint8_t font[10][7] = {
        {0x1F, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1F}, // 0
        {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E}, // 1
        {0x1F, 0x01, 0x01, 0x1F, 0x10, 0x10, 0x1F}, // 2
        {0x1F, 0x01, 0x01, 0x1F, 0x01, 0x01, 0x1F}, // 3
        {0x11, 0x11, 0x11, 0x1F, 0x01, 0x01, 0x01}, // 4
        {0x1F, 0x10, 0x10, 0x1F, 0x01, 0x01, 0x1F}, // 5
        {0x1F, 0x10, 0x10, 0x1F, 0x11, 0x11, 0x1F}, // 6
        {0x1F, 0x01, 0x01, 0x02, 0x04, 0x08, 0x10}, // 7
        {0x1F, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x1F}, // 8
        {0x1F, 0x11, 0x11, 0x1F, 0x01, 0x01, 0x1F}  // 9
    };
    
    if (digit < 0 || digit > 9) return;
    
    for (int row = 0; row < 7; row++) {
        uint8_t line = font[digit][row];
        for (int col = 0; col < 5; col++) {
            if (line & (1 << (4 - col))) {
                for (int sy = 0; sy < scale; sy++) {
                    for (int sx = 0; sx < scale; sx++) {
                        set_pixel(frame, width, height, 
                                x + col * scale + sx, 
                                y + row * scale + sy, 
                                r, g, b, a, color_model);
                    }
                }
            }
        }
    }
}

static void draw_number(uint32_t* frame, int width, int height,
                       int x, int y, int number, int scale,
                       uint8_t r, uint8_t g, uint8_t b, uint8_t a, int color_model) {
    if (number == 0) {
        draw_digit(frame, width, height, x, y, 0, scale, r, g, b, a, color_model);
        return;
    }
    
    // Count digits
    int temp = number;
    int digits = 0;
    while (temp > 0) {
        digits++;
        temp /= 10;
    }
    
    // Draw each digit
    temp = number;
    for (int i = digits - 1; i >= 0; i--) {
        int digit = temp % 10;
        draw_digit(frame, width, height, x + i * 6 * scale, y, digit, scale, r, g, b, a, color_model);
        temp /= 10;
    }
}

static void draw_zone_plate_patch(uint32_t* frame, int width, int height,
                                  int cx, int cy, int size, int frame_num, int color_model) {
    double phase = frame_num * 0.05;
    int half = size / 2;
    
    for (int dy = -half; dy < half; dy++) {
        for (int dx = -half; dx < half; dx++) {
            double dist = sqrt(dx * dx + dy * dy);
            double freq = dist * 0.3;
            double value = (sin(freq + phase) + 1.0) * 0.5;
            
            uint8_t intensity = (uint8_t)(value * 255);
            set_pixel(frame, width, height, cx + dx, cy + dy,
                     intensity, intensity, intensity, 255, color_model);
        }
    }
}

void generate_animated_test_pattern(uint32_t* frame, int width, int height, int frame_num, int color_model) {
    // 1. Animated radial gradient background
    int center_x = width / 2;
    int center_y = height / 2;
    double max_dist = sqrt(center_x * center_x + center_y * center_y);
    
    // Color cycling through the animation
    double color_phase = frame_num * 0.02;
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int dx = x - center_x;
            int dy = y - center_y;
            double dist = sqrt(dx * dx + dy * dy);
            double norm_dist = dist / max_dist;
            
            // Create a pulsing radial gradient with color shift
            double angle = atan2(dy, dx);
            double r_val = (sin(norm_dist * M_PI + color_phase) + 1.0) * 0.5;
            double g_val = (sin(norm_dist * M_PI + color_phase + M_PI * 2.0 / 3.0) + 1.0) * 0.5;
            double b_val = (sin(norm_dist * M_PI + color_phase + M_PI * 4.0 / 3.0) + 1.0) * 0.5;
            
            // Add some angular variation
            double angular_mod = (sin(angle * 3.0 + color_phase * 0.5) + 1.0) * 0.2;
            
            uint8_t r = (uint8_t)((r_val + angular_mod) * 127 + 32);
            uint8_t g = (uint8_t)((g_val + angular_mod) * 127 + 32);
            uint8_t b = (uint8_t)((b_val + angular_mod) * 127 + 32);
            
            if (color_model == 0) {
                frame[y * width + x] = (255 << 24) | (r << 16) | (g << 8) | b;
            } else {
                frame[y * width + x] = (255 << 24) | (b << 16) | (g << 8) | r;
            }
        }
    }
    
    // 2. Bouncing circle
    int circle_radius = 40;
    int circle_x = (int)(center_x + sin(frame_num * 0.05) * (center_x - circle_radius - 20));
    int circle_y = (int)(center_y + cos(frame_num * 0.037) * (center_y - circle_radius - 20));
    
    draw_circle(frame, width, height, circle_x, circle_y, circle_radius,
                255, 200, 0, 255, color_model);
    
    // 3. Rotating square
    int square_size = 60;
    int square_x = width / 4;
    int square_y = height / 4;
    double rotation = frame_num * 0.03;
    
    draw_rotated_square(frame, width, height, square_x, square_y, square_size, rotation,
                       0, 255, 200, 255, color_model);
    
    // 4. Horizontal moving bar
    int bar_height = 20;
    int bar_y = (int)(height * 0.75 + sin(frame_num * 0.04) * (height * 0.15));
    
    draw_filled_rect(frame, width, height, 0, bar_y - bar_height/2, 
                    width - 1, bar_y + bar_height/2,
                    255, 100, 200, 255, color_model);
    
    // 5. Zone plate patch in corner
    int zone_size = 80;
    draw_zone_plate_patch(frame, width, height, 
                         width - zone_size/2 - 10, 
                         zone_size/2 + 10, 
                         zone_size, frame_num, color_model);
    
    // 6. Frame counter
    draw_number(frame, width, height, 10, 10, frame_num, 2,
               255, 255, 255, 255, color_model);
}
