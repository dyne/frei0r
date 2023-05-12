/* curves.c
 * Copyright (C) 2009 Maksim Golovkin (m4ks1k@gmail.com)
 * Copyright (C) 2010 Till Theato (root@ttill.de)
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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

#include "frei0r.h"
#include "frei0r_math.h"

#define MAX3(a, b, c) ( ( a > b && a > c) ? a : (b > c ? b : c) )
#define MIN3(a, b, c) ( ( a < b && a < c) ? a : (b < c ? b : c) )

#define POS_TOP_LEFT 0
#define POS_TOP_RIGHT 1
#define POS_BOTTOM_LEFT 2
#define POS_BOTTOM_RIGHT 3

#define POINT "Point "
#define INPUT_VALUE " input value"
#define OUTPUT_VALUE " output value"

enum CHANNELS { CHANNEL_RED = 0, CHANNEL_GREEN, CHANNEL_BLUE, CHANNEL_ALPHA, CHANNEL_LUMA, CHANNEL_RGB, CHANNEL_HUE, CHANNEL_SATURATION };

typedef struct position
{
    double x;
    double y;
} position;

typedef position bspline_point[3]; // [0] = handle1, [1] = point, [2] = handle2


typedef struct curves_instance
{
  unsigned int width;
  unsigned int height;
  enum CHANNELS channel;
  double pointNumber;
  double points[10];
  double drawCurves;
  double curvesPosition;
  double formula;

  char *bspline;
  double *bsplineMap;
  double *csplineMap;
  float *curveMap;
} curves_instance_t;


// color conversion functions taken from:
// http://www.cs.rit.edu/~ncs/color/t_convert.html
// slightly modified

// r,g,b values are from 0 to 255
// h = [0,360], s = [0,1], v = [0,1]
//              if s == 0, then h = -1 (undefined)
void RGBtoHSV(double r, double g, double b, double *h, double *s, double *v)
{
    double min = MIN3(r, g, b);
    double max = MAX3(r, g, b);
    *v = max / 255.;

    double delta = max - min;

    if (delta != 0) {
        *s = delta / max;               // s
    } else {
       // r = g = b                    // s = 0
       *s = 0;
       *h = -1;
       return;
    }

    if (r == max)
        *h = (g - b) / delta;         // between yellow & magenta
    else if (g == max)
        *h = 2 + (b - r) / delta;     // between cyan & yellow
    else
        *h = 4 + (r - g) / delta;     // between magenta & cyan

    *h *= 60;                         // degrees
    if (*h < 0)
        *h += 360;
}

// r,g,b values are from 0 to 1
// h = [0,360], s = [0,1], v = [0,1]
void HSVtoRGB(double *r, double *g, double *b, double h, double s, double v)
{
    if (s == 0) {
        // achromatic (grey)
        *r = *g = *b = v;
        return;
    }

    h /= 60;                        // sector 0 to 5
    int i = (int)h;
    double f = h - i;               // factorial part of h
    double p = v * (1 - s);

    if (i & 1) {
        double q = v * (1 - s * f);
        switch (i) {
        case 1:
            *r = q;
            *g = v;
            *b = p;
            break;
        case 3:
            *r = p;
            *g = q;
            *b = v;
            break;
        case 5:
            *r = v;
            *g = p;
            *b = q;
        break;
        }
    } else {
        double t = v * (1 - s * (1 - f));
        switch (i) {
        case 0:
            *r = v;
            *g = t;
            *b = p;
            break;
        case 2:
            *r = p;
            *g = v;
            *b = t;
            break;
        case 4:
            *r = t;
            *g = p;
            *b = v;
            break;
        }
    }
}

void updateBsplineMap(f0r_instance_t instance);
void updateCsplineMap(f0r_instance_t instance);

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
  curves_info->author = "Maksim Golovkin, Till Theato";
  curves_info->plugin_type = F0R_PLUGIN_TYPE_FILTER;
  curves_info->color_model = F0R_COLOR_MODEL_RGBA8888;
  curves_info->frei0r_version = FREI0R_MAJOR_VERSION;
  curves_info->major_version = 0;
  curves_info->minor_version = 4;
  curves_info->num_params = 16;
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
    info->explanation = "Channel to adjust (0 = red, 0.1 = green, 0.2 = blue, 0.3 = alpha, 0.4 = luma, 0.5 = rgb, 0.6 = hue, 0.7 = saturation)";
    break;
  case 1:
    info->name = "Show curves";
    info->type = F0R_PARAM_BOOL;
    info->explanation = "Draw curve graph on output image";
    break;
  case 2:
    info->name = "Graph position";
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "Output image corner where curve graph will be drawn (0.1 = TOP,LEFT; 0.2 = TOP,RIGHT; 0.3 = BOTTOM,LEFT; 0.4 = BOTTOM, RIGHT)";
    break;
  case 3:
    info->name = "Curve point number";
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "Number of points to use to build curve (/10 to fit [0,1] parameter range). Minimum 2 (0.2), Maximum 5 (0.5). Not relevant for Bézier spline.";
    break;
  case 4:
    info->name = "Luma formula";
    info->type = F0R_PARAM_BOOL;
    info->explanation = "Use Rec. 601 (false) or Rec. 709 (true)";
    break;
  case 5:
    info->name = "Bézier spline";
    info->type = F0R_PARAM_STRING;
    info->explanation = "Use cubic Bézier spline. Has to be a sorted list of points in the format 'handle1x;handle1y#pointx;pointy#handle2x;handle2y'(pointx = in, pointy = out). Points are separated by a '|'.The values can have 'double' precision. x, y for points should be in the range 0-1. x,y for handles might also be out of this range.";
  default:
	if (param_index > 5) {
	  info->name = get_param_name(param_index - 6);
	  info->type = F0R_PARAM_DOUBLE;
	  info->explanation = get_param_name(param_index - 6);
	}
    break;
  }
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
  curves_instance_t* inst = (curves_instance_t*)calloc(1, sizeof(*inst));
  inst->width = width; inst->height = height;
  inst->channel = CHANNEL_RGB;
  inst->drawCurves = 1;
  inst->curvesPosition = 3;
  inst->pointNumber = 2;
  inst->formula = 1;
  inst->bspline = calloc(1, sizeof(char));
  inst->bsplineMap = NULL;
  inst->csplineMap = NULL;
  inst->curveMap = NULL;
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
  curves_instance_t* inst = (curves_instance_t*)instance;
  if (inst->bspline)   free(inst->bspline);
  if(inst->bsplineMap) free(inst->bsplineMap);
  if(inst->csplineMap) free(inst->csplineMap);
  if(inst->curveMap)   free(inst->curveMap);
  free(inst);
}

void f0r_set_param_value(f0r_instance_t instance,
                         f0r_param_t param, int param_index)
{
  assert(instance);
  curves_instance_t* inst = (curves_instance_t*)instance;

  double tmp;
  f0r_param_string bspline;

  switch(param_index)
  {
	case 0:
          tmp = *((f0r_param_double *)param);
          if (tmp >= 1) {
              // legacy support
              if (tmp == 3) {
                  if (inst->channel != CHANNEL_LUMA) {
                    inst->channel = CHANNEL_LUMA;
                    if (strlen(inst->bspline))
                        updateBsplineMap(instance);
                    else
                        updateCsplineMap(instance);
                  }
              } else {
                  if ((int)inst->channel != (int)tmp) {
                    inst->channel = (enum CHANNELS)((int)tmp);
                    if (strlen(inst->bspline))
                        updateBsplineMap(instance);
                    else
                        updateCsplineMap(instance);
                  }
              }
          } else {
              if ((int)inst->channel != (int)(tmp * 10)) {
                inst->channel = (enum CHANNELS)(tmp * 10);
                if (strlen(inst->bspline))
                    updateBsplineMap(instance);
                else
                    updateCsplineMap(instance);
              }
          }
	  break;
	case 1:
	  inst->drawCurves =  *((f0r_param_double *)param);
	  break;
	case 2:
	  inst->curvesPosition =  floor(*((f0r_param_double *)param) * 10);
	  break;
	case 3:
	  inst->pointNumber = floor(*((f0r_param_double *)param) * 10);
	  break;
        case 4:
          inst->formula = *((f0r_param_double *)param);
          break;
        case 5:
          bspline = *((f0r_param_string *)param);
          if (strcmp(inst->bspline, bspline) != 0) {
              free(inst->bspline);
              inst->bspline = strdup(bspline);
              updateBsplineMap(instance);
          }
          break;
	default:
    if (param_index > 5){
        inst->points[param_index - 6] = *((f0r_param_double *)param); //Assigning value to curve point
      updateCsplineMap(instance);
    }

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
	*((f0r_param_double *)param) = inst->channel / 10.;
	break;
  case 1:
	*((f0r_param_double *)param) = inst->drawCurves;
	break;
  case 2:
	*((f0r_param_double *)param) = inst->curvesPosition / 10.;
	break;
  case 3:
	*((f0r_param_double *)param) = inst->pointNumber / 10.;
	break;
  case 4:
        *((f0r_param_double *)param) = inst->formula;
        break;
  case 5:
        *((f0r_param_string *)param) = inst->bspline;
        break;
  default:
	if (param_index > 5)
	  *((f0r_param_double *)param) = inst->points[param_index - 6]; //Fetch curve point value
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
    /* This should never be reached, statement passifies the compiler*/
    return -1.0;
}

