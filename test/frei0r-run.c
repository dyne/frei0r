#include <unistd.h>
#include <libgen.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <frei0r.h>

// frei0r function prototypes
typedef int (*f0r_init_f)(void);
typedef void (*f0r_deinit_f)(void);
typedef void (*f0r_get_plugin_info_f)(f0r_plugin_info_t *info);
typedef void (*f0r_get_param_info_f)(f0r_param_info_t *info, int param_index);
typedef f0r_instance_t (*f0r_construct_f)(unsigned int width, unsigned int height);
typedef void (*f0r_update_f)(f0r_instance_t instance,
		double time, const uint32_t* inframe, uint32_t* outframe);
typedef void (*f0r_destruct_f)(f0r_instance_t instance);
typedef void (*f0r_set_param_value_f)(f0r_instance_t instance, f0r_param_t param, int param_index);
typedef void (*f0r_get_param_value_f)(f0r_instance_t instance, f0r_param_t param, int param_index);


// Generate a simple color bar test pattern
void generate_test_pattern(uint32_t* frame, int width, int height) {
    // Create color bars: red, green, blue, white, black, cyan, magenta, yellow
    int bar_width = width / 8;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int bar_index = x / bar_width;
            if (bar_index >= 8) bar_index = 7;

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

            frame[y * width + x] = (a << 24) | (b << 16) | (g << 8) | r;
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

  const char *usage = "Usage: frei0r-test [-td] <frei0r_plugin_file>";
  if (argc < 2) {
	fprintf(stderr,"%s\n",usage);
	return -1;
  }

  int opt;
  int headless = 0;
  int debug = 0;
  int frames = 100; // Number of frames to test
  char plugin_file[512];
  while((opt =  getopt(argc, argv, "tdf:p:")) != -1) {
	switch(opt) {
	case 't':
	  headless = 1;
	  break;
	case 'd':
	  debug = 1;
	  break;
	case 'f':
	  frames = atoi(optarg);
	  break;
	case 'p':
	  snprintf(plugin_file, 511, "%s", optarg);
	  break;
	}
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

  // TODO: just filters for now
  if( pi.plugin_type != F0R_PLUGIN_TYPE_FILTER ) {
	fprintf(stderr,"Plugin is not of filter type, skip for now\n");
	f0r_deinit();
	dlclose(dl_handle);
	exit(0);
  }
  if( pi.color_model != F0R_COLOR_MODEL_RGBA8888 ) {
	fprintf(stderr,"Filter color model not supported: %s\n",frei0r_color_model);
	f0r_deinit();
	dlclose(dl_handle);
	exit(1);
  }

  instance = f0r_construct(frame_width, frame_height);

  uint32_t *input_buffer;
  uint32_t *output_buffer;
  input_buffer = (uint32_t*)calloc(4, frame_width * frame_height);
  output_buffer = (uint32_t*)calloc(4, frame_width * frame_height);

  // Generate test pattern
  generate_test_pattern(input_buffer, frame_width, frame_height);

  // Test the plugin with different parameter values
  for (int frame = 0; frame < frames; frame++) {
      // Update parameters if the plugin has any
      if (pi.num_params > 0 && f0r_set_param_value) {
          test_parameters(instance, f0r_set_param_value, f0r_get_param_info, pi.num_params, frame);
      }

      // Apply filter to test pattern
      f0r_update(instance, (double)frame / (double)fps, (const uint32_t*)input_buffer, output_buffer);

      if (!headless && frame % 10 == 0) {
          printf("Frame %d processed\n", frame);
      }
  }

  if (!headless) {
      printf("Test completed successfully. Plugin: %s\n", pi.name);
      printf("Tested %d frames with %d parameters\n", frames, pi.num_params);
      printf("Input: %dx%d test pattern\n", frame_width, frame_height);
      printf("Output: %dx%d processed frames\n", frame_width, frame_height);
  }

  free(input_buffer);
  free(output_buffer);

  f0r_destruct(instance);
  f0r_deinit();

  dlclose(dl_handle);

  return 0;
}
