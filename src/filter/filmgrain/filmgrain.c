#include <stdlib.h>
#include <stdint.h>
#include "frei0r.h"
#include "frei0r/math.h"


typedef struct flimgrain_instance
{
    // image dimensions
    int width;
    int height;

    // parameters
    double grain_amt;
    double grain_r;
    double grain_g;
    double grain_b;
    double blur_amt;
    double dust_amt;
    double flicker_amt;

    // persistent scratch buffer for blur pass
    uint32_t* buf;
} filmgrain_instance_t;


// these functions are for the effect
static inline uint8_t random_range_uint8(uint8_t x)
{
    // never divide by zero
    if(x < 1)
    {
        return 0;
    }
    return (uint8_t)(rand() % x);
}

static inline uint32_t reduce_color_range(uint32_t color, uint8_t threshold, int flicker)
{
    return CLAMP0255(CLAMP((int32_t)color, (int32_t)(threshold >> 1), (int32_t)(255 - threshold)) + flicker);
}

#define DUST_RAND_LIMIT 1000000000
static inline int big_rand(void)
{
    #if RAND_MAX > DUST_RAND_LIMIT
    return rand() % DUST_RAND_LIMIT;
    #else
    return (rand() * (RAND_MAX + 1) + rand()) % DUST_RAND_LIMIT;
    #endif
}

static inline uint32_t sum_row_swar(const uint32_t* row, int count, uint32_t* g_acc)
{
    uint32_t rb = 0;
    uint32_t g = 0;
    for (int k = 0; k < count; ++k)
    {
        uint32_t px = row[k];
        rb += px & 0x00FF00FFu;
        g += px & 0x0000FF00u;
    }
    *g_acc += g;
    return rb;
}

// these functions are for frei0r
// mostly copy/paste/slightly modified from the other frei0r effects
int f0r_init()
{
    return 1;
}

void f0r_deinit()
{}

void f0r_get_plugin_info(f0r_plugin_info_t* info)
{
    info->name = "Film Grain";
    info->author = "esmane";
    info->explanation = "Adds film-like grain and softens the picture.";
    info->plugin_type = F0R_PLUGIN_TYPE_FILTER;
    info->color_model = F0R_COLOR_MODEL_RGBA8888;
    info->frei0r_version = FREI0R_MAJOR_VERSION;
    info->major_version = 0;
    info->minor_version = 1;
    info->num_params = 7;
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
    switch(param_index)
    {
    case 0:
        info->name = "Grain Amount";
        info->explanation = "The intensity of the grain.";
        info->type = F0R_PARAM_DOUBLE;
        break;

    case 1:
        info->name = "Red Grain";
        info->explanation = "The percentage of grain applied to the red channel.";
        info->type = F0R_PARAM_DOUBLE;
        break;

    case 2:
        info->name = "Green Grain";
        info->explanation = "The percentage of grain applied to the green channel.";
        info->type = F0R_PARAM_DOUBLE;
        break;

    case 3:
        info->name = "Blue Grain";
        info->explanation = "The percentage of grain applied to the blue channel.";
        info->type = F0R_PARAM_DOUBLE;
        break;

    case 4:
        info->name = "Blur Amount";
        info->explanation = "The intensity of the blur.";
        info->type = F0R_PARAM_DOUBLE;
        break;

    case 5:
        info->name = "Dust Amount";
        info->explanation = "The amount of dust particles on the image.";
        info->type = F0R_PARAM_DOUBLE;
        break;

    case 6:
        info->name = "Flicker";
        info->explanation = "The amount of variation in brightness between frames.";
        info->type = F0R_PARAM_DOUBLE;
        break;
    }
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
    filmgrain_instance_t* inst = (filmgrain_instance_t*)calloc(1, sizeof(*inst));

    inst->width = (int)width;
    inst->height = (int)height;
    inst->grain_amt = 0.5;
    inst->grain_r = 0.75;
    inst->grain_g = 1.0;
    inst->grain_b = 0.5;
    inst->blur_amt = 0.5;
    inst->dust_amt = 0.2;
    inst->flicker_amt = 0.5;
    inst->buf = (uint32_t*)malloc((size_t)width * height * sizeof(uint32_t));

    return (f0r_instance_t)inst;
}

