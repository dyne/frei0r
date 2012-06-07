/* 
 * This file is a port of com.jhlabs.image.DitherFilter.java
 * Copyright 2006 Jerry Huxtable
 *
 * dither.c
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
#include <math.h>

#include "frei0r.h"
#include "frei0r_math.h"

int ditherMagic2x2Matrix[] = {
	 	 0, 2,
	 	 3, 1
	};

int ditherMagic4x4Matrix[] = {
	 	 0, 14,  3, 13,
		11,  5,  8,  6,
		12,  2, 15,  1,
		 7,  9,  4, 10
	};

int ditherOrdered4x4Matrix[] = {
	 	 0,  8,  2, 10,
		12,  4, 14,  6,
		 3, 11,  1,  9,
		15,  7, 13,  5
	};

int ditherLines4x4Matrix[] = {
	 	 0,  1,  2,  3,
		 4,  5,  6,  7,
		 8,  9, 10, 11,
		12, 13, 14, 15
	};

int dither90Halftone6x6Matrix[] = {
	 	29, 18, 12, 19, 30, 34,
		17,  7,  4,  8, 20, 28,
		11,  3,  0,  1,  9, 27,
		16,  6,  2,  5, 13, 26,
		25, 15, 10, 14, 21, 31,
		33, 25, 24, 23, 33, 36
	};

int ditherOrdered6x6Matrix[] = {
		 1, 59, 15, 55,  2, 56, 12, 52,
		33, 17, 47, 31, 34, 18, 44, 28,
		 9, 49,  5, 63, 10, 50,  6, 60,
		41, 25, 37, 21, 42, 26, 38, 22,
		 3, 57, 13, 53,  0, 58, 14, 54,
		35, 19, 45, 29, 32, 16, 46, 30,
		11, 51,  7, 61,  8, 48,  4, 62,
		43, 27, 39, 23, 40, 24, 36, 20 
	};

int ditherOrdered8x8Matrix[] = {
		  1,235, 59,219, 15,231, 55,215,  2,232, 56,216, 12,228, 52,212,
		129, 65,187,123,143, 79,183,119,130, 66,184,120,140, 76,180,116,
		 33,193, 17,251, 47,207, 31,247, 34,194, 18,248, 44,204, 28,244,
		161, 97,145, 81,175,111,159, 95,162, 98,146, 82,172,108,156, 92,
		  9,225, 49,209,  5,239, 63,223, 10,226, 50,210,  6,236, 60,220,
		137, 73,177,113,133, 69,191,127,138, 74,178,114,134, 70,188,124,
		 41,201, 25,241, 37,197, 21,255, 42,202, 26,242, 38,198, 22,252,
		169,105,153, 89,165,101,149, 85,170,106,154, 90,166,102,150, 86,
		  3,233, 57,217, 13,229, 53,213,  0,234, 58,218, 14,230, 54,214,
		131, 67,185,121,141, 77,181,117,128, 64,186,122,142, 78,182,118,
		 35,195, 19,249, 45,205, 29,245, 32,192, 16,250, 46,206, 30,246,
		163, 99,147, 83,173,109,157, 93,160, 96,144, 80,174,110,158, 94,
		 11,227, 51,211,  7,237, 61,221,  8,224, 48,208,  4,238, 62,222,
		139, 75,179,115,135, 71,189,125,136, 72,176,112,132, 68,190,126,
		 43,203, 27,243, 39,199, 23,253, 40,200, 24,240, 36,196, 20,254,
		171,107,155, 91,167,103,151, 87,168,104,152, 88,164,100,148, 84 };

int ditherCluster3Matrix[] = {
		 9,11,10, 8, 6, 7,
		12,17,16, 5, 0, 1,
		13,14,15, 4, 3, 2,
		 8, 6, 7, 9,11,10,
		 5, 0, 1,12,17,16,
		 4, 3, 2,13,14,15 };

int ditherCluster4Matrix[] = {
		18,20,19,16,13,11,12,15,
		27,28,29,22, 4, 3, 2, 9,
		26,31,30,21, 5, 0, 1,10,
		23,25,24,17, 8, 6, 7,14,
		13,11,12,15,18,20,19,16,
		 4, 3, 2, 9,27,28,29,22,
		 5, 0, 1,10,26,31,30,21,
		 8, 6, 7,14,23,25,24,17 };

int ditherCluster8Matrix[] = {
		 64, 69, 77, 87, 86, 76, 68, 67, 63, 58, 50, 40, 41, 51, 59, 60,
		 70, 94,100,109,108, 99, 93, 75, 57, 33, 27, 18, 19, 28, 34, 52,
		 78,101,114,116,115,112, 98, 83, 49, 26, 13, 11, 12, 15, 29, 44,
		 88,110,123,124,125,118,107, 85, 39, 17,  4,  3,  2,  9, 20, 42,
		 89,111,122,127,126,117,106, 84, 38, 16,  5,  0,  1, 10, 21, 43,
		 79,102,119,121,120,113, 97, 82, 48, 25,  8,  6,  7, 14, 30, 45,
		 71, 95,103,104,105, 96, 92, 74, 56, 32, 24, 23, 22, 31, 35, 53,
		 65, 72, 80, 90, 91, 81, 73, 66, 62, 55, 47, 37, 36, 46, 54, 61,
		 63, 58, 50, 40, 41, 51, 59, 60, 64, 69, 77, 87, 86, 76, 68, 67,
		 57, 33, 27, 18, 19, 28, 34, 52, 70, 94,100,109,108, 99, 93, 75,
		 49, 26, 13, 11, 12, 15, 29, 44, 78,101,114,116,115,112, 98, 83,
		 39, 17,  4,  3,  2,  9, 20, 42, 88,110,123,124,125,118,107, 85,
		 38, 16,  5,  0,  1, 10, 21, 43, 89,111,122,127,126,117,106, 84,
		 48, 25,  8,  6,  7, 14, 30, 45, 79,102,119,121,120,113, 97, 82,
		 56, 32, 24, 23, 22, 31, 35, 53, 71, 95,103,104,105, 96, 92, 74,
		 62, 55, 47, 37, 36, 46, 54, 61, 65, 72, 80, 90, 91, 81, 73, 66 };

int matrixSizes[] = {4, 16, 16, 16, 36, 64, 256, 36, 64, 256};
int* matrixes[] = {ditherMagic2x2Matrix, ditherMagic4x4Matrix, ditherOrdered4x4Matrix, 
                  ditherLines4x4Matrix, dither90Halftone6x6Matrix, ditherOrdered6x6Matrix, 
                  ditherOrdered8x8Matrix, ditherCluster3Matrix, ditherCluster4Matrix, ditherCluster8Matrix};

typedef struct dither_instance
{
  unsigned int width;
  unsigned int height;
  double levels;
  double matrixid;
} dither_instance_t;

int f0r_init()
{
  return 1;
}

void f0r_deinit()
{ /* no initialization required */ }

