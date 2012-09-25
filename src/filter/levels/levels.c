/* levels.c
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

typedef struct levels_instance
{
  unsigned int width;
  unsigned int height;
  double inputMin; // 0 - 1
  double inputMax; // 0 - 1
  double outputMin; // 0 - 1
  double outputMax; // 0 - 1
  double gamma;
  double channel;
  double showHistogram;
  double histogramPosition;
} levels_instance_t;

int f0r_init()
{
  return 1;
}

void f0r_deinit()
{ /* no initialization required */ }

void f0r_get_plugin_info(f0r_plugin_info_t* levels_instance_t)
{
  levels_instance_t->name = "Levels";
  levels_instance_t->author = "Maksim Golovkin";
  levels_instance_t->plugin_type = F0R_PLUGIN_TYPE_FILTER;
  levels_instance_t->color_model = F0R_COLOR_MODEL_RGBA8888;
  levels_instance_t->frei0r_version = FREI0R_MAJOR_VERSION;
  levels_instance_t->major_version = 0; 
  levels_instance_t->minor_version = 3; 
  levels_instance_t->num_params = 8; 
  levels_instance_t->explanation = "Adjust luminance or color channel intensity";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
  switch(param_index)
  {
  case 0:
    info->name = "Channel";
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "Channel to adjust levels";
    break;
  case 1:
    info->name = "Input black level";
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "Input black level";
    break;
  case 2:
    info->name = "Input white level";
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "Input white level";
    break;
  case 3:
    info->name = "Gamma";
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "Gamma";
    break;
  case 4:
    info->name = "Black output";
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "Black output";
    break;
  case 5:
    info->name = "White output";
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "White output";
	break;
  case 6:
    info->name = "Show histogram";
    info->type = F0R_PARAM_BOOL;
    info->explanation = "Show histogram";
	break;
  case 7:
    info->name = "Histogram position";
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "Histogram position";
    break;
  }
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
  levels_instance_t* inst = (levels_instance_t*)calloc(1, sizeof(*inst));
  inst->width = width; inst->height = height;
  inst->inputMin = 0;
  inst->inputMax = 1;
  inst->outputMin = 0;
  inst->outputMax = 1;
  inst->gamma = 1;
  inst->channel = 0;
  inst->showHistogram = 1;
  inst->histogramPosition = 3;
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
  levels_instance_t* inst = (levels_instance_t*)instance;

  switch(param_index)
  {
	case 0:
	  inst->channel = floor(*((f0r_param_double *)param) * 10);
	  break;
	case 1:
	  inst->inputMin =  *((f0r_param_double *)param);
	  break;
	case 2:
	  inst->inputMax =  *((f0r_param_double *)param);
	  break;
	case 3:
	  inst->gamma = *((f0r_param_double *)param) * 4;
	  break;
	case 4:
	  inst->outputMin = *((f0r_param_double *)param);
	  break;
	case 5:
	  inst->outputMax = *((f0r_param_double *)param);
	  break;
	case 6:
	  inst->showHistogram = *((f0r_param_bool *)param);
	  break;
	case 7:
	  inst->histogramPosition = floor(*((f0r_param_double *)param) * 10);
	  break;
  }
}

void f0r_get_param_value(f0r_instance_t instance,
                         f0r_param_t param, int param_index)
{
  assert(instance);
  levels_instance_t* inst = (levels_instance_t*)instance;
  
  switch(param_index)
  {
  case 0:
	*((f0r_param_double *)param) = inst->channel / 10.;
	break;
  case 1:
	*((f0r_param_double *)param) = inst->inputMin;
	break;
  case 2:
	*((f0r_param_double *)param) = inst->inputMax;
	break;
  case 3:
	*((f0r_param_double *)param) = inst->gamma / 4;
	break;
  case 4:
	*((f0r_param_double *)param) = inst->outputMin;
	break;
  case 5:
	*((f0r_param_double *)param) = inst->outputMax;
	break;
  case 6:
	*((f0r_param_bool *)param) = inst->showHistogram;
	break;
  case 7:
	*((f0r_param_double *)param) = inst->histogramPosition / 10.;
	break;
  }
}

