#include <unistd.h>
#include <libgen.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

  const char *usage = "Usage: frei0r-test [-td] <frei0r_plugin_file>";
  if (argc < 2) {
	fprintf(stderr,"%s\n",usage);
	return -1;
  }

  int opt;
  int headless = 0;
  int debug = 0;
  char plugin_file[512];
  while((opt =  getopt(argc, argv, "tdp:")) != -1) {
	switch(opt) {
	case 't':
	  headless = 1;
	  break;
	case 'd':
	  debug = 1;
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

  // Apply filter to test pattern
  f0r_update(instance, 0.0, (const uint32_t*)input_buffer, output_buffer);

  // For headless mode, we're done
  if (!headless) {
      printf("Test pattern processed successfully. Plugin: %s\n", pi.name);
      printf("Input: %dx%d test pattern\n", frame_width, frame_height);
      printf("Output: %dx%d processed frame\n", frame_width, frame_height);
  }

  free(input_buffer);
  free(output_buffer);

  f0r_destruct(instance);
  f0r_deinit();

  dlclose(dl_handle);

  return 0;
}
