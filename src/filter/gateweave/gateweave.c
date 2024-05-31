#include <stdlib.h>
#include <math.h>
#include "frei0r.h"
#include "frei0r_math.h"


// let's try to walk through everything because i'm a moron
// this is the struct for the effect instance
typedef struct gateweave_instance
{
    // image dimensions
    unsigned int width;
    unsigned int height;

    // parameters
    double interval;
    double max_move_x;
    double max_move_y;

    // other variables that the effect uses to keep track of things
    double next_key_x;
    double next_key_y;
    double prev_key_x;
    double prev_key_y;

} gateweave_instance_t;


// these functions are for the effect
static double gateweave_random_range(double range, double last)
{
    if(range <= 0)
    {
        return 0;
    }

    // the maximum shift is 10 pixels
    // since range includes fractional values, we want to multiply it by 100
    // this will generate an integer between -100 and 100 for the shift
    // and then we divide by 100 to receive a decimal answer between -10.00 and 10.00
    range *= 10;
    int int_range = range * 100;
    int_range = (rand() % (int_range * 2)) - int_range;
    double ret = int_range / 100.0;

    // we don't want to generate a value similar to one we already generated twice in a row
    ret = CLAMP(ret, -range, range);
    if((ret > 0 && ret >= last - 0.12) || (ret < 0 && ret <= last + 0.12))
    {
        ret *= -1;
    }
    return ret;
}

static inline double gateweave_lerp(double v0, double v1, double t)
{
    return v0 + t * (v1 - v0);
}


// this function mixes the colors of two pixels
// a is the original pixel that will be changed
// b is the second pixel that will not be changed
// the amount is how much of the second pixel will be mixed
// an amount of 1 means pixel a will be equal to pixel b
// an amount of 0 means pixel a will be equal to pixel a (no change)
// an amount of 0.5 means pixel a will be equal to 0.5 * pixel a + 0.5 * pixel b
static uint32_t gateweave_blend_color(uint32_t a, uint32_t b, double amount)
{
    uint8_t c_a;
    uint8_t c_b;
    uint8_t c_g;
    uint8_t c_r;
    uint32_t output;

    //   RRGGBBAA
    // 0xFFFFFFFF
    c_a = CLAMP0255(((a & 0xFF000000) >> 24) * (1 - amount) + ((b & 0xFF000000) >> 24) * (amount));
    c_b = CLAMP0255(((a & 0x00FF0000) >> 16) * (1 - amount) + ((b & 0x00FF0000) >> 16) * (amount));
    c_g = CLAMP0255(((a & 0x0000FF00) >>  8) * (1 - amount) + ((b & 0x0000FF00) >>  8) * (amount));
    c_r = CLAMP0255(((a & 0x000000FF))       * (1 - amount) + ((b & 0x000000FF))       * (amount));

    output = (output & 0xFFFFFF00) |  (uint32_t)c_r;
    output = (output & 0xFFFF00FF) | ((uint32_t)c_g <<  8);
    output = (output & 0xFF00FFFF) | ((uint32_t)c_b << 16);
    output = (output & 0x00FFFFFF) | ((uint32_t)c_a << 24);

    return output;
}

