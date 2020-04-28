#include "s0rter.h"

// #define S0RTER_DEBUG 1;

int f0r_init() {
  return 1;
}

void f0r_deinit() {
  // intentionally empty
}

void f0r_get_plugin_info(f0r_plugin_info_t* info) {
  info->name = "s0rter";
  info->author = "Sean Barag";
  info->plugin_type = F0R_PLUGIN_TYPE_FILTER;
  info->color_model = F0R_COLOR_MODEL_RGBA8888;
  info->frei0r_version = FREI0R_MAJOR_VERSION;
  info->major_version = 1;
  info->minor_version = 0;
  info->num_params = 0;
  info->explanation = "Sorts the pixels in each frame by (h, s, l) tuple.";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index) {
  // intentionally empty, but reference params because of -Wextra
  (void)info;
  (void)param_index;
}

void f0r_set_param_value(f0r_instance_t instance,
                         f0r_param_t param,
                         int param_index) {
  // intentionally empty, but reference params because of -Wextra
  (void)instance;
  (void)param;
  (void)param_index;
}

void f0r_get_param_value(f0r_instance_t instance,
                         f0r_param_t param,
                         int param_index) {
  // intentionally empty, but reference params because of -Wextra
  (void)instance;
  (void)param;
  (void)param_index;
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height) {
  s0rter_instance_t* instance =
      (s0rter_instance_t*)calloc(1, sizeof(*instance));
  instance->width = width;
  instance->height = height;
  instance->hsl_buffer = (hsl_t*)calloc(width * height, sizeof(hsl_t));

  return (f0r_instance_t)instance;
}

void f0r_destruct(f0r_instance_t f0r_instance) {
  s0rter_instance_t* instance = (s0rter_instance_t*)f0r_instance;
  free(instance);
}

void rgb_to_hsl(const uint32_t* rgb, hsl_t* hsl) {
  const double_t red   = ((*rgb & 0x0000ff) >>  0) / 255.0;
  const double_t green = ((*rgb & 0x00ff00) >>  8) / 255.0;
  const double_t blue  = ((*rgb & 0xff0000) >> 16) / 255.0;

  #ifdef S0RTER_DEBUG
    printf(">>>>>rgb_to_hsl\n");
    printf("rgba(%2X, %2X, %2X, FF)\n", (uint8_t) (red * 255), (uint8_t)(green * 255), (uint8_t)(blue * 255));
    printf("rgba(%f, %f, %f, %f)\n", red, green, blue, 1.0);
  #endif

  const double_t x_max = fmax(fmax(red, green), blue);
  const double_t x_min = fmin(fmin(red, green), blue);
  const double_t chroma = x_max - x_min;
  const double_t lightness = (x_max + x_min) / 2.0;

  #ifdef S0RTER_DEBUG
    printf("x_max = %f\n", x_max);
    printf("x_min = %f\n", x_min);
    printf("chroma = %f\n", chroma);
    printf("lightness = %f\n", lightness);
  #endif

  hsl->lightness = lightness;

  double_t hue = 0;
  if (chroma == 0) {
    hue = 0;
  } else if (x_max == red) {
    hue = 60 * (0 + ((green - blue) / chroma));
  } else if (x_max == green) {
    hue = 60 * (2 + ((blue - red) / chroma));
  } else if (x_max == blue) {
    hue = 60 * (4 + ((red - green) / chroma));
  }

  hsl->hue = fmod(hue + 360, 360);

  if (lightness == 0 || lightness == 1) {
    hsl->saturation = 0;
  } else {
    hsl->saturation = (x_max - lightness) / (fmin(lightness, 1 - lightness));
  }

  #ifdef S0RTER_DEBUG
    printf("-----\nhsl = (%f, %f, %f)\n", hsl->hue, hsl->saturation, hsl->lightness);
    assert(hsl->hue >= 0 && hsl->hue <= 360);
    assert(hsl->saturation >= 0 && hsl->saturation <= 1.0);
    assert(hsl->lightness >= 0 && hsl->lightness <= 1.0);
    printf("<<<<<rgb_to_hsl\n");
  #endif
}

