#include <stdlib.h>

#include "frei0r.h"
#include "frei0r/math.h"

#include "crt_core.h"

// the actual NTSC emulation code is modified from here: https://github.com/LMP88959/NTSC-CRT

typedef struct ntsc_instance
{
    // image dimensions
    int width;
    int height;

    // parameters
    struct CRT crt;
    struct NTSC_SETTINGS ntsc_settings;

    int noise;
    double vhs_speed;

    int field;
    int progressive;

} ntsc_instance_t;

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
    info->name = "NTSC";
    info->author = "EMMIR, esmane";
    info->explanation = "Simulates NTSC analog video.";
    info->plugin_type = F0R_PLUGIN_TYPE_FILTER;
    info->color_model = F0R_COLOR_MODEL_RGBA8888;
    info->frei0r_version = FREI0R_MAJOR_VERSION;
    info->major_version = 0;
    info->minor_version = 1;
    info->num_params = 8;
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
    switch(param_index)
    {
    case 0:
        info->name = "Signal Noise";
        info->explanation = "Amount of noise introduced into the NTSC signal.";
        info->type = F0R_PARAM_DOUBLE;
        break;

    case 1:
        info->name = "VHS Speed";
        info->explanation = "Simulates VHS. 0 = off, 1 = SP, 2 = LP, 3 = EP";
        info->type = F0R_PARAM_DOUBLE;
        break;

    case 2:
        info->name = "VHS Noise";
        info->explanation = "Toggles VHS noise at the bottom of the screen";
        info->type = F0R_PARAM_BOOL;
        break;

    case 3:
        info->name = "Aberration";
        info->explanation = "Toggles VHS aberration at the bottom of the screen. Not visible if V-sync is on.";
        info->type = F0R_PARAM_BOOL;
        break;

    case 4:
        info->name = "Force V-sync";
        info->explanation = "Forces V-sync even when the signal is really noisy.";
        info->type = F0R_PARAM_BOOL;
        break;

    case 5:
        info->name = "Force H-sync";
        info->explanation = "Forces V-sync even when the signal is really noisy.";
        info->type = F0R_PARAM_BOOL;
        break;

    case 6:
        info->name = "Progressive Scan";
        info->explanation = "Toggles progressive scan (Interlaced if off).";
        info->type = F0R_PARAM_BOOL;
        break;

    case 7:
        info->name = "Blend";
        info->explanation = "Blends frames.";
        info->type = F0R_PARAM_BOOL;
        break;
    }
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
    ntsc_instance_t* inst = (ntsc_instance_t*)calloc(1, sizeof(*inst));

    inst->width = width;
    inst->height = height;

    inst->vhs_speed = 0.0;
    inst->field = 0;
    inst->progressive = 1;

    crt_init(&(inst->crt), width, height, NULL);

    // init NTSC_SETTINGS
    inst->ntsc_settings.data = NULL;
    inst->ntsc_settings.w = width;
    inst->ntsc_settings.h = height;
    inst->ntsc_settings.raw = 0;
    inst->ntsc_settings.as_color = 1;
    inst->ntsc_settings.field = 0;
    inst->ntsc_settings.frame = 0;
    inst->ntsc_settings.hue = 0;
    inst->ntsc_settings.xoffset = 0;
    inst->ntsc_settings.yoffset = 0;
    inst->ntsc_settings.do_aberration = 0;

    // these are changed by the vhs mode
    inst->ntsc_settings.vhs_mode = 0;
    inst->ntsc_settings.y_freq = 0;
    inst->ntsc_settings.i_freq = 0;
    inst->ntsc_settings.q_freq = 0;

    /* make sure your NTSC_SETTINGS struct is zeroed out before you do anything */
    inst->ntsc_settings.iirs_initialized = 0;

    inst->noise = 0;

    return (f0r_instance_t)inst;
}

void f0r_destruct(f0r_instance_t instance)
{
    ntsc_instance_t* inst = (ntsc_instance_t*)instance;
    free(instance);
}


void f0r_set_param_value(f0r_instance_t instance, f0r_param_t param, int param_index)
{
    ntsc_instance_t* inst = (ntsc_instance_t*)instance;
    switch(param_index)
    {
    case 0:
        inst->noise = *((double*)param) * 100;
        break;
    case 1:
        inst->ntsc_settings.vhs_mode = (int)CLAMP(*((double*)param) * 4, 0, 4);
        inst->ntsc_settings.iirs_initialized = 0;
        break;
    case 2:
        inst->crt.do_vhs_noise = (*((double*)param) >= 0.5);
        break;
    case 3:
        inst->ntsc_settings.do_aberration = (*((double*)param) >= 0.5);
        break;
    case 4:
        inst->crt.crt_do_vsync = !(*((double*)param) >= 0.5);
        break;
    case 5:
        inst->crt.crt_do_hsync = !(*((double*)param) >= 0.5);
        break;
    case 6:
        inst->progressive = (*((double*)param) >= 0.5);
        break;
    case 7:
        inst->crt.blend = (*((double*)param) >= 0.5);
        break;
    }
}

void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index)
{
    ntsc_instance_t* inst = (ntsc_instance_t*)instance;
    switch(param_index)
    {
    case 0:
        *((double*)param) = (inst->noise / 100);
        break;
    case 1:
        *((double*)param) = (inst->ntsc_settings.vhs_mode / 4);
        break;
    case 2:
        *((double*)param) = (inst->crt.do_vhs_noise ? 1.0 : 0.0);
        break;
    case 3:
        *((double*)param) = (inst->ntsc_settings.do_aberration ? 1.0 : 0.0);
        break;
    case 4:
        *((double*)param) = !(inst->crt.crt_do_vsync ? 1.0 : 0.0);
        break;
    case 5:
        *((double*)param) = !(inst->crt.crt_do_hsync ? 1.0 : 0.0);
        break;
    case 6:
        *((double*)param) = (inst->progressive ? 1.0 : 0.0);
        break;
    case 7:
        *((double*)param) = (inst->crt.blend ? 1.0 : 0.0);
        break;
    }
}


void f0r_update(f0r_instance_t instance, double time, const uint32_t* inframe, uint32_t* outframe)
{
    ntsc_instance_t* inst = (ntsc_instance_t*)instance;

    inst->crt.out = (char*)outframe;
    inst->ntsc_settings.data = (const char*)inframe;

    inst->ntsc_settings.field = inst->field & 1;
    if (inst->ntsc_settings.field == 0) {
        /* a frame is two fields */
        inst->ntsc_settings.frame ^= 1;
    }

    crt_modulate(&(inst->crt), &(inst->ntsc_settings));
    crt_demodulate(&(inst->crt), inst->noise);
    if(!inst->progressive)
    {
        inst->field ^= 1;
    }
}