void f0r_get_plugin_info(f0r_plugin_info_t* dither_info)
{
  dither_info->name = "dither";
  dither_info->author = "Janne Liljeblad";
  dither_info->plugin_type = F0R_PLUGIN_TYPE_FILTER;
  dither_info->color_model = F0R_COLOR_MODEL_RGBA8888;
  dither_info->frei0r_version = FREI0R_MAJOR_VERSION;
  dither_info->major_version = 0; 
  dither_info->minor_version = 1; 
  dither_info->num_params =  2; 
  dither_info->explanation = "Dithers the image and reduces the number of available colors";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
  switch(param_index)
  {
  case 0:
    info->name = "levels";
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "Number of values per channel";
    break;
  case 1:
    info->name = "matrixid";
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "Id of matrix used for dithering";
    break;
  }
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
	dither_instance_t* inst = (dither_instance_t*)calloc(1, sizeof(*inst));
	inst->width = width; 
  inst->height = height;
	inst->levels = 5.0 / 48.0;// input range 0.0 - 1.0 will be interpreted as levels range 2 - 50
  inst->matrixid = 1.0; // input range 0.0 - 1.0 will be interpreted as matrixid 0 - 9
                        // e.g. values 0.0, 0.12, 0.23, 0.34, 0.45, 0.56, 0.67, 0.78, 0.89, 1.0
                        // will select matrixes 0 to 9
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
  dither_instance_t* inst = (dither_instance_t*)instance;

  switch(param_index)
  {
  case 0:
    inst->levels = *((double*)param);
    break;
  case 1:
    inst->matrixid = *((double*)param);
    break;
  }
}

void f0r_get_param_value(f0r_instance_t instance,
                         f0r_param_t param, int param_index)
{
  assert(instance);
  dither_instance_t* inst = (dither_instance_t*)instance;
  
  switch(param_index)
  {
  case 0:
    *((double*)param) = inst->levels;
    break;
  case 1:
    *((double*)param) = inst->matrixid;
    break;
  }
}

void f0r_update(f0r_instance_t instance, double time,
                const uint32_t* inframe, uint32_t* outframe)
{
  //init and get params
  assert(instance);
  dither_instance_t* inst = (dither_instance_t*)instance;
  unsigned char* dst = (unsigned char*)outframe;
  const unsigned char* src = (unsigned char*)inframe;

  double levelsInput = inst->levels * 48.0;
  levelsInput = CLAMP(levelsInput, 0.0, 48.0) + 2.0;
  int levels = (int)levelsInput;

  double matrixIdInput = inst->matrixid * 9.0;
  matrixIdInput = CLAMP(matrixIdInput, 0.0, 9.0);
  int matrixid = (int)matrixIdInput;
  int* matrix = matrixes[matrixid];
  int matrixLength = matrixSizes[matrixid];

  // init look-ups
	int rows, cols;
  rows = cols = (int)sqrt(matrixLength);
  int map[levels];
  int i,v;
	for (i = 0; i < levels; i++)
  {
		v = 255 * i / (levels-1);
		map[i] = v;
	}

	int div[256];
	int mod[256];
	int rc = (rows * cols + 1);
	for (i = 0; i < 256; i++)
  {
		div[i] = (levels-1) * i / 256;
		mod[i] = i * rc /256;
	}

  // filter image
  unsigned int width = inst->width;
  unsigned int height = inst->height;
  unsigned int x,y,col,row;
  unsigned char r, g, b;
  for (y = 0; y < height; ++y)
  {
      for (x=0; x < width; ++x)
      {
        r = *src++;
        g = *src++;
        b = *src++;

	      col = x % cols;
	      row = y % rows;

	      v = matrix[ row * cols + col];
	      r = map[mod[r] > v ? div[r] + 1 : div[r]];
		    g = map[mod[g] > v ? div[g] + 1 : div[g]];
		    b = map[mod[b] > v ? div[b] + 1 : div[b]];

        *dst++ = r;
        *dst++ = g;
        *dst++ = b;
        *dst++ = *src++;//copy alpha
      }
  }
}