void hsl_to_rgb(const hsl_t* hsl, uint32_t* rgb) {
  const double_t chroma = (1 - fabs((2 * hsl->lightness) - 1)) * hsl->saturation;
  const double_t h_prime = hsl->hue / 60.0;
  const double_t x = chroma * (1 - fabs(fmod(h_prime, 2) - 1));

  const double_t offset = hsl->lightness - (chroma / 2);

  #ifdef S0RTER_DEBUG
    printf(">>>>>hsl_to_rgb\n");
    printf("hsl = (%f, %f, %f)\n", hsl->hue, hsl->saturation, hsl->lightness);
    printf("chroma = %f\n", chroma);
    printf("h_prime = %f\n", h_prime);
    printf("x = %f\n", x);
    printf("offset = %f\n", offset);
  #endif

  double_t red = 0;
  double_t green = 0;
  double_t blue = 0;
  if (h_prime <= 1) {
    red = chroma;
    green = x;
    blue = 0;
  } else if (h_prime <= 2) {
    red = x;
    green = chroma;
    blue = 0;
  } else if (h_prime <= 3) {
    red = 0;
    green = chroma;
    blue = x;
  } else if (h_prime <= 4) {
    red = 0;
    green = x;
    blue = chroma;
  } else if (h_prime <= 5) {
    red = x;
    green = 0;
    blue = chroma;
  } else if (h_prime <= 6) {
    red = chroma;
    green = 0;
    blue = x;
  } else {
    red = 0;
    green = 0;
    blue = 0;
  }

  red = red + offset;
  green = green + offset;
  blue = blue + offset;

  #ifdef S0RTER_DEBUG
    printf("rgba(%f, %f, %f, %f)\n", red, green, blue, 1.0);
    printf("rgba(%f, %f, %f, %f)\n", ceil(red), green, blue, 1.0);
  #endif

  int r = round(red * 255);
  int g = round(green * 255);
  int b = round(blue * 255);

  #ifdef S0RTER_DEBUG
    printf("rgba(%d, %d, %d, %d)\n", r, g, b, 255);
    assert(r >= 0 && r <= 255);
    assert(g >= 0 && g <= 255);
    assert(b >= 0 && b <= 255);
    printf("<<<<<hsl_to_rgb\n");
  #endif

  uint32_t combined = r << 0 | g << 8 | b << 16 | 0xFF << 24;

  *rgb = combined;
}

void hslify_frame(const s0rter_instance_t* instance,
                  const uint32_t* rgb_frame) {
  const uint32_t* rgb = rgb_frame;
  hsl_t* hsl_buffer = instance->hsl_buffer;

  for (unsigned int y = 0; y < instance->height; y++) {
    for (unsigned int x = 0; x < instance->width; x++, rgb++, hsl_buffer++) {
      rgb_to_hsl(rgb, hsl_buffer);
    }
  }
}

void rgbify_frame(const s0rter_instance_t* instance,
                  uint32_t* rgb_frame) {
  uint32_t* rgb = rgb_frame;
  const hsl_t* hsl_buffer = instance->hsl_buffer;

  for (unsigned int y = 0; y < instance->height; y++) {
    for (unsigned int x = 0; x < instance->width; x++, rgb++, hsl_buffer++) {
      hsl_to_rgb(hsl_buffer, rgb);
    }
  }
}

int compare_hsl(const void *x, const void *y) {
  const hsl_t* left  = (hsl_t*)x;
  const hsl_t* right = (hsl_t*)y;

  if (left->lightness == 0 && right->lightness != 0) { return -1; }
  if (right->lightness == 0 && left->lightness != 0) { return 1; }

  if (left->lightness == 1 && right->lightness != 1) { return 1; }
  if (right->lightness == 1 && left->lightness != 1) { return -1; }

  if (left->hue < right->hue) { return -1; }
  if (left->hue > right->hue) { return 1; }

  if (left->saturation < right->saturation) { return -1; }
  if (left->saturation > right->saturation) { return 1; }

  if (left->lightness < right->lightness) { return -1; }
  if (left->lightness > right->lightness) { return 1; }

  return 0;
}

void f0r_update(f0r_instance_t f0r_instance,
                double time,
                const uint32_t* inframe,
                uint32_t* outframe) {
  (void)time;

  assert(f0r_instance);
  s0rter_instance_t* instance = (s0rter_instance_t*)f0r_instance;

  hslify_frame(instance, inframe);
  qsort(
    instance->hsl_buffer, // array to sort
    instance-> width * instance->height, // number of elements to sort
    sizeof(hsl_t), // size of each element
    compare_hsl // comparator to use
  );
  rgbify_frame(instance, outframe);
}
