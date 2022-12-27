
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <dlfcn.h>

#include <frei0r.h>

typedef void (*f0r_get_plugin_info_f)(f0r_plugin_info_t *info);
/* typedef void (*f0r_get_param_info_f)(f0r_param_info_t *info, int param_index); */
/* typedef void (*f0r_get_param_value_f)(f0r_instance_t instance, f0r_param_t param, int param_index); */

/* f0r_get_param_info_f  get_param_info; */
/* f0r_get_param_value_f get_param_value; */

int main(int argc, char **argv) {
  static void *dl_handle;
  static f0r_plugin_info_t pi;
  static f0r_get_plugin_info_f f0r_get_plugin_info;
  if(argc<2) exit(1);
  // load shared library
  dl_handle = dlopen(argv[1], RTLD_NOW|RTLD_LOCAL);
  if(!dl_handle) {
	fprintf(stderr,"error: %s\n",dlerror());
	exit(1);
  }
  // get plugin info
  f0r_get_plugin_info = dlsym(dl_handle,"f0r_get_plugin_info");
  f0r_get_plugin_info(&pi);
  fprintf(stdout,
		  "{\n \"name\":\"%s\",\n \"type\":\"%s\",\n \"author\":\"%s\",\n"
		  "\"explanation\":\"%s\",\n \"color_model\":\"%s\",\n"
		  "\"frei0r_version\":\"%d\",\n \"version\":\"%d.%d\",\n \"num_params\":\"%d\"\n},\n",
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

  dlclose(dl_handle);
  exit(0);
}
