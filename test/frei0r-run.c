/* This file is part of frei0r (https://frei0r.dyne.org)
 *
 * Copyright (C) 2024-2025 Dyne.org foundation
 * designed, written and maintained by Denis Roio <jaromil@dyne.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include <unistd.h>
#include <libgen.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <frei0r.h>
#include "test-pattern.h"

#if defined(__linux__) && defined(GUI)
#pragma message "Compiling GUI test"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

// frei0r function prototypes
typedef int (*f0r_init_f)(void);
typedef void (*f0r_deinit_f)(void);
typedef void (*f0r_get_plugin_info_f)(f0r_plugin_info_t *info);
typedef void (*f0r_get_param_info_f)(f0r_param_info_t *info, int param_index);
typedef f0r_instance_t (*f0r_construct_f)(unsigned int width, unsigned int height);
typedef void (*f0r_update_f)(f0r_instance_t instance,
    double time, const uint32_t* inframe, uint32_t* outframe);
typedef void (*f0r_update2_f)(f0r_instance_t instance, double time,
    const uint32_t* inframe1, const uint32_t* inframe2,
    const uint32_t* inframe3, uint32_t* outframe);
typedef void (*f0r_destruct_f)(f0r_instance_t instance);
typedef void (*f0r_set_param_value_f)(f0r_instance_t instance, f0r_param_t param, int param_index);
typedef void (*f0r_get_param_value_f)(f0r_instance_t instance, f0r_param_t param, int param_index);


// Generate a simple color bar test pattern
void generate_test_pattern(uint32_t* frame, int width, int height, int color_model, int pattern_variant) {
    // Create color bars: red, green, blue, white, black, cyan, magenta, yellow
    int bar_width = width / 8;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int bar_index = x / bar_width;
            if (bar_index >= 8) bar_index = 7;

            // Rotate pattern based on variant
            if (pattern_variant == 1) {
                // Vertical bars instead
                bar_index = y / (height / 8);
                if (bar_index >= 8) bar_index = 7;
            } else if (pattern_variant == 2) {
                // Checkerboard
                bar_index = ((x / bar_width) + (y / (height / 8))) % 8;
            }

            uint8_t r, g, b, a = 255;

            switch (bar_index) {
                case 0: // Red
                    r = 255; g = 0; b = 0;
                    break;
                case 1: // Green
                    r = 0; g = 255; b = 0;
                    break;
                case 2: // Blue
                    r = 0; g = 0; b = 255;
                    break;
                case 3: // White
                    r = 255; g = 255; b = 255;
                    break;
                case 4: // Black
                    r = 0; g = 0; b = 0;
                    break;
                case 5: // Cyan
                    r = 0; g = 255; b = 255;
                    break;
                case 6: // Magenta
                    r = 255; g = 0; b = 255;
                    break;
                case 7: // Yellow
                    r = 255; g = 255; b = 0;
                    break;
                default:
                    r = 128; g = 128; b = 128;
                    break;
            }

            // Add some vertical variation for visual interest
            if (y < height / 4) {
                // Top quarter: keep original colors
            } else if (y < height / 2) {
                // Second quarter: darker
                r = r * 0.7;
                g = g * 0.7;
                b = b * 0.7;
            } else if (y < 3 * height / 4) {
                // Third quarter: even darker
                r = r * 0.4;
                g = g * 0.4;
                b = b * 0.4;
            } else {
                // Bottom quarter: much darker
                r = r * 0.2;
                g = g * 0.2;
                b = b * 0.2;
            }

            if (color_model == F0R_COLOR_MODEL_BGRA8888) {
                frame[y * width + x] = (a << 24) | (r << 16) | (g << 8) | b;
            } else {
                frame[y * width + x] = (a << 24) | (b << 16) | (g << 8) | r;
            }
        }
    }
}

// Test parameters by cycling through different values
void test_parameters(f0r_instance_t instance, f0r_set_param_value_f f0r_set_param_value,
                     f0r_get_param_info_f f0r_get_param_info, int num_params, int frame_count) {
    f0r_param_info_t param_info;
    double double_val;
    f0r_param_color_t color_val;
    f0r_param_position_t position_val;

    for (int i = 0; i < num_params; i++) {
        f0r_get_param_info(&param_info, i);

        switch (param_info.type) {
            case F0R_PARAM_BOOL:
                // Alternate between 0.0 and 1.0 every 30 frames
                double_val = (frame_count / 30) % 2;
                f0r_set_param_value(instance, (f0r_param_t)&double_val, i);
                break;

            case F0R_PARAM_DOUBLE:
                // Cycle through values 0.0 to 1.0 over 60 frames
                double_val = (frame_count % 60) / 60.0;
                f0r_set_param_value(instance, (f0r_param_t)&double_val, i);
                break;

            case F0R_PARAM_COLOR:
                // Cycle through different colors
                switch ((frame_count / 20) % 4) {
                    case 0: // Red
                        color_val.r = 1.0; color_val.g = 0.0; color_val.b = 0.0;
                        break;
                    case 1: // Green
                        color_val.r = 0.0; color_val.g = 1.0; color_val.b = 0.0;
                        break;
                    case 2: // Blue
                        color_val.r = 0.0; color_val.g = 0.0; color_val.b = 1.0;
                        break;
                    case 3: // White
                        color_val.r = 1.0; color_val.g = 1.0; color_val.b = 1.0;
                        break;
                }
                f0r_set_param_value(instance, (f0r_param_t)&color_val, i);
                break;

            case F0R_PARAM_POSITION:
                // Move position in a circular pattern
                position_val.x = 0.5 + 0.4 * sin(frame_count * 0.1);
                position_val.y = 0.5 + 0.4 * cos(frame_count * 0.1);
                f0r_set_param_value(instance, (f0r_param_t)&position_val, i);
                break;

            case F0R_PARAM_STRING:
            {
                // For string parameters, use "0" as default
                char *string_val = "0";
                f0r_set_param_value(instance, (f0r_param_t)&string_val, i);
                break;
            }

            default:
                // For unknown parameter types, set to middle value
                double_val = 0.5;
                f0r_set_param_value(instance, (f0r_param_t)&double_val, i);
                break;
        }
    }
}

int main(int argc, char* argv[]) {
  // instance frei0r pointers
  static void *dl_handle;
  static f0r_init_f f0r_init;
  static f0r_deinit_f f0r_deinit;
  static f0r_plugin_info_t pi;
  static f0r_get_plugin_info_f f0r_get_plugin_info;
  static f0r_get_param_info_f f0r_get_param_info;
  static f0r_param_info_t param;
  static f0r_instance_t instance;
  static f0r_construct_f f0r_construct;
  static f0r_update_f f0r_update;
  static f0r_destruct_f f0r_destruct;
  static f0r_set_param_value_f f0r_set_param_value;
  static f0r_get_param_value_f f0r_get_param_value;

  const char *usage = "Usage: frei0r-test [-tdg] [-f frames] -p <frei0r_plugin_file>\n"
                      "  -d         debug mode\n"
                      "  -g         graphical display mode (Linux/WSL)\n"
                      "  -f frames  number of frames to process (default: 100)\n"
                      "  -p plugin  path to frei0r plugin file";
  if (argc < 2) {
  fprintf(stderr,"%s\n",usage);
  return -1;
  }

  int opt;
#if defined(GUI)
  int graphical = 1;
#else
  int graphical = 0;
#endif
  int debug = 0;
  int frames = 100; // Number of frames to test
  char plugin_file[512];
  plugin_file[0] = '\0';
  while((opt =  getopt(argc, argv, "tdgf:p:")) != -1) {
  switch(opt) {
  case 'd':
    debug = 1;
    break;
  case 'g':
    graphical = 1;
    break;
  case 'f':
    frames = atoi(optarg);
    break;
  case 'p':
    snprintf(plugin_file, 511, "%s", optarg);
    break;
  }
  }

  if (plugin_file[0] == '\0') {
    fprintf(stderr, "Error: plugin file required (-p option)\n%s\n", usage);
    return -1;
  }

  // Set fixed video properties for test pattern
  int frame_width = 640;
  int frame_height = 480;
  int fps = 30;

  const char *file = basename(plugin_file);
  const char *dir = dirname(plugin_file);
  char path[256];;
  snprintf(path, 255,"%s/%s",dir,file);
  // fprintf(stderr,"%s %s\n",argv[0], file);
  // load shared library
  dl_handle = dlopen(path, RTLD_NOW|RTLD_LOCAL|RTLD_NODELETE);
  if(!dl_handle) {
  fprintf(stderr,"error: %s\n",dlerror());
  exit(1);
  }
  // get plugin function calls
  f0r_init = (f0r_init_f) dlsym(dl_handle,"f0r_init");
  f0r_deinit = (f0r_deinit_f) dlsym(dl_handle,"f0r_deinit");
  f0r_get_plugin_info = (f0r_get_plugin_info_f) dlsym(dl_handle,"f0r_get_plugin_info");
  f0r_get_param_info = (f0r_get_param_info_f) dlsym(dl_handle,"f0r_get_param_info");
  f0r_construct = (f0r_construct_f) dlsym(dl_handle,"f0r_construct");
  f0r_update = (f0r_update_f) dlsym(dl_handle,"f0r_update");
  f0r_destruct = (f0r_destruct_f) dlsym(dl_handle,"f0r_destruct");
  f0r_set_param_value = (f0r_set_param_value_f) dlsym(dl_handle,"f0r_set_param_value");
  f0r_get_param_value = (f0r_get_param_value_f) dlsym(dl_handle,"f0r_get_param_value");

  // always initialize plugin first
  f0r_init();
  // get info about plugin
  f0r_get_plugin_info(&pi);
  const char *frei0r_color_model = (pi.color_model == F0R_COLOR_MODEL_BGRA8888 ? "bgra8888" :
  pi.color_model == F0R_COLOR_MODEL_RGBA8888 ? "rgba8888" :
  pi.color_model == F0R_COLOR_MODEL_PACKED32 ? "packed32" : "unknown");

  if(debug) {
    fprintf(stderr,"{\n \"name\":\"%s\",\n \"type\":\"%s\",\n \"color_model\":\"%s\",\n \"num_params\":\"%d\"\n}",
            pi.name,
            pi.plugin_type == F0R_PLUGIN_TYPE_FILTER ? "filter" :
            pi.plugin_type == F0R_PLUGIN_TYPE_SOURCE ? "source" :
            pi.plugin_type == F0R_PLUGIN_TYPE_MIXER2 ? "mixer2" :
            pi.plugin_type == F0R_PLUGIN_TYPE_MIXER3 ? "mixer3" : "unknown",
            frei0r_color_model,
            pi.num_params);
    // Print parameter information
    if (pi.num_params > 0) {
      fprintf(stderr,",\n \"parameters\":[\n");
      for (int i = 0; i < pi.num_params; i++) {
        f0r_get_param_info(&param, i);
        const char* param_type =
          param.type == F0R_PARAM_BOOL ? "bool" :
          param.type == F0R_PARAM_DOUBLE ? "double" :
          param.type == F0R_PARAM_COLOR ? "color" :
          param.type == F0R_PARAM_POSITION ? "position" :
          param.type == F0R_PARAM_STRING ? "string" : "unknown";
        fprintf(stderr,"  {\"name\":\"%s\",\"type\":\"%s\",\"explanation\":\"%s\"}",
                param.name, param_type, param.explanation);
        if (i < pi.num_params - 1) fprintf(stderr,",\n");
      }
      fprintf(stderr,"\n ]\n");
    }
    fprintf(stderr,"}\n");
  }

  instance = f0r_construct(frame_width, frame_height);

  uint32_t *input_buffer = NULL;
  uint32_t *input_buffer2 = NULL;
  uint32_t *input_buffer3 = NULL;
  uint32_t *output_buffer;

  // Allocate buffers based on plugin type
  if (pi.plugin_type == F0R_PLUGIN_TYPE_FILTER) {
      input_buffer = (uint32_t*)calloc(4, frame_width * frame_height);
  } else if (pi.plugin_type == F0R_PLUGIN_TYPE_MIXER2) {
      input_buffer = (uint32_t*)calloc(4, frame_width * frame_height);
      input_buffer2 = (uint32_t*)calloc(4, frame_width * frame_height);
  } else if (pi.plugin_type == F0R_PLUGIN_TYPE_MIXER3) {
      input_buffer = (uint32_t*)calloc(4, frame_width * frame_height);
      input_buffer2 = (uint32_t*)calloc(4, frame_width * frame_height);
      input_buffer3 = (uint32_t*)calloc(4, frame_width * frame_height);
  }
  // SOURCE type needs no input buffer

  output_buffer = (uint32_t*)calloc(4, frame_width * frame_height);

#if defined(GUI)
  // Generate initial test patterns
  if (input_buffer)
      generate_animated_test_pattern(input_buffer, frame_width, frame_height, 0, pi.color_model);
  if (input_buffer2)
      generate_animated_test_pattern(input_buffer2, frame_width, frame_height, 0, pi.color_model);
  if (input_buffer3)
      generate_animated_test_pattern(input_buffer3, frame_width, frame_height, 0, pi.color_model);
#else
  if (input_buffer)
      generate_test_pattern(input_buffer, frame_width, frame_height, pi.color_model, 0);
  if (input_buffer2)
      generate_test_pattern(input_buffer2, frame_width, frame_height, pi.color_model, 1);
  if (input_buffer3)
      generate_test_pattern(input_buffer3, frame_width, frame_height, pi.color_model, 2);
#endif

#if defined(__linux__) && defined(GUI)
  Display *display = NULL;
  Window window;
  GC gc;
  XImage *ximage = NULL;

  if (graphical) {
      display = XOpenDisplay(NULL);
      if (!display) {
          fprintf(stderr, "Warning: Cannot open X display, falling back to headless mode\n");
          graphical = 0;
      } else {
          int screen = DefaultScreen(display);
          window = XCreateSimpleWindow(display, RootWindow(display, screen),
                                       0, 0, frame_width, frame_height, 1,
                                       BlackPixel(display, screen),
                                       WhitePixel(display, screen));

          XStoreName(display, window, pi.name);
          XSelectInput(display, window, ExposureMask | KeyPressMask);
          XMapWindow(display, window);
          gc = XCreateGC(display, window, 0, NULL);

          Visual *visual = DefaultVisual(display, screen);
          ximage = XCreateImage(display, visual, 24, ZPixmap, 0,
                               (char*)output_buffer, frame_width, frame_height, 32, 0);

          XFlush(display);
      }
  }
#endif

  // Load f0r_update2 for mixers
  f0r_update2_f f0r_update2 = NULL;
  if (pi.plugin_type == F0R_PLUGIN_TYPE_MIXER2 || pi.plugin_type == F0R_PLUGIN_TYPE_MIXER3) {
      f0r_update2 = (f0r_update2_f)dlsym(dl_handle, "f0r_update2");
      if (!f0r_update2) {
          fprintf(stderr, "Error: Cannot load f0r_update2 for mixer plugin\n");
          if (input_buffer) free(input_buffer);
          if (input_buffer2) free(input_buffer2);
          if (input_buffer3) free(input_buffer3);
          free(output_buffer);
          f0r_destruct(instance);
          f0r_deinit();
          dlclose(dl_handle);
          return 1;
      }
  }

  // Test the plugin with different parameter values
  for (int frame = 0; frame < frames; frame++) {
#if defined(GUI)
      // Generate animated test patterns for this frame
      if (input_buffer)
          generate_animated_test_pattern(input_buffer, frame_width, frame_height, frame, pi.color_model);
      if (input_buffer2)
          generate_animated_test_pattern(input_buffer2, frame_width, frame_height, frame + 10, pi.color_model);
      if (input_buffer3)
          generate_animated_test_pattern(input_buffer3, frame_width, frame_height, frame + 20, pi.color_model);
#else
      if (input_buffer)
          generate_test_pattern(input_buffer, frame_width, frame_height, pi.color_model, 0);
      if (input_buffer2)
          generate_test_pattern(input_buffer2, frame_width, frame_height, pi.color_model, 1);
      if (input_buffer3)
          generate_test_pattern(input_buffer3, frame_width, frame_height, pi.color_model, 2);
#endif

      // Update parameters if the plugin has any
      if (pi.num_params > 0 && f0r_set_param_value) {
          test_parameters(instance, f0r_set_param_value, f0r_get_param_info, pi.num_params, frame);
      }

      // Apply plugin based on type
      double time = (double)frame / (double)fps;

      switch (pi.plugin_type) {
          case F0R_PLUGIN_TYPE_SOURCE:
              f0r_update(instance, time, NULL, output_buffer);
              break;
          case F0R_PLUGIN_TYPE_FILTER:
              f0r_update(instance, time, (const uint32_t*)input_buffer, output_buffer);
              break;
          case F0R_PLUGIN_TYPE_MIXER2:
              f0r_update2(instance, time, (const uint32_t*)input_buffer,
                         (const uint32_t*)input_buffer2, NULL, output_buffer);
              break;
          case F0R_PLUGIN_TYPE_MIXER3:
              f0r_update2(instance, time, (const uint32_t*)input_buffer,
                         (const uint32_t*)input_buffer2, (const uint32_t*)input_buffer3,
                         output_buffer);
              break;
          default:
              fprintf(stderr, "Unknown plugin type: %d\n", pi.plugin_type);
              break;
      }

#if defined(__linux__) && defined(GUI)
      if (graphical && display) {
          ximage->data = (char*)output_buffer;
          XPutImage(display, window, gc, ximage, 0, 0, 0, 0, frame_width, frame_height);
          XFlush(display);

          // Check for key press to exit early
          while (XPending(display)) {
              XEvent event;
              XNextEvent(display, &event);
              if (event.type == KeyPress) {
                  fprintf(stderr, "\nInterrupted by user at frame %d\n", frame);
                  frame = frames; // Exit loop
                  break;
              }
          }

          usleep(1000000 / fps); // Frame delay
      }
#endif

      if (!graphical && frame % 10 == 0 && debug) {
          printf("Frame %d processed\n", frame);
      }
  }

#if defined(__linux__) && defined(GUI)
  if (graphical && display) {
      ximage->data = NULL; // Prevent XDestroyImage from freeing our buffer
      XDestroyImage(ximage);
      XFreeGC(display, gc);
      XDestroyWindow(display, window);
      XCloseDisplay(display);
  }
#endif

  if(debug) {
    const char *plugin_type_name = "unknown";
    switch (pi.plugin_type) {
        case F0R_PLUGIN_TYPE_SOURCE: plugin_type_name = "source"; break;
        case F0R_PLUGIN_TYPE_FILTER: plugin_type_name = "filter"; break;
        case F0R_PLUGIN_TYPE_MIXER2: plugin_type_name = "mixer2"; break;
        case F0R_PLUGIN_TYPE_MIXER3: plugin_type_name = "mixer3"; break;
    }
    printf("Test completed successfully. Plugin: %s (type: %s)\n", pi.name, plugin_type_name);
    printf("Tested %d frames with %d parameters\n", frames, pi.num_params);
    if (pi.plugin_type != F0R_PLUGIN_TYPE_SOURCE) {
        printf("Input: %dx%d test pattern(s)\n", frame_width, frame_height);
    }
    printf("Output: %dx%d processed frames\n", frame_width, frame_height);
  }

  if (input_buffer) free(input_buffer);
  if (input_buffer2) free(input_buffer2);
  if (input_buffer3) free(input_buffer3);
  free(output_buffer);

  f0r_destruct(instance);
  f0r_deinit();

  dlclose(dl_handle);

  return 0;
}
