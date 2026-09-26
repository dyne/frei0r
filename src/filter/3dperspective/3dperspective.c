/* 3dperspective.c
 * Copyright (C) 2026 Harveyes
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

#include <frei0r.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct {
    unsigned int width, height;
    double yaw, pitch, roll, perspective, zoom, pan_x, pan_y;
} Yaw3D;

int f0r_init() { return 1; }
void f0r_deinit() { }

void f0r_get_plugin_info(f0r_plugin_info_t* info) {
    info->name = "3D Perspective";
    info->author = "Harveyes";
    info->plugin_type = F0R_PLUGIN_TYPE_FILTER;
    info->color_model = F0R_COLOR_MODEL_BGRA8888;
    info->frei0r_version = FREI0R_MAJOR_VERSION;
    info->major_version = 0;
    info->minor_version = 3;
    info->num_params = 7;
    info->explanation = "Simulated 3D plane rotation (yaw/pitch/roll) with adjustable perspective and position, like a fake-3D camera transform.";
}

void f0r_get_param_info(f0r_param_info_t* info, int idx) {
    switch (idx) {
        case 0: info->name = "yaw";         info->type = F0R_PARAM_DOUBLE; info->explanation = "Rotation around vertical axis (left/right turn), degrees"; break;
        case 1: info->name = "pitch";       info->type = F0R_PARAM_DOUBLE; info->explanation = "Rotation around horizontal axis (tilt up/down), degrees"; break;
        case 2: info->name = "roll";        info->type = F0R_PARAM_DOUBLE; info->explanation = "In-plane spin, degrees"; break;
        case 3: info->name = "perspective"; info->type = F0R_PARAM_DOUBLE; info->explanation = "Strength of 3D perspective distortion (%)"; break;
        case 4: info->name = "zoom";        info->type = F0R_PARAM_DOUBLE; info->explanation = "Zoom compensation (%)"; break;
        case 5: info->name = "x";           info->type = F0R_PARAM_DOUBLE; info->explanation = "Horizontal position, 0..1 (0.5 = centered, no shift)"; break;
        case 6: info->name = "y";           info->type = F0R_PARAM_DOUBLE; info->explanation = "Vertical position, 0..1 (0.5 = centered, no shift)"; break;
    }
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height) {
    Yaw3D* inst = (Yaw3D*)calloc(1, sizeof(Yaw3D));
    inst->width = width;
    inst->height = height;
    inst->yaw = 0.0;
    inst->pitch = 0.0;
    inst->roll = 0.0;
    inst->perspective = 0.7;
    inst->zoom = 1.0;
    inst->pan_x = 0.5;
    inst->pan_y = 0.5;
    return (f0r_instance_t)inst;
}

void f0r_destruct(f0r_instance_t instance) {
    free(instance);
}

void f0r_set_param_value(f0r_instance_t instance, f0r_param_t param, int idx) {
    Yaw3D* inst = (Yaw3D*)instance;
    double v = *((double*)param);
    switch (idx) {
        case 0: inst->yaw = v; break;
        case 1: inst->pitch = v; break;
        case 2: inst->roll = v; break;
        case 3: inst->perspective = v; break;
        case 4: inst->zoom = v; break;
        case 5: inst->pan_x = v; break;
        case 6: inst->pan_y = v; break;
    }
}

void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int idx) {
    Yaw3D* inst = (Yaw3D*)instance;
    double* v = (double*)param;
    switch (idx) {
        case 0: *v = inst->yaw; break;
        case 1: *v = inst->pitch; break;
        case 2: *v = inst->roll; break;
        case 3: *v = inst->perspective; break;
        case 4: *v = inst->zoom; break;
        case 5: *v = inst->pan_x; break;
        case 6: *v = inst->pan_y; break;
    }
}

static void rotate_point(double* x, double* y, double* z, double yaw, double pitch, double roll) {
    double cx = *x, cz = *z;
    *x = cx * cos(yaw) + cz * sin(yaw);
    *z = -cx * sin(yaw) + cz * cos(yaw);

    double cy = *y; cz = *z;
    *y = cy * cos(pitch) - cz * sin(pitch);
    *z = cy * sin(pitch) + cz * cos(pitch);

    cx = *x; cy = *y;
    *x = cx * cos(roll) - cy * sin(roll);
    *y = cx * sin(roll) + cy * cos(roll);
}

static uint32_t sample_bilinear(const uint32_t* frame, unsigned int w, unsigned int h, double sx, double sy) {
    if (sx < 0) sx = 0;
    if (sy < 0) sy = 0;
    if (sx > w - 1.001) sx = w - 1.001;
    if (sy > h - 1.001) sy = h - 1.001;

    int x0 = (int)sx, y0 = (int)sy;
    int x1 = x0 + 1, y1 = y0 + 1;
    if (x1 >= (int)w) x1 = w - 1;
    if (y1 >= (int)h) y1 = h - 1;

    double fx = sx - x0, fy = sy - y0;

    uint32_t p00 = frame[y0 * w + x0];
    uint32_t p10 = frame[y0 * w + x1];
    uint32_t p01 = frame[y1 * w + x0];
    uint32_t p11 = frame[y1 * w + x1];

    double a = ((p00>>24)&0xFF)*(1-fx)*(1-fy) + ((p10>>24)&0xFF)*fx*(1-fy) + ((p01>>24)&0xFF)*(1-fx)*fy + ((p11>>24)&0xFF)*fx*fy;
    double r = ((p00>>16)&0xFF)*(1-fx)*(1-fy) + ((p10>>16)&0xFF)*fx*(1-fy) + ((p01>>16)&0xFF)*(1-fx)*fy + ((p11>>16)&0xFF)*fx*fy;
    double g = ((p00>>8)&0xFF)*(1-fx)*(1-fy)  + ((p10>>8)&0xFF)*fx*(1-fy)  + ((p01>>8)&0xFF)*(1-fx)*fy  + ((p11>>8)&0xFF)*fx*fy;
    double b = (p00&0xFF)*(1-fx)*(1-fy)       + (p10&0xFF)*fx*(1-fy)       + (p01&0xFF)*(1-fx)*fy       + (p11&0xFF)*fx*fy;

    return ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

void f0r_update(f0r_instance_t instance, double time, const uint32_t* inframe, uint32_t* outframe) {
    Yaw3D* inst = (Yaw3D*)instance;
    unsigned int w = inst->width, h = inst->height;
    double hw = w / 2.0, hh = h / 2.0;

    double yaw = inst->yaw * M_PI / 180.0;
    double pitch = inst->pitch * M_PI / 180.0;
    double roll = inst->roll * M_PI / 180.0;
    double persp_norm = inst->perspective;
    double zoom = inst->zoom;

    double maxdim = (w > h ? w : h);
    double D = maxdim * (0.35 + (1.0 - persp_norm) * 4.5);

    /* pan_x/pan_y are 0..1 with 0.5 = centered; map to a pixel offset
     * spanning a full frame width/height either side of center so the
     * plane can be pushed anywhere from fully off-screen to fully across */
    double offset_x = (inst->pan_x - 0.5) * w;
    double offset_y = (inst->pan_y - 0.5) * h;

    double local[4][3] = {
        {-hw, -hh, 0},
        { hw, -hh, 0},
        { hw,  hh, 0},
        {-hw,  hh, 0}
    };

    double qx[4], qy[4];
    for (int i = 0; i < 4; i++) {
        double x = local[i][0], y = local[i][1], z = local[i][2];
        rotate_point(&x, &y, &z, yaw, pitch, roll);
        double scale = (D / (z + D)) * zoom;
        qx[i] = hw + x * scale + offset_x;
        qy[i] = hh + y * scale + offset_y;
    }

    double dx1 = qx[1] - qx[2], dx2 = qx[3] - qx[2], dx3 = qx[0] - qx[1] + qx[2] - qx[3];
    double dy1 = qy[1] - qy[2], dy2 = qy[3] - qy[2], dy3 = qy[0] - qy[1] + qy[2] - qy[3];

    double a, b, c, d, e, f, g, gh;
    if (fabs(dx3) < 1e-9 && fabs(dy3) < 1e-9) {
        a = qx[1] - qx[0]; b = qx[3] - qx[0]; c = qx[0];
        d = qy[1] - qy[0]; e = qy[3] - qy[0]; f = qy[0];
        g = 0; gh = 0;
    } else {
        double denom = dx1 * dy2 - dx2 * dy1;
        if (fabs(denom) < 1e-9) denom = 1e-9;
        g = (dx3 * dy2 - dx2 * dy3) / denom;
        gh = (dx1 * dy3 - dx3 * dy1) / denom;
        a = qx[1] - qx[0] + g * qx[1];
        b = qx[3] - qx[0] + gh * qx[3];
        c = qx[0];
        d = qy[1] - qy[0] + g * qy[1];
        e = qy[3] - qy[0] + gh * qy[3];
        f = qy[0];
    }

    for (unsigned int py = 0; py < h; py++) {
        for (unsigned int px = 0; px < w; px++) {
            double X = px + 0.5, Y = py + 0.5;

            double A1 = a - X * g, B1 = b - X * gh, C1 = X - c;
            double A2 = d - Y * g, B2 = e - Y * gh, C2 = Y - f;

            double det = A1 * B2 - B1 * A2;
            uint32_t outpx = 0x00000000;

            if (fabs(det) > 1e-9) {
                double u = (C1 * B2 - B1 * C2) / det;
                double v = (A1 * C2 - C1 * A2) / det;
                if (u >= 0.0 && u <= 1.0 && v >= 0.0 && v <= 1.0) {
                    double sx = u * w - 0.5;
                    double sy = v * h - 0.5;
                    outpx = sample_bilinear(inframe, w, h, sx, sy);
                }
            }
            outframe[py * w + px] = outpx;
        }
    }
}
