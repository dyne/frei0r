/*
 * flarefx.c - a frei0r port of OpenShot's LensFlare effect
 *
 * OpenShot's LensFlare.cpp is itself based on the classic FlareFX plug-in
 * for GIMP 0.99 (Karl-Johan Andersson, 1997-1998; modified by Tim
 * Copperfield, 2000). This file re-implements the same algorithm - the
 * fixed table of 19 "reflectors", the ring falloff formulas, and the
 * HSV-shifted tinting - directly on raw RGBA8888 pixel buffers, since
 * frei0r has no Qt/QImage/QPainter available. Visual behavior should match
 * the original closely; exact byte-for-byte output will differ slightly
 * because Qt's QPainter::CompositionMode_Plus (used by the original for
 * the final composite) has its own internal premultiplication behavior
 * that isn't reproduced bit-for-bit here - this uses a direct, explicit
 * additive composite instead (see the comment above the final blend).
 *
 * Original GIMP FlareFX copyright (C) 1997-1998 Karl-Johan Andersson
 * OpenShot LensFlare Copyright (c) 2008-2025 OpenShot Studios, LLC
 * This file: GNU GPL v2 (or any later version), same as the original.
 *
 * Color model: RGBA8888 (4 bytes per pixel: R,G,B,A)
 */

#include <frei0r.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ---- parameter indices ------------------------------------------------ */
enum {
  P_POS_X = 0,     /* F0R_PARAM_DOUBLE : 0..1 -> light source X position */
  P_POS_Y,         /* F0R_PARAM_DOUBLE : 0..1 -> light source Y position */
  P_BRIGHTNESS,    /* F0R_PARAM_DOUBLE : 0..1 -> overlay opacity/intensity */
  P_SIZE,          /* F0R_PARAM_DOUBLE : 0..1 -> maps to original 0.1..3.0 scale */
  P_SPREAD,        /* F0R_PARAM_DOUBLE : 0..1 -> how far reflectors travel through center */
  P_COLOR,         /* F0R_PARAM_COLOR  : tint */
  P_NUM_PARAMS
};

typedef struct {
  unsigned int width;
  unsigned int height;

  double pos_x, pos_y;              /* 0..1 */
  double brightness;                /* 0..1 */
  double size;                      /* 0..1 -> 0.1..3.0 */
  double spread;                    /* 0..1 */
  double color_r, color_g, color_b; /* 0..1 each */
} flarefx_instance_t;

/* ---- small helpers ------------------------------------------------------ */