void swap(double *points, int i, int j) {
  int offsetX = i * 2, offsetY = j * 2;
  double tempX = points[offsetX], tempY = points[offsetX + 1];
  points[offsetX] = points[offsetY];
  points[offsetX + 1] = points[offsetY + 1];
  points[offsetY] = tempX;
  points[offsetY + 1] = tempY;
}


/**
 * Calculates a x,y pair for \param t on the cubic Bézier curve defined by \param points.
 * \param t "time" in the range 0-1
 * \param points points[0] = point1, point[1] = handle1, point[2] = handle2, point[3] = point2
 */
position pointOnBezier(double t, position points[4])
{
    position pos;

    /*
     * Calculating a point on the bezier curve using the coefficients from Bernstein basis polynomial of degree 3.
     * Using the De Casteljau algorithm would be slightly faster when calculating a lot of values
     * but the difference is far from noticable here since we update the spline only when the parameter changes
     */
    double c1 = (1-t) * (1-t) * (1-t);
    double c2 = 3 * t * (1-t) * (1-t);
    double c3 = 3 * t * t * (1-t);
    double c4 = t * t * t;
    pos.x = points[0].x*c1 + points[1].x*c2 + points[2].x*c3 + points[3].x*c4;
    pos.y = points[0].y*c1 + points[1].y*c2 + points[2].y*c3 + points[3].y*c4;
    return pos;
}

