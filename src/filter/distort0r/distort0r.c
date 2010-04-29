#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include "frei0r.h"


#define GRID_SIZE_LOG 3
#define GRID_SIZE (1<<GRID_SIZE_LOG)

typedef struct grid_point
{
  int32_t u;
  int32_t v;
} grid_point_t;


typedef struct distorter_instance
{
  unsigned int width, height;
  double amplitude, frequency;
  grid_point_t* grid;
} distorter_instance_t;

//const double AMPLTUDE_SCALE = 10.0;
const double FREQUENCY_SCALE = 200.0;

void interpolateGrid(grid_point_t* grid, unsigned int w, unsigned int h,
		     const uint32_t* src, uint32_t* dst);

int f0r_init()
{
  return 1;
}

void f0r_deinit()
{}

void f0r_get_plugin_info(f0r_plugin_info_t* distorterInfo)
{
  distorterInfo->name = "Distort0r";
  distorterInfo->author = "Gephex crew";
  distorterInfo->plugin_type = F0R_PLUGIN_TYPE_FILTER;
  distorterInfo->color_model = F0R_COLOR_MODEL_BGRA8888;
  distorterInfo->frei0r_version = FREI0R_MAJOR_VERSION;
  distorterInfo->major_version = 0; 
  distorterInfo->minor_version = 9; 
  distorterInfo->num_params =  2; 
  distorterInfo->explanation = "Plasma";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
  switch(param_index)
    {
    case 0:
      info->name = "Amplitude";
      info->type = F0R_PARAM_DOUBLE;
      info->explanation = "The amplitude of the plasma signal";
      break;
    case 1:
      info->name = "Frequency";
      info->type = F0R_PARAM_DOUBLE;
      info->explanation = "The frequency of the plasma signal";
      break;
    }
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
  distorter_instance_t* inst = (distorter_instance_t*)calloc(1, sizeof(*inst));
  inst->width = width; inst->height = height;
  inst->grid = 
    (grid_point_t*)malloc(sizeof(grid_point_t)*
			  ((width/GRID_SIZE)+1)*((height/GRID_SIZE)+1));
  inst->amplitude = 1.0;
  inst->frequency = 1.0;
  return (f0r_instance_t)inst;
}

void f0r_destruct(f0r_instance_t instance)
{
  distorter_instance_t* inst = (distorter_instance_t*)instance;
  free(inst->grid);
  free(inst);
}

void f0r_set_param_value(f0r_instance_t instance, 
			 f0r_param_t param, int param_index)
{ 
  assert(instance);
  distorter_instance_t* inst = (distorter_instance_t*)instance;

  switch(param_index)
    {
    case 0:
      // scale
      inst->amplitude = *((double*)param);
      break;
    case 1:
      // scale
      inst->frequency = *((double*)param) * FREQUENCY_SCALE;
      break;
    }  
}

void f0r_get_param_value(f0r_instance_t instance,
			 f0r_param_t param, int param_index)
{
  double scaled;

  assert(instance);
  distorter_instance_t* inst = (distorter_instance_t*)instance;
 

  switch(param_index)
    {
    case 0:
      // don't scale
      *((double*)param) = inst->amplitude;
      break;
    case 1:
      // scale to [0..1]
      scaled = inst->frequency / FREQUENCY_SCALE;
      *((double*)param) = scaled;
      break;
    }

}

/* this will compute a displacement value such that 
   0<=x_retval<xsize and 0<=y_retval<ysize. */ 
static inline void plasmaFunction
(int32_t* x_retval, int32_t* y_retval, 
 unsigned int x, unsigned int y,
 unsigned int w, unsigned int h,
 double amp, double freq, double t)
{
  double time = fmod(t, 2*M_PI);
  double h_ = (double)h -1; double w_ = (double)w-1;
  double dx = (-4./(w_*w_)*x + 4./w_)*x;
  double dy = (-4./(h_*h_)*y + 4./h_)*y;
  *x_retval = (int32_t)(65536.0*((double)x+amp*(w/4)*dx*sin(freq*y/h + time)));
  *y_retval = (int32_t)(65536.0*((double)y+amp*(h/4)*dy*sin(freq*x/w + time)));

}

