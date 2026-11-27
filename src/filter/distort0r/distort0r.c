#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#if defined(_MSC_VER)
#define _USE_MATH_DEFINES
#endif
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
  double amplitude, frequency, change_speed;
  grid_point_t* grid;
  double* dx_tab;
  double* sin_x_tab;
  double time_stack;
  double mode;
} distorter_instance_t;

//const double AMPLTUDE_SCALE = 10.0;
static const double FREQUENCY_SCALE = 200.0;
static const double SPEED_SCALE = 2.0;

static void interpolateGrid(const grid_point_t* __restrict__ grid, unsigned int w, unsigned int h,
                            const uint32_t* __restrict__ src, uint32_t* __restrict__ dst);

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
  distorterInfo->color_model = F0R_COLOR_MODEL_PACKED32;
  distorterInfo->frei0r_version = FREI0R_MAJOR_VERSION;
  distorterInfo->major_version = 0; 
  distorterInfo->minor_version = 10;
  distorterInfo->num_params =  4;
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
    case 2:
      info->name = "Use Velocity";
      info->type = F0R_PARAM_BOOL;
      info->explanation = "\'Time Based\' or \'Adjustable Velocity\'";
      break;
    case 3:
      info->name = "Velocity";
      info->type = F0R_PARAM_DOUBLE;
      info->explanation = "Changing speed of the plasma signal";
      break;
    }
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
  distorter_instance_t* inst = (distorter_instance_t*)calloc(1, sizeof(*inst));
  inst->width = width; inst->height = height;
  unsigned int gx_count = (width / GRID_SIZE) + 1;
  unsigned int gy_count = (height / GRID_SIZE) + 1;
  inst->grid = (grid_point_t*)malloc(sizeof(grid_point_t) * gx_count * gy_count);
  inst->dx_tab = (double*)malloc(sizeof(double) * gx_count);
  inst->sin_x_tab = (double*)malloc(sizeof(double) * gx_count);
  inst->amplitude = 1.0;
  inst->frequency = 1.0;
  inst->change_speed = 1.0;
  inst->time_stack = 0.0;
  inst->mode = 0.0;
  return (f0r_instance_t)inst;
}

void f0r_destruct(f0r_instance_t instance)
{
  distorter_instance_t* inst = (distorter_instance_t*)instance;
  free(inst->dx_tab);
  free(inst->sin_x_tab);
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
      // don't scale
      inst->amplitude = *((double*)param);
      break;
    case 1:
      // scale
      inst->frequency = *((double*)param) * FREQUENCY_SCALE;
      break;
    case 2:
      inst->mode = *((f0r_param_bool *)param);
      break;
    case 3:
      // scale
      inst->change_speed = *((double*)param) * SPEED_SCALE;
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
    case 2:
      *((f0r_param_bool *)param) = inst->mode;
      break;
    case 3:
      // scale to [0..1]
      scaled = inst->change_speed / SPEED_SCALE;
      *((double*)param) = scaled;
      break;
    }

}

void f0r_update(f0r_instance_t instance, double time,
                const uint32_t* inframe, uint32_t* outframe)
{
  assert(instance);
  distorter_instance_t* inst = (distorter_instance_t*)instance;
  const unsigned int w = inst->width;
  const unsigned int h = inst->height;

  inst->time_stack += inst->change_speed;

  const double amp = inst->amplitude;
  const double freq = inst->frequency;
  const double t_mod = fmod(inst->mode ? inst->time_stack : time, 2 * M_PI);
  const double h_ = (double)h - 1;
  const double w_ = (double)w - 1;
  const double amp_w = amp * (w / 4);
  const double amp_h = amp * (h / 4);

  /* Precompute 1D x-dependent terms (reduces sin() calls from O(gx*gy) to O(gx+gy)) */
  double* __restrict__ dx_tab = inst->dx_tab;
  double* __restrict__ sin_x_tab = inst->sin_x_tab;
  unsigned int gx_count = 0;
  for (unsigned int x = 0; x <= w; x += GRID_SIZE, ++gx_count)
    {
      dx_tab[gx_count] = (-4. / (w_ * w_) * x + 4. / w_) * x;
      sin_x_tab[gx_count] = sin(freq * x / w + t_mod);
    }

  grid_point_t* __restrict__ pt = inst->grid;
  for (unsigned int y = 0; y <= h; y += GRID_SIZE)
    {
      const double dy = (-4. / (h_ * h_) * y + 4. / h_) * y;
      const double sin_y = sin(freq * y / h + t_mod);
      const double amp_h_dy = amp_h * dy;
      const double y_d = (double)y;

      unsigned int gx = 0;
      for (unsigned int x = 0; x <= w; x += GRID_SIZE, ++gx, ++pt)
        {
          pt->u = (int32_t)(65536.0 * ((double)x + amp_w * dx_tab[gx] * sin_y));
          pt->v = (int32_t)(65536.0 * (y_d + amp_h_dy * sin_x_tab[gx]));
        }
    }

  interpolateGrid(inst->grid, w, h, inframe, outframe);
}

