/*
sharpness.c

This frei0r plugin is a port of Mplayer's unsharp mask filter
original by by Remi Guyomarch <rguyom@pobox.com>

Version 0.1	jul 2010

Copyright (C) 2010  Marko Cebokli    http://lea.hamradio.si/~s57uuu


 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/


//compile: gcc -c -fPIC -Wall sharpness.c -o sharpness.o
//link: gcc -shared -o sharpness.so sharpness.o

//#include <stdio.h>
#include <frei0r.h>
#include <stdlib.h>
//#include <math.h>
#include <assert.h>
#include <string.h>

#define MIN_MATRIX_SIZE 3
#define MAX_MATRIX_SIZE 63

typedef struct FilterParam {
    int msizeX, msizeY;
    double amount;
    uint32_t *SC[MAX_MATRIX_SIZE-1];
} FilterParam;


//----------------------------------------
//struktura za instanco efekta
typedef struct
{
    int h;
    int w;

    FilterParam fp;
    int size, ac;
    unsigned char *row_in[3];   /* L1-resident single-row planar input (R, G, B) */
    uint32_t *hbuf[3][5];       /* Ring buffer of horizontally blurred rows for 3x3 / 5x5 */
    uint32_t *hrow;             /* Scratch horizontal blur row for general sizes */
    uint32_t *sc_rgb[3][MAX_MATRIX_SIZE-1]; /* Per-channel vertical FSM state rows */
    unsigned char *row_out[3];  /* Single-row planar output (R, G, B) */
} inst;

static inline uint8_t clamp_u8(int32_t v)
{
    return (uint8_t)(v > 255 ? 255 : (v < 0 ? 0 : v));
}

static inline void unpack_row_rgb(const uint32_t *restrict in_row,
                                  uint8_t *restrict r, uint8_t *restrict g, uint8_t *restrict b,
                                  int w)
{
    for (int x = 0; x < w; x++) {
        uint32_t px = in_row[x];
        r[x] = (uint8_t)(px & 0xFFu);
        g[x] = (uint8_t)((px >> 8) & 0xFFu);
        b[x] = (uint8_t)((px >> 16) & 0xFFu);
    }
}

static inline void hblur_3(const uint8_t *restrict src, uint32_t *restrict dst, int w)
{
    if (w == 1) {
        dst[0] = (uint32_t)src[0] * 4u;
        return;
    }
    dst[0] = (uint32_t)src[0] * 3u + (uint32_t)src[1];
    for (int x = 1; x < w - 1; x++) {
        dst[x] = (uint32_t)src[x - 1] + ((uint32_t)src[x] << 1) + (uint32_t)src[x + 1];
    }
    dst[w - 1] = (uint32_t)src[w - 2] + (uint32_t)src[w - 1] * 3u;
}

static inline void hblur_5(const uint8_t *restrict src, uint32_t *restrict dst, int w)
{
    if (w < 5) {
        for (int x = 0; x < w; x++) {
            int xm2 = x - 2 < 0 ? 0 : x - 2;
            int xm1 = x - 1 < 0 ? 0 : x - 1;
            int xp1 = x + 1 >= w ? w - 1 : x + 1;
            int xp2 = x + 2 >= w ? w - 1 : x + 2;
            dst[x] = (uint32_t)src[xm2] + 4u * src[xm1] + 6u * src[x] + 4u * src[xp1] + src[xp2];
        }
        return;
    }
    dst[0] = 11u * src[0] + 4u * src[1] + src[2];
    dst[1] = 5u * src[0] + 6u * src[1] + 4u * src[2] + src[3];
    for (int x = 2; x < w - 2; x++) {
        dst[x] = (uint32_t)src[x - 2] + 4u * src[x - 1] + 6u * src[x] + 4u * src[x + 1] + src[x + 2];
    }
    dst[w - 2] = (uint32_t)src[w - 4] + 4u * src[w - 3] + 6u * src[w - 2] + 5u * src[w - 1];
    dst[w - 1] = (uint32_t)src[w - 3] + 4u * src[w - 2] + 11u * src[w - 1];
}

