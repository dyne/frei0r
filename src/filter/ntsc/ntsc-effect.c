#include <stdlib.h>
#include <string.h>

#include "frei0r.h"
#include "frei0r/math.h"

#include "crt_core.h"

// the actual NTSC emulation code is from here: https://github.com/LMP88959/NTSC-CRT


typedef struct ntsc_instance
{
    // image dimensions
    int width;
    int height;

    // parameters
    struct CRT crt;
    struct NTSC_SETTINGS ntsc;

    int noise;
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
    info->minor_version = 2;
    info->num_params = 3;
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
        info->name = "Progressive Scan";
        info->explanation = "Toggles progressive scan (Interlaced if off).";
        info->type = F0R_PARAM_BOOL;
        break;

    case 2:
        info->name = "Scanlines";
        info->explanation = "Draw borders between scanlines.";
        info->type = F0R_PARAM_BOOL;
        break;
    }
}


f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
    ntsc_instance_t* inst = (ntsc_instance_t*)calloc(1, sizeof(*inst));

    inst->width = width;
    inst->height = height;

    inst->ntsc.format = CRT_PIX_FORMAT_RGBA;    
    inst->ntsc.w = width;
    inst->ntsc.h = height;
    inst->ntsc.raw = 0;
    inst->ntsc.field = 0;
    inst->ntsc.frame = 0;
    inst->ntsc.as_color = 1;
    inst->ntsc.hue = 0;
    inst->ntsc.xoffset = 0;
    inst->ntsc.yoffset = 0;
    inst->ntsc.iirs_initialized = 0;
    
    inst->noise = 0;
    inst->progressive = 0;
    inst->field = 0;

    crt_init(&(inst->crt), width, height, CRT_PIX_FORMAT_RGBA, NULL);
    inst->crt.blend = 0;
    inst->crt.scanlines = 0;
    

    
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
        inst->noise = *((double*)param) * 200;
        break;
    case 1:
        inst->progressive = (*((double*)param) >= 0.5);
        break;
    case 2:
        inst->crt.scanlines = (*((double*)param) >= 0.5);
        break;
    }
}

void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index)
{
    ntsc_instance_t* inst = (ntsc_instance_t*)instance;
    switch(param_index)
    {
    case 0:
        *((double*)param) = (inst->noise / 200);
        break;
    case 1:
        *((double*)param) = (inst->progressive ? 1.0 : 0.0);
        break;
    case 2:
        *((double*)param) = (inst->crt.scanlines ? 1.0 : 0.0);
        break;
    }
}


void f0r_update(f0r_instance_t instance, double time, const uint32_t* inframe, uint32_t* outframe)
{    
    ntsc_instance_t* inst = (ntsc_instance_t*)instance;
    
    // clear the output frame
    memset(outframe, 0, inst->width * inst->height * sizeof(uint32_t));
    inst->crt.blend = 0;
    
    // set everything up for the simulation
    inst->crt.out = (char*)outframe;
    inst->ntsc.data = (const char*)inframe;

_render_field:
    inst->ntsc.field = inst->field & 1;

    if (inst->ntsc.field == 0) {
        /* a frame is two fields */
        inst->ntsc.frame ^= 1;
    }

    // encode and decode ntsc signal
    crt_modulate(&(inst->crt), &(inst->ntsc));
    crt_demodulate(&(inst->crt), inst->noise);
    
    inst->field ^= 1;
    // if we are in progressive mode, we render both fields onto the frame.
    // in interlaced mode, we will hit the opposite field on the next frame.
    if(inst->field && inst->progressive)
    {
        // if we are not leaving scanlines blank, we will want to blend the frames in progressive mode
        if(!inst->crt.scanlines)
        {
            inst->crt.blend = 1;
        }
        goto _render_field;
    }
}
