/*
 * This file is a port of Colorize plug-in from Gimp.
 * It contains code from files listed below:
 * gimpoperationcolorize.c
 * gimpcolortypes.h
 * gimpcolorspace.c
 * gimprgb.h
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Copyright (C) 2007 Michael Natterer
 *
 * This file, colorize.c
 * Copyright 2012 Janne Liljeblad 
 *
 * This file is a Frei0r plugin.
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

#include <stdlib.h>
#include <assert.h>

#include "frei0r.h"
#include "frei0r_math.h"

#define GIMP_RGB_LUMINANCE_RED    (0.2126)
#define GIMP_RGB_LUMINANCE_GREEN  (0.7152)
#define GIMP_RGB_LUMINANCE_BLUE   (0.0722)

#define GIMP_RGB_LUMINANCE(r,g,b) ((r) * GIMP_RGB_LUMINANCE_RED   + \
                                   (g) * GIMP_RGB_LUMINANCE_GREEN + \
                                   (b) * GIMP_RGB_LUMINANCE_BLUE)

typedef struct colorize_instance
{
  unsigned int width;
  unsigned int height;
  double hue;
  double saturation;
  double lightness;
} colorize_instance_t;

typedef struct _GimpRGB  GimpRGB;
typedef struct _GimpHSL  GimpHSL;

struct _GimpRGB
{
  double r, g, b, a;
};

struct _GimpHSL
{
  double h, s, l, a;
};

static inline double
gimp_hsl_value (double n1,
                double n2,
                double hue)
{
  double val;

  if (hue > 6.0)
    hue -= 6.0;
  else if (hue < 0.0)
    hue += 6.0;

  if (hue < 1.0)
    val = n1 + (n2 - n1) * hue;
  else if (hue < 3.0)
    val = n2;
  else if (hue < 4.0)
    val = n1 + (n2 - n1) * (4.0 - hue);
  else
    val = n1;

  return val;
}

static inline void
gimp_hsl_to_rgb (const GimpHSL *hsl,
                 GimpRGB       *rgb)
{
  if (hsl->s == 0)
    {
      /*  achromatic case  */
      rgb->r = hsl->l;
      rgb->g = hsl->l;
      rgb->b = hsl->l;
    }
  else
    {
      double m1, m2;

      if (hsl->l <= 0.5)
        m2 = hsl->l * (1.0 + hsl->s);
      else
        m2 = hsl->l + hsl->s - hsl->l * hsl->s;

      m1 = 2.0 * hsl->l - m2;

      rgb->r = gimp_hsl_value (m1, m2, hsl->h * 6.0 + 2.0);
      rgb->g = gimp_hsl_value (m1, m2, hsl->h * 6.0);
      rgb->b = gimp_hsl_value (m1, m2, hsl->h * 6.0 - 2.0);
    }

  rgb->a = hsl->a;
}


int f0r_init()
{
  return 1;
}

void f0r_deinit()
{ /* no initialization required */ }

void f0r_get_plugin_info(f0r_plugin_info_t* colorize_info)
{
  colorize_info->name = "colorize";
  colorize_info->author = "Janne Liljeblad";
  colorize_info->plugin_type = F0R_PLUGIN_TYPE_FILTER;
  colorize_info->color_model = F0R_COLOR_MODEL_RGBA8888;
  colorize_info->frei0r_version = FREI0R_MAJOR_VERSION;
  colorize_info->major_version = 0; 
  colorize_info->minor_version = 1; 
  colorize_info->num_params =  3; 
  colorize_info->explanation = "Colorizes image to selected hue, saturation and lightness";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
  switch(param_index)
  {
  case 0:
    info->name = "hue"; 
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "Color shade of the colorized image";
    break;
  case 1:
    info->name = "saturation"; 
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "Amount of color in the colorized image";
    break;
  case 2:
    info->name = "lightness"; 
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "Lightness of the colorized image";
    break;
  }
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
	colorize_instance_t* inst = (colorize_instance_t*)calloc(1, sizeof(*inst));
	inst->width = width; 
  inst->height = height;
	inst->hue = 0.5;
	inst->saturation = 0.5;
	inst->lightness = 0.5;
	return (f0r_instance_t)inst;
}

void f0r_destruct(f0r_instance_t instance)
{
  free(instance);
}

void f0r_set_param_value(f0r_instance_t instance, 
                         f0r_param_t param, int param_index)
{
  assert(instance);
  colorize_instance_t* inst = (colorize_instance_t*)instance;

  switch(param_index)
  {
  case 0:
    inst->hue = *((double*)param);
    break;
  case 1:
    inst->saturation = *((double*)param);
    break;
  case 2:
    inst->lightness = *((double*)param);
    break;
  }
}

void f0r_get_param_value(f0r_instance_t instance,
                         f0r_param_t param, int param_index)
{
  assert(instance);
  colorize_instance_t* inst = (colorize_instance_t*)instance;
  
  switch(param_index)
  {
  case 0:
    *((double*)param) = inst->hue;
    break;
  case 1:
    *((double*)param) = inst->saturation;
    break;
  case 2:
    *((double*)param) = inst->lightness;
    break;
  }
}

void f0r_update(f0r_instance_t instance, double time,
                const uint32_t* inframe, uint32_t* outframe)
{
  assert(instance);
  colorize_instance_t* inst = (colorize_instance_t*)instance;
  unsigned int len = inst->width * inst->height;
  
  GimpHSL hsl;
  GimpRGB rgb;

  hsl.h = inst->hue;
  hsl.s = inst->saturation;

  double lightness = inst->lightness - 0.5;

  unsigned char* dst = (unsigned char*)outframe;
  const unsigned char* src = (unsigned char*)inframe;
  double lum, r, g, b = 0;
  while (len--)
  {
    r = *src++ / 255.0;
    g = *src++ / 255.0;
    b = *src++ / 255.0;

    lum = GIMP_RGB_LUMINANCE (r, g, b);

    if (lightness > 0)
    {
      lum = lum * (1.0 - lightness);
      lum += 1.0 - (1.0 - lightness);
    }
    else if (lightness < 0)
    {
      lum = lum * (lightness + 1.0);
    }

    hsl.l = lum;
    gimp_hsl_to_rgb (&hsl, &rgb);

    *dst++ = (unsigned char) (rgb.r * 255.0);
    *dst++ = (unsigned char) (rgb.g * 255.0);
    *dst++ = (unsigned char) (rgb.b * 255.0);
    *dst++ = *src++;//copy alpha
  }
}

