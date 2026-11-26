/* three_point_balance.c
 * Copyright (C) 2009 Maksim Golovkin (m4ks1k@gmail.com)
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
#include <math.h>
#include <string.h>
#include <stdio.h>

#include "frei0r.h"
#include "frei0r/math.h"

typedef struct three_point_balance_instance
{
  unsigned int width;
  unsigned int height;
  f0r_param_color_t blackColor;
  f0r_param_color_t grayColor;
  f0r_param_color_t whiteColor;
  double splitPreview;
  double srcPosition;
  uint8_t mapRed[256];
  uint8_t mapGreen[256];
  uint8_t mapBlue[256];
  int lutDirty;
} three_point_balance_instance_t;

int f0r_init()
{
  return 1;
}

void f0r_deinit()
{ }

void f0r_get_plugin_info(f0r_plugin_info_t* three_point_balance_info)
{
  three_point_balance_info->name = "3 point color balance";
  three_point_balance_info->author = "Maksim Golovkin";
  three_point_balance_info->plugin_type = F0R_PLUGIN_TYPE_FILTER;
  three_point_balance_info->color_model = F0R_COLOR_MODEL_RGBA8888;
  three_point_balance_info->frei0r_version = FREI0R_MAJOR_VERSION;
  three_point_balance_info->major_version = 0; 
  three_point_balance_info->minor_version = 1; 
  three_point_balance_info->num_params = 5; 
  three_point_balance_info->explanation = "Adjust color balance with 3 color points";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
  switch(param_index)
  {
  case 0:
    info->name = "Black color";
    info->type = F0R_PARAM_COLOR;
    info->explanation = "Black color";
    break;
  case 1:
    info->name = "Gray color";
    info->type = F0R_PARAM_COLOR;
    info->explanation = "Gray color";
    break;
  case 2:
    info->name = "White color";
    info->type = F0R_PARAM_COLOR;
    info->explanation = "White color";
    break;
  case 3:
    info->name = "Split preview";
    info->type = F0R_PARAM_BOOL;
    info->explanation = "Split privew";
    break;
  case 4:
    info->name = "Source image on left side";
    info->type = F0R_PARAM_BOOL;
    info->explanation = "Source image on left side";
    break;
  }
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
  three_point_balance_instance_t* inst = (three_point_balance_instance_t*)calloc(1, sizeof(*inst));
  inst->width = width; inst->height = height;
  inst->blackColor.r = 0;
  inst->blackColor.g = 0;
  inst->blackColor.b = 0;
  inst->grayColor.r = .5;
  inst->grayColor.g = .5;
  inst->grayColor.b = .5;
  inst->whiteColor.r = 1;
  inst->whiteColor.g = 1;
  inst->whiteColor.b = 1;
  inst->splitPreview = 1;
  inst->srcPosition = 1;
  inst->lutDirty = 1;
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
  three_point_balance_instance_t* inst = (three_point_balance_instance_t*)instance;

  switch(param_index)
  {
	case 0:
	  inst->blackColor = *((f0r_param_color_t *)param);
	  inst->lutDirty = 1;
	  break;
	case 1:
	  inst->grayColor = *((f0r_param_color_t *)param);
	  inst->lutDirty = 1;
	  break;
	case 2:
	  inst->whiteColor = *((f0r_param_color_t *)param);
	  inst->lutDirty = 1;
	  break;
	case 3:
	  inst->splitPreview = *((double *)param);
	  break;
	case 4:
	  inst->srcPosition = *((double *)param);
	  break;
  }
}

void f0r_get_param_value(f0r_instance_t instance,
                         f0r_param_t param, int param_index)
{
  assert(instance);
  three_point_balance_instance_t* inst = (three_point_balance_instance_t *)instance;
  
  switch(param_index)
  {
  case 0:
	*((f0r_param_color_t *)param) = inst->blackColor;
	break;
  case 1:
	*((f0r_param_color_t *)param) = inst->grayColor;
	break;
  case 2:
	*((f0r_param_color_t *)param) = inst->whiteColor;
	break;
  case 3:
	*((double *)param) = inst->splitPreview;
	break;
  case 4:
	*((double *)param) = inst->srcPosition;
	break;
  }
}

static void gaussSLESolve(int size, double* A, double* solution) {
	int extSize = size + 1;
	//direct way: transform matrix A to triangular form
	for(int row = 0; row < size; row++) {
		int col = row;
		int lastRowToSwap = size - 1;
		while (A[row * extSize + col] == 0 && lastRowToSwap > row) { //checking if current and lower rows can be swapped
			for(int i = 0; i < extSize; i++) {
				double tmp = A[row * extSize + i];
				A[row * extSize + i] = A[lastRowToSwap * extSize + i];
				A[lastRowToSwap * extSize + i] = tmp;
			}
			lastRowToSwap--;
		}
		double coeff = A[row * extSize + col];
		for(int j = 0; j < extSize; j++)  
			A[row * extSize + j] /= coeff;
		if (lastRowToSwap > row) {
			for(int i = row + 1; i < size; i++) {
				double rowCoeff = -A[i * extSize + col];
				for(int j = col; j < extSize; j++)
					A[i * extSize + j] += A[row * extSize + j] * rowCoeff;
			}
		}
	}
	//backward way: find solution from last to first
	for(int i = size - 1; i >= 0; i--) {
		solution[i] = A[i * extSize + size];
		for(int j = size - 1; j > i; j--) {
			solution[i] -= solution[j] * A[i * extSize + j];
		}
	}
}

static void calcParabolaCoeffs(const double* points, double* coeffs) {
  double m[3 * 4];
  for(int i = 0; i < 3; i++) {
	int offset = i * 2;
	m[i * 4] = points[offset] * points[offset];
	m[i * 4 + 1] = points[offset];
	m[i * 4 + 2] = 1;
	m[i * 4 + 3] = points[offset + 1];
  }
  gaussSLESolve(3, m, coeffs);
}

static inline double parabola(double x, const double* coeffs) {
  return (coeffs[0] * x + coeffs[1]) * x + coeffs[2];
}

static void rebuild_luts(three_point_balance_instance_t* inst) {
  double redPoints[6] = {inst->blackColor.r, 0, inst->grayColor.r, 0.5, inst->whiteColor.r, 1};
  double greenPoints[6] = {inst->blackColor.g, 0, inst->grayColor.g, 0.5, inst->whiteColor.g, 1};
  double bluePoints[6] = {inst->blackColor.b, 0, inst->grayColor.b, 0.5, inst->whiteColor.b, 1};
  double redCoeffs[3], greenCoeffs[3], blueCoeffs[3];
  calcParabolaCoeffs(redPoints, redCoeffs);
  calcParabolaCoeffs(greenPoints, greenCoeffs);
  calcParabolaCoeffs(bluePoints, blueCoeffs);

  for(int i = 0; i < 256; i++) {
	double w = parabola(i / 255., redCoeffs);
	inst->mapRed[i] = (uint8_t)(int)(CLAMP(w, 0, 1) * 255);
	w = parabola(i / 255., greenCoeffs);
	inst->mapGreen[i] = (uint8_t)(int)(CLAMP(w, 0, 1) * 255);
	w = parabola(i / 255., blueCoeffs);
	inst->mapBlue[i] = (uint8_t)(int)(CLAMP(w, 0, 1) * 255);
  }
  inst->lutDirty = 0;
}

static inline void apply_lut_span(const uint32_t* src, uint32_t* dst, size_t count,
                                  const uint8_t* mapR, const uint8_t* mapG, const uint8_t* mapB)
{
  size_t i = 0;
  for (; i + 3 < count; i += 4) {
    uint32_t p0 = src[i];
    uint32_t p1 = src[i + 1];
    uint32_t p2 = src[i + 2];
    uint32_t p3 = src[i + 3];
    dst[i]     = (uint32_t)mapR[p0 & 0xFFu] | ((uint32_t)mapG[(p0 >> 8) & 0xFFu] << 8)
               | ((uint32_t)mapB[(p0 >> 16) & 0xFFu] << 16) | (p0 & 0xFF000000u);
    dst[i + 1] = (uint32_t)mapR[p1 & 0xFFu] | ((uint32_t)mapG[(p1 >> 8) & 0xFFu] << 8)
               | ((uint32_t)mapB[(p1 >> 16) & 0xFFu] << 16) | (p1 & 0xFF000000u);
    dst[i + 2] = (uint32_t)mapR[p2 & 0xFFu] | ((uint32_t)mapG[(p2 >> 8) & 0xFFu] << 8)
               | ((uint32_t)mapB[(p2 >> 16) & 0xFFu] << 16) | (p2 & 0xFF000000u);
    dst[i + 3] = (uint32_t)mapR[p3 & 0xFFu] | ((uint32_t)mapG[(p3 >> 8) & 0xFFu] << 8)
               | ((uint32_t)mapB[(p3 >> 16) & 0xFFu] << 16) | (p3 & 0xFF000000u);
  }
  for (; i < count; i++) {
    uint32_t p = src[i];
    dst[i] = (uint32_t)mapR[p & 0xFFu] | ((uint32_t)mapG[(p >> 8) & 0xFFu] << 8)
           | ((uint32_t)mapB[(p >> 16) & 0xFFu] << 16) | (p & 0xFF000000u);
  }
}

void f0r_update(f0r_instance_t instance, double time,
                const uint32_t* inframe, uint32_t* outframe)
{
  assert(instance);
  three_point_balance_instance_t* inst = (three_point_balance_instance_t*)instance;

  if (inst->lutDirty) {
    rebuild_luts(inst);
  }

  const uint8_t* mapR = inst->mapRed;
  const uint8_t* mapG = inst->mapGreen;
  const uint8_t* mapB = inst->mapBlue;
  unsigned int w = inst->width;
  unsigned int h = inst->height;

  if (!inst->splitPreview) {
    apply_lut_span(inframe, outframe, (size_t)w * h, mapR, mapG, mapB);
  } else {
    unsigned int half = w / 2;
    unsigned int rest = w - half;
    if (inst->srcPosition) {
      for (unsigned int y = 0; y < h; y++) {
        const uint32_t* srow = inframe + (size_t)y * w;
        uint32_t* drow = outframe + (size_t)y * w;
        if (drow != srow) {
          memcpy(drow, srow, (size_t)half * sizeof(uint32_t));
        }
        apply_lut_span(srow + half, drow + half, rest, mapR, mapG, mapB);
      }
    } else {
      for (unsigned int y = 0; y < h; y++) {
        const uint32_t* srow = inframe + (size_t)y * w;
        uint32_t* drow = outframe + (size_t)y * w;
        apply_lut_span(srow, drow, half, mapR, mapG, mapB);
        if (drow != srow) {
          memcpy(drow + half, srow + half, (size_t)rest * sizeof(uint32_t));
        }
      }
    }
  }
}