static void unsharp_3x3_rgba(const uint32_t *restrict inframe, uint32_t *restrict outframe, inst *in)
{
    const int w = in->w;
    const int h = in->h;
    const int32_t amount = (int32_t)(in->fp.amount * 65536.0);

    uint8_t *restrict ri = in->row_in[0];
    uint8_t *restrict gi = in->row_in[1];
    uint8_t *restrict bi = in->row_in[2];

    /* Precompute row 0 horizontal blur */
    unpack_row_rgb(inframe, ri, gi, bi, w);
    hblur_3(ri, in->hbuf[0][0], w);
    hblur_3(gi, in->hbuf[1][0], w);
    hblur_3(bi, in->hbuf[2][0], w);
    /* Clamp top border: row -1 == row 0 */
    memcpy(in->hbuf[0][2], in->hbuf[0][0], (size_t)w * sizeof(uint32_t));
    memcpy(in->hbuf[1][2], in->hbuf[1][0], (size_t)w * sizeof(uint32_t));
    memcpy(in->hbuf[2][2], in->hbuf[2][0], (size_t)w * sizeof(uint32_t));

    for (int y = 0; y < h; y++) {
        int prev_slot = (y + 2) % 3;
        int cur_slot  = y % 3;
        int next_slot = (y + 1) % 3;

        if (y + 1 < h) {
            unpack_row_rgb(&inframe[(size_t)(y + 1) * w], ri, gi, bi, w);
            hblur_3(ri, in->hbuf[0][next_slot], w);
            hblur_3(gi, in->hbuf[1][next_slot], w);
            hblur_3(bi, in->hbuf[2][next_slot], w);
        } else {
            next_slot = cur_slot;
        }

        const uint32_t *restrict r0 = in->hbuf[0][prev_slot];
        const uint32_t *restrict r1 = in->hbuf[0][cur_slot];
        const uint32_t *restrict r2 = in->hbuf[0][next_slot];
        const uint32_t *restrict g0 = in->hbuf[1][prev_slot];
        const uint32_t *restrict g1 = in->hbuf[1][cur_slot];
        const uint32_t *restrict g2 = in->hbuf[1][next_slot];
        const uint32_t *restrict b0 = in->hbuf[2][prev_slot];
        const uint32_t *restrict b1 = in->hbuf[2][cur_slot];
        const uint32_t *restrict b2 = in->hbuf[2][next_slot];

        const uint32_t *restrict in_row = &inframe[(size_t)y * w];
        uint32_t *restrict out_row = &outframe[(size_t)y * w];

        for (int x = 0; x < w; x++) {
            uint32_t px = in_row[x];
            int32_t or_ = (int32_t)(px & 0xFFu);
            int32_t og_ = (int32_t)((px >> 8) & 0xFFu);
            int32_t ob_ = (int32_t)((px >> 16) & 0xFFu);

            int32_t br = (int32_t)((r0[x] + (r1[x] << 1) + r2[x] + 8u) >> 4);
            int32_t bg = (int32_t)((g0[x] + (g1[x] << 1) + g2[x] + 8u) >> 4);
            int32_t bb = (int32_t)((b0[x] + (b1[x] << 1) + b2[x] + 8u) >> 4);

            int32_t nr = or_ + (((or_ - br) * amount) >> 16);
            int32_t ng = og_ + (((og_ - bg) * amount) >> 16);
            int32_t nb = ob_ + (((ob_ - bb) * amount) >> 16);

            out_row[x] = (uint32_t)clamp_u8(nr)
                       | ((uint32_t)clamp_u8(ng) << 8)
                       | ((uint32_t)clamp_u8(nb) << 16)
                       | (px & 0xFF000000u);
        }
    }
}

