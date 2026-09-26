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
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <frei0r.h>
#include "test-pattern.h"

#if defined(__unix__) && defined(GUI)
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

#define FRAME_ALIGNMENT 16
#define FRAME_GUARD_SIZE 64
#define FRAME_GUARD_VALUE 0xa5

typedef struct guarded_frame {
    unsigned char *storage;
    uint32_t *pixels;
    size_t bytes;
} guarded_frame_t;

static int allocate_frame(guarded_frame_t *frame, size_t bytes)
{
    uintptr_t aligned;
    size_t allocation_size = bytes + 2 * FRAME_GUARD_SIZE + FRAME_ALIGNMENT - 1;

    frame->storage = (unsigned char *)malloc(allocation_size);
    if (!frame->storage) return 0;
    aligned = ((uintptr_t)frame->storage + FRAME_GUARD_SIZE +
               FRAME_ALIGNMENT - 1) & ~(uintptr_t)(FRAME_ALIGNMENT - 1);
    frame->pixels = (uint32_t *)aligned;
    frame->bytes = bytes;
    memset((unsigned char *)frame->pixels - FRAME_GUARD_SIZE,
           FRAME_GUARD_VALUE, FRAME_GUARD_SIZE);
    memset(frame->pixels, 0, bytes);
    memset((unsigned char *)frame->pixels + bytes,
           FRAME_GUARD_VALUE, FRAME_GUARD_SIZE);
    return 1;
}

static void free_frame(guarded_frame_t *frame)
{
    free(frame->storage);
    memset(frame, 0, sizeof(*frame));
}

static int frame_guards_valid(const guarded_frame_t *frame)
{
    const unsigned char *before;
    const unsigned char *after;

    if (!frame->pixels) return 1;
    before = (const unsigned char *)frame->pixels - FRAME_GUARD_SIZE;
    after = (const unsigned char *)frame->pixels + frame->bytes;
    for (size_t i = 0; i < FRAME_GUARD_SIZE; i++) {
        if (before[i] != FRAME_GUARD_VALUE || after[i] != FRAME_GUARD_VALUE)
            return 0;
    }
    return 1;
}