/**
 * Splits given string into sub-strings at given delimiter.
 * \param string input string
 * \param delimiter delimiter
 * \param tokens pointer to array of strings, will be filled with sub-strings
 * \return Number of sub-strings
 */
int tokenise(char *string, const char *delimiter, char ***tokens)
{
    int count = 0;
    char *input = strdup(string);
    char *result = NULL;
    result = strtok(input, delimiter);
    while (result != NULL) {
        *tokens = realloc(*tokens, (count + 1) * sizeof(char *));
        (*tokens)[count++] = strdup(result);
        result = strtok(NULL, delimiter);
    }
    free(input);
    return count;
}

/**
 * Updates the color map according to the bézier spline described in the "Bézier spline" parameter.
 */
void updateBsplineMap(f0r_instance_t instance)
{
    assert(instance);
    curves_instance_t* inst = (curves_instance_t*)instance;

    int range = inst->channel == CHANNEL_HUE ? 361 : 256;
    free(inst->bsplineMap);
    inst->bsplineMap = malloc(range * sizeof(double));
    // fill with default values, in case the spline does not cover the whole range
    if (inst->channel == CHANNEL_HUE) {
        for(int i = 0; i < 361; ++i)
            inst->bsplineMap[i] = i;
    } else if (inst->channel == CHANNEL_LUMA || inst->channel == CHANNEL_SATURATION) {
        for(int i = 0; i < 256; ++i)
            inst->bsplineMap[i] = inst->channel == CHANNEL_LUMA ? 1 : i / 255.;
    } else {
        for(int i = 0; i < 256; ++i)
            inst->bsplineMap[i] = i;
    }

    /*
     * string -> list of points
     */
    char **pointStr = calloc(1, sizeof(char *));
    int count = tokenise(inst->bspline, "|", &pointStr);

    bspline_point *points = (bspline_point *) malloc(count * sizeof(bspline_point));

    for (int i = 0; i < count; ++i) {
        char **positionsStr = calloc(1, sizeof(char *));
        int positionsNum = tokenise(pointStr[i], "#", &positionsStr);
        if (positionsNum == 3) { // h1, p, h2
            for (int j = 0; j < positionsNum; ++j) {
                char **coords = calloc(1, sizeof(char *));
                int coordsNum = tokenise(positionsStr[j], ";", &coords);
                if (coordsNum == 2) { // x, y
                    points[i][j].x = atof(coords[0]);
                    points[i][j].y = atof(coords[1]);
                }
                for (int k = 0; k < coordsNum; ++k)
                    free(coords[k]);
                free(coords);
            }
        }
        for(int j = 0; j < positionsNum; ++j)
            free(positionsStr[j]);
        free(positionsStr);
    }

    for (int i = 0; i < count; ++i)
        free(pointStr[i]);
    free(pointStr);

    /*
     * Actual work: calculate curves between points and fill map
     */
    position p[4];
    double t, step, diff, y;
    int pn, c, k;
    for (int i = 0; i < count - 1; ++i) {
        p[0] = points[i][1];
        p[1] = points[i][2];
        p[2] = points[i+1][0];
        p[3] = points[i+1][1];

        // make sure points are in correct order
        if (p[0].x > p[3].x)
            continue;

        // try to avoid loops and other cases of one x having multiple y
        p[1].x = CLAMP(p[1].x, p[0].x, p[3].x);
        p[2].x = CLAMP(p[2].x, p[0].x, p[3].x);

        t = 0;
        pn = 0;
        // number of points calculated for this curve
        // x range * 10 should give enough points
        c = (int)((p[3].x - p[0].x) * range * 10);
        if (c == 0) {
            // points have same x value -> will result in a jump in the curve
            // calculate anyways, in case we only have these two points (with more points this x value will be calculated three times)
            c = 1;
        }
        step = 1 / (double)c;
        position *curve = (position *) malloc((c + 1) * sizeof(position));
        while (t <= 1) {
            curve[pn++] = pointOnBezier(t, p);
            t += step;
        }

        // Fill the map in range this curve provides
        k = 0;
        // connection points will be written twice (but therefore no special case for the last point is required)
        for (int j = (int)(p[0].x * (range-1)); j <= (int)(p[3].x * (range-1)); ++j) {
            if (k > 0)
                --k;
            diff = fabs(j / ((double)(range-1)) - curve[k].x);
            y = curve[k].y;
            // Find point closest to the one needed (integers 0 - range)
            while (++k < pn) {
                if (fabs(j / ((double)(range-1)) - curve[k].x) > diff)
                    break;
                diff = fabs(j / ((double)(range-1)) - curve[k].x);
                y = curve[k++].y;
            }

            if (inst->channel == CHANNEL_HUE)
                inst->bsplineMap[j] = CLAMP(y * 360, 0, 360);
            else if (inst->channel == CHANNEL_LUMA)
                inst->bsplineMap[j] = y / (j == 0 ? 1 : j / 255.);
            else if (inst->channel == CHANNEL_SATURATION)
                inst->bsplineMap[j] = CLAMP(y, 0, 1);
            else
                inst->bsplineMap[j] = CLAMP0255(ROUND(y * 255));
        }

        free(curve);
    }

    free(points);
}