void f0r_destruct(f0r_instance_t instance)
{
    filmgrain_instance_t* inst = (filmgrain_instance_t*)instance;
    free(inst->buf);
    free(instance);
}


void f0r_set_param_value(f0r_instance_t instance, f0r_param_t param, int param_index)
{
    filmgrain_instance_t* inst = (filmgrain_instance_t*)instance;
    switch(param_index)
    {
    case 0:
        inst->grain_amt = *((double*)param);
        break;
    case 1:
        inst->grain_r = *((double*)param);
        break;
    case 2:
        inst->grain_g = *((double*)param);
        break;
    case 3:
        inst->grain_b = *((double*)param);
        break;
    case 4:
        inst->blur_amt = *((double*)param);
        break;
    case 5:
        inst->dust_amt = *((double*)param);
        break;
    case 6:
        inst->flicker_amt = *((double*)param);
        break;
    }
}

void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index)
{
    filmgrain_instance_t* inst = (filmgrain_instance_t*)instance;
    switch(param_index)
    {
    case 0:
        *((double*)param) = inst->grain_amt;
        break;
    case 1:
        *((double*)param) = inst->grain_r;
        break;
    case 2:
        *((double*)param) = inst->grain_g;
        break;
    case 3:
        *((double*)param) = inst->grain_b;
        break;
    case 4:
        *((double*)param) = inst->blur_amt;
        break;
    case 5:
        *((double*)param) = inst->dust_amt;
        break;
    case 6:
        *((double*)param) = inst->flicker_amt;
        break;
    }
}