static void unsharp_5x5_rgba(const uint32_t *restrict inframe, uint32_t *restrict outframe, inst *in)
{
    const int w = in->w;
    const int h = in->h;
    const int32_t amount = (int32_t)(in->fp.amount * 65536.0);

    uint8_t *restrict ri = in->row_in[0];
    uint8_t *restrict gi = in->row_in[1];
    uint8_t *restrict bi = in->row_in[2];

    /* Precompute rows 0 and 1 */
    unpack_row_rgb(inframe, ri, gi, bi, w);
    hblur_5(ri, in->hbuf[0][0], w);
    hblur_5(gi, in->hbuf[1][0], w);
    hblur_5(bi, in->hbuf[2][0], w);

    /* Rows -2 and -1 clamp to row 0 */
    memcpy(in->hbuf[0][3], in->hbuf[0][0], (size_t)w * sizeof(uint32_t));
    memcpy(in->hbuf[1][3], in->hbuf[1][0], (size_t)w * sizeof(uint32_t));
    memcpy(in->hbuf[2][3], in->hbuf[2][0], (size_t)w * sizeof(uint32_t));
    memcpy(in->hbuf[0][4], in->hbuf[0][0], (size_t)w * sizeof(uint32_t));
    memcpy(in->hbuf[1][4], in->hbuf[1][0], (size_t)w * sizeof(uint32_t));
    memcpy(in->hbuf[2][4], in->hbuf[2][0], (size_t)w * sizeof(uint32_t));

    if (h > 1) {
        unpack_row_rgb(&inframe[w], ri, gi, bi, w);
        hblur_5(ri, in->hbuf[0][1], w);
        hblur_5(gi, in->hbuf[1][1], w);
        hblur_5(bi, in->hbuf[2][1], w);
    } else {
        memcpy(in->hbuf[0][1], in->hbuf[0][0], (size_t)w * sizeof(uint32_t));
        memcpy(in->hbuf[1][1], in->hbuf[1][0], (size_t)w * sizeof(uint32_t));
        memcpy(in->hbuf[2][1], in->hbuf[2][0], (size_t)w * sizeof(uint32_t));
    }

    for (int y = 0; y < h; y++) {
        int s0 = (y + 3) % 5;
        int s1 = (y + 4) % 5;
        int s2 = y % 5;
        int s3 = (y + 1) % 5;
        int s4 = (y + 2) % 5;

        if (y + 2 < h) {
            unpack_row_rgb(&inframe[(size_t)(y + 2) * w], ri, gi, bi, w);
            hblur_5(ri, in->hbuf[0][s4], w);
            hblur_5(gi, in->hbuf[1][s4], w);
            hblur_5(bi, in->hbuf[2][s4], w);
        } else {
            /* Clamp bottom border to row h-1 */
            int last_slot = (h - 1) % 5;
            s4 = last_slot;
            if (y + 1 >= h) s3 = last_slot;
        }

        const uint32_t *restrict r0 = in->hbuf[0][s0];
        const uint32_t *restrict r1 = in->hbuf[0][s1];
        const uint32_t *restrict r2 = in->hbuf[0][s2];
        const uint32_t *restrict r3 = in->hbuf[0][s3];
        const uint32_t *restrict r4 = in->hbuf[0][s4];

        const uint32_t *restrict g0 = in->hbuf[1][s0];
        const uint32_t *restrict g1 = in->hbuf[1][s1];
        const uint32_t *restrict g2 = in->hbuf[1][s2];
        const uint32_t *restrict g3 = in->hbuf[1][s3];
        const uint32_t *restrict g4 = in->hbuf[1][s4];

        const uint32_t *restrict b0 = in->hbuf[2][s0];
        const uint32_t *restrict b1 = in->hbuf[2][s1];
        const uint32_t *restrict b2 = in->hbuf[2][s2];
        const uint32_t *restrict b3 = in->hbuf[2][s3];
        const uint32_t *restrict b4 = in->hbuf[2][s4];

        const uint32_t *restrict in_row = &inframe[(size_t)y * w];
        uint32_t *restrict out_row = &outframe[(size_t)y * w];

        for (int x = 0; x < w; x++) {
            uint32_t px = in_row[x];
            int32_t or_ = (int32_t)(px & 0xFFu);
            int32_t og_ = (int32_t)((px >> 8) & 0xFFu);
            int32_t ob_ = (int32_t)((px >> 16) & 0xFFu);

            int32_t br = (int32_t)((r0[x] + 4u * r1[x] + 6u * r2[x] + 4u * r3[x] + r4[x] + 128u) >> 8);
            int32_t bg = (int32_t)((g0[x] + 4u * g1[x] + 6u * g2[x] + 4u * g3[x] + g4[x] + 128u) >> 8);
            int32_t bb = (int32_t)((b0[x] + 4u * b1[x] + 6u * b2[x] + 4u * b3[x] + b4[x] + 128u) >> 8);

            int32_t nr = or_ + (((or_ - br) * amount) >> 16);
            int32_t ng = og_ + (((og_ - bg) * amount) >> 16);
            int32_t nb = ob_ + (((ob_ - bb) * amount) >> 16);

            out_row[x] = (uint32_t)clamp_u8(nr)
                       | ((uint32_t)clamp_u8(ng) << 8)
                       | ((uint32_t)clamp_u8(nb) << 16)
                       | (px & 0xFF000000u);
        }
    }
}