/**
 * Updates the color map according to the cubic spline described in the "Curve Point" parameter.
 */
void updateCsplineMap(f0r_instance_t instance)
{
    assert(instance);
    curves_instance_t* inst = (curves_instance_t*)instance;

    int range = inst->channel == CHANNEL_HUE ? 361 : 256;
    free(inst->csplineMap);
    inst->csplineMap = malloc(range * sizeof(double));
    // fill with default values, in case the spline does not cover the whole range
    if (inst->channel == CHANNEL_HUE) {
        for(int i = 0; i < 361; ++i)
            inst->csplineMap[i] = i;
    } else if (inst->channel == CHANNEL_LUMA || inst->channel == CHANNEL_SATURATION) {
        for(int i = 0; i < 256; ++i)
            inst->csplineMap[i] = inst->channel == CHANNEL_LUMA ? 1 : i / 255.;
    } else {
        for(int i = 0; i < 256; ++i)
            inst->csplineMap[i] = i;
    }

    /*
     * Retrieve points
     */
    double *points = (double*)calloc(inst->pointNumber * 2, sizeof(double));
    int i = inst->pointNumber * 2;
    //copy point values
    while(--i > 0)
        points[i] = inst->points[i];
    //sort point values by X component
    for(i = 1; i < inst->pointNumber; i++)
        for(int j = i; j > 0 && points[j * 2] < points[(j - 1) * 2]; j--)
            swap(points, j, j - 1);
    //calculating spline coeffincients
    double *coeffs = calcSplineCoeffs(points, (size_t)inst->pointNumber);

    /*
     * Actual work: calculate curves between points and fill map
     */
    for(int j = 0; j < range; j++) {
        double x = j / (double)(range - 1);
        double y = spline(x, points, (size_t)inst->pointNumber, coeffs);
        if (inst->channel == CHANNEL_HUE)
            inst->csplineMap[j] = CLAMP(y * 360, 0, 360);
        else if (inst->channel == CHANNEL_LUMA)
            inst->csplineMap[j] = y / (j == 0 ? 1 : j / 255.);
        else if (inst->channel == CHANNEL_SATURATION)
            inst->csplineMap[j] = CLAMP(y, 0, 1);
        else
            inst->csplineMap[j] = CLAMP0255(ROUND(y * 255));
    }
    if (inst->drawCurves) {
        int scale = inst->height / 2;
        free(inst->curveMap);
        inst->curveMap = malloc(scale * sizeof(float));
        for(i = 0; i < scale; i++)
            inst->curveMap[i] = spline((double)i / scale, points, (size_t)inst->pointNumber, coeffs) * scale;
    }

    free(coeffs);
    free(points);
}