void f0r_update(f0r_instance_t instance, double time,
                const uint32_t* inframe, uint32_t* outframe)
{
  assert(instance);
  levels_instance_t* inst = (levels_instance_t*)instance;
  unsigned int len = inst->width * inst->height;
  unsigned int maxHisto = 0;
  
  unsigned char* dst = (unsigned char*)outframe;
  const unsigned char* src = (unsigned char*)inframe;
  int r, g, b;

  double levels[256];
  unsigned int map[256];

  double inScale = inst->inputMax != inst->inputMin?inst->inputMax - inst->inputMin:1;
  double exp = inst->gamma == 0?1:1/inst->gamma;
  double outScale = inst->outputMax - inst->outputMin;

  for(int i = 0; i < 256; i++) {
	double v = i / 255. - inst->inputMin;
	if (v < 0.0) {
		v = 0.0;
	}           
	double w = pow(v / inScale, exp) * outScale + inst->outputMin;
	map[i] = CLAMP0255(lrintf(w * 255.0));
  }

  if (inst->showHistogram)
	for(int i = 0; i < 256; i++)
	  levels[i] = 0;  

  while (len--)
  {
	r = *src++;
	g = *src++;
	b = *src++;

	if (inst->showHistogram) {
	  int intensity = 
	    inst->channel == CHANNEL_RED?r:
	    inst->channel == CHANNEL_GREEN?g:
	    inst->channel == CHANNEL_BLUE?b:
	        CLAMP0255(b * .114 + g * .587 + r * .299);
	  int index = CLAMP0255(intensity);
	  levels[index]++;
	  if (levels[index] > maxHisto)
		maxHisto = levels[index];
	}
	
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
	  *dst++ = map[r];
	  *dst++ = map[g];
	  *dst++ = map[b];
	  break;
	}

	*dst++ = *src++;  // copy alpha
  }
  if (inst->showHistogram) {
	dst = (unsigned char *)outframe;
	src = (unsigned char *)inframe;
	int thirdY = inst->height / 3;
	int thirdX = inst->width / 3;
	int barHeight = inst->height / 27;
	int histoHeight = thirdY - barHeight * 3;
	int xOffset = 0;
	int yOffset = 0;
	if (inst->histogramPosition == POS_BOTTOM_RIGHT || inst->histogramPosition == POS_BOTTOM_LEFT)
	  yOffset += 2 * thirdY;
	if (inst->histogramPosition == POS_BOTTOM_RIGHT || inst->histogramPosition == POS_TOP_RIGHT)
	  xOffset += 2 * thirdX;
	for(int y = 0; y < histoHeight; y++) {
	  double pointValue = (double)(histoHeight - y) / histoHeight;
	  for(int x = 0; x < thirdX; x++) {
		int offset = ((y + yOffset) * inst->width + x + xOffset) * 4;
		int drawPoint = pointValue < (double)levels[CLAMP0255(x * 255 / thirdX)] / maxHisto;
		dst[offset] = drawPoint?(CHANNEL_RED != inst->channel || CHANNEL_LUMA == inst->channel?0:255):127 + src[offset]/2;
		dst[offset + 1] = drawPoint?(CHANNEL_GREEN != inst->channel || CHANNEL_LUMA == inst->channel?0:255):127 + src[offset + 1]/2;
		dst[offset + 2] = drawPoint?(CHANNEL_BLUE != inst->channel || CHANNEL_LUMA == inst->channel?0:255):127 + src[offset + 2]/2;
		dst[offset + 3] = src[offset + 3];
	  }
	}
	int posInMin = inst->inputMin * thirdX;
	int posInMax = inst->inputMax * thirdX;
	int posOutMin = inst->outputMin * thirdX;
	int posOutMax = inst->outputMax * thirdX;
	double b = -7.0/12;
	double a = .5 - b;
	int posGamma = posInMin + (posInMax - posInMin) * pow(inst->gamma, .5) *.5;
	int color[3];
	color[0] = CHANNEL_RED == inst->channel || CHANNEL_LUMA == inst->channel?255:0;
	color[1] = CHANNEL_GREEN == inst->channel || CHANNEL_LUMA == inst->channel?255:0;
	color[2] = CHANNEL_BLUE == inst->channel || CHANNEL_LUMA == inst->channel?255:0;
	int midColor[3];
	midColor[0] = color[0]>>1;
	midColor[1] = color[1]>>1;
	midColor[2] = color[2]>>1;
	for(int y = histoHeight; y < thirdY - barHeight * 2; y++) {
	  int offsettedY = (y + yOffset) * inst->width; 
	  int offsettedYlower = (y + yOffset + barHeight * 2) * inst->width; 
	  for(int x = 0; x < thirdX; x++) {
		int offset = (offsettedY + x + xOffset) * 4;
		dst[offset] = 127 + dst[offset]/2;
		dst[offset + 1] = 127 + dst[offset + 1]/2;
		dst[offset + 2] = 127 + dst[offset + 2]/2;
		offset = (offsettedYlower + x + xOffset) * 4;
		dst[offset] = 127 + dst[offset]/2;
		dst[offset + 1] = 127 + dst[offset + 1]/2;
		dst[offset + 2] = 127 + dst[offset + 2]/2;
	  }
	  int delta = (y - histoHeight)/2;
	  
	  for(int x = -delta; x < delta; x++) {
		int xInMin = x + posInMin;
		int xInMax = x + posInMax;
		int xOutMin = x + posOutMin;
		int xOutMax = x + posOutMax;
		int xGamma = x + posGamma;
		if (xInMin >= 0 && xInMin < thirdX) {
		  int offset = (offsettedY + xInMin + xOffset) * 4;
		  dst[offset] = 0;
		  dst[offset + 1] = 0;
		  dst[offset + 2] = 0;
		}
		if (xInMax >= 0 && xInMax < thirdX) {
		  int offset = (offsettedY + xInMax + xOffset) * 4;
		  dst[offset] = color[0];
		  dst[offset + 1] = color[1];
		  dst[offset + 2] = color[2];
		}
		if (xGamma >= 0 && xGamma < thirdX) {
		  int offset = (offsettedY + xGamma + xOffset) * 4;
		  dst[offset] = midColor[0];
		  dst[offset + 1] = midColor[1];
		  dst[offset + 2] = midColor[2];
		}
		if (xOutMin >= 0 && xOutMin < thirdX) {
		  int offset = (offsettedYlower + xOutMin + xOffset) * 4;
		  dst[offset] = 0;
		  dst[offset + 1] = 0;
		  dst[offset + 2] = 0;
		}
		if (xOutMax >= 0 && xOutMax < thirdX) {
		  int offset = (offsettedYlower + xOutMax + xOffset) * 4;
		  dst[offset] = color[0];
		  dst[offset + 1] = color[1];
		  dst[offset + 2] = color[2];
		}
	  }
	}
	for(int y = thirdY - barHeight * 2; y < thirdY - barHeight; y++) {
	  for(int x = 0; x < thirdX; x++) {
		int offset = ((y + yOffset) * inst->width + x + xOffset) * 4;
		int pointValue = CLAMP0255(x * 255 / thirdX);
		dst[offset] = inst->channel == CHANNEL_RED || inst->channel == CHANNEL_LUMA?pointValue:0;
		dst[offset + 1] = inst->channel == CHANNEL_GREEN || inst->channel == CHANNEL_LUMA?pointValue:0;
		dst[offset + 2] = inst->channel == CHANNEL_BLUE || inst->channel == CHANNEL_LUMA?pointValue:0;
	  }
	}
  }
}