/* General Waltz & Miller FSM unsharp for larger matrix sizes (7x7..11x11), streaming row-by-row */
static void unsharp_general_rgba(const uint32_t *restrict inframe, uint32_t *restrict outframe, inst *in)
{
    const int width = in->w;
    const int height = in->h;
    const int stepsX = in->fp.msizeX / 2;
    const int stepsY = in->fp.msizeY / 2;
    const int scalebits = (stepsX + stepsY) * 2;
    const uint32_t halfscale = 1u << (scalebits - 1);
    const int32_t amount = (int32_t)(in->fp.amount * 65536.0);
    uint32_t *restrict hrow = in->hrow;

    for (int c = 0; c < 3; c++) {
        for (int z = 0; z < 2 * stepsY; z++) {
            memset(in->sc_rgb[c][z], 0, (size_t)width * sizeof(uint32_t));
        }
    }

    for (int y = -stepsY; y < height + stepsY; y++) {
        int src_y = y < 0 ? 0 : (y >= height ? height - 1 : y);
        unpack_row_rgb(&inframe[(size_t)src_y * width], in->row_in[0], in->row_in[1], in->row_in[2], width);

        for (int c = 0; c < 3; c++) {
            const uint8_t *restrict src2 = in->row_in[c];
            uint32_t SR[MAX_MATRIX_SIZE - 1];
            memset(SR, 0, sizeof(SR[0]) * (size_t)(2 * stepsX));

            for (int x = -stepsX; x < width + stepsX; x++) {
                uint32_t Tmp1 = x <= 0 ? src2[0] : (x >= width ? src2[width - 1] : src2[x]);
                for (int z = 0; z < stepsX * 2; z += 2) {
                    uint32_t Tmp2 = SR[z + 0] + Tmp1; SR[z + 0] = Tmp1;
                    Tmp1 = SR[z + 1] + Tmp2; SR[z + 1] = Tmp2;
                }
                if (x >= stepsX) {
                    hrow[x - stepsX] = Tmp1;
                }
            }

            uint32_t **SC = in->sc_rgb[c];
            if (y >= stepsY) {
                const int out_y = y - stepsY;
                const uint32_t *restrict orig_row = &inframe[(size_t)out_y * width];
                uint8_t *restrict dst_c = in->row_out[c];
                const int shift = c * 8;
                for (int x = 0; x < width; x++) {
                    uint32_t Tmp1 = hrow[x];
                    for (int z = 0; z < stepsY * 2; z += 2) {
                        uint32_t Tmp2 = SC[z + 0][x] + Tmp1; SC[z + 0][x] = Tmp1;
                        Tmp1 = SC[z + 1][x] + Tmp2; SC[z + 1][x] = Tmp2;
                    }
                    int32_t orig = (int32_t)((orig_row[x] >> shift) & 0xFFu);
                    int32_t res = orig + (((orig - (int32_t)((Tmp1 + halfscale) >> scalebits)) * amount) >> 16);
                    dst_c[x] = clamp_u8(res);
                }
            } else {
                for (int x = 0; x < width; x++) {
                    uint32_t Tmp1 = hrow[x];
                    for (int z = 0; z < stepsY * 2; z += 2) {
                        uint32_t Tmp2 = SC[z + 0][x] + Tmp1; SC[z + 0][x] = Tmp1;
                        Tmp1 = SC[z + 1][x] + Tmp2; SC[z + 1][x] = Tmp2;
                    }
                }
            }
        }

        if (y >= stepsY) {
            const int out_y = y - stepsY;
            const uint32_t *restrict orig_row = &inframe[(size_t)out_y * width];
            uint32_t *restrict out_row = &outframe[(size_t)out_y * width];
            const uint8_t *restrict ro = in->row_out[0];
            const uint8_t *restrict go = in->row_out[1];
            const uint8_t *restrict bo = in->row_out[2];
            for (int x = 0; x < width; x++) {
                out_row[x] = (uint32_t)ro[x] | ((uint32_t)go[x] << 8) | ((uint32_t)bo[x] << 16)
                           | (orig_row[x] & 0xFF000000u);
            }
        }
    }
}

//-----------------------------------------------------
//stretch [0...1] to parameter range [min...max] linear
static float map_value_forward(double v, float min, float max)
{
return min+(max-min)*v;
}

//-----------------------------------------------------
//collapse from parameter range [min...max] to [0...1] linear
static double map_value_backward(float v, float min, float max)
{
return (v-min)/(max-min);
}

//***********************************************
// OBVEZNE FREI0R FUNKCIJE

//-----------------------------------------------
int f0r_init()
{
return 1;
}

//------------------------------------------------
void f0r_deinit()
{
}

