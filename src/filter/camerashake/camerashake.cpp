/* camerashake.cpp
 * Copyright (C) 2026 PakeMPC
 * This file is a Frei0r plugin.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
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

#include <math.h>
#include <stdlib.h>
#include <string.h>
#if defined(_MSC_VER)
#define _USE_MATH_DEFINES
#endif

extern "C" {
    #include <frei0r.h>

    struct CameraShakeInstance {
        double amp_x, amp_y, rotation, zoom, speed, opacity, blur;
        float bg_r, bg_g, bg_b;
        unsigned int width, height;
    };

int f0r_init(void) { return 1; }
void f0r_deinit(void) {}

    void f0r_get_plugin_info(f0r_plugin_info_t* info) {
        info->name = "Camera Shake Ultimate";
        info->author = "PakeMPC";
        info->plugin_type = F0R_PLUGIN_TYPE_FILTER;
        info->color_model = F0R_COLOR_MODEL_RGBA8888;
        info->frei0r_version = FREI0R_MAJOR_VERSION;
        info->major_version = 2; 
        info->minor_version = 0;
        info->num_params = 8;
        info->explanation = "Camera movement effect with rotation, opacity, and blur.";
    }

    void f0r_get_param_info(f0r_param_info_t *info, int param_index) {
      switch (param_index) {
        case 0:
          info->name = "amplitude_x";
          info->type = F0R_PARAM_DOUBLE;
          info->explanation = "Maximum horizontal shake amplitude";
          break;
        case 1:
          info->name = "amplitude_y";
          info->type = F0R_PARAM_DOUBLE;
          info->explanation = "Maximum vertical shake amplitude";
         break;
        case 2:
          info->name = "rotation";
          info->type = F0R_PARAM_DOUBLE;
          info->explanation = "Maximum rotation shake";
          break;
          case 3:
          info->name = "zoom";
          info->type = F0R_PARAM_DOUBLE;
          info->explanation = "Zoom factor to hide black borders";
         break;
        case 4:
          info->name = "speed";
          info->type = F0R_PARAM_DOUBLE;
          info->explanation = "Speed or frequency of the shake";
          break;
        case 5:
          info->name = "opacity";
          info->type = F0R_PARAM_DOUBLE;
          info->explanation = "Opacity of the effect";
          break;
        case 6:
          info->name = "blur";
          info->type = F0R_PARAM_DOUBLE;
          info->explanation = "Amount of motion blur";
          break;
        case 7:
          info->name = "bg_color";
          info->type = F0R_PARAM_COLOR;
          info->explanation = "Background color for exposed borders";
          break;
        }
      }

    f0r_instance_t f0r_construct(unsigned int width, unsigned int height) {
        CameraShakeInstance* inst = new CameraShakeInstance();
        inst->amp_x = 0.25;
        inst->amp_y = 0.25;
        inst->rotation = 0.0;
        inst->zoom = 0.2;
        inst->speed = 0.25;
        inst->opacity = 1.0;
        inst->blur = 0.0;
        inst->bg_r = 0.0f;
        inst->bg_g = 0.0f;
        inst->bg_b = 0.0f;
        inst->width = width;
        inst->height = height;
        return (f0r_instance_t)inst;
      }

    void f0r_destruct(f0r_instance_t instance) { delete (CameraShakeInstance*)instance; }

    void f0r_set_param_value(f0r_instance_t instance, f0r_param_t param, int param_index) {
        CameraShakeInstance* inst = (CameraShakeInstance*)instance;
        double val = *((double*)param);
        if (param_index == 0) inst->amp_x = val;
        else if (param_index == 1) inst->amp_y = val;
        else if (param_index == 2) inst->rotation = val;
        else if (param_index == 3) inst->zoom = val;
        else if (param_index == 4) inst->speed = val;
        else if (param_index == 5) inst->opacity = val;
        else if (param_index == 6) inst->blur = val;
        else if (param_index == 7) {
            f0r_param_color_t* color = (f0r_param_color_t*)param;
            inst->bg_r = color->r; inst->bg_g = color->g; inst->bg_b = color->b;
        }
    }


void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index) {
        CameraShakeInstance* inst = (CameraShakeInstance*)instance;
        if (param_index == 0) *((double*)param) = inst->amp_x;
        else if (param_index == 1) *((double*)param) = inst->amp_y;
        else if (param_index == 2) *((double*)param) = inst->rotation;
        else if (param_index == 3) *((double*)param) = inst->zoom;
        else if (param_index == 4) *((double*)param) = inst->speed;
        else if (param_index == 5) *((double*)param) = inst->opacity;
        else if (param_index == 6) *((double*)param) = inst->blur;
        else if (param_index == 7) {
            f0r_param_color_t* color = (f0r_param_color_t*)param;
            color->r = inst->bg_r; color->g = inst->bg_g; color->b = inst->bg_b;
        }
    }

    void f0r_update(f0r_instance_t instance, double time, const uint32_t* inframe, uint32_t* outframe) {
        CameraShakeInstance* inst = (CameraShakeInstance*)instance;
        int w = inst->width;
        int h = inst->height;

        double speed_factor = inst->speed * 100.0;

        // 1. Zoom (0.0-1.0) mapped to scale (1.0-5.0)
        double scale = 1.0 + (inst->zoom * 4.0);

        // 2. Shake X and Y (using the direct values up to 500px)
        double max_amp_x = inst->amp_x * 500.0;
        double max_amp_y = inst->amp_y * 500.0;
        double shake_x = sin(time * speed_factor) * cos(time * speed_factor * 0.73) * max_amp_x;
        double shake_y = sin(time * speed_factor * 0.89) * cos(time * speed_factor * 1.1) * max_amp_y;

        // 3. Rotation
        double max_angle = inst->rotation * (M_PI / 4.0);
        double theta = sin(time * speed_factor * 1.15) * max_angle;
        double cos_t = cos(-theta);
        double sin_t = sin(-theta);

        // 4. Blur and Opacity Parameters
        int blur_radius = (int)(inst->blur * 20.0);
        double alpha_mult = inst->opacity;

        double cx = w / 2.0;
        double cy = h / 2.0;

        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {

                // Inverse mapping of rotation and scale matrix
                double dx = (x - cx) / scale;
                double dy = (y - cy) / scale;

                double rx = dx * cos_t - dy * sin_t;
                double ry = dx * sin_t + dy * cos_t;

                int base_src_x = (int)(cx + rx + shake_x);
                int base_src_y = (int)(cy + ry + shake_y);

                uint8_t *out_ptr = (uint8_t *)&outframe[y * w + x];

                // Check limits
                if (base_src_x < 0 || base_src_x >= w || base_src_y < 0 || base_src_y >= h) {
                    out_ptr[0] = (uint8_t)(inst->bg_r * 255.0f);
                    out_ptr[1] = (uint8_t)(inst->bg_g * 255.0f);
                    out_ptr[2] = (uint8_t)(inst->bg_b * 255.0f);
                    out_ptr[3] = (uint8_t)(255 * alpha_mult);
                    continue;
                }

                // If there's NO blur, paint directly
                if (blur_radius == 0) {
                    uint8_t *in_ptr = (uint8_t *)&inframe[base_src_y * w + base_src_x];
                    out_ptr[0] = in_ptr[0];
                    out_ptr[1] = in_ptr[1];
                    out_ptr[2] = in_ptr[2];
                    out_ptr[3] = (uint8_t)(in_ptr[3] * alpha_mult);
                }
                // If there is blur, do a "Fast Cross-Blur"
                else {
                    int sum_r = 0, sum_g = 0, sum_b = 0, sum_a = 0;
                    int samples = 0;

                    // Horizontal and vertical sampling (cross shape for yield)
                    for (int i = -blur_radius; i <= blur_radius; i += 2) {
                        // Horizontal
                        int sx = base_src_x + i;
                        if (sx >= 0 && sx < w) {
                            uint8_t* p = (uint8_t*)&inframe[base_src_y * w + sx];
                            sum_r += p[0]; sum_g += p[1]; sum_b += p[2]; sum_a += p[3];
                            samples++;
                        }
                        // Vertical
                        int sy = base_src_y + i;
                        if (sy >= 0 && sy < h && i != 0) { // i!=0 to avoid adding the center twice
                            uint8_t* p = (uint8_t*)&inframe[sy * w + base_src_x];
                            sum_r += p[0]; sum_g += p[1]; sum_b += p[2]; sum_a += p[3];
                            samples++;
                        }
                    }

                    out_ptr[0] = sum_r / samples;
                    out_ptr[1] = sum_g / samples;
                    out_ptr[2] = sum_b / samples;
                    out_ptr[3] = (uint8_t)((sum_a / samples) * alpha_mult);
                }
            }
        }
    }
}