static uint64_t frame_checksum(const uint32_t *pixels, size_t bytes)
{
    const unsigned char *data = (const unsigned char *)pixels;
    uint64_t hash = UINT64_C(1469598103934665603);

    for (size_t i = 0; i < bytes; i++) {
        hash ^= data[i];
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

static int contract_error(const char *plugin, int param_index, const char *format, ...)
{
    va_list args;

    if (param_index >= 0) {
        fprintf(stderr, "API contract violation in %s parameter %d: ",
                plugin ? plugin : "<unknown>", param_index);
    } else {
        fprintf(stderr, "API contract violation in %s: ",
                plugin ? plugin : "<unknown>");
    }
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputc('\n', stderr);
    return 1;
}

static int valid_utf8(const char *string)
{
    const unsigned char *s = (const unsigned char *)string;

    if (!s) return 0;
    while (*s) {
        if (*s <= 0x7f) {
            s++;
        } else if (*s >= 0xc2 && *s <= 0xdf &&
                   s[1] >= 0x80 && s[1] <= 0xbf) {
            s += 2;
        } else if (*s == 0xe0 &&
                   s[1] >= 0xa0 && s[1] <= 0xbf &&
                   s[2] >= 0x80 && s[2] <= 0xbf) {
            s += 3;
        } else if (((*s >= 0xe1 && *s <= 0xec) ||
                    (*s >= 0xee && *s <= 0xef)) &&
                   s[1] >= 0x80 && s[1] <= 0xbf &&
                   s[2] >= 0x80 && s[2] <= 0xbf) {
            s += 3;
        } else if (*s == 0xed &&
                   s[1] >= 0x80 && s[1] <= 0x9f &&
                   s[2] >= 0x80 && s[2] <= 0xbf) {
            s += 3;
        } else if (*s == 0xf0 &&
                   s[1] >= 0x90 && s[1] <= 0xbf &&
                   s[2] >= 0x80 && s[2] <= 0xbf &&
                   s[3] >= 0x80 && s[3] <= 0xbf) {
            s += 4;
        } else if (*s >= 0xf1 && *s <= 0xf3 &&
                   s[1] >= 0x80 && s[1] <= 0xbf &&
                   s[2] >= 0x80 && s[2] <= 0xbf &&
                   s[3] >= 0x80 && s[3] <= 0xbf) {
            s += 4;
        } else if (*s == 0xf4 &&
                   s[1] >= 0x80 && s[1] <= 0x8f &&
                   s[2] >= 0x80 && s[2] <= 0xbf &&
                   s[3] >= 0x80 && s[3] <= 0xbf) {
            s += 4;
        } else {
            return 0;
        }
    }
    return 1;
}

static int unit_value(double value)
{
    return isfinite(value) && value >= 0.0 && value <= 1.0;
}

static char *duplicate_string(const char *string)
{
    size_t size = strlen(string) + 1;
    char *copy = (char *)malloc(size);

    if (copy) memcpy(copy, string, size);
    return copy;
}

static int validate_plugin_info(const f0r_plugin_info_t *info)
{
    int errors = 0;
    const char *name = info->name ? info->name : "<unknown>";

    if (!valid_utf8(info->name))
        errors += contract_error(name, -1, "name must be non-null UTF-8");
    if (!valid_utf8(info->author))
        errors += contract_error(name, -1, "author must be non-null UTF-8");
    if (info->explanation && !valid_utf8(info->explanation))
        errors += contract_error(name, -1, "explanation must be UTF-8 when present");
    if (info->plugin_type < F0R_PLUGIN_TYPE_FILTER ||
        info->plugin_type > F0R_PLUGIN_TYPE_MIXER3)
        errors += contract_error(name, -1, "unknown plugin type");
    if (info->color_model < F0R_COLOR_MODEL_BGRA8888 ||
        info->color_model > F0R_COLOR_MODEL_PACKED32)
        errors += contract_error(name, -1, "unknown color model");
    if (info->plugin_type == F0R_PLUGIN_TYPE_SOURCE &&
        info->color_model == F0R_COLOR_MODEL_PACKED32)
        errors += contract_error(name, -1, "source plugins cannot use PACKED32");
    if (info->frei0r_version <= 0 || info->frei0r_version > FREI0R_MAJOR_VERSION)
        errors += contract_error(name, -1, "unsupported frei0r API version");
    if (info->major_version < 0 || info->minor_version < 0)
        errors += contract_error(name, -1, "plugin version components cannot be negative");
    if (info->num_params < 0)
        errors += contract_error(name, -1, "parameter count cannot be negative");
    return errors;
}

static int validate_param_info(const char *plugin, const f0r_param_info_t *info,
                               int param_index)
{
    int errors = 0;

    if (!valid_utf8(info->name))
        errors += contract_error(plugin, param_index, "name must be non-null UTF-8");
    if (info->explanation && !valid_utf8(info->explanation))
        errors += contract_error(plugin, param_index,
                                 "explanation must be UTF-8 when present");
    if (info->type < F0R_PARAM_BOOL || info->type > F0R_PARAM_STRING)
        errors += contract_error(plugin, param_index, "unknown parameter type");
    return errors;
}

static int validate_parameter_value(const char *plugin, f0r_instance_t instance,
                                    f0r_set_param_value_f set_value,
                                    f0r_get_param_value_f get_value,
                                    const f0r_param_info_t *info, int param_index,
                                    int round_trip)
{
    int errors = 0;

    switch (info->type) {
        case F0R_PARAM_BOOL:
        case F0R_PARAM_DOUBLE: {
            double first = NAN;
            double second = NAN;
            get_value(instance, &first, param_index);
            if (!unit_value(first))
                return contract_error(plugin, param_index,
                                      "value %.17g must be finite and in [0, 1]", first);
            if (!round_trip) break;
            set_value(instance, &first, param_index);
            get_value(instance, &second, param_index);
            if (!unit_value(second))
                errors += contract_error(plugin, param_index,
                                         "round-tripped value %.17g left [0, 1]", second);
            break;
        }
        case F0R_PARAM_COLOR: {
            f0r_param_color_t first = {NAN, NAN, NAN};
            f0r_param_color_t second = {NAN, NAN, NAN};
            get_value(instance, &first, param_index);
            if (!unit_value(first.r) || !unit_value(first.g) || !unit_value(first.b))
                return contract_error(plugin, param_index,
                                      "color components must be finite and in [0, 1]");
            if (!round_trip) break;
            set_value(instance, &first, param_index);
            get_value(instance, &second, param_index);
            if (!unit_value(second.r) || !unit_value(second.g) || !unit_value(second.b))
                errors += contract_error(plugin, param_index,
                                         "round-tripped color left [0, 1]");
            break;
        }
        case F0R_PARAM_POSITION: {
            f0r_param_position_t first = {NAN, NAN};
            f0r_param_position_t second = {NAN, NAN};
            get_value(instance, &first, param_index);
            if (!unit_value(first.x) || !unit_value(first.y))
                return contract_error(plugin, param_index,
                                      "position components must be finite and in [0, 1]");
            if (!round_trip) break;
            set_value(instance, &first, param_index);
            get_value(instance, &second, param_index);
            if (!unit_value(second.x) || !unit_value(second.y))
                errors += contract_error(plugin, param_index,
                                         "round-tripped position left [0, 1]");
            break;
        }
        case F0R_PARAM_STRING: {
            char *first = NULL;
            char *second = NULL;
            char *copy;
            char *expected;
            get_value(instance, &first, param_index);
            if (!valid_utf8(first))
                return contract_error(plugin, param_index,
                                      "string value must be non-null UTF-8");
            if (!round_trip) break;
            copy = duplicate_string(first);
            expected = duplicate_string(first);
            if (!copy || !expected) {
                free(copy);
                free(expected);
                return contract_error(plugin, param_index,
                                      "could not allocate round-trip string");
            }
            set_value(instance, &copy, param_index);
            if (copy[0]) copy[0] ^= 1;
            get_value(instance, &second, param_index);
            if (!valid_utf8(second))
                errors += contract_error(plugin, param_index,
                                         "round-tripped string must be non-null UTF-8");
            else if (strcmp(expected, second) != 0)
                errors += contract_error(plugin, param_index,
                                         "setter did not retain an independent string copy");
            free(copy);
            free(expected);
            break;
        }
    }
    return errors;
}


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
  int contract_errors = 0;

  const char *usage = "Usage: frei0r-run [-tdg] [-f frames] -p <frei0r_plugin_file>\n"
                      "  -d         debug mode\n"
                      "  -g         graphical display mode (Linux/WSL)\n"
                      "  -f frames  number of frames to process (default: 100)\n"
                      "  -p plugin  path to frei0r plugin file";
  if (argc < 2) {
  fprintf(stderr,"%s\n",usage);
  return -1;
  }

  int opt;
  int graphical = 0;
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
  dl_handle = dlopen(path, RTLD_NOW|RTLD_LOCAL);
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

  int missing_entry_points = 0;
  if (!f0r_init)
    missing_entry_points += contract_error(file, -1, "missing f0r_init");
  if (!f0r_deinit)
    missing_entry_points += contract_error(file, -1, "missing f0r_deinit");
  if (!f0r_get_plugin_info)
    missing_entry_points += contract_error(file, -1, "missing f0r_get_plugin_info");
  if (!f0r_get_param_info)
    missing_entry_points += contract_error(file, -1, "missing f0r_get_param_info");
  if (!f0r_construct)
    missing_entry_points += contract_error(file, -1, "missing f0r_construct");
  if (!f0r_destruct)
    missing_entry_points += contract_error(file, -1, "missing f0r_destruct");
  if (!f0r_set_param_value)
    missing_entry_points += contract_error(file, -1, "missing f0r_set_param_value");
  if (!f0r_get_param_value)
    missing_entry_points += contract_error(file, -1, "missing f0r_get_param_value");
  if (missing_entry_points) {
    dlclose(dl_handle);
    return 1;
  }

  // always initialize plugin first
  f0r_init();
  // get info about plugin
  f0r_get_plugin_info(&pi);
  contract_errors += validate_plugin_info(&pi);
  if (contract_errors) {
    f0r_deinit();
    dlclose(dl_handle);
    return 1;
  }

  for (int i = 0; i < pi.num_params; i++) {
    memset(&param, 0, sizeof(param));
    f0r_get_param_info(&param, i);
    contract_errors += validate_param_info(pi.name, &param, i);
  }
  if (contract_errors) {
    f0r_deinit();
    dlclose(dl_handle);
    return 1;
  }

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
                param.name, param_type, param.explanation ? param.explanation : "");
        if (i < pi.num_params - 1) fprintf(stderr,",\n");
      }
      fprintf(stderr,"\n ]\n");
    }
    fprintf(stderr,"}\n");
  }

  instance = f0r_construct(frame_width, frame_height);
  if (!instance) {
    contract_error(pi.name, -1, "construction failed for valid frame dimensions");
    f0r_deinit();
    dlclose(dl_handle);
    return 1;
  }

  for (int i = 0; i < pi.num_params; i++) {
    memset(&param, 0, sizeof(param));
    f0r_get_param_info(&param, i);
    contract_errors += validate_parameter_value(pi.name, instance,
                                                f0r_set_param_value,
                                                f0r_get_param_value,
                                                &param, i, 1);
  }

  guarded_frame_t input_frame = {0};
  guarded_frame_t input_frame2 = {0};
  guarded_frame_t input_frame3 = {0};
  guarded_frame_t output_frame = {0};
  size_t frame_bytes = (size_t)frame_width * frame_height * sizeof(uint32_t);
  uint32_t *input_buffer = NULL;
  uint32_t *input_buffer2 = NULL;
  uint32_t *input_buffer3 = NULL;
  uint32_t *output_buffer = NULL;
  int buffers_ok = 1;

  // Allocate buffers based on plugin type
  if (pi.plugin_type == F0R_PLUGIN_TYPE_FILTER) {
      buffers_ok = allocate_frame(&input_frame, frame_bytes);
      input_buffer = input_frame.pixels;
  } else if (pi.plugin_type == F0R_PLUGIN_TYPE_MIXER2) {
      buffers_ok = allocate_frame(&input_frame, frame_bytes) &&
                   allocate_frame(&input_frame2, frame_bytes);
      input_buffer = input_frame.pixels;
      input_buffer2 = input_frame2.pixels;
  } else if (pi.plugin_type == F0R_PLUGIN_TYPE_MIXER3) {
      buffers_ok = allocate_frame(&input_frame, frame_bytes) &&
                   allocate_frame(&input_frame2, frame_bytes) &&
                   allocate_frame(&input_frame3, frame_bytes);
      input_buffer = input_frame.pixels;
      input_buffer2 = input_frame2.pixels;
      input_buffer3 = input_frame3.pixels;
  }
  // SOURCE type needs no input buffer

  buffers_ok = buffers_ok && allocate_frame(&output_frame, frame_bytes);
  output_buffer = output_frame.pixels;
  if (!buffers_ok) {
      contract_error(pi.name, -1, "could not allocate aligned frame buffers");
      free_frame(&input_frame);
      free_frame(&input_frame2);
      free_frame(&input_frame3);
      free_frame(&output_frame);
      f0r_destruct(instance);
      f0r_deinit();
      dlclose(dl_handle);
      return 1;
  }

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

