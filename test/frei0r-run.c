#include <unistd.h>
#include <libgen.h>
#include <dlfcn.h>

#include <opencv2/opencv.hpp>

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

  const char *usage = "Usage: frei0r-test [-td] <video_file> <frei0r_plugin_file>";
  if (argc <3) {
	fprintf(stderr,"%s\n",usage);
	return -1;
  }

  int opt;
  int headless = 0;
  int debug = 0;
  char video_file[512];
  char plugin_file[512];
  while((opt =  getopt(argc, argv, "tdv:p:")) != -1) {
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
	case 'v':
	  snprintf(video_file, 511, "%s", optarg);
	  break;
	}
  }

  // Open the image file using OpenCV
  cv::VideoCapture cap(video_file);
  if (!cap.isOpened()) {
	printf("Error opening image file\n");
	return -1;
  }
  // Get video properties
  int frame_width = (int) cap.get(cv::CAP_PROP_FRAME_WIDTH);
  int frame_height = (int) cap.get(cv::CAP_PROP_FRAME_HEIGHT);
  int fps = (int) cap.get(cv::CAP_PROP_FPS);

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
	cap.release();
	f0r_deinit();
	dlclose(dl_handle);
	exit(0);
  }
  if( pi.color_model != F0R_COLOR_MODEL_RGBA8888 ) {
	fprintf(stderr,"Filter color model not supported: %s\n",frei0r_color_model);
	cap.release();
	f0r_deinit();
	dlclose(dl_handle);
	exit(1);
  }

  instance = f0r_construct(frame_width, frame_height);

  // Create a window to display the video
  if(!headless) {
	cv::namedWindow("frei0r", cv::WINDOW_NORMAL);
	cv::resizeWindow("frei0r", frame_width, frame_height);
	cv::namedWindow("source", cv::WINDOW_NORMAL);
	cv::resizeWindow("source", frame_width, frame_height);
  }

  uint32_t *buffer;
  /* //posix_memalign( (void**) &outframe, 16, frame_width * frame_height * 4 ); */
  buffer = (uint32_t*)calloc(4, frame_width * frame_height );

  // Read the image file frame by frame
  cv::Mat frame;
  while (cap.read(frame)) {
	// Convert the OpenCV image to an RGBA pixel buffer
	cv::Mat frame_rgba;
	cv::cvtColor(frame, frame_rgba, cv::COLOR_RGB2RGBA);

	// Create an SDL2 surface from the RGBA pixel buffer
	f0r_update(instance, 0.0, (const uint32_t*)frame_rgba.data, buffer);

	// Display the frames
	if(!headless) {
	  cv::imshow("source", frame);
	  memcpy(frame_rgba.data, buffer, frame_width * frame_height * 4);
	  cv::imshow("frei0r", frame_rgba);
	  // Wait for a key press
	  if(cv::waitKey(1000/fps) >= 0) break;
	}
  }

  free(buffer);

  cap.release(); // Release the video capture object

  f0r_destruct(instance);
  f0r_deinit();

  dlclose(dl_handle);

  return 0;
}
