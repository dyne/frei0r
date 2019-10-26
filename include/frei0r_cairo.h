/*
 * frei0r_cairo.h
 * Copyright 2012 Janne Liljeblad 
 *
 * This file is part of Frei0r.
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


#include <cairo.h>
#include <string.h>
#include "frei0r_math.h"

/**
* String identifiers for gradient types available using Cairo.
*/
#define GRADIENT_LINEAR "gradient_linear"
#define GRADIENT_RADIAL "gradient_radial"

/**
* String identifiers for blend modes available using Cairo.
*/
#define NORMAL        "normal"
#define ADD           "add"
#define SATURATE      "saturate"
#define MULTIPLY      "multiply"
#define SCREEN        "screen"
#define OVERLAY       "overlay"
#define DARKEN        "darken"
#define LIGHTEN       "lighten"
#define COLORDODGE    "colordodge"
#define COLORBURN     "colorburn"
#define HARDLIGHT     "hardlight"
#define SOFTLIGHT     "softlight"
#define DIFFERENCE    "difference"
#define EXCLUSION     "exclusion"
#define HSLHUE        "hslhue"
#define HSLSATURATION "hslsaturation"
#define HSLCOLOR      "hslcolor"
#define HSLLUMINOSITY "hslluminosity"


/**
* frei0r_cairo_set_operator
* @cr: Cairo context
* @op: String identifier for a blend mode
*
* Sets cairo context to use the defined blend mode for all paint operations.
*/
void frei0r_cairo_set_operator(cairo_t *cr, char *op)
{
  if(strcmp(op, NORMAL) == 0)
  {
    cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
  }
  else if(strcmp(op, ADD) == 0)
  {
    cairo_set_operator (cr, CAIRO_OPERATOR_ADD);
  }
  else if(strcmp(op, SATURATE) == 0)
  {
    cairo_set_operator (cr, CAIRO_OPERATOR_SATURATE);
  }
  else if(strcmp(op, MULTIPLY) == 0)
  {
    cairo_set_operator (cr, CAIRO_OPERATOR_MULTIPLY);
  }
  else if(strcmp(op, SCREEN) == 0)
  {
    cairo_set_operator (cr, CAIRO_OPERATOR_SCREEN);
  }
  else if(strcmp(op, OVERLAY) == 0)
  {
    cairo_set_operator (cr, CAIRO_OPERATOR_OVERLAY);
  }
  else if(strcmp(op, DARKEN) == 0)
  {
    cairo_set_operator (cr, CAIRO_OPERATOR_DARKEN);
  }
  else if(strcmp(op, LIGHTEN) == 0)
  {
    cairo_set_operator (cr, CAIRO_OPERATOR_LIGHTEN);
  }
  else if(strcmp(op, COLORDODGE) == 0)
  {
    cairo_set_operator (cr, CAIRO_OPERATOR_COLOR_DODGE);
  }
  else if(strcmp(op, COLORBURN) == 0)
  {
    cairo_set_operator (cr, CAIRO_OPERATOR_COLOR_BURN);
  }
  else if(strcmp(op, HARDLIGHT) == 0)
  {
    cairo_set_operator (cr, CAIRO_OPERATOR_HARD_LIGHT);
  }
  else if(strcmp(op, SOFTLIGHT) == 0)
  {
    cairo_set_operator (cr, CAIRO_OPERATOR_SOFT_LIGHT);
  }
  else if(strcmp(op, DIFFERENCE) == 0)
  {
    cairo_set_operator (cr, CAIRO_OPERATOR_DIFFERENCE);
  }
  else if(strcmp(op, EXCLUSION) == 0)
  {
    cairo_set_operator (cr, CAIRO_OPERATOR_EXCLUSION);
  }
  else if(strcmp(op, HSLHUE) == 0)
  {
    cairo_set_operator (cr, CAIRO_OPERATOR_HSL_HUE);
  }
  else if(strcmp(op, HSLSATURATION) == 0)
  {
    cairo_set_operator (cr, CAIRO_OPERATOR_HSL_SATURATION);
  }
  else if(strcmp(op, HSLCOLOR) == 0)
  {
    cairo_set_operator (cr, CAIRO_OPERATOR_HSL_COLOR);
  }
  else if(strcmp(op, HSLLUMINOSITY ) == 0)
  {
    cairo_set_operator (cr, CAIRO_OPERATOR_HSL_LUMINOSITY);
  }
  else
  {
    cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
  }
}


/**
* frei0r_cairo_set_rgba_LITTLE_ENDIAN
* @cr: Cairo context
* @red: red component, 0 - 1
* @green: green component, 0 - 1
* @blue: blue component, 0 - 1
* @alpha: opacity of color, 0 -1
*
* Sets cairo context to use the defined color paint operations.
* Switches red and blue channels to get correct color on little endian machines. 
* This method only works correctly on little endian machines.
*/
void frei0r_cairo_set_rgba_LITTLE_ENDIAN(cairo_t* cr, double red, double green, double blue, double alpha)
{
  cairo_set_source_rgba (cr, blue, green, red, alpha);
}

