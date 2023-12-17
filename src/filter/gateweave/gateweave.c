#include <stdlib.h>
#include "frei0r.h"


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
    unsigned int step;
    double next_key_x;
    double next_key_y;
    double prev_key_x;
    double prev_key_y;

    uint32_t* buf;

} gateweave_instance_t;


// these functions are for the effect
inline double gateweave_random_range(double range, double last)
{
    // the maximum shift is 10 pixels
    // since range includes fractional values, we want to multiply it by 100
    // this will generate an integer between -100 and 100 for the shift
    // and then we divide by 100 to receive a decimal answer between -10.00 and 10.00
    range *= 10;
    int int_range = range * 100;
    int_range = (rand() % (int_range * 2)) - int_range;
    double ret = int_range / 100;

    // we don't want to generate a value we already generate twice in a row
    if(ret > range)
    {
        ret = range;
    }
    else if(ret < -range)
    {
        ret = -range;
    }
    if(abs(ret) >= abs(last) - 0.12)
    {
        ret *= -1;
    }
    return ret;
}

double gateweave_lerp(double v0, double v1, double t)
{
    return v0 + t * (v1 - v0);
}

inline double gateweave_abs(double x)
{
    if(x < 0)
    {
        return -x;
    }
    else
    {
        return x;
    }
}

inline int gateweave_clamp(double x)
{
    if(x > 255)
    {
        return 255;
    }
    else if(x < 0)
    {
        return 0;
    }
    else
    {
        return (int)x;
    }
}

// this function mixes the colors of two pixels
// a is the original pixel that will be changed
// b is the second pixel that will not be changed
// the amount is how much of the second pixel will be mixed
// an amount of 1 means pixel a will be equal to pixel b
// an amount of 0 means pixel a will be equal to pixel a (no change)
// an amount of 0.5 means pixel a will be equal to 0.5 * pixel a + 0.5 * pixel b
uint32_t gateweave_blend_color(uint32_t a, uint32_t b, double amount)
{
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t alpha;
    uint32_t output;

    //   RRGGBBAA
    // 0xFFFFFFFF
    red   = gateweave_clamp(((a & 0xFF000000) >> 24) * (1 - amount) + ((b & 0xFF000000) >> 24) * (amount));
    green = gateweave_clamp(((a & 0x00FF0000) >> 16) * (1 - amount) + ((b & 0x00FF0000) >> 16) * (amount));
    blue  = gateweave_clamp(((a & 0x0000FF00) >>  8) * (1 - amount) + ((b & 0x0000FF00) >>  8) * (amount));
    alpha = gateweave_clamp(((a & 0x000000FF)) * (1 - amount) + ((b & 0x000000FF)) * (amount));

    output = (output & 0xFFFFFF00) | alpha;
    output = (output & 0xFFFF00FF) | ((uint32_t)blue  <<  8);
    output = (output & 0xFF00FFFF) | ((uint32_t)green << 16);
    output = (output & 0x00FFFFFF) | ((uint32_t)red   << 24);

    return output;
}

// this function is the one that ultimately manipulates the image
// it shifts the image and can do so to a value less than one pixel
void gateweave_shift_picture(const uint32_t* in, uint32_t* out, uint32_t* buf, double shift_x, double shift_y, int w, int h)
{
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
    if(gateweave_abs(shift_x) > gateweave_abs(shift_y))
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
    inst->interval = 0.15;
    inst->max_move_x = 0.2;
    inst->max_move_y = 0.2;

    // other variables that the effect uses to keep track of things
    inst->step = 0;
    inst->next_key_x = gateweave_random_range(inst->max_move_x, 0);
    inst->next_key_y = gateweave_random_range(inst->max_move_y, 0);
    inst->prev_key_x = 0;
    inst->prev_key_y = 0;

    // and allocate memory for the buffers
    inst->buf = (uint32_t*)calloc(width * height, sizeof(uint32_t));

    return (f0r_instance_t)inst;
}

void f0r_destruct(f0r_instance_t instance)
{
    gateweave_instance_t* inst = (gateweave_instance_t*)instance;
    free(inst->buf);
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

    if(inst->step > inst->interval * 20)
    {
        // create a new position
        inst->step = 0;
        inst->prev_key_x = inst->next_key_x;
        inst->prev_key_y = inst->next_key_y;
        if(inst->max_move_x > 0)
        {
            inst->next_key_x = gateweave_random_range(inst->max_move_x, inst->next_key_x);
        }
        else
        {
            inst->next_key_x = 0;
        }

        if(inst->max_move_y > 0)
        {
            inst->next_key_y = gateweave_random_range(inst->max_move_y, inst->next_key_y);
        }
        else
        {
            inst->next_key_y = 0;
        }
    }
    /* old way of doing this
    double xs = inst->prev_key_x + (inst->next_key_x - inst->prev_key_x) * (inst->step / (inst->interval * 40));
    double yx = inst->prev_key_y + (inst->next_key_y - inst->prev_key_y) * (inst->step / (inst->interval * 40));


    inst->step++;
    gateweave_shift_picture(inframe, outframe, inst->buf, xs, yx, inst->width, inst->height);
    */

    // new way of doing this
    inst->prev_key_x = gateweave_lerp(inst->prev_key_x, inst->next_key_x, 0.1);
    inst->prev_key_y = gateweave_lerp(inst->prev_key_y, inst->next_key_y, 0.1);

    inst->step++;
    gateweave_shift_picture(inframe, outframe, inst->buf, inst->prev_key_x, inst->prev_key_y, inst->width, inst->height);
}