//-----------------------------------------------
void f0r_get_plugin_info(f0r_plugin_info_t* info)
{

info->name="Sharpness";
info->author="Marko Cebokli, Remi Guyomarch";
info->plugin_type=F0R_PLUGIN_TYPE_FILTER;
info->color_model=F0R_COLOR_MODEL_RGBA8888;
info->frei0r_version=FREI0R_MAJOR_VERSION;
info->major_version=0;
info->minor_version=2;
info->num_params=2;
info->explanation="Unsharp masking (port from Mplayer)";
}

//--------------------------------------------------
void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
switch(param_index)
	{
	case 0:
		info->name = "Amount";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "";
		break;
	case 1:
		info->name = "Size";
		info->type = F0R_PARAM_DOUBLE;
		info->explanation = "";
		break;
	}
}

//----------------------------------------------
f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
inst *in;
int c, z;

in=calloc(1,sizeof(inst));
in->w=width;
in->h=height;

for (c = 0; c < 3; c++) {
    in->row_in[c] = calloc(width, sizeof(unsigned char));
    in->row_out[c] = calloc(width, sizeof(unsigned char));
    for (z = 0; z < 5; z++) {
        in->hbuf[c][z] = calloc(width, sizeof(uint32_t));
    }
}
in->hrow = calloc(width, sizeof(uint32_t));

//defaults
in->fp.amount=0.0;
in->size=3;
in->fp.msizeX=3;
in->fp.msizeY=3;
in->ac=0;

memset(in->sc_rgb, 0, sizeof(in->sc_rgb));
for (c = 0; c < 3; c++) {
    for (z = 0; z < in->fp.msizeY; z++) {
        in->sc_rgb[c][z] = calloc(width, sizeof(uint32_t));
    }
}

return (f0r_instance_t)in;
}

//---------------------------------------------------
void f0r_destruct(f0r_instance_t instance)
{
inst *in;
int c, i;

in=(inst*)instance;

for (c = 0; c < 3; c++) {
    free(in->row_in[c]);
    free(in->row_out[c]);
    for (i = 0; i < 5; i++) free(in->hbuf[c][i]);
    for (i = 0; i < in->fp.msizeY; i++) free(in->sc_rgb[c][i]);
}
free(in->hrow);

free(instance);
}

//-----------------------------------------------------
void f0r_set_param_value(f0r_instance_t instance, f0r_param_t parm, int param_index)
{
inst *p;
double tmpf;
int tmpi,chg,c,z;

p=(inst*)instance;

chg=0;
switch(param_index)
	{
	case 0:
                tmpf=map_value_forward(*((double*)parm), -1.5, 3.5); 
		if (tmpf!=p->fp.amount) chg=1;
		p->fp.amount=tmpf;
		break;
	case 1:
		tmpi=map_value_forward(*((double*)parm), 3.0, 11.0);
		if (p->size != tmpi) chg=1;
		p->size=tmpi;
		break;
	}

if (chg==0) return;

if (p->fp.msizeY != p->size) {
    for (c = 0; c < 3; c++) {
        for (z = 0; z < p->fp.msizeY; z++) {
            free(p->sc_rgb[c][z]);
            p->sc_rgb[c][z] = NULL;
        }
    }
    p->fp.msizeX = p->size;
    p->fp.msizeY = p->size;
    for (c = 0; c < 3; c++) {
        for (z = 0; z < p->fp.msizeY; z++) {
            p->sc_rgb[c][z] = calloc(p->w, sizeof(uint32_t));
        }
    }
}
}

//--------------------------------------------------
void f0r_get_param_value(f0r_instance_t instance, f0r_param_t param, int param_index)
{
inst *p;

p=(inst*)instance;

switch(param_index)
	{
	case 0:
                *((double*)param)=map_value_backward(p->fp.amount, -1.5, 3.5);
		break;
	case 1:
                *((double*)param)=map_value_backward(p->size, 3.0, 11.0);
		break;
	}
}

//-------------------------------------------------
void f0r_update(f0r_instance_t instance, double time, const uint32_t* inframe, uint32_t* outframe)
{
inst *in;

assert(instance);
(void)time;
in=(inst*)instance;

if (!in->fp.amount) {
    if (outframe != inframe) {
        memcpy(outframe, inframe, (size_t)in->w * in->h * sizeof(uint32_t));
    }
    return;
}

int steps = in->fp.msizeX / 2;
if (steps == 1 && in->fp.msizeY / 2 == 1) {
    unsharp_3x3_rgba(inframe, outframe, in);
} else if (steps == 2 && in->fp.msizeY / 2 == 2) {
    unsharp_5x5_rgba(inframe, outframe, in);
} else {
    unsharp_general_rgba(inframe, outframe, in);
}
}
