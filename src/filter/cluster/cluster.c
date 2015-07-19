/* cluster.c
 * Copyright (C) 2008 binarymillenium
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

#define MAXNUM 40

struct cluster_center
{
	int x;
	int y;


	unsigned char r;
	unsigned char g;
	unsigned char b;


	/// aggregate color and positions
	float aggr_r;
	float aggr_g;
	float aggr_b;
	float aggr_x;
	float aggr_y;
	
	/// number of pixels in the cluster
	float numpix; 
};

typedef struct cluster_instance
{
  unsigned int width;
  unsigned int height;

	/// number of clusters, must be smaller than maxnum
	unsigned int num;
	float dist_weight;
	//float color_weight;

	struct cluster_center clusters[MAXNUM];

	int initted;
} cluster_instance_t;


int f0r_init()
{
  return 1;
}

void f0r_deinit()
{ /* no initialization required */ }

void f0r_get_plugin_info(f0r_plugin_info_t* inverterInfo)
{
  inverterInfo->name = "K-Means Clustering";
  inverterInfo->author = "binarymillenium";
  inverterInfo->plugin_type = F0R_PLUGIN_TYPE_FILTER;
  inverterInfo->color_model = F0R_COLOR_MODEL_RGBA8888;
  inverterInfo->frei0r_version = FREI0R_MAJOR_VERSION;
  inverterInfo->major_version = 0; 
  inverterInfo->minor_version = 1; 
  inverterInfo->num_params =  2; 
  inverterInfo->explanation = "Clusters of a source image by color and spatial distance";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
  switch(param_index)
  {
  case 0:
    info->name = "Num";
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "The number of clusters";
    break;
 case 1:
    info->name = "Dist weight";
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "The weight on distance";
    break;
	#if 0
 case 2:
    info->name = "Color weight";
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "The weight on color";
    break;
#endif

  }
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
	cluster_instance_t* inst = (cluster_instance_t*)calloc(1, sizeof(*inst));

	inst->width = width; inst->height = height;

	inst->num = MAXNUM/2;
	inst->dist_weight = 0.5;
	//inst->color_weight = 1.0;

	int k;
	for (k = 0; k < MAXNUM; k++) {
		struct cluster_center* cc = &inst->clusters[k];	

		int x = rand()%inst->width;
		int y = rand()%inst->height;

		cc->x = x;
		cc->y = y;

/*
		const unsigned char* src2 = (unsigned char*)(&inframe[x+inst->width*y]);

		inst->clusters[k].r = src2[0];
		inst->clusters[k].g = src2[1];
		inst->clusters[k].b = src2[2];
*/
		inst->clusters[k].r = rand()%255;
		inst->clusters[k].g = rand()%255;
		inst->clusters[k].b = rand()%255;


		cc->numpix = 0;
		cc->aggr_x = 0;
		cc->aggr_y = 0;
		cc->aggr_r = 0;
		cc->aggr_g = 0;
		cc->aggr_b = 0;
	}		 

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
  cluster_instance_t* inst = (cluster_instance_t*)instance;

  switch(param_index)
  {
    int val;
	float fval;
  case 0:
    /* val is 0-1.0 */
	fval =  ((*((double*)param) )); 

	val = (int) (fval*MAXNUM);

	if (val > MAXNUM) val = MAXNUM;
    if (val < 0) val = 0;

	if (val != inst->num)
    {
      inst->num = val;
    }
    break;

	 case 1:
    /* val is 0-1.0 */
	//fval =  2.0 * ((*((double*)param) ) - 0.5); 
	fval =  ((*((double*)param) ) ); 

	if (fval != inst->dist_weight)
    {
      inst->dist_weight = fval;
    }
    break;

#if 0
	 case 2:
    /* val is 0-1.0 */
	//fval =  2.0 * ((*((double*)param) ) - 0.5); 
	fval =  ((*((double*)param) ) ); 


	if (fval != inst->color_weight)
    {
      inst->color_weight = fval;
    }
    break;
#endif
  }
}

