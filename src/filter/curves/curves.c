/* curves.c
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
#include "frei0r_math.h"
#define CHANNEL_RED 0
#define CHANNEL_GREEN 1
#define CHANNEL_BLUE 2
#define CHANNEL_LUMA 3

#define POS_TOP_LEFT 0
#define POS_TOP_RIGHT 1
#define POS_BOTTOM_LEFT 2
#define POS_BOTTOM_RIGHT 3

#define POINT "Point "
#define INPUT_VALUE " input value"
#define OUTPUT_VALUE " output value"

typedef struct curves_instance
{
  unsigned int width;
  unsigned int height;
  double channel;
  double pointNumber;
  double points[10];
  double drawCurves;
  double curvesPosition;
} curves_instance_t;

char **param_names = NULL;
int f0r_init()
{
  param_names = (char**)calloc(10, sizeof(char *));
  for(int i = 0; i < 10; i++) {
	char *val = i % 2 == 0?INPUT_VALUE:OUTPUT_VALUE;
	param_names[i] = (char*)calloc(strlen(POINT) + 2 + strlen(val), sizeof(char));
	sprintf(param_names[i], "%s%d%s", POINT, i / 2 + 1, val); 
  }
  return 1;
}

void f0r_deinit()
{ 
  for(int i = 0; i < 10; i++)
	free(param_names[i]);
  free(param_names);
}

void f0r_get_plugin_info(f0r_plugin_info_t* curves_info)
{
  curves_info->name = "Curves";
  curves_info->author = "Maksim Golovkin";
  curves_info->plugin_type = F0R_PLUGIN_TYPE_FILTER;
  curves_info->color_model = F0R_COLOR_MODEL_RGBA8888;
  curves_info->frei0r_version = FREI0R_MAJOR_VERSION;
  curves_info->major_version = 0; 
  curves_info->minor_version = 1; 
  curves_info->num_params = 14; 
  curves_info->explanation = "Adjust luminance or color channel intensity with curve level mapping";
}

char *get_param_name(int param_index) {
  return param_names[param_index];
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
  switch(param_index)
  {
  case 0:
    info->name = "Channel";
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "Channel to adjust levels (1 = RED; 2 = GREEN; 3 = BLUE; 4 = LUMA)";
    break;
  case 1:
    info->name = "Show curves";
    info->type = F0R_PARAM_BOOL;
    info->explanation = "Draw curve graph on output image";
    break;
  case 2:
    info->name = "Graph position";
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "Output image corner where curve graph will be drawn (1 = TOP,LEFT; 2 = TOP,RIGHT; 3 = BOTTOM,LEFT; 4 = BOTTOM, RIGHT)";
    break;
  case 3:
    info->name = "Curve point number";
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "Number of point to use to build curve";
    break;
  default:
	if (param_index > 3) {
	  info->name = get_param_name(param_index - 4);
	  info->type = F0R_PARAM_DOUBLE;
	  info->explanation = get_param_name(param_index - 4);
	}
    break;
  }
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
  curves_instance_t* inst = (curves_instance_t*)calloc(1, sizeof(*inst));
  inst->width = width; inst->height = height;
  inst->channel = 0;
  inst->drawCurves = 1;
  inst->curvesPosition = 3;
  inst->pointNumber = 2;
  inst->points[0] = 0;
  inst->points[1] = 0;
  inst->points[2] = 1;
  inst->points[3] = 1;
  inst->points[4] = 0;
  inst->points[5] = 0;
  inst->points[6] = 0;
  inst->points[7] = 0;
  inst->points[8] = 0;
  inst->points[9] = 0;
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
  curves_instance_t* inst = (curves_instance_t*)instance;

  switch(param_index)
  {
	case 0:
	  inst->channel = *((f0r_param_double *)param);
	  break;
	case 1:
	  inst->drawCurves =  *((f0r_param_double *)param);
	  break;
	case 2:
	  inst->curvesPosition =  *((f0r_param_double *)param);
	  break;
	case 3:
	  inst->pointNumber = *((f0r_param_double *)param);
	  break;
	default:
	  if (param_index > 3)
		inst->points[param_index - 4] = *((f0r_param_double *)param); //Assigning value to curve point
	  break;
  }
}

void f0r_get_param_value(f0r_instance_t instance,
                         f0r_param_t param, int param_index)
{
  assert(instance);
  curves_instance_t* inst = (curves_instance_t*)instance;
  
  switch(param_index)
  {
  case 0:
	*((f0r_param_double *)param) = inst->channel;
	break;
  case 1:
	*((f0r_param_double *)param) = inst->drawCurves;
	break;
  case 2:
	*((f0r_param_double *)param) = inst->curvesPosition;
	break;
  case 3:
	*((f0r_param_double *)param) = inst->pointNumber;
	break;
  default:
	if (param_index > 3)
	  *((f0r_param_double *)param) = inst->points[3]; //Fetch curve point value
	break;
  }
}

double* gaussSLESolve(size_t size, double* A) {
	int extSize = size + 1;
	//direct way: tranform matrix A to triangular form
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
	double *solution = (double*)calloc(size, sizeof(double));
	for(int i = size - 1; i >= 0; i--) {
		solution[i] = A[i * extSize + size];// 
		for(int j = size - 1; j > i; j--) {
			solution[i] -= solution[j] * A[i * extSize + j];
		}
	}
	return solution;
}



double* calcSplineCoeffs(double* points, size_t pointsSize) {
	double* coeffs = NULL;
	int size = pointsSize;
	int mxSize = size > 3?4:size;
	int extMxSize = mxSize + 1;
	if (size == 2) { //coefficients of linear function Ax + B = y
		double *m = (double*)calloc(mxSize * extMxSize, sizeof(double));
		for(int i = 0; i < size; i++) {
			int offset = i * 2;
			m[i * extMxSize] = points[offset];
			m[i * extMxSize + 1] = 1;
			m[i * extMxSize + 2] = points[offset + 1];
		}
		coeffs = gaussSLESolve(size, m);
		free(m);
	} else if (size == 3) { //coefficients of quadrant function Ax^2 + Bx + C = y
		double *m = (double*)calloc(mxSize * extMxSize, sizeof(double));
		for(int i = 0; i < size; i++) {
			int offset = i * 2;
			m[i * extMxSize] = points[offset]*points[offset];
			m[i * extMxSize + 1] = points[offset];
			m[i * extMxSize + 2] = 1;
			m[i * extMxSize + 3] = points[offset + 1];
		}
		coeffs = gaussSLESolve(size, m);
		free(m);
	} else if (size > 3) { //coefficients of cubic spline Ax^3 + Bx^2 + Cx + D = y
		coeffs = (double*)calloc(5 * size,sizeof(double));
		for(int i = 0; i < size; i++) {
			int offset = i * 5;
			int srcOffset = i * 2;
			coeffs[offset] = points[srcOffset];
			coeffs[offset + 1] = points[srcOffset + 1];
		}
		coeffs[3] = coeffs[(size - 1) * 5 + 3] = 0;
		double *alpha = (double*)calloc(size - 1,sizeof(double));
		double *beta = (double*)calloc(size - 1,sizeof(double));
		alpha[0] = beta[0] = 0;
		for(int i = 1; i < size - 1; i++) {
			int srcI = i * 2;
			int srcI_1 = (i - 1) * 2;
			int srcI1 = (i + 1) * 2;
			double 	h_i = points[srcI] - points[srcI_1], 
					h_i1 = points[srcI1] - points[srcI];
			double A = h_i;
			double C = 2. * (h_i + h_i1);
			double B = h_i1;
			double F = 6. * ((points[srcI1 + 1] - points[srcI + 1]) / h_i1 - (points[srcI + 1] - points[srcI_1 + 1]) / h_i);
			double z = (A * alpha[i - 1] + C);
			alpha[i] = -B / z;
			beta[i] = (F - A * beta[i - 1]) / z;
		}
		for (int i = size - 2; i > 0; --i)
			coeffs[i * 5 + 3] = alpha[i] * coeffs[(i + 1) * 5 + 3] + beta[i];
		free(beta);
		free(alpha);
	  
		for (int i = size - 1; i > 0; --i){
			int srcI = i * 2;
			int srcI_1 = (i - 1) * 2;
			double h_i = points[srcI] - points[srcI_1];
			int offset = i * 5;
			coeffs[offset + 4] = (coeffs[offset + 3] - coeffs[offset - 2]) / h_i;
			coeffs[offset + 2] = h_i * (2. * coeffs[offset + 3] + coeffs[offset - 2]) / 6. + (points[srcI + 1] - points[srcI_1 + 1]) / h_i;
		}			
	}
	return coeffs;
}

double spline(double x, double* points, size_t pointSize, double* coeffs) {
	int size = pointSize;
	if (size == 2) {
		return coeffs[0] * x + coeffs[1];
	} else if (size == 3) {
		return (coeffs[0] * x + coeffs[1]) * x + coeffs[2];
	} else if (size > 3) {
		int offset = 5;
		if (x <= points[0]) //find valid interval of cubic spline
			offset = 1;
		else if (x >= points[(size - 1) * 2])
			offset = size - 1;
		else {
			int i = 0, j = size - 1;
			while (i + 1 < j) {
				int k = i + (j - i) / 2;
				if (x <= points[k * 2])
					j = k;
				else
					i = k;
			}
			offset = j;
		}
		offset *= 5;
		double dx = x - coeffs[offset];
		return ((coeffs[offset + 4] * dx / 6. + coeffs[offset + 3] / 2.) * dx + coeffs[offset + 2]) * dx + coeffs[offset + 1];
	}
}

void swap(double *points, int i, int j) {
  int offsetX = i * 2, offsetY = j * 2;
  double tempX = points[offsetX], tempY = points[offsetX + 1];
  points[offsetX] = points[offsetY];
  points[offsetX + 1] = points[offsetY + 1];
  points[offsetY] = tempX;
  points[offsetY + 1] = tempY;
}

void f0r_update(f0r_instance_t instance, double time,
                const uint32_t* inframe, uint32_t* outframe)
{
  assert(instance);
  curves_instance_t* inst = (curves_instance_t*)instance;
  unsigned int len = inst->width * inst->height;
  
  unsigned char* dst = (unsigned char*)outframe;
  const unsigned char* src = (unsigned char*)inframe;
  int b, g, r;
  int luma;

  int map[256];
  double mapLuma[256];
  float *mapCurves = NULL;
  int scale = inst->height / 2;
  double *points = (double*)calloc(inst->pointNumber * 2, sizeof(double));
  int i = inst->pointNumber * 2;
  //copy point values 
  while(--i)
	points[i] = inst->points[i];
  //sort point values by X component
  for(int i = 1; i < inst->pointNumber; i++)
	for(int j = i; j > 0 && points[j * 2] < points[(j - 1) * 2]; j--)
	  swap(points, j, j - 1);
  //calculating spline coeffincients
  double *coeffs = calcSplineCoeffs(points, (size_t)inst->pointNumber);

  //building map for values from 0 to 255
  for(int i = 0; i < 256; i++) {
	double v = i / 255.;
	double w = spline(v, points, (size_t)inst->pointNumber, coeffs);
	map[i] = CLAMP(w, 0, 1) * 255;
	mapLuma[i] = i == 0?w:w / v;	
  }
  //building map for drawing curve
  if (inst->drawCurves) {
	mapCurves = (float*)calloc(scale, sizeof(float));
	for(int i = 0; i < scale; i++)
	  mapCurves[i] = spline((float)i / scale, points, (size_t)inst->pointNumber, coeffs) * scale;
  }
  free(coeffs);

  while (len--)
  {
	r = *src++;
	g = *src++;
	b = *src++;

	//calculating point luminance value
	if (inst->channel == CHANNEL_LUMA)
	  luma = CLAMP0255((unsigned int)(b * .114 + g * .587 + r * .299));
	
	//mapping curve values to current point
	switch ((int)inst->channel) {
	case CHANNEL_RED:
	  *dst++ = map[r];
	  *dst++ = g;
	  *dst++ = b;
	  break;
	case CHANNEL_GREEN:
	  *dst++ = r;
	  *dst++ = map[g];
	  *dst++ = b;
	  break;
	case CHANNEL_BLUE:
	  *dst++ = r;
	  *dst++ = g;
	  *dst++ = map[b];
	  break;
	case CHANNEL_LUMA:
	  if (luma == 0) {
		*dst++ = mapLuma[luma];
		*dst++ = mapLuma[luma];
		*dst++ = mapLuma[luma];
	  } else {
		*dst++ = CLAMP0255((unsigned int)(r * mapLuma[luma]));
		*dst++ = CLAMP0255((unsigned int)(g * mapLuma[luma]));
		*dst++ = CLAMP0255((unsigned int)(b * mapLuma[luma]));
	  }
	  break;
	}

	*dst++ = *src++;  // copy alpha
  }
  
  if (inst->drawCurves) {
	unsigned char color[] = {0, 0, 0};
	if (inst->channel != CHANNEL_LUMA)
	  color[(int)inst->channel] = 255;
	//calculating graph offset by given position values
	int graphXOffset = inst->curvesPosition == POS_TOP_LEFT || inst->curvesPosition == POS_BOTTOM_LEFT?0:inst->width - scale;
	int graphYOffset = inst->curvesPosition == POS_TOP_LEFT || inst->curvesPosition == POS_TOP_RIGHT?0:inst->height - scale;
	int maxYvalue = scale - 1;
	int stride = inst->width;
	dst = (unsigned char*)outframe;
	float lineWidth = scale / 254.;
	int cellSize = floor(lineWidth * 32);
	//filling up background and drawing grid
	for(int i = 0; i < scale; i++) {
	  if (i % cellSize > lineWidth) //point doesn't aly on the grid
		for(int j = 0; j < scale; j++) {
		  if (j % cellSize > lineWidth) { //point doesn't aly on the grid
			int offset = ((maxYvalue - i + graphYOffset) * stride + j + graphXOffset) * 4;
			dst[offset] = (dst[offset++] >> 1) + 0x7F;
			dst[offset] = (dst[offset++] >> 1) + 0x7F;
			dst[offset] = (dst[offset++] >> 1) + 0x7F;
		  }
		}
	}
	float doubleLineWidth = 4 * lineWidth;
	//drawing points on the graph
	for(int i = 0; i < inst->pointNumber; i++) {
	  int pointOffset = i * 2;
	  int xPoint = points[pointOffset++] * maxYvalue;
	  int yPoint = points[pointOffset] * maxYvalue;
	  for(int x = (int)floor(xPoint - doubleLineWidth); x <= xPoint + doubleLineWidth; x++) {
		if (x >= 0 && x < scale) {
		  for(int y = (int)floor(yPoint - doubleLineWidth); y <= yPoint + doubleLineWidth; y++) {
			if (y >= 0 && y < scale) {
			  int offset = ((maxYvalue - y + graphYOffset) * stride + x + graphXOffset) * 4;
			  dst[offset++] = color[0];
			  dst[offset++] = color[1];
			  dst[offset++] = color[2];
			}
		  }
		}
	  }
	}
	free(points);
	//drawing curve on the graph
	float halfLineWidth = lineWidth * .5;
	float coeff = 255. / scale;
	float prevY = 0;
	for(int j = 0; j < scale; j++) {
	  float y = mapCurves[j];
	  if (j == 0 || y == prevY) {
		for(int i = (int)floor(y - halfLineWidth); i <= ceil(y + halfLineWidth); i++) {
		  int clampedI = i < 0?0:i >= scale?scale - 1:i;
		  int offset = ((maxYvalue - clampedI + graphYOffset) * stride + j + graphXOffset) * 4;
		  dst[offset++] = color[0];
		  dst[offset++] = color[1];
		  dst[offset++] = color[2];
		}
	  } else {
		int factor = prevY > y?-1:1;
		float gap = halfLineWidth * factor;
		//medium value between previous value and current value
		float mid = (y - prevY) * .5 + prevY; 
		//drawing line from previous value to mid point
		for(int i = (int)round(prevY - gap); factor * i < factor * (mid + gap); i += factor) { 
		  int clampedI = i < 0?0:i >= scale?scale - 1:i;
		  int offset = ((maxYvalue - clampedI + graphYOffset) * stride + j - 1 + graphXOffset) * 4;
		  dst[offset++] = color[0];
		  dst[offset++] = color[1];
		  dst[offset++] = color[2];
		}
		  //drawing line from mid point to current value
		for(int i = (int)round(mid - gap); factor * i < factor * ceil(y + gap); i += factor) {
		  int clampedI = i < 0?0:i >= scale?scale - 1:i;
		  int offset = ((maxYvalue - clampedI + graphYOffset) * stride + j + graphXOffset) * 4;
		  dst[offset++] = color[0];
		  dst[offset++] = color[1];
		  dst[offset++] = color[2];
		}
	  }
	  prevY = y;
	}
	free(mapCurves);
  }
}

