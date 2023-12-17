#include <stdlib.h>
#include "frei0r.h"


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

    uint32_t* buf;

} filmgrain_instance_t;


// these functions are for the effect
inline uint8_t random_range_uint8(uint8_t min, uint8_t max)
{
    if(min == max)
    {
        return min;
    }
    return (rand() % (max - min)) + min;
}

inline uint8_t clamp_grain(int x)
{
    if(x < 0)
    {
        return 0;
    }
    if(x > 255)
    {
        return 255;
    }
    return (uint8_t)x;
}

inline uint32_t reduce_color_range(uint32_t color, uint8_t threshold, int flicker)
{
    if(color > 255 - threshold)
    {
        return 255 - threshold;
    }
    if(color < threshold)
    {
        return threshold;
    }
    return clamp_grain(color + flicker);
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

    inst->width = width;
    inst->height = height;
    inst->grain_amt = 0.5;
    inst->grain_r = 0.75;
    inst->grain_g = 1.0;
    inst->grain_b = 0.5;
    inst->blur_amt = 0.5;
    inst->dust_amt = 0.2;
    inst->flicker_amt = 0.5;
    inst->buf = (uint32_t*)calloc(width * height, sizeof(uint32_t));

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
    case 6:
        *((double*)param) = inst->flicker_amt;
        break;
    }
}


void f0r_update(f0r_instance_t instance, double time, const uint32_t* inframe, uint32_t* outframe)
{
    filmgrain_instance_t* inst = (filmgrain_instance_t*)instance;

    uint32_t r;
    uint32_t g;
    uint32_t b;
    uint32_t a;
    uint8_t grain;
    uint8_t reduce_t = random_range_uint8(0, inst->flicker_amt * 5) + inst->grain_amt * 40;
    int flicker = random_range_uint8(0, inst->flicker_amt * 8);

    if(rand() % 2)
    {
        flicker *= -1;
    }

    // first grain
    if(inst->blur_amt == 0.0)
    {
        inst->buf = outframe;
    }
    for(unsigned int i = 0; i < inst->height * inst->width; i++)
    {
        // dust
        if(rand() < 2 && rand() < 2)
        {
            if(rand() < inst->dust_amt * 30)
            {
                if(rand() % 2 == 0)
                {
                    r = 0;
                    g = 0;
                    b = 0;
                }
                else
                {
                    r = 255;
                    g = 255;
                    b = 255;
                }
            }
        }
        else
        {
            // reducing the range of each color helps look more "filmish"
            b = reduce_color_range((*(inframe + i) & 0x00FF0000) >> 16, reduce_t, flicker);
            g = reduce_color_range((*(inframe + i) & 0x0000FF00) >>  8, reduce_t, flicker);
            r = reduce_color_range( *(inframe + i) & 0x000000FF       , reduce_t, flicker);

            grain = random_range_uint8(0, inst->grain_amt * (40 + ((r + g + b) >> 5)));

            b = clamp_grain(b - (grain * inst->grain_b));
            g = clamp_grain(g - (grain * inst->grain_g));
            r = clamp_grain(r - (grain * inst->grain_r));
        }

        *(inst->buf + i) = (*(inst->buf + i) & 0xFFFFFF00) | r;
        *(inst->buf + i) = (*(inst->buf + i) & 0xFFFF00FF) | ((uint32_t)g <<  8);
        *(inst->buf + i) = (*(inst->buf + i) & 0xFF00FFFF) | ((uint32_t)b << 16);
    }

    // then blur
    if(inst->blur_amt != 0.0)
    {
        unsigned int pixel_count;
        int blur_range;
        for(int i = 0; i < inst->height * inst->width; i++)
        {
            pixel_count = 1;
            a = (*(inframe + i) & 0xFF000000) >> 24;
            b = ((*(inst->buf + i) & 0x00FF0000) >> 16);
            g = ((*(inst->buf + i) & 0x0000FF00) >>  8);
            r = ((*(inst->buf + i) & 0x000000FF)      );


            blur_range = random_range_uint8(0, inst->blur_amt * 4);
            for(int xx = -blur_range - 1; xx < blur_range; xx++)
            {
                for(int yy = -blur_range - 1; yy < blur_range; yy++)
                {
                    if((i + xx + (yy * inst->width)) > 0 && (i + xx + (yy * inst->width)) < (inst->width * inst->height) - 1)
                    {
                        b += (*(inst->buf + i + xx + (yy * inst->width)) & 0x00FF0000) >> 16;
                        g += (*(inst->buf + i + xx + (yy * inst->width)) & 0x0000FF00) >>  8;
                        r += (*(inst->buf + i + xx + (yy * inst->width)) & 0x000000FF);
                        pixel_count++;
                    }
                }
            }

            b = b / pixel_count;
            g = g / pixel_count;
            r = r / pixel_count;

            *(outframe + i) = (*(outframe + i) & 0xFFFFFF00) | r;
            *(outframe + i) = (*(outframe + i) & 0xFFFF00FF) | ((uint32_t)g <<  8);
            *(outframe + i) = (*(outframe + i) & 0xFF00FFFF) | ((uint32_t)b << 16);
        }
    }
}
