
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <dlfcn.h>
#include <libgen.h>

#include <frei0r.h>

// frei0r function prototypes
typedef int (*f0r_init_f)(void);
typedef void (*f0r_deinit_f)(void);
typedef void (*f0r_get_plugin_info_f)(f0r_plugin_info_t *info);
typedef void (*f0r_get_param_info_f)(f0r_param_info_t *info, int param_index);

int main(int argc, char **argv) {

  // instance frei0r pointers
  static void *dl_handle;
  static f0r_init_f f0r_init;
  static f0r_init_f f0r_deinit;
  static f0r_plugin_info_t pi;
  static f0r_get_plugin_info_f f0r_get_plugin_info;
  static f0r_get_param_info_f f0r_get_param_info;
  static f0r_param_info_t param;

  int c;

  if(argc<2) exit(1);
  const char *file = basename(argv[1]);
  const char *dir = dirname(argv[1]);
  char path[256];;
  snprintf(path, 255,"%s/%s",dir,file);
  // fprintf(stderr,"%s %s\n",argv[0], file);
  // load shared library
  dl_handle = dlopen(path, RTLD_NOW|RTLD_LOCAL);
  if(!dl_handle) {
	fprintf(stderr,"error: %s\n",dlerror());
	exit(1);
  }
  // get plugin function calls
  f0r_init = dlsym(dl_handle,"f0r_init");
  f0r_deinit = dlsym(dl_handle,"f0r_deinit");
  f0r_get_plugin_info = dlsym(dl_handle,"f0r_get_plugin_info");
  f0r_get_param_info = dlsym(dl_handle,"f0r_get_param_info");
  // always initialize plugin first
  f0r_init();
  // get info about plugin
  f0r_get_plugin_info(&pi);
  fprintf(stdout,
		  "{\n \"name\":\"%s\",\n \"type\":\"%s\",\n \"author\":\"%s\",\n"
		  " \"explanation\":\"%s\",\n \"color_model\":\"%s\",\n"
		  " \"frei0r_version\":\"%d\",\n \"version\":\"%d.%d\",\n \"num_params\":\"%d\"",
		  pi.name,
		  pi.plugin_type == F0R_PLUGIN_TYPE_FILTER ? "filter" :
		  pi.plugin_type == F0R_PLUGIN_TYPE_SOURCE ? "source" :
		  pi.plugin_type == F0R_PLUGIN_TYPE_MIXER2 ? "mixer2" :
		  pi.plugin_type == F0R_PLUGIN_TYPE_MIXER3 ? "mixer3" : "unknown",
		  pi.author, pi.explanation,
		  pi.color_model == F0R_COLOR_MODEL_BGRA8888 ? "bgra8888" :
		  pi.color_model == F0R_COLOR_MODEL_RGBA8888 ? "rgba8888" :
		  pi.color_model == F0R_COLOR_MODEL_PACKED32 ? "packed32" : "unknown",
		  pi.frei0r_version, pi.major_version, pi.minor_version, pi.num_params);

  /* // check icon */
  /* char icon[256]; */
  /* char *dot = rindex(file, '.'); */
  /* *dot = 0x0; */
  /* snprintf(icon,255,"%s/%s.png",dir,file); */
  /* FILE *icon_fd = fopen(icon,"r"); */
  /* if(icon_fd) { */
  /* 	fprintf(stderr," icon found: %s\n",icon); */
  /* } */

  // get info about params
  if(pi.num_params>0) {
	fprintf(stdout,",\n \"params\":[\n");
	for(c=0; c<pi.num_params; c++) {
	  f0r_get_param_info(&param, c);
	  fprintf(stdout,
			  "  {\n   \"name\":\"%s\",\n   \"type\":\"%s\",\n   \"explanation\":\"%s\"\n  }",
			  param.name,
			  param.type == F0R_PARAM_BOOL ? "bool" :
			  param.type == F0R_PARAM_COLOR ? "color" :
			  param.type == F0R_PARAM_DOUBLE ? "number" :
			  param.type == F0R_PARAM_POSITION ? "position" :
			  param.type == F0R_PARAM_STRING ? "string" : "unknown",
			  param.explanation);
	  if(pi.num_params>c+1) {
		fprintf(stdout,",\n");
	  } else {
		fprintf(stdout,"\n");
	  }
	}
	fprintf(stdout," ]\n");
  }
  fprintf(stdout,"\n},\n");
  fflush(stdout);
  f0r_deinit();
  dlclose(dl_handle);
  exit(0);
}
