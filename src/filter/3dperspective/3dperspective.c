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
#include <math.h>
#include <stdint.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Degree ranges exposed to hosts via the normalized [0,1] frei0r param API */
#define YAW_ROLL_MIN_DEG  -180.0
#define YAW_ROLL_MAX_DEG   180.0
#define PITCH_MIN_DEG      -90.0
#define PITCH_MAX_DEG       90.0
#define ZOOM_MIN_VAL         0.0   /* 0.0x scale (zoomed way out) */
#define ZOOM_MAX_VAL         4.0   /* 4.0x scale (zoomed way in) */

typedef struct {
    unsigned int width, height;
    /* yaw/pitch/roll are stored internally in DEGREES for the rotation
     * math in f0r_update(). They must be converted to/from the frei0r
     * normalized [0,1] range at the f0r_get/set_param_value() boundary. */
    double yaw, pitch, roll, perspective, zoom, pan_x, pan_y;
} Yaw3D;

/* ---- normalized [0,1] <-> degree helpers ------------------------------ */

static double norm_to_deg(double norm, double min_deg, double max_deg) {
    if (norm < 0.0) norm = 0.0;
    if (norm > 1.0) norm = 1.0;
    return min_deg + norm * (max_deg - min_deg);
}

static double deg_to_norm(double deg, double min_deg, double max_deg) {
    double norm = (deg - min_deg) / (max_deg - min_deg);
    if (norm < 0.0) norm = 0.0;
    if (norm > 1.0) norm = 1.0;
    return norm;
}

static double yaw_roll_norm_to_deg(double norm) { return norm_to_deg(norm, YAW_ROLL_MIN_DEG, YAW_ROLL_MAX_DEG); }
static double yaw_roll_deg_to_norm(double deg)  { return deg_to_norm(deg, YAW_ROLL_MIN_DEG, YAW_ROLL_MAX_DEG); }
static double pitch_norm_to_deg(double norm)    { return norm_to_deg(norm, PITCH_MIN_DEG, PITCH_MAX_DEG); }
static double pitch_deg_to_norm(double deg)     { return deg_to_norm(deg, PITCH_MIN_DEG, PITCH_MAX_DEG); }

/* norm_to_deg()/deg_to_norm() are just linear range scalers despite the
 * "deg" name, so they're reused here for the zoom scale factor too. */
static double zoom_norm_to_val(double norm) { return norm_to_deg(norm, ZOOM_MIN_VAL, ZOOM_MAX_VAL); }
static double zoom_val_to_norm(double val)  { return deg_to_norm(val, ZOOM_MIN_VAL, ZOOM_MAX_VAL); }

/* ------------------------------------------------------------------------ */

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
        case 0: info->name = "yaw";         info->type = F0R_PARAM_DOUBLE; info->explanation = "Rotation around vertical axis (left/right turn). Normalized 0..1 maps to -180..180 degrees (0.5 = 0 deg)"; break;
        case 1: info->name = "pitch";       info->type = F0R_PARAM_DOUBLE; info->explanation = "Rotation around horizontal axis (tilt up/down). Normalized 0..1 maps to -90..90 degrees (0.5 = 0 deg)"; break;
        case 2: info->name = "roll";        info->type = F0R_PARAM_DOUBLE; info->explanation = "In-plane spin. Normalized 0..1 maps to -180..180 degrees (0.5 = 0 deg)"; break;
        case 3: info->name = "perspective"; info->type = F0R_PARAM_DOUBLE; info->explanation = "Strength of 3D perspective distortion (%)"; break;
        case 4: info->name = "zoom";        info->type = F0R_PARAM_DOUBLE; info->explanation = "Zoom compensation. Normalized 0..1 maps to 0.1x..4.0x scale"; break;
        case 5: info->name = "x";           info->type = F0R_PARAM_DOUBLE; info->explanation = "Horizontal position, 0..1 (0.5 = centered, no shift)"; break;
        case 6: info->name = "y";           info->type = F0R_PARAM_DOUBLE; info->explanation = "Vertical position, 0..1 (0.5 = centered, no shift)"; break;
    }
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height) {
    Yaw3D* inst = (Yaw3D*)calloc(1, sizeof(Yaw3D));
    if (!inst) return NULL;  /* let the host see construction failure */
    inst->width = width;
    inst->height = height;
    inst->yaw = 0.0;    /* degrees; normalized 0.5 */
    inst->pitch = 0.0;  /* degrees; normalized 0.5 */
    inst->roll = 0.0;   /* degrees; normalized 0.5 */
    inst->perspective = 0.7;
    inst->zoom = 1.0;  /* raw scale factor; normalized via ZOOM_MIN_VAL..ZOOM_MAX_VAL */
    inst->pan_x = 0.5;
    inst->pan_y = 0.5;
    return (f0r_instance_t)inst;
}