static inline double clampd(double v, double lo, double hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

static inline int clampi(int v, int lo, int hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

/* RGB (0..1) -> HSV (h,s,v all 0..1); achromatic gives h=0 */
static void rgb_to_hsv(double r, double g, double b, double *h, double *s, double *v) {
  double max = r > g ? (r > b ? r : b) : (g > b ? g : b);
  double min = r < g ? (r < b ? r : b) : (g < b ? g : b);
  double delta = max - min;

  *v = max;
  *s = (max <= 0.0) ? 0.0 : delta / max;

  if (delta <= 0.0) {
    *h = 0.0;
    return;
  }

  double hue;
  if (r >= max) hue = fmod((g - b) / delta, 6.0);
  else if (g >= max) hue = (b - r) / delta + 2.0;
  else hue = (r - g) / delta + 4.0;

  hue /= 6.0;
  if (hue < 0.0) hue += 1.0;
  *h = hue;
}

/* HSV (0..1 each) -> RGB (0..1 each) */
static void hsv_to_rgb(double h, double s, double v, double *r, double *g, double *b) {
  if (s <= 0.0) {
    *r = *g = *b = v;
    return;
  }
  double hh = fmod(h, 1.0) * 6.0;
  int i = (int)floor(hh);
  double f = hh - i;
  double p = v * (1.0 - s);
  double q = v * (1.0 - s * f);
  double t = v * (1.0 - s * (1.0 - f));

  switch (i % 6) {
    case 0: *r = v; *g = t; *b = p; break;
    case 1: *r = q; *g = v; *b = p; break;
    case 2: *r = p; *g = v; *b = t; break;
    case 3: *r = p; *g = q; *b = v; break;
    case 4: *r = t; *g = p; *b = v; break;
    default: *r = v; *g = p; *b = q; break;
  }
}

/* additively blend color c (0..1 RGBA) onto integer 0..255 pixel channels
 * with strength p (0..1) - matches the original's blendAdd() */
static void blend_add(int *dr, int *dg, int *db, int *da,
                       double cr, double cg, double cb, double ca, double p) {
  *dr = clampi(*dr + (int)((255 - *dr) * p * cr), 0, 255);
  *dg = clampi(*dg + (int)((255 - *dg) * p * cg), 0, 255);
  *db = clampi(*db + (int)((255 - *db) * p * cb), 0, 255);
  *da = clampi(*da + (int)((255 - *da) * p * ca), 0, 255);
}

/* base reflector color, hue-shifted/saturation+value-scaled by the tint
 * (rather than simply multiplied) - matches the original's shifted_hsv() */
static void shifted_hsv(double br, double bg, double bb,
                         double h_shift, double s_scale, double v_scale, double a_scale,
                         double *outr, double *outg, double *outb, double *outa) {
  double h, s, v;
  rgb_to_hsv(br, bg, bb, &h, &s, &v);
  if (s == 0.0) h = 0.0;
  h = fmod(h + h_shift + 1.0, 1.0);
  s = clampd(s * s_scale, 0.0, 1.0);
  v = clampd(v * v_scale, 0.0, 1.0);
  *outa = clampd(1.0 * a_scale, 0.0, 1.0);
  hsv_to_rgb(h, s, v, outr, outg, outb);
}

/* ---- reflector table (verbatim from the GIMP FlareFX / OpenShot source) ---- */

typedef struct {
  int type;                 /* 1..4, controls the falloff shape */
  double xp, yp, size;       /* pixel-space center + radius */
  double r, g, b, a;         /* 0..1, already tint-adjusted */
} reflect_t;

typedef struct {
  int type;
  double fx, fy, fsize, r, g, b;
} reflect_def_t;

static const reflect_def_t REFLECTOR_DEFS[] = {
  {1,  0.6699,  0.6699, 0.027,   0.0/255.0,  14.0/255.0, 113.0/255.0},
  {1,  0.2692,  0.2692, 0.010,  90.0/255.0, 181.0/255.0, 142.0/255.0},
  {1, -0.0112, -0.0112, 0.005,  56.0/255.0, 140.0/255.0, 106.0/255.0},
  {2,  0.6490,  0.6490, 0.031,   9.0/255.0,  29.0/255.0,  19.0/255.0},
  {2,  0.4696,  0.4696, 0.015,  24.0/255.0,  14.0/255.0,   0.0/255.0},
  {2,  0.4087,  0.4087, 0.037,  24.0/255.0,  14.0/255.0,   0.0/255.0},
  {2, -0.2003, -0.2003, 0.022,  42.0/255.0,  19.0/255.0,   0.0/255.0},
  {2, -0.4103, -0.4103, 0.025,   0.0/255.0,   9.0/255.0,  17.0/255.0},
  {2, -0.4503, -0.4503, 0.058,  10.0/255.0,   4.0/255.0,   0.0/255.0},
  {2, -0.5112, -0.5112, 0.017,   5.0/255.0,   5.0/255.0,  14.0/255.0},
  {2, -1.4960, -1.4960, 0.20,    9.0/255.0,   4.0/255.0,   0.0/255.0},
  {2, -1.4960, -1.4960, 0.50,    9.0/255.0,   4.0/255.0,   0.0/255.0},
  {3,  0.4487,  0.4487, 0.075,  34.0/255.0,  19.0/255.0,   0.0/255.0},
  {3,  1.0000,  1.0000, 0.10,   14.0/255.0,  26.0/255.0,   0.0/255.0},
  {3, -1.3010, -1.3010, 0.039,  10.0/255.0,  25.0/255.0,  13.0/255.0},
  {4,  1.3090,  1.3090, 0.19,    9.0/255.0,   0.0/255.0,  17.0/255.0},
  {4,  1.3090,  1.3090, 0.195,   9.0/255.0,  16.0/255.0,   5.0/255.0},
  {4,  1.3090,  1.3090, 0.20,   17.0/255.0,   4.0/255.0,   0.0/255.0},
  {4, -1.3010, -1.3010, 0.038,  17.0/255.0,   4.0/255.0,   0.0/255.0},
};
#define NUM_REFLECTORS (sizeof(REFLECTOR_DEFS) / sizeof(REFLECTOR_DEFS[0]))

static void init_reflectors(reflect_t *refs, double DX, double DY,
                             int width, int height,
                             double tint_r, double tint_g, double tint_b,
                             double S) {
  double halfW = width * 0.5;
  double halfH = height * 0.5;
  double matt = width;

  double tint_h, tint_s, tint_v;
  rgb_to_hsv(tint_r, tint_g, tint_b, &tint_h, &tint_s, &tint_v);
  int white_tint = (tint_s < 0.01);

  for (size_t i = 0; i < NUM_REFLECTORS; i++) {
    const reflect_def_t *d = &REFLECTOR_DEFS[i];
    reflect_t *r = &refs[i];
    r->type = d->type;
    r->size = d->fsize * matt * S;
    r->xp = halfW + d->fx * DX;
    r->yp = halfH + d->fy * DY;

    if (white_tint) {
      r->r = d->r; r->g = d->g; r->b = d->b; r->a = 1.0;
    } else {
      shifted_hsv(d->r, d->g, d->b, tint_h, tint_s, tint_v, 1.0,
                  &r->r, &r->g, &r->b, &r->a);
    }
  }
}

/* apply one reflector's contribution to an (r,g,b,a) accumulator (0..255 ints) */
static void apply_reflector(int *pr, int *pg, int *pb, int *pa,
                             const reflect_t *r, int cx, int cy) {
  double dx = r->xp - cx, dy = r->yp - cy;
  double d = sqrt(dx * dx + dy * dy);
  double p;

  switch (r->type) {
    case 1:
      p = (r->size - d) / r->size;
      if (p > 0.0) {
        p *= p;
        blend_add(pr, pg, pb, pa, r->r, r->g, r->b, r->a, p);
      }
      break;
    case 2:
      p = (r->size - d) / (r->size * 0.15);
      if (p > 0.0) {
        if (p > 1.0) p = 1.0;
        blend_add(pr, pg, pb, pa, r->r, r->g, r->b, r->a, p);
      }
      break;
    case 3:
      p = (r->size - d) / (r->size * 0.12);
      if (p > 0.0) {
        if (p > 1.0) p = 1.0;
        p = 1.0 - (p * 0.12);
        blend_add(pr, pg, pb, pa, r->r, r->g, r->b, r->a, p);
      }
      break;
    case 4:
      p = fabs((d - r->size) / (r->size * 0.04));
      if (p < 1.0) {
        blend_add(pr, pg, pb, pa, r->r, r->g, r->b, r->a, 1.0 - p);
      }
      break;
  }
}

/* ---- frei0r plugin info ------------------------------------------------- */

int f0r_init(void) { return 1; }
void f0r_deinit(void) { }

void f0r_get_plugin_info(f0r_plugin_info_t* info) {
  info->name = "FlareFX";
  info->author = "Ported from OpenShot's LensFlare / GIMP FlareFX";
  info->plugin_type = F0R_PLUGIN_TYPE_FILTER;
  info->color_model = F0R_COLOR_MODEL_RGBA8888;
  info->frei0r_version = FREI0R_MAJOR_VERSION;
  info->major_version = 1;
  info->minor_version = 0;
  info->num_params = P_NUM_PARAMS;
  info->explanation = "Simulates sunlight hitting a camera lens: a bright core, "
                       "colored rings, and a scattering of small reflectors that "
                       "travel through the frame center as the light source moves, "
                       "based on the classic GIMP FlareFX algorithm";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index) {
  switch (param_index) {
    case P_POS_X:
      info->name = "X";
      info->type = F0R_PARAM_DOUBLE;
      info->explanation = "Horizontal position of the light source (0=left, 1=right)";
      break;
    case P_POS_Y:
      info->name = "Y";
      info->type = F0R_PARAM_DOUBLE;
      info->explanation = "Vertical position of the light source (0=top, 1=bottom)";
      break;
    case P_BRIGHTNESS:
      info->name = "Brightness";
      info->type = F0R_PARAM_DOUBLE;
      info->explanation = "Overall opacity/intensity of the flare overlay";
      break;
    case P_SIZE:
      info->name = "Size";
      info->type = F0R_PARAM_DOUBLE;
      info->explanation = "Size of the flare; maps to the original effect's 0.1..3.0 scale range";
      break;
    case P_SPREAD:
      info->name = "Spread";
      info->type = F0R_PARAM_DOUBLE;
      info->explanation = "How far the small reflectors travel through the frame center as the light moves off-center";
      break;
    case P_COLOR:
      info->name = "Color";
      info->type = F0R_PARAM_COLOR;
      info->explanation = "Tint color of the flare";
      break;
  }
}

/* ---- construct / destruct ----------------------------------------------- */

f0r_instance_t f0r_construct(unsigned int width, unsigned int height) {
  flarefx_instance_t* inst = (flarefx_instance_t*)calloc(1, sizeof(flarefx_instance_t));
  inst->width = width;
  inst->height = height;

  /* matches the original's defaults: x=-0.5,y=-0.5 in its own -1..1 scheme
   * is roughly upper-left-of-center; here expressed directly in 0..1 
   * brightness adjusted to 0.603 and spread to 0.788 for personal preference */
  inst->pos_x = 0.25;
  inst->pos_y = 0.25;
  inst->brightness = 0.603;
  inst->size = (1.0 - 0.1) / 2.9;   /* normalized param that maps back to original S=1.0 */
  inst->spread = 0.788;
  inst->color_r = 1.0;
  inst->color_g = 1.0;
  inst->color_b = 1.0;

  return (f0r_instance_t)inst;
}

void f0r_destruct(f0r_instance_t instance) {
  free(instance);
}

/* ---- parameter get/set ---------------------------------------------------- */

void f0r_set_param_value(f0r_instance_t instance, f0r_param_t param, int param_index) {
  flarefx_instance_t* inst = (flarefx_instance_t*)instance;

  switch (param_index) {
    case P_POS_X:
      inst->pos_x = *((double*)param);
      break;
    case P_POS_Y:
      inst->pos_y = *((double*)param);
      break;
    case P_BRIGHTNESS:
      inst->brightness = *((double*)param);
      break;
    case P_SIZE:
      inst->size = *((double*)param);
      break;
    case P_SPREAD:
      inst->spread = *((double*)param);
      break;
    case P_COLOR: {
      f0r_param_color_t* c = (f0r_param_color_t*)param;
      inst->color_r = c->r;
      inst->color_g = c->g;
      inst->color_b = c->b;
      break;
    }
  }
}

void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index) {
  flarefx_instance_t* inst = (flarefx_instance_t*)instance;

  switch (param_index) {
    case P_POS_X:
      *((double*)param) = inst->pos_x;
      break;
    case P_POS_Y:
      *((double*)param) = inst->pos_y;
      break;
    case P_BRIGHTNESS:
      *((double*)param) = inst->brightness;
      break;
    case P_SIZE:
      *((double*)param) = inst->size;
      break;
    case P_SPREAD:
      *((double*)param) = inst->spread;
      break;
    case P_COLOR: {
      f0r_param_color_t* c = (f0r_param_color_t*)param;
      c->r = inst->color_r;
      c->g = inst->color_g;
      c->b = inst->color_b;
      break;
    }
  }
}

/* ---- the actual effect ---------------------------------------------------- */

void f0r_update(f0r_instance_t instance, double time,
                 const uint32_t* inframe, uint32_t* outframe) {
  (void) time;
  flarefx_instance_t* inst = (flarefx_instance_t*)instance;
  int w = (int)inst->width;
  int h = (int)inst->height;

  const unsigned char* src = (const unsigned char*)inframe;
  unsigned char* dst = (unsigned char*)outframe;
  memcpy(dst, src, (size_t)w * h * 4);

  double X = clampd(inst->pos_x, 0.0, 1.0);
  double Y = clampd(inst->pos_y, 0.0, 1.0);
  double I = clampd(inst->brightness, 0.0, 1.0);
  double S = 0.1 + clampd(inst->size, 0.0, 1.0) * 2.9;   /* -> original 0.1..3.0 */
  double SP = clampd(inst->spread, 0.0, 1.0);
  double tint_r = clampd(inst->color_r, 0.0, 1.0);
  double tint_g = clampd(inst->color_g, 0.0, 1.0);
  double tint_b = clampd(inst->color_b, 0.0, 1.0);

  double halfW = w * 0.5, halfH = h * 0.5;
  double px = X * w;
  double py = Y * h;
  double DX = (halfW - px) * SP;
  double DY = (halfH - py) * SP;

  double matt = w;
  double scolor = matt * 0.0375 * S;
  double sglow  = matt * 0.078125 * S;
  double sinner = matt * 0.1796875 * S;
  double souter = matt * 0.3359375 * S;
  double shalo  = matt * 0.084375 * S;

  /* base ring hues, tinted by direct RGB multiply (matches the original's
   * "tintify" lambda - a simpler multiply, as opposed to the reflectors'
   * HSV-shift tinting below) */
  double c_color_r = (239.0/255.0) * tint_r, c_color_g = (239.0/255.0) * tint_g, c_color_b = (239.0/255.0) * tint_b;
  double c_glow_r  = (245.0/255.0) * tint_r, c_glow_g  = (245.0/255.0) * tint_g, c_glow_b  = (245.0/255.0) * tint_b;
  double c_inner_r = 1.0           * tint_r, c_inner_g = (38.0/255.0)  * tint_g, c_inner_b = (43.0/255.0)  * tint_b;
  double c_outer_r = (69.0/255.0)  * tint_r, c_outer_g = (59.0/255.0)  * tint_g, c_outer_b = (64.0/255.0)  * tint_b;
  double c_halo_r  = (80.0/255.0)  * tint_r, c_halo_g  = (15.0/255.0)  * tint_g, c_halo_b  = (4.0/255.0)   * tint_b;

  reflect_t refs[NUM_REFLECTORS];
  init_reflectors(refs, DX, DY, w, h, tint_r, tint_g, tint_b, S);

  for (int yy = 0; yy < h; yy++) {
    for (int xx = 0; xx < w; xx++) {
      int r = 0, g = 0, b = 0, a = 0;
      double dx = xx - px, dy = yy - py;
      double d = sqrt(dx * dx + dy * dy);

      if (d < scolor) {
        double p = (scolor - d) / scolor; p *= p;
        blend_add(&r, &g, &b, &a, c_color_r, c_color_g, c_color_b, 1.0, p);
      }
      if (d < sglow) {
        double p = (sglow - d) / sglow; p *= p;
        blend_add(&r, &g, &b, &a, c_glow_r, c_glow_g, c_glow_b, 1.0, p);
      }
      if (d < sinner) {
        double p = (sinner - d) / sinner; p *= p;
        blend_add(&r, &g, &b, &a, c_inner_r, c_inner_g, c_inner_b, 1.0, p);
      }
      if (d < souter) {
        double p = (souter - d) / souter; /* linear, no squaring - matches original */
        blend_add(&r, &g, &b, &a, c_outer_r, c_outer_g, c_outer_b, 1.0, p);
      }
      {
        double p = fabs((d - shalo) / (shalo * 0.07));
        if (p < 1.0) {
          blend_add(&r, &g, &b, &a, c_halo_r, c_halo_g, c_halo_b, 1.0, 1.0 - p);
        }
      }
      for (size_t i = 0; i < NUM_REFLECTORS; i++) {
        apply_reflector(&r, &g, &b, &a, &refs[i], xx, yy);
      }

      /* overlay alpha = coverage estimate, matching the original's
       * "force alpha = max(R,G,B)" */
      int overlay_a = r; if (g > overlay_a) overlay_a = g; if (b > overlay_a) overlay_a = b;

      /* Explicit additive composite onto the destination pixel, scaled by
       * brightness I. The original relies on QPainter's
       * CompositionMode_Plus + setOpacity(I) for this step; here it's done
       * directly since frei0r has no compositor to lean on. Alpha is
       * rebuilt as max(original, overlay*I), exactly as the original does. */
      size_t idx = ((size_t)yy * w + xx) * 4;
      int dst_r = dst[idx + 0], dst_g = dst[idx + 1], dst_b = dst[idx + 2], dst_a = dst[idx + 3];

      dst[idx + 0] = (unsigned char)clampi(dst_r + (int)(r * I), 0, 255);
      dst[idx + 1] = (unsigned char)clampi(dst_g + (int)(g * I), 0, 255);
      dst[idx + 2] = (unsigned char)clampi(dst_b + (int)(b * I), 0, 255);

      double oa = dst_a / 255.0;
      double fa = (overlay_a / 255.0) * I;
      double na = oa > fa ? oa : fa;
      dst[idx + 3] = (unsigned char)clampi((int)(na * 255.0 + 0.5), 0, 255);
    }
  }
}