// this function is the one that ultimately manipulates the image
// it shifts the image and can do so to a value less than one pixel
static inline void gateweave_shift_picture(const uint32_t* in, uint32_t* out, double shift_x, double shift_y, int w, int h)
{
    uint32_t* buf = (uint32_t*)calloc(w * h, sizeof(uint32_t));

    // first we shift to the nearest whole pixel
    int int_shift_x = shift_x;
    int int_shift_y = shift_y;
    int pixel_shift = int_shift_x + (int_shift_y * w);
    for(unsigned int i = 0; i < w * h; i++)
    {
        if(i + pixel_shift < 0 || i + pixel_shift >= w * h)
        {
            *(buf + i) = 0;
        }
        else
        {
            *(buf + i) = *(in + i + pixel_shift);
        }
    }

    // then we shift the fractional component
    // let's think about how this works
    // let's suppose we shift left 1.5 pixels and shift down 1.25 pixels
    // after shifting 1 pixel left and 1 pixel down, we still have the 0.5 and 0.25 shifts left to do
    // to do this we want each pixel to equal 50% the current pixel and 50% the pixel to its left
    // we also want each pixel to equal 75% the current pixel and 25% the pixel below it
    // but this isn't actually possible to do
    shift_x -= int_shift_x;
    shift_y -= int_shift_y;

    int shift_component_x;
    int shift_component_y;

    char larger_x_comp = 0;
    if(fabs(shift_x) > fabs(shift_y))
    {
        larger_x_comp = 1;
    }

    if(shift_x < 0)
    {
        shift_component_x = -1;
    }
    else
    {
        shift_component_x = 1;
    }
    if(shift_y < 0)
    {
        shift_component_y = -w;
    }
    else
    {
        shift_component_y = w;
    }

    uint32_t v = 0;
    for(unsigned int i = 0; i < w * h; i++)
    {
        if(!(i + shift_component_x < 0 || i + shift_component_x >= w * h || i + shift_component_x + shift_component_y < 0 || i + shift_component_x + shift_component_y >= w * h))
        {
            if(larger_x_comp)
            {
                v = gateweave_blend_color(*(buf + i + shift_component_x), *(buf + i + shift_component_x + shift_component_y), shift_y);
                *(out + i) = gateweave_blend_color(*(buf + i), v, shift_x);
            }
            else
            {
                v = gateweave_blend_color(*(buf + i + shift_component_y), *(buf + i + shift_component_x + shift_component_y), shift_x);
                *(out + i) = gateweave_blend_color(*(buf + i), v, shift_y);
            }
        }
    }
    free(buf);
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
    info->name = "Gate Weave";
    info->author = "esmane";
    info->explanation = "Randomly moves frame around to simulate film gate weave.";
    info->plugin_type = F0R_PLUGIN_TYPE_FILTER;
    info->color_model = F0R_COLOR_MODEL_RGBA8888;
    info->frei0r_version = FREI0R_MAJOR_VERSION;
    info->major_version = 0;
    info->minor_version = 2;
    info->num_params = 3;
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
    switch(param_index)
    {
    case 0:
        info->name = "Interval";
        info->explanation = "The amount of time before the position is randomized again. The larger the number the slower the picture will move.";
        info->type = F0R_PARAM_DOUBLE;
        break;

    case 1:
        info->name = "Maximum Horizontal Movement";
        info->explanation = "The maximum distance the picture could move left or right. The larger the number the more the picture moves and the less subtle the effect.";
        info->type = F0R_PARAM_DOUBLE;
        break;

    case 2:
        info->name = "Maximum Vertical Movement";
        info->explanation = "The maximum distance the picture could move up or down. The larger the number the more the picture moves and the less subtle the effect.";
        info->type = F0R_PARAM_DOUBLE;
        break;
    }
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
    gateweave_instance_t* inst = (gateweave_instance_t*)calloc(1, sizeof(*inst));

    inst->width = width;
    inst->height = height;
    inst->interval = 0.6;
    inst->max_move_x = 0.2;
    inst->max_move_y = 0.2;

    // other variables that the effect uses to keep track of things
    inst->next_key_x = gateweave_random_range(inst->max_move_x, 0);
    inst->next_key_y = gateweave_random_range(inst->max_move_y, 0);
    inst->prev_key_x = 0;
    inst->prev_key_y = 0;

    return (f0r_instance_t)inst;
}

void f0r_destruct(f0r_instance_t instance)
{
    gateweave_instance_t* inst = (gateweave_instance_t*)instance;
    free(instance);
}

void f0r_set_param_value(f0r_instance_t instance, f0r_param_t param, int param_index)
{
    gateweave_instance_t* inst = (gateweave_instance_t*)instance;
    switch(param_index)
    {
    case 0:
        inst->interval = *((double*)param);
        break;
    case 1:
        inst->max_move_x = *((double*)param);
        break;
    case 2:
        inst->max_move_y = *((double*)param);
        break;
    }
}

void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index)
{
    gateweave_instance_t* inst = (gateweave_instance_t*)instance;
    switch(param_index)
    {
    case 0:
        *((double*)param) = inst->interval;
        break;
    case 1:
        *((double*)param) = inst->max_move_x;
        break;
    case 2:
        *((double*)param) = inst->max_move_y;
        break;
    }
}

void f0r_update(f0r_instance_t instance, double time, const uint32_t* inframe, uint32_t* outframe)
{
    gateweave_instance_t* inst = (gateweave_instance_t*)instance;
    inst->next_key_x = gateweave_random_range(inst->max_move_x, inst->next_key_x);
    inst->next_key_y = gateweave_random_range(inst->max_move_y, inst->next_key_y);

    inst->prev_key_x = gateweave_lerp(inst->next_key_x, inst->prev_key_x, inst->interval);
    inst->prev_key_y = gateweave_lerp(inst->next_key_y, inst->prev_key_y, inst->interval);

    gateweave_shift_picture(inframe, outframe, inst->prev_key_x, inst->prev_key_y, inst->width, inst->height);
}