void f0r_update(f0r_instance_t instance, double time,
                const uint32_t* inframe, uint32_t* outframe)
{
  assert(instance);
  curves_instance_t* inst = (curves_instance_t*)instance;
  unsigned int len = inst->width * inst->height;

  // test initalization c/b spline
  double *splinemap = strlen(inst->bspline)>0 ? inst->bsplineMap : inst->csplineMap;
  if(!splinemap) {
	memcpy(outframe,inframe,inst->width * inst->height * 4);
	return;
  }

  unsigned char* dst = (unsigned char*)outframe;
  const unsigned char* src = (unsigned char*)inframe;

  int i = 0;
  double *map = NULL;
  int scale = inst->height / 2;
  double *points = NULL;
  if (strlen(inst->bspline) == 0) {
      points = (double*)calloc(inst->pointNumber * 2, sizeof(double));
      i = inst->pointNumber * 2;
      //copy point values
      while(--i > 0)
          points[i] = inst->points[i];
      //sort point values by X component
      for(i = 1; i < inst->pointNumber; i++)
          for(int j = i; j > 0 && points[j * 2] < points[(j - 1) * 2]; j--)
              swap(points, j, j - 1);

      map = inst->csplineMap;
  } else {
      map = inst->bsplineMap;
  }

  int r, g, b, luma;
  double factorR, factorG, factorB, lumaValue;
  double rf, gf, bf, hue, sat, val;

  switch ((int)inst->channel) {
  case CHANNEL_RGB:
      while (len--) {
          *dst++ = map[*src++];        // r
          *dst++ = map[*src++];        // g
          *dst++ = map[*src++];        // b
          *dst++ = *src++;              // a
      }
      break;
  case CHANNEL_RED:
      memcpy(outframe, inframe, len*sizeof(uint32_t));
      while (len--) {
          *dst = map[*dst];
          dst += 4;
      }
      break;
  case CHANNEL_GREEN:
      memcpy(outframe, inframe, len*sizeof(uint32_t));
      dst += 1;
      while (len--) {
          *dst = map[*dst];
          dst += 4;
      }
      break;
  case CHANNEL_BLUE:
      memcpy(outframe, inframe, len*sizeof(uint32_t));
      dst += 2;
      while (len--) {
          *dst = map[*dst];
          dst += 4;
      }
      break;
  case CHANNEL_ALPHA:
      memcpy(outframe, inframe, len*sizeof(uint32_t));
      dst += 3;
      while (len--) {
          *dst = map[*dst];
          dst += 4;
      }
      break;
  case CHANNEL_LUMA:
      if (inst->formula) {      // Rec.709
          factorR = .2126;
          factorG = .7152;
          factorB = .0722;
      } else {                  // Rec. 601
          factorR = .299;
          factorG = .587;
          factorB = .114;
      }
      while (len--) {
          r = *src++;
          g = *src++;
          b = *src++;
          luma = ROUND(factorR * r + factorG * g + factorB * b);
          lumaValue = map[luma];
          if (luma == 0) {
              *dst++ = lumaValue;
              *dst++ = lumaValue;
              *dst++ = lumaValue;
          } else {
              *dst++ = CLAMP0255((int)(r * lumaValue));
              *dst++ = CLAMP0255((int)(g * lumaValue));
              *dst++ = CLAMP0255((int)(b * lumaValue));
          }
          *dst++ = *src++;
      }
      break;
  case CHANNEL_HUE:
      while (len--) {
          rf = *src++;
          gf = *src++;
          bf = *src++;
          RGBtoHSV(rf, gf, bf, &hue, &sat, &val);
          if (hue != -1) {
              HSVtoRGB(&rf, &gf, &bf, map[(int)hue], sat, val);
              *dst++ = rf * 255;
              *dst++ = gf * 255;
              *dst++ = bf * 255;
          } else {
              *dst++ = rf;
              *dst++ = gf;
              *dst++ = bf;
          }
          *dst++ = *src++;
      }
      break;
  case CHANNEL_SATURATION:
      while (len--) {
          rf = *src++;
          gf = *src++;
          bf = *src++;
          RGBtoHSV(rf, gf, bf, &hue, &sat, &val);
          HSVtoRGB(&rf, &gf, &bf, hue, map[(int)(sat * 255)], val);
          *dst++ = rf * 255;
          *dst++ = gf * 255;
          *dst++ = bf * 255;
          *dst++ = *src++;
      }
  }


  if (inst->drawCurves && !strlen(inst->bspline)) {
	unsigned char color[] = {0, 0, 0};
	if (inst->channel == CHANNEL_RED || inst->channel == CHANNEL_GREEN || inst->channel == CHANNEL_BLUE)
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
	for(i = 0; i < scale; i++) {
	  if (i % cellSize > lineWidth) //point doesn't aly on the grid
		for(int j = 0; j < scale; j++) {
		  if (j % cellSize > lineWidth) { //point doesn't aly on the grid
			int offset = ((maxYvalue - i + graphYOffset) * stride + j + graphXOffset) * 4;
			dst[offset] = (dst[offset] >> 1) + 0x7F;
			offset++;
			dst[offset] = (dst[offset] >> 1) + 0x7F;
			offset++;
			dst[offset] = (dst[offset] >> 1) + 0x7F;
			offset++;
		  }
		}
	}
	float doubleLineWidth = 4 * lineWidth;
	//drawing points on the graph
	for(i = 0; i < inst->pointNumber; i++) {
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
	float prevY = 0;
	for(int j = 0; j < scale; j++) {
	  float y = inst->curveMap[j];
	  if (j == 0 || y == prevY) {
		for(i = (int)floor(y - halfLineWidth); i <= ceil(y + halfLineWidth); i++) {
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
		for(i = ROUND(prevY - gap); factor * i < factor * (mid + gap); i += factor) {
		  int clampedI = i < 0?0:i >= scale?scale - 1:i;
		  int offset = ((maxYvalue - clampedI + graphYOffset) * stride + j - 1 + graphXOffset) * 4;
		  dst[offset++] = color[0];
		  dst[offset++] = color[1];
		  dst[offset++] = color[2];
		}
		  //drawing line from mid point to current value
		for(i = ROUND(mid - gap); factor * i < factor * ceil(y + gap); i += factor) {
		  int clampedI = i < 0?0:i >= scale?scale - 1:i;
		  int offset = ((maxYvalue - clampedI + graphYOffset) * stride + j + graphXOffset) * 4;
		  dst[offset++] = color[0];
		  dst[offset++] = color[1];
		  dst[offset++] = color[2];
		}
	  }
	  prevY = y;
	}
  }
}