/**
* frei0r_cairo_set_rgb_LITTLE_ENDIAN
* @cr: Cairo context
* @red: red component, 0 - 1
* @green: green component, 0 - 1
* @blue: blue component, 0 - 1
*
* Sets cairo context to use the defined color paint operations.
* Switches red and blue channels to get correct color on little endian machines. 
* This method only works correctly on little endian machines.
*/
void frei0r_cairo_set_rgb_LITTLE_ENDIAN(cairo_t* cr, double red, double green, double blue)
{
  cairo_set_source_rgb (cr, blue, green, red);
}

/**
* freior_cairo_set_color_stop_rgba_LITTLE_ENDIAN(
* @pat: Cairo pattern
* @offset: offset of color position in pattern space, 0 - 1
* @red: red component, 0 - 1
* @green: green component, 0 - 1
* @blue: blue component, 0 - 1
* @alpha: opacity of color, 0 -1
*
* Sets color stop for cairo patterns.
* Switches red and blue channels to get correct color on little endian machines. 
* This method only works correctly on little endian machines.
*/
void freior_cairo_set_color_stop_rgba_LITTLE_ENDIAN(cairo_pattern_t *pat, double offset, 
                                                    double red, double green, double blue, double alpha)
{                               
  cairo_pattern_add_color_stop_rgba (pat, offset, blue, green, red, alpha);
}

/**
* frei0r_cairo_get_pixel_position
* @norm_pos: position in range 0 - 1, either x or y
* @dim: dimension, either witdh or height
*
* Converts double range [0 -> 1] to pixel range [-2*dim -> 3*dim]. Input 0.4 gives position 0.
*
* Returns: position in pixels
*/ 
double frei0r_cairo_get_pixel_position (double norm_pos, int dim)
{
  double pos_o = -(dim * 2.0);
  return pos_o + norm_pos * dim * 5.0;  
}

/**
* frei0r_cairo_get_scale
* @norm_scale: scale in range 0 - 1
*
* Converts double range [0 -> 1] to scale range [0 -> 5]. Input 0.2 gives scale 1.0. 
*
* Returns: scale
*/ 
double frei0r_cairo_get_scale (double norm_scale)
{
  return norm_scale * 5.0;
}

/**
 * Convert frei0r RGBA to pre-multiplied alpha as needed by Cairo.
 *
 * \param rgba the image buffer with format F0R_COLOR_MODEL_RGBA8888
 * \param pixels the size of the image buffer in number of pixels
 * \param alpha if >= 0, the alpha channel will be set to this value
 * \see frei0r_cairo_unpremultiply_rgba
 */
void frei0r_cairo_premultiply_rgba (unsigned char *rgba, int pixels, int alpha)
{
  int i = pixels + 1;
  while ( --i ) {
    register unsigned char a = rgba[3];
    if (a == 0) {
      *((uint32_t *)rgba) = 0;
    } else if (a < 0xff) {
      rgba[0] = ( rgba[0] * a ) >> 8;
      rgba[1] = ( rgba[1] * a ) >> 8;
      rgba[2] = ( rgba[2] * a ) >> 8;
    }
    if (alpha >= 0) rgba[3] = alpha;
    rgba += 4;
  }
}

/**
 * Convert Cairo ARGB pre-multiplied alpha to frei0r straight RGBA.
 *
 * \param rgba the image buffer with format CAIRO_FORMAT_ARGB32
 * \param pixels the size of the image buffer in number of pixels
 * \see frei0r_cairo_premultiply_rgba
 */
void frei0r_cairo_unpremultiply_rgba (unsigned char *rgba, int pixels)
{
  int i = pixels + 1;
  while ( --i ) {
    register unsigned char a = rgba[3];
    if (a > 0 && a < 0xff) {
      rgba[0] = MIN(( rgba[0] << 8 ) / a, 255);
      rgba[1] = MIN(( rgba[1] << 8 ) / a, 255);
      rgba[2] = MIN(( rgba[2] << 8 ) / a, 255);
    }
    rgba += 4;
  }
}

/**
 * Convert frei0r RGBA to pre-multiplied alpha as needed by Cairo.
 *
 * \param rgba the image buffer with format F0R_COLOR_MODEL_RGBA8888
 * \param pixels the size of the image buffer in number of pixels
 * \param alpha if >= 0, the alpha channel will be set to this value
 * \see frei0r_cairo_premultiply_rgba
 *
 * This is the same as frei0r_cairo_premultiply_rgba but it writes the
 * output to a different buffer.
 */
void frei0r_cairo_premultiply_rgba2 (unsigned char *in, unsigned char *out,
                                     int pixels, int alpha)
{
  int i = pixels + 1;
  while ( --i ) {
    register unsigned char a = in[3];
    if (a == 0) {
      *((uint32_t *)out) = 0;
    } else if (a == 0xff) {
      memcpy(out, in, 4);
    } else {
      out[0] = ( in[0] * a ) >> 8;
      out[1] = ( in[1] * a ) >> 8;
      out[2] = ( in[2] * a ) >> 8;
    }
    if (alpha >= 0)
        out[3] = alpha;
    in += 4;
    out += 4;
  }
}
