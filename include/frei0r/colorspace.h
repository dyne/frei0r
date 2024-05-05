/* frei0r/colorspace.h
 * Copyright (C) 2004 Mathieu Guindon, Julien Keable, Jean-Sebastien Senecal
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

#ifndef INCLUDED_FREI0R_COLORSPACE_H
#define INCLUDED_FREI0R_COLORSPACE_H

#include "frei0r/math.h"
#include <stdlib.h>
#include <math.h>

// # Basic colorspace convert functions (from the Gimp gimpcolorspace.h) ####

/*  int functions  */

/**
 * rgb_to_hsv_int
 * @red: The red channel value, returns the Hue channel
 * @green: The green channel value, returns the Saturation channel
 * @blue: The blue channel value, returns the Value channel
 *
 * The arguments are pointers to int representing channel values in
 * the RGB colorspace, and the values pointed to are all in the range
 * [0, 255].
 *
 * The function changes the arguments to point to the HSV value
 * corresponding, with the returned values in the following
 * ranges: H [0, 360], S [0, 255], V [0, 255].
 **/
inline void
rgb_to_hsv_int (int *red         /* returns hue        */,
                int *green       /* returns saturation */,
                int *blue        /* returns value      */)
{
  double  r, g, b;
  double  h, s, v;
  double  min;
  double  delta;

  r = *red;
  g = *green;
  b = *blue;

  if (r > g)
  {
    v = MAX (r, b);
    min = MIN (g, b);
  }
  else
  {
    v = MAX (g, b);
    min = MIN (r, b);
  }

  delta = v - min;

  if (v == 0.0)
    s = 0.0;
  else
    s = delta / v;

  if (s == 0.0)
    h = 0.0;
  else
  {
    if (r == v)
      h = 60.0 * (g - b) / delta;
    else if (g == v)
      h = 120 + 60.0 * (b - r) / delta;
    else
      h = 240 + 60.0 * (r - g) / delta;

    if (h < 0.0)
      h += 360.0;
    if (h > 360.0)
      h -= 360.0;
  }

  *red   = ROUND (h);
  *green = ROUND (s * 255.0);
  *blue  = ROUND (v);  
}

/**
 * hsv_to_rgb_int
 * @hue: The hue channel, returns the red channel
 * @saturation: The saturation channel, returns the green channel
 * @value: The value channel, returns the blue channel
 *
 * The arguments are pointers to int, with the values pointed to in the
 * following ranges:  H [0, 360], S [0, 255], V [0, 255].
 *
 * The function changes the arguments to point to the RGB value
 * corresponding, with the returned values all in the range [0, 255].
 **/
inline void
hsv_to_rgb_int (int *hue         /* returns red        */,
                int *saturation  /* returns green      */,
                int *value       /* returns blue       */)
{
  double h, s, v, h_temp;
  double f, p, q, t;
  int i;

  if (*saturation == 0)
  {
    *hue        = *value;
    *saturation = *value;
    //    *value      = *value;
  }
  else
  {
    h = *hue;
    s = *saturation / 255.0;
    v = *value      / 255.0;

    if (h == 360)
      h_temp = 0;
    else
      h_temp = h;

    h_temp = h_temp / 60.0;
    i = (int) floor (h_temp);
    f = h_temp - i;
    p = v * (1.0 - s);
    q = v * (1.0 - (s * f));
    t = v * (1.0 - (s * (1.0 - f)));

    switch (i)
    {
    case 0:
      *hue        = ROUND (v * 255.0);
      *saturation = ROUND (t * 255.0);
      *value      = ROUND (p * 255.0);
      break;

    case 1:
      *hue        = ROUND (q * 255.0);
      *saturation = ROUND (v * 255.0);
      *value      = ROUND (p * 255.0);
      break;

    case 2:
      *hue        = ROUND (p * 255.0);
      *saturation = ROUND (v * 255.0);
      *value      = ROUND (t * 255.0);
      break;

    case 3:
      *hue        = ROUND (p * 255.0);
      *saturation = ROUND (q * 255.0);
      *value      = ROUND (v * 255.0);
      break;

    case 4:
      *hue        = ROUND (t * 255.0);
      *saturation = ROUND (p * 255.0);
      *value      = ROUND (v * 255.0);
      break;

    case 5:
      *hue        = ROUND (v * 255.0);
      *saturation = ROUND (p * 255.0);
      *value      = ROUND (q * 255.0);
      break;
    }
  }
}

/**
 * rgb_to_hsl_int
 * @red: Red channel, returns Hue channel
 * @green: Green channel, returns Lightness channel
 * @blue: Blue channel, returns Saturation channel
 *
 * The arguments are pointers to int representing channel values in the
 * RGB colorspace, and the values pointed to are all in the range [0, 255].
 *
 * The function changes the arguments to point to the corresponding HLS
 * value with the values pointed to in the following ranges:  H [0, 360],
 * L [0, 255], S [0, 255].
 **/