void f0r_get_param_value(f0r_instance_t instance,
                         f0r_param_t param, int param_index)
{
  assert(instance);
  cluster_instance_t* inst = (cluster_instance_t*)instance;
  
  switch(param_index)
  {
  case 0:
    *((double*)param) = (double) ( (inst->num)  )/MAXNUM;
    break;
  case 1:
    *((double*)param) = (double) ( (inst->dist_weight));
    break;

  }
}

float find_dist(
		int r1, int g1, int b1, int x1, int y1,
		int r2, int g2, int b2, int x2, int y2,
		float max_space_dist, float dist_weight) //, float color_weight)
{
	/// make this a define?
	float max_color_dist = sqrtf(255*255*3);

	float dr = r1-r2;
	float dg = g1-g2;
	float db = b1-b2;

	float color_dist = sqrtf(dr*dr + dg*dg + db*db)/max_color_dist; 

	float dx = x1-x2;
	float dy = y1-y2;
	float space_dist = sqrtf(dx*dx + dy*dy)/max_space_dist;

	/// add parameter weighting later
	//return sqrtf(color_weight*color_dist*color_dist + dist_weight*space_dist*space_dist);
	return sqrtf((1.0-dist_weight)*color_dist*color_dist + dist_weight*space_dist*space_dist);
}

void f0r_update(f0r_instance_t instance, double time,
                const uint32_t* inframe, uint32_t* outframe)
{
  assert(instance);
  cluster_instance_t* inst = (cluster_instance_t*)instance;
  
  int x,y,k;
 
	float max_space_dist = sqrtf(inst->width*inst->width + inst->height*inst->height);

	/*
	 if (inst->has_initted) {
		inst->has_initted = true;
	 }
	 
	 */

  
  for (y=0; y < inst->height; ++y) {
  for (x=0; x < inst->width; ++x) {

	  const unsigned char* src2 = (unsigned char*)( &inframe[x+inst->width*y]);
	  unsigned char* dst2 		= (unsigned char*)(&outframe[x+inst->width*y]);

	  float dist = max_space_dist;
	  int dist_ind = 0;

	  for (k = 0; k < inst->num; k++) {
		  struct cluster_center cc = inst->clusters[k];	

		  float kdist = find_dist(src2[0], src2[1], src2[2], x,y, 
				  cc.r, cc.g, cc.b, cc.x, cc.y,
				  max_space_dist, inst->dist_weight); //, inst->color_weight);

		  if (kdist < dist) {
			  dist = kdist;
			  dist_ind = k;
		  }
	  }

	  struct cluster_center* cc = &inst->clusters[dist_ind];	
	  cc->aggr_x += x;
	  cc->aggr_y += y;
	  cc->aggr_r += src2[0];
	  cc->aggr_g += src2[1];
	  cc->aggr_b += src2[2];
	  cc->numpix += 1.0;

	  dst2[0] = cc->r;
	  dst2[1] = cc->g;
	  dst2[2] = cc->b;
	  dst2[3] = src2[3];

	

  }
  }

  /// update cluster_centers
  for (k = 0; k < inst->num; k++) {

	  struct cluster_center* cc = &inst->clusters[k];	

	  if (cc->numpix > 0) {
		  cc->x = (int)  (cc->aggr_x/cc->numpix);
		  cc->y = (int)  (cc->aggr_y/cc->numpix);
		  cc->r = (unsigned char) (cc->aggr_r/cc->numpix);
		  cc->g = (unsigned char) (cc->aggr_g/cc->numpix);
		  cc->b = (unsigned char) (cc->aggr_b/cc->numpix);

			//printf("%d, %d %d %d\t", k, (unsigned int)cc->r, (unsigned int)cc->g, (unsigned int)cc->b);
			//printf("%g, %g %g %g\n", cc->numpix, cc->aggr_r, cc->aggr_g, cc->aggr_b);
	  }
	
	  cc->numpix = 0;
	  cc->aggr_x = 0;
	  cc->aggr_y = 0;
	  cc->aggr_r = 0;
	  cc->aggr_g = 0;
	  cc->aggr_b = 0;

  	}
	//printf("\n");

}