void f0r_update(f0r_instance_t instance, double time, const uint32_t* inframe, uint32_t* outframe)
{
    (void)time;
    filmgrain_instance_t* inst = (filmgrain_instance_t*)instance;
    const int width = inst->width;
    const int height = inst->height;
    const int total = width * height;
    const int do_blur = (inst->blur_amt != 0.0);

    uint8_t reduce_t = (uint8_t)(random_range_uint8((uint8_t)(inst->flicker_amt * 5)) + inst->grain_amt * 40);
    int flicker = random_range_uint8((uint8_t)(inst->flicker_amt * 8));

    if(rand() % 2)
    {
        flicker *= -1;
    }

    /* Precompute LUTs for pass 1 */
    uint8_t reduce_lut[256];
    for (int c = 0; c < 256; ++c)
    {
        reduce_lut[c] = (uint8_t)reduce_color_range((uint32_t)c, reduce_t, flicker);
    }

    uint8_t grain_max_lut[24];
    for (int k = 0; k < 24; ++k)
    {
        grain_max_lut[k] = (uint8_t)(inst->grain_amt * (40 + k));
    }

    double grain_r_lut[256], grain_g_lut[256], grain_b_lut[256];
    for (int g = 0; g < 256; ++g)
    {
        grain_r_lut[g] = g * inst->grain_r;
        grain_g_lut[g] = g * inst->grain_g;
        grain_b_lut[g] = g * inst->grain_b;
    }

    const double dust_limit = inst->dust_amt * 1000.0;
    uint32_t* __restrict__ buf = do_blur ? inst->buf : outframe;
    const uint32_t* __restrict__ src = inframe;

    /* Pass 1: grain + dust (exact rand() sequence preserved) */
    for (int i = 0; i < total; ++i)
    {
        uint32_t px = src[i];
        uint32_t r, g, b;

        if (big_rand() < dust_limit)
        {
            if (rand() % 2 == 0)
            {
                r = 0; g = 0; b = 0;
            }
            else
            {
                r = 255; g = 255; b = 255;
            }
        }
        else
        {
            r = reduce_lut[px & 0xFFu];
            g = reduce_lut[(px >> 8) & 0xFFu];
            b = reduce_lut[(px >> 16) & 0xFFu];

            uint8_t grain = random_range_uint8(grain_max_lut[(r + g + b) >> 5]);

            b = CLAMP0255((int32_t)(b - grain_b_lut[grain]));
            g = CLAMP0255((int32_t)(g - grain_g_lut[grain]));
            r = CLAMP0255((int32_t)(r - grain_r_lut[grain]));
        }

        buf[i] = (px & 0xFF000000u) | r | (g << 8) | (b << 16);
    }

    /* Pass 2: variable-radius blur */
    if (do_blur)
    {
        const uint8_t blur_max_param = (uint8_t)(inst->blur_amt * 4);
        const int max_br = (blur_max_param > 0) ? (int)(blur_max_param - 1) : 0;
        const int safe_start = (max_br + 1) * width + (max_br + 2);
        const int safe_end = total - 1 - max_br * width - max_br;

        uint32_t* __restrict__ dst = outframe;
        const uint32_t* __restrict__ bsrc = buf;

        for (int i = 0; i < total; ++i)
        {
            const int blur_range = (int)random_range_uint8(blur_max_param);
            const uint32_t self_px = bsrc[i];
            uint32_t rb = self_px & 0x00FF00FFu;
            uint32_t g_acc = self_px & 0x0000FF00u;
            uint32_t r, g, b;

            if (i >= safe_start && i < safe_end)
            {
                if (blur_range == 0)
                {
                    uint32_t n0 = bsrc[i - width - 1];
                    rb += n0 & 0x00FF00FFu;
                    g_acc += n0 & 0x0000FF00u;
                    r = (rb & 0xFFFFu) >> 1;
                    b = (rb >> 16) >> 1;
                    g = (g_acc >> 8) >> 1;
                }
                else if (blur_range == 1)
                {
                    const uint32_t* row0 = bsrc + i - 2 * width - 2;
                    const uint32_t* row1 = row0 + width;
                    const uint32_t* row2 = row1 + width;
                    rb += (row0[0] & 0x00FF00FFu) + (row0[1] & 0x00FF00FFu) + (row0[2] & 0x00FF00FFu)
                        + (row1[0] & 0x00FF00FFu) + (row1[1] & 0x00FF00FFu) + (row1[2] & 0x00FF00FFu)
                        + (row2[0] & 0x00FF00FFu) + (row2[1] & 0x00FF00FFu) + (row2[2] & 0x00FF00FFu);
                    g_acc += (row0[0] & 0x0000FF00u) + (row0[1] & 0x0000FF00u) + (row0[2] & 0x0000FF00u)
                           + (row1[0] & 0x0000FF00u) + (row1[1] & 0x0000FF00u) + (row1[2] & 0x0000FF00u)
                           + (row2[0] & 0x0000FF00u) + (row2[1] & 0x0000FF00u) + (row2[2] & 0x0000FF00u);
                    /* Exact division by 10 for 0..2550 */
                    r = ((rb & 0xFFFFu) * 52429u) >> 19;
                    b = ((rb >> 16) * 52429u) >> 19;
                    g = ((g_acc >> 8) * 52429u) >> 19;
                }
                else
                {
                    const int span = 2 * blur_range + 1;
                    const uint32_t pixel_count = (uint32_t)(1 + span * span);
                    const uint32_t* row = bsrc + i - (blur_range + 1) * width - (blur_range + 1);
                    for (int yy = 0; yy < span; ++yy)
                    {
                        rb += sum_row_swar(row, span, &g_acc);
                        row += width;
                    }
                    r = (rb & 0xFFFFu) / pixel_count;
                    b = (rb >> 16) / pixel_count;
                    g = (g_acc >> 8) / pixel_count;
                }
            }
            else
            {
                unsigned int pixel_count = 1;
                for (int yy = -blur_range - 1; yy < blur_range; ++yy)
                {
                    int row_base = i + yy * width;
                    for (int xx = -blur_range - 1; xx < blur_range; ++xx)
                    {
                        int idx = row_base + xx;
                        if (idx > 0 && idx < total - 1)
                        {
                            uint32_t n = bsrc[idx];
                            rb += n & 0x00FF00FFu;
                            g_acc += n & 0x0000FF00u;
                            pixel_count++;
                        }
                    }
                }
                r = (rb & 0xFFFFu) / pixel_count;
                b = (rb >> 16) / pixel_count;
                g = (g_acc >> 8) / pixel_count;
            }

            dst[i] = (self_px & 0xFF000000u) | r | (g << 8) | (b << 16);
        }
    }
}