inline void
rgb_to_hsl_int (unsigned int *red         /* returns red        */,
                unsigned int *green       /* returns green      */,
                unsigned int *blue        /* returns blue       */)
{
  unsigned int r, g, b;
  double h, s, l;
  unsigned int    min, max;
  unsigned int    delta;

  r = *red;
  g = *green;
  b = *blue;

  if (r > g)
  {
    max = MAX (r, b);
    min = MIN (g, b);
  }
  else
  {
    max = MAX (g, b);
    min = MIN (r, b);
  }

  l = (max + min) / 2.0;

  if (max == min)
  {
    s = 0.0;
    h = 0.0;
  }
  else
  {
    delta = (max - min);

    if (l < 128)
      s = 255 * (double) delta / (double) (max + min);
    else
      s = 255 * (double) delta / (double) (511 - max - min);

    if (r == max)
      h = (g - b) / (double) delta;
    else if (g == max)
      h = 2 + (b - r) / (double) delta;
    else
      h = 4 + (r - g) / (double) delta;

    h = h * 42.5;

    if (h < 0)
      h += 255;
    else if (h > 255)
      h -= 255;
  }

  *red   = ROUND (h);
  *green = ROUND (s);
  *blue  = ROUND (l);
}

inline int
hsl_value_int (double n1,
               double n2,
               double hue)
{
  double value;

  if (hue > 255)
    hue -= 255;
  else if (hue < 0)
    hue += 255;

  if (hue < 42.5)
    value = n1 + (n2 - n1) * (hue / 42.5);
  else if (hue < 127.5)
    value = n2;
  else if (hue < 170)
    value = n1 + (n2 - n1) * ((170 - hue) / 42.5);
  else
    value = n1;

  return ROUND (value * 255.0);
}

/**
 * hsl_to_rgb_int
 * @hue: Hue channel, returns Red channel
 * @saturation: Saturation channel, returns Green channel
 * @lightness: Lightness channel, returns Blue channel
 *
 * The arguments are pointers to int, with the values pointed to in the
 * following ranges:  H [0, 360], L [0, 255], S [0, 255].
 *
 * The function changes the arguments to point to the RGB value
 * corresponding, with the returned values all in the range [0, 255].
 **/
inline void
hsl_to_rgb_int (unsigned int *hue         /* returns red        */,
                unsigned int *saturation  /* returns green      */,
                unsigned int *lightness   /* returns blue       */)
{
  double h, s, l;

  h = *hue;
  s = *saturation;
  l = *lightness;

  if (s == 0)
  {
    /*  achromatic case  */
    *hue        = (int)l;
    *lightness  = (int)l;
    *saturation = (int)l;
  }
  else
  {
    double m1, m2;

    if (l < 128)
      m2 = (l * (255 + s)) / 65025.0;
    else
      m2 = (l + s - (l * s) / 255.0) / 255.0;

    m1 = (l / 127.5) - m2;

    /*  chromatic case  */
    *hue        = hsl_value_int (m1, m2, h + 85);
    *saturation = hsl_value_int (m1, m2, h);
    *lightness  = hsl_value_int (m1, m2, h - 85);
  }
}

/**
 * gimp_rgb_to_cmyk_int:
 * @red:     the red channel; returns the cyan value (0-255)
 * @green:   the green channel; returns the magenta value (0-255)
 * @blue:    the blue channel; returns the yellow value (0-255)
 * @pullout: the percentage of black to pull out (0-100); returns
 *           the black value (0-255)
 *
 * Does a naive conversion from RGB to CMYK colorspace. A simple
 * formula that doesn't take any color-profiles into account is used.
 * The amount of black pullout how can be controlled via the @pullout
 * parameter. A @pullout value of 0 makes this a conversion to CMY.
 * A value of 100 causes the maximum amount of black to be pulled out.
 **/
inline void
gimp_rgb_to_cmyk_int (int *red,
                      int *green,
                      int *blue,
                      int *pullout)
{
  int c, m, y;

  c = 255 - *red;
  m = 255 - *green;
  y = 255 - *blue;

  if (*pullout == 0)
    {
      *red   = c;
      *green = m;
      *blue  = y;
    }
  else
    {
      int k = 255;

      if (c < k)  k = c;
      if (m < k)  k = m;
      if (y < k)  k = y;

      k = (k * CLAMP (*pullout, 0, 100)) / 100;

      *red   = ((c - k) << 8) / (256 - k);
      *green = ((m - k) << 8) / (256 - k);
      *blue  = ((y - k) << 8) / (256 - k);
      *pullout = k;
    }
}

/**
 * gimp_cmyk_to_rgb_int:
 * @cyan:    the cyan channel; returns the red value (0-255)
 * @magenta: the magenta channel; returns the green value (0-255)
 * @yellow:  the yellow channel; returns the blue value (0-255)
 * @black:   the black channel (0-255); doesn't change
 *
 * Does a naive conversion from CMYK to RGB colorspace. A simple
 * formula that doesn't take any color-profiles into account is used.
 **/
inline void
cmyk_to_rgb_int (int *cyan,
                      int *magenta,
                      int *yellow,
                      int *black)
{
  int c, m, y, k;

  c = *cyan;
  m = *magenta;
  y = *yellow;
  k = *black;

  if (k)
    {
      c = ((c * (256 - k)) >> 8) + k;
      m = ((m * (256 - k)) >> 8) + k;
      y = ((y * (256 - k)) >> 8) + k;
    }

  *cyan    = 255 - c;
  *magenta = 255 - m;
  *yellow  = 255 - y;
}


#endif