void f0r_destruct(f0r_instance_t instance) {
    free(instance);
}

void f0r_set_param_value(f0r_instance_t instance, f0r_param_t param, int idx) {
    Yaw3D* inst = (Yaw3D*)instance;
    double v = *((double*)param);  /* host always passes normalized [0,1] */
    switch (idx) {
        case 0: inst->yaw   = yaw_roll_norm_to_deg(v); break;
        case 1: inst->pitch = pitch_norm_to_deg(v);    break;
        case 2: inst->roll  = yaw_roll_norm_to_deg(v); break;
        case 3: inst->perspective = v; break;
        case 4: inst->zoom = zoom_norm_to_val(v); break;
        case 5: inst->pan_x = v; break;
        case 6: inst->pan_y = v; break;
    }
}

void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int idx) {
    Yaw3D* inst = (Yaw3D*)instance;
    double* v = (double*)param;  /* must return normalized [0,1] */
    switch (idx) {
        case 0: *v = yaw_roll_deg_to_norm(inst->yaw);   break;
        case 1: *v = pitch_deg_to_norm(inst->pitch);    break;
        case 2: *v = yaw_roll_deg_to_norm(inst->roll);  break;
        case 3: *v = inst->perspective; break;
        case 4: *v = zoom_val_to_norm(inst->zoom); break;
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

/* Bilinearly blend one 8-bit colour channel across the 2x2 pixel
 * neighbourhood (p00,p10 top row; p01,p11 bottom row) around a sample
 * point. `shift` picks which byte of the packed BGRA8888 pixel to read
 * (24=A, 16=R, 8=G, 0=B). fx/fy are the sample point's fractional
 * offset (each 0..1) between the neighbourhood's left/right and
 * top/bottom pixels. This is the standard four-tap bilinear filter:
 * weight each corner by its area on the opposite side of the sample
 * point, so the four weights (1-fx)(1-fy), fx(1-fy), (1-fx)fy, fx*fy
 * always sum to 1. */
static double bilinear_channel(uint32_t p00, uint32_t p10, uint32_t p01, uint32_t p11,
                                int shift, double fx, double fy) {
    double c00 = (p00 >> shift) & 0xFF;
    double c10 = (p10 >> shift) & 0xFF;
    double c01 = (p01 >> shift) & 0xFF;
    double c11 = (p11 >> shift) & 0xFF;
    return c00 * (1 - fx) * (1 - fy) +
           c10 * fx       * (1 - fy) +
           c01 * (1 - fx) * fy       +
           c11 * fx       * fy;
}

/* Sample `frame` at the (possibly fractional, possibly out-of-bounds)
 * pixel coordinate (sx, sy) using bilinear filtering, clamping to the
 * frame edges so callers don't need to bounds-check first. */
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

    double a = bilinear_channel(p00, p10, p01, p11, 24, fx, fy);
    double r = bilinear_channel(p00, p10, p01, p11, 16, fx, fy);
    double g = bilinear_channel(p00, p10, p01, p11, 8,  fx, fy);
    double b = bilinear_channel(p00, p10, p01, p11, 0,  fx, fy);

    return ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

static void fill_transparent(uint32_t* outframe, unsigned int w, unsigned int h) {
    for (unsigned int i = 0; i < w * h; i++) outframe[i] = 0x00000000;
}

/* Derive the projective (homography) mapping that sends the unit
 * square's corners u,v in {0,1}x{0,1} -- (0,0),(1,0),(1,1),(0,1), in
 * that order -- to the four screen-space corners (qx[0..3], qy[0..3]),
 * i.e. the perspective-projected quad computed earlier in f0r_update().
 *
 * A general quad-to-quad map isn't affine: it needs two extra
 * projective terms g, gh (the coefficients of u and v in the shared
 * denominator) alongside the familiar affine terms a,b,c,d,e,f. This
 * is the standard closed-form solution (as in Heckbert's classic
 * "Fundamentals of Texture Mapping and Image Warping", 1989): first
 * solve a 2x2 system for g/gh from how far the quad deviates from a
 * parallelogram, then back out a..f from that.
 *
 * When qx[0]-qx[1]+qx[2]-qx[3] and the corresponding y term are both
 * ~0, the quad IS a parallelogram (yaw/pitch/roll all put the plane
 * exactly fronto-parallel, or math happens to cancel out) and the
 * mapping degenerates to a plain affine one (g = gh = 0), which is
 * cheaper and numerically nicer, so that case is handled separately.
 *
 * The caller later inverts this forward (u,v)->(screen) mapping per
 * output pixel to get the corresponding (u,v), and from there the
 * source sample coordinate. */
static void compute_quad_homography(const double qx[4], const double qy[4],
                                     double* a, double* b, double* c,
                                     double* d, double* e, double* f,
                                     double* g, double* gh) {
    double dx1 = qx[1] - qx[2], dx2 = qx[3] - qx[2], dx3 = qx[0] - qx[1] + qx[2] - qx[3];
    double dy1 = qy[1] - qy[2], dy2 = qy[3] - qy[2], dy3 = qy[0] - qy[1] + qy[2] - qy[3];

    if (fabs(dx3) < 1e-9 && fabs(dy3) < 1e-9) {
        /* Parallelogram case: no keystoning, purely affine. */
        *a = qx[1] - qx[0]; *b = qx[3] - qx[0]; *c = qx[0];
        *d = qy[1] - qy[0]; *e = qy[3] - qy[0]; *f = qy[0];
        *g = 0; *gh = 0;
    } else {
        double denom = dx1 * dy2 - dx2 * dy1;
        if (fabs(denom) < 1e-9) denom = 1e-9;
        *g = (dx3 * dy2 - dx2 * dy3) / denom;
        *gh = (dx1 * dy3 - dx3 * dy1) / denom;
        *a = qx[1] - qx[0] + (*g) * qx[1];
        *b = qx[3] - qx[0] + (*gh) * qx[3];
        *c = qx[0];
        *d = qy[1] - qy[0] + (*g) * qy[1];
        *e = qy[3] - qy[0] + (*gh) * qy[3];
        *f = qy[0];
    }
}

void f0r_update(f0r_instance_t instance, double time, const uint32_t* inframe, uint32_t* outframe) {
    Yaw3D* inst = (Yaw3D*)instance;
    unsigned int w = inst->width, h = inst->height;
    double hw = w / 2.0, hh = h / 2.0;

    /* inst->yaw/pitch/roll are already in degrees (converted in
     * f0r_set_param_value from the normalized host value) */
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
    int degenerate = 0;
    /* A corner whose rotated z lands within this distance of -D sits on
     * (or effectively on) the camera's focal plane; D / (z + D) would
     * blow up toward +/-infinity there. The threshold scales with D
     * itself (D ranges roughly 0.35x..4.85x the frame's max dimension
     * depending on the perspective param) rather than using one fixed
     * absolute epsilon, so the guard stays equally tight whether D is
     * small (strong perspective) or large (flat, near-orthographic). A
     * tiny absolute floor covers the pathological case where D itself
     * is ~0 (e.g. a 0x0 frame). */
    const double Z_PLUS_D_REL_EPS = 1e-6;
    const double z_plus_d_eps = fmax(D * Z_PLUS_D_REL_EPS, 1e-9);
    for (int i = 0; i < 4; i++) {
        double x = local[i][0], y = local[i][1], z = local[i][2];
        rotate_point(&x, &y, &z, yaw, pitch, roll);

        double denom_z = z + D;
        if (fabs(denom_z) < z_plus_d_eps) {
            degenerate = 1;
            break;
        }

        double scale = (D / denom_z) * zoom;
        qx[i] = hw + x * scale + offset_x;
        qy[i] = hh + y * scale + offset_y;

        if (!isfinite(qx[i]) || !isfinite(qy[i])) {
            degenerate = 1;
            break;
        }
    }

    if (degenerate) {
        /* Reject the whole degenerate quad rather than risk propagating
         * inf/nan into the homography solve below; render nothing. */
        fill_transparent(outframe, w, h);
        return;
    }

    double a, b, c, d, e, f, g, gh;
    compute_quad_homography(qx, qy, &a, &b, &c, &d, &e, &f, &g, &gh);

    /* For each output pixel, invert the forward (u,v) -> (screen X,Y)
     * homography to recover the (u,v) that landed here, so this is a
     * backward/gather-style warp (one source lookup per destination
     * pixel) rather than forward/scatter -- it can't leave holes. A1/B1/C1
     * and A2/B2/C2 below are just the two linear equations
     * X = (a*u + b*v + c) / (g*u + gh*v + 1) and the equivalent for Y,
     * rearranged to solve the 2x2 system for u and v via Cramer's rule. */
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
                if (isfinite(u) && isfinite(v) &&
                    u >= 0.0 && u <= 1.0 && v >= 0.0 && v <= 1.0) {
                    double sx = u * w - 0.5;
                    double sy = v * h - 0.5;
                    if (isfinite(sx) && isfinite(sy)) {
                        outpx = sample_bilinear(inframe, w, h, sx, sy);
                    }
                }
            }
            outframe[py * w + px] = outpx;
        }
    }
}