void f0r_update(f0r_instance_t instance, double time,
		const uint32_t* inframe, uint32_t* outframe)
{
  assert(instance);
  distorter_instance_t* inst = (distorter_instance_t*)instance;
  unsigned int w = inst->width;
  unsigned int h = inst->height;
  unsigned int x,y;
  
  grid_point_t* pt = inst->grid;
  for(y=0;y<=h;y+= GRID_SIZE)    
      for(x=0;x<=w;x+=GRID_SIZE,++pt)
	{
	  plasmaFunction(&pt->u, &pt->v, x, y, w, h, 
			 inst->amplitude, inst->frequency, time);
	}

  interpolateGrid(inst->grid, w, h, inframe, outframe);
}

void interpolateGrid(grid_point_t* grid, unsigned int w, unsigned int h,
		     const uint32_t* src, uint32_t* dst)
{
  unsigned int x, y, block_x, block_y;
  unsigned int tex_x = 0, tex_y = 0;
  unsigned int grid_x = (w / GRID_SIZE);
  unsigned int grid_y = (h / GRID_SIZE);
  for(y=0, tex_y=0; y < grid_y; y++)
    {
      for(x=0, tex_x=0; x < grid_x; x++)
	{
	  unsigned int offset = x + y*(grid_x+1); 
	  
	  grid_point_t* upper_left  = grid + offset; 
	  grid_point_t* lower_left  = grid + offset + grid_x + 1; 
	  grid_point_t* upper_right = grid + offset + 1; 
	  grid_point_t* lower_right = grid + offset + grid_x + 2; 
	  
	  int32_t u_left, u_right, v_left, v_right; 
	  
	  int32_t start_col_uu = upper_left->u; 
	  int32_t start_col_vv = upper_left->v; 
	  int32_t end_col_uu   = upper_right->u; 
	  int32_t end_col_vv   = upper_right->v; 
	  
	  int32_t step_start_col_u = (lower_left->u - upper_left->u) 
	    >> GRID_SIZE_LOG; 
	  int32_t step_start_col_v = (lower_left->v - upper_left->v) 
	    >> GRID_SIZE_LOG; 
	  int32_t step_end_col_u   = (lower_right->u - upper_right->u) 
	    >> GRID_SIZE_LOG; 
	  int32_t step_end_col_v   = (lower_right->v - upper_right->v) 
	    >> GRID_SIZE_LOG; 
	  
	  int32_t u_line_index, v_line_index; 
	  int32_t step_line_u, step_line_v; 
	  
	  
	  uint32_t* pos = dst+ (y<<GRID_SIZE_LOG)*w + (x<<GRID_SIZE_LOG);
      
	  for(block_y = 0; block_y < GRID_SIZE; ++block_y) 
	    { 
	      u_left  = start_col_uu; 
	      u_right = end_col_uu; 
	      v_left  = start_col_vv; 
	      v_right = end_col_vv; 
	      
	      u_line_index = start_col_uu; 
	      v_line_index = start_col_vv; 
	      
	      step_line_u = (int32_t) ((u_right-u_left) >> GRID_SIZE_LOG); 
	      step_line_v = (int32_t) ((v_right-v_left) >> GRID_SIZE_LOG); 
	      
	      for(block_x=0; block_x < GRID_SIZE; ++block_x) 
		{ 
		  int uu = u_line_index >> 16;
		  int vv = v_line_index >> 16;
		  
		  u_line_index += step_line_u; 
		  v_line_index += step_line_v; 
		  
		  *pos++ = src[uu + vv * w]; 
		} 
	      
	      start_col_uu += step_start_col_u; 
	      end_col_uu   += step_end_col_u; 
	      start_col_vv += step_start_col_v; 
	      end_col_vv   += step_end_col_v; 
	      
	      pos += (w - GRID_SIZE); 
	    }   
	}
    }
}