static void interpolateGrid(const grid_point_t* __restrict__ grid, unsigned int w, unsigned int h,
                            const uint32_t* __restrict__ src, uint32_t* __restrict__ dst)
{
  const unsigned int grid_x = (w / GRID_SIZE);
  const unsigned int grid_y = (h / GRID_SIZE);
  const unsigned int grid_stride = grid_x + 1;

  for (unsigned int y = 0; y < grid_y; ++y)
    {
      const grid_point_t* row_top = grid + (size_t)y * grid_stride;
      const grid_point_t* row_bot = row_top + grid_stride;
      uint32_t* dst_band = dst + (size_t)(y << GRID_SIZE_LOG) * w;

      for (unsigned int x = 0; x < grid_x; ++x)
        {
          const grid_point_t ul = row_top[x];
          const grid_point_t ur = row_top[x + 1];
          const grid_point_t ll = row_bot[x];
          const grid_point_t lr = row_bot[x + 1];

          int32_t start_col_uu = ul.u;
          int32_t start_col_vv = ul.v;
          int32_t end_col_uu   = ur.u;
          int32_t end_col_vv   = ur.v;

          const int32_t step_start_col_u = (ll.u - ul.u) >> GRID_SIZE_LOG;
          const int32_t step_start_col_v = (ll.v - ul.v) >> GRID_SIZE_LOG;
          const int32_t step_end_col_u   = (lr.u - ur.u) >> GRID_SIZE_LOG;
          const int32_t step_end_col_v   = (lr.v - ur.v) >> GRID_SIZE_LOG;

          uint32_t* __restrict__ pos = dst_band + (x << GRID_SIZE_LOG);

          for (unsigned int block_y = 0; block_y < GRID_SIZE; ++block_y)
            {
              int32_t u_idx = start_col_uu;
              int32_t v_idx = start_col_vv;
              const int32_t step_u = (end_col_uu - start_col_uu) >> GRID_SIZE_LOG;
              const int32_t step_v = (end_col_vv - start_col_vv) >> GRID_SIZE_LOG;

              const unsigned int u0 = (unsigned int)(u_idx >> 16);
              const unsigned int u7 = (unsigned int)((u_idx + 7 * step_u) >> 16);
              const unsigned int v0 = (unsigned int)(v_idx >> 16);
              const unsigned int v7 = (unsigned int)((v_idx + 7 * step_v) >> 16);

              if (u0 < w && u7 < w && v0 < h && v7 < h)
                {
                  pos[0] = src[u0 + (size_t)v0 * w];
                  u_idx += step_u; v_idx += step_v;
                  pos[1] = src[(unsigned int)(u_idx >> 16) + (size_t)(unsigned int)(v_idx >> 16) * w];
                  u_idx += step_u; v_idx += step_v;
                  pos[2] = src[(unsigned int)(u_idx >> 16) + (size_t)(unsigned int)(v_idx >> 16) * w];
                  u_idx += step_u; v_idx += step_v;
                  pos[3] = src[(unsigned int)(u_idx >> 16) + (size_t)(unsigned int)(v_idx >> 16) * w];
                  u_idx += step_u; v_idx += step_v;
                  pos[4] = src[(unsigned int)(u_idx >> 16) + (size_t)(unsigned int)(v_idx >> 16) * w];
                  u_idx += step_u; v_idx += step_v;
                  pos[5] = src[(unsigned int)(u_idx >> 16) + (size_t)(unsigned int)(v_idx >> 16) * w];
                  u_idx += step_u; v_idx += step_v;
                  pos[6] = src[(unsigned int)(u_idx >> 16) + (size_t)(unsigned int)(v_idx >> 16) * w];
                  u_idx += step_u; v_idx += step_v;
                  pos[7] = src[u7 + (size_t)v7 * w];
                }
              else
                {
                  for (unsigned int block_x = 0; block_x < GRID_SIZE; ++block_x)
                    {
                      unsigned int uu = (unsigned int)(u_idx >> 16);
                      unsigned int vv = (unsigned int)(v_idx >> 16);
                      u_idx += step_u;
                      v_idx += step_v;
                      pos[block_x] = (uu < w && vv < h) ? src[uu + (size_t)vv * w] : 0u;
                    }
                }

              start_col_uu += step_start_col_u;
              end_col_uu   += step_end_col_u;
              start_col_vv += step_start_col_v;
              end_col_vv   += step_end_col_v;

              pos += w;
            }
        }
    }
}