#if defined(__unix__) && defined(GUI)
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
          free_frame(&input_frame);
          free_frame(&input_frame2);
          free_frame(&input_frame3);
          free_frame(&output_frame);
          f0r_destruct(instance);
          f0r_deinit();
          dlclose(dl_handle);
          return 1;
      }
  } else if (!f0r_update) {
      fprintf(stderr, "API contract violation in %s: missing f0r_update\n", pi.name);
      free_frame(&input_frame);
      free_frame(&output_frame);
      f0r_destruct(instance);
      f0r_deinit();
      dlclose(dl_handle);
      return 1;
  }

  // Test the plugin with different parameter values
  for (int frame = 0; frame < frames; frame++) {
      int verify_inputs = frame == 0 || frame == frames / 2 || frame == frames - 1;
      uint64_t input_checksum = 0;
      uint64_t input_checksum2 = 0;
      uint64_t input_checksum3 = 0;
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
          for (int i = 0; i < pi.num_params; i++) {
              memset(&param, 0, sizeof(param));
              f0r_get_param_info(&param, i);
              contract_errors += validate_parameter_value(pi.name, instance,
                                                          f0r_set_param_value,
                                                          f0r_get_param_value,
                                                          &param, i, 0);
          }
      }

      // Apply plugin based on type
      double time = (double)frame / (double)fps;
      if (verify_inputs && input_buffer)
          input_checksum = frame_checksum(input_buffer, frame_bytes);
      if (verify_inputs && input_buffer2)
          input_checksum2 = frame_checksum(input_buffer2, frame_bytes);
      if (verify_inputs && input_buffer3)
          input_checksum3 = frame_checksum(input_buffer3, frame_bytes);

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

      if (verify_inputs && input_buffer &&
          input_checksum != frame_checksum(input_buffer, frame_bytes))
          contract_errors += contract_error(pi.name, -1, "modified input frame 1");
      if (verify_inputs && input_buffer2 &&
          input_checksum2 != frame_checksum(input_buffer2, frame_bytes))
          contract_errors += contract_error(pi.name, -1, "modified input frame 2");
      if (verify_inputs && input_buffer3 &&
          input_checksum3 != frame_checksum(input_buffer3, frame_bytes))
          contract_errors += contract_error(pi.name, -1, "modified input frame 3");
      if (!frame_guards_valid(&input_frame) ||
          !frame_guards_valid(&input_frame2) ||
          !frame_guards_valid(&input_frame3) ||
          !frame_guards_valid(&output_frame))
          contract_errors += contract_error(pi.name, -1, "wrote outside a frame buffer");

#if defined(__unix__) && defined(GUI)
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

#if defined(__unix__) && defined(GUI)
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
    printf("Test completed %s. Plugin: %s (type: %s)\n",
           contract_errors ? "with API contract violations" : "successfully",
           pi.name, plugin_type_name);
    printf("Tested %d frames with %d parameters\n", frames, pi.num_params);
    if (pi.plugin_type != F0R_PLUGIN_TYPE_SOURCE) {
        printf("Input: %dx%d test pattern(s)\n", frame_width, frame_height);
    }
    printf("Output: %dx%d processed frames\n", frame_width, frame_height);
  }

  free_frame(&input_frame);
  free_frame(&input_frame2);
  free_frame(&input_frame3);
  free_frame(&output_frame);

  f0r_destruct(instance);
  f0r_deinit();

  dlclose(dl_handle);

  return contract_errors ? 1 : 0;
}
