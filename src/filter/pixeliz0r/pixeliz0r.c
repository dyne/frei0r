#include "frei0r.h"
#include "frei0r/math.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#if defined(__SSE2__)
#include <emmintrin.h>
#endif

typedef struct pixelizer_instance
{
    unsigned int width;
    unsigned int height;
    unsigned int block_size_x;
    unsigned int block_size_y;
    unsigned char pass_alpha;
} pixelizer_instance_t;

static inline uint64_t fast_div_magic(unsigned int d)
{
    return ((1ULL << 55) + d - 1) / d;
}

static inline uint32_t average_block(const uint32_t* __restrict__ start,
                                     unsigned int bxsize, unsigned int bysize,
                                     unsigned int xsize, unsigned int area, uint64_t magic)
{
    uint32_t s0 = 0, s1 = 0, s2 = 0, s3 = 0;
#if defined(__SSE2__)
    if (bxsize >= 4)
    {
        const __m128i zero = _mm_setzero_si128();
        if (area <= 255u)
        {
            __m128i acc16_a = _mm_setzero_si128();
            __m128i acc16_b = _mm_setzero_si128();
            const uint32_t* p = start;
            for (unsigned int y = 0; y < bysize; ++y)
            {
                unsigned int x = 0;
                for (; x + 4 <= bxsize; x += 4)
                {
                    __m128i v = _mm_loadu_si128((const __m128i*)(p + x));
                    acc16_a = _mm_add_epi16(acc16_a, _mm_unpacklo_epi8(v, zero));
                    acc16_b = _mm_add_epi16(acc16_b, _mm_unpackhi_epi8(v, zero));
                }
                for (; x < bxsize; ++x)
                {
                    uint32_t c = p[x];
                    s0 += c & 0xffu;
                    s1 += (c >> 8) & 0xffu;
                    s2 += (c >> 16) & 0xffu;
                    s3 += c >> 24;
                }
                p += xsize;
            }
            /* Combine 4 RGBA lanes of uint16_t into 4 uint32_t channel sums */
            __m128i sum16 = _mm_add_epi16(acc16_a, acc16_b);
            __m128i lo32 = _mm_unpacklo_epi16(sum16, zero);
            __m128i hi32 = _mm_unpackhi_epi16(sum16, zero);
            __m128i tot32 = _mm_add_epi32(lo32, hi32);
            uint32_t tmp[4];
            _mm_storeu_si128((__m128i*)tmp, tot32);
            s0 += tmp[0]; s1 += tmp[1]; s2 += tmp[2]; s3 += tmp[3];
        }
        else
        {
            __m128i acc32 = _mm_setzero_si128();
            const uint32_t* p = start;
            for (unsigned int y = 0; y < bysize; ++y)
            {
                __m128i row16 = _mm_setzero_si128();
                unsigned int x = 0;
                unsigned int chunk = 0;
                for (; x + 4 <= bxsize; x += 4)
                {
                    __m128i v = _mm_loadu_si128((const __m128i*)(p + x));
                    row16 = _mm_add_epi16(row16, _mm_unpacklo_epi8(v, zero));
                    row16 = _mm_add_epi16(row16, _mm_unpackhi_epi8(v, zero));
                    if (++chunk == 64)
                    {
                        acc32 = _mm_add_epi32(acc32, _mm_unpacklo_epi16(row16, zero));
                        acc32 = _mm_add_epi32(acc32, _mm_unpackhi_epi16(row16, zero));
                        row16 = _mm_setzero_si128();
                        chunk = 0;
                    }
                }
                if (chunk)
                {
                    acc32 = _mm_add_epi32(acc32, _mm_unpacklo_epi16(row16, zero));
                    acc32 = _mm_add_epi32(acc32, _mm_unpackhi_epi16(row16, zero));
                }
                for (; x < bxsize; ++x)
                {
                    uint32_t c = p[x];
                    s0 += c & 0xffu;
                    s1 += (c >> 8) & 0xffu;
                    s2 += (c >> 16) & 0xffu;
                    s3 += c >> 24;
                }
                p += xsize;
            }
            uint32_t tmp[4];
            _mm_storeu_si128((__m128i*)tmp, acc32);
            s0 += tmp[0]; s1 += tmp[1]; s2 += tmp[2]; s3 += tmp[3];
        }
    }
    else
#endif
    {
        const uint32_t* p = start;
        for (unsigned int y = 0; y < bysize; ++y)
        {
            for (unsigned int x = 0; x < bxsize; ++x)
            {
                uint32_t c = p[x];
                s0 += c & 0xffu;
                s1 += (c >> 8) & 0xffu;
                s2 += (c >> 16) & 0xffu;
                s3 += c >> 24;
            }
            p += xsize;
        }
    }

    if (area <= 11000000u)
    {
        uint32_t a0 = (uint32_t)(((uint64_t)s0 * magic) >> 55) & 0xffu;
        uint32_t a1 = (uint32_t)(((uint64_t)s1 * magic) >> 55) & 0xffu;
        uint32_t a2 = (uint32_t)(((uint64_t)s2 * magic) >> 55) & 0xffu;
        uint32_t a3 = (uint32_t)(((uint64_t)s3 * magic) >> 55) & 0xffu;
        return a0 | (a1 << 8) | (a2 << 16) | (a3 << 24);
    }
    return ((s0 / area) & 0xffu) | (((s1 / area) & 0xffu) << 8)
         | (((s2 / area) & 0xffu) << 16) | (((s3 / area) & 0xffu) << 24);
}

static inline void fill_block_fast(uint32_t* __restrict__ start,
                                   const uint32_t* __restrict__ src_start,
                                   unsigned int bxsize, unsigned int bysize,
                                   unsigned int xsize, uint32_t col, unsigned char pass_alpha)
{
    if (!pass_alpha)
    {
#if defined(__SSE2__)
        if (bxsize >= 4)
        {
            __m128i vcol = _mm_set1_epi32((int)col);
            uint32_t* p = start;
            for (unsigned int y = 0; y < bysize; ++y)
            {
                unsigned int x = 0;
                for (; x + 4 <= bxsize; x += 4)
                    _mm_storeu_si128((__m128i*)(p + x), vcol);
                for (; x < bxsize; ++x)
                    p[x] = col;
                p += xsize;
            }
            return;
        }
#endif
        uint32_t* p = start;
        for (unsigned int y = 0; y < bysize; ++y)
        {
            for (unsigned int x = 0; x < bxsize; ++x)
                p[x] = col;
            p += xsize;
        }
    }
    else
    {
        uint32_t col_rgb = col & 0x00FFFFFFu;
#if defined(__SSE2__)
        if (bxsize >= 4)
        {
            __m128i vcol = _mm_set1_epi32((int)col_rgb);
            __m128i amask = _mm_set1_epi32((int)0xFF000000u);
            uint32_t* p = start;
            const uint32_t* s = src_start;
            for (unsigned int y = 0; y < bysize; ++y)
            {
                unsigned int x = 0;
                for (; x + 4 <= bxsize; x += 4)
                {
                    __m128i sv = _mm_loadu_si128((const __m128i*)(s + x));
                    _mm_storeu_si128((__m128i*)(p + x),
                                     _mm_or_si128(_mm_and_si128(sv, amask), vcol));
                }
                for (; x < bxsize; ++x)
                    p[x] = (s[x] & 0xFF000000u) | col_rgb;
                p += xsize;
                s += xsize;
            }
            return;
        }
#endif
        uint32_t* p = start;
        const uint32_t* s = src_start;
        for (unsigned int y = 0; y < bysize; ++y)
        {
            for (unsigned int x = 0; x < bxsize; ++x)
                p[x] = (s[x] & 0xFF000000u) | col_rgb;
            p += xsize;
            s += xsize;
        }
    }
}

int f0r_init()
{
    return 1;
}

void f0r_deinit()
{ /* no initialization required */ }

void f0r_get_plugin_info(f0r_plugin_info_t* pixelizerInfo)
{
    pixelizerInfo->name = "pixeliz0r";
    pixelizerInfo->author = "Gephex crew";
    pixelizerInfo->plugin_type = F0R_PLUGIN_TYPE_FILTER;
    pixelizerInfo->color_model = F0R_COLOR_MODEL_PACKED32;
    pixelizerInfo->frei0r_version = FREI0R_MAJOR_VERSION;
    pixelizerInfo->major_version = 2;
    pixelizerInfo->minor_version = 0;
    pixelizerInfo->num_params =  3;
    pixelizerInfo->explanation = "Pixelize input image.";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
    switch(param_index)
    {
    case 0:
        info->name = "Block width";
        info->type = F0R_PARAM_DOUBLE;
        info->explanation = "Horizontal size of one 'pixel'";
        break;
    case 1:
        info->name = "Block height";
        info->type = F0R_PARAM_DOUBLE;
        info->explanation = "Vertical size of one 'pixel'";
        break;
    case 2:
        info->name = "Pass-through alpha";
        info->type = F0R_PARAM_BOOL;
        info->explanation = "Do not average the alpha channel over each block";
        break;
    }
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
    pixelizer_instance_t* inst = (pixelizer_instance_t*)calloc(1, sizeof(*inst));
    inst->width = width; inst->height = height;
    /* Keep defaults representable by the normalized parameter range on
     * small, but valid, frames. */
    inst->block_size_x = MIN(8, 1 + width / 2);
    inst->block_size_y = MIN(8, 1 + height / 2);
    return (f0r_instance_t)inst;
}

void f0r_destruct(f0r_instance_t instance)
{
    free(instance);
}

void f0r_set_param_value(f0r_instance_t instance, 
                         f0r_param_t param, int param_index)
{ 
    assert(instance);
    pixelizer_instance_t* inst = (pixelizer_instance_t*)instance;

    switch(param_index)
    {
    case 0:
        // scale to [1..width]
        inst->block_size_x =  MAX(1 + ( *((double*)param) * (inst->width/2)), 1);
        break;
    case 1:
        // scale to [1..height]
        inst->block_size_y =  MAX(1 + ( *((double*)param) * (inst->height/2)), 1);
        break;
    case 2:
        inst->pass_alpha = *((double*)param) >= 0.5;
        break;
    }
}

void f0r_get_param_value(f0r_instance_t instance,
                         f0r_param_t param, int param_index)
{ 
    assert(instance);
    pixelizer_instance_t* inst = (pixelizer_instance_t*)instance;

    switch(param_index)
    {
    case 0:
        // scale back to [0..1]
        *((double*)param) = (double)(inst->block_size_x-1)/(inst->width/2);
        break;
    case 1:
        // scale back to [0..1]
        *((double*)param) = (double)(inst->block_size_y-1)/(inst->height/2);
        break;
    case 2:
        *((double*)param) = inst->pass_alpha ? 1.0 : 0.0;
        break;
    }
}

void f0r_update(f0r_instance_t instance, double time,
                const uint32_t* inframe, uint32_t* outframe)
{
    (void)time;
    assert(instance);
    pixelizer_instance_t* inst = (pixelizer_instance_t*)instance;
    unsigned int xsize = inst->width;
    unsigned int ysize = inst->height;
    unsigned int bsizex = inst->block_size_x;
    unsigned int bsizey = inst->block_size_y;
    unsigned char pass_alpha = inst->pass_alpha;

    if (bsizex == 1 && bsizey == 1)
    {
        memcpy(outframe, inframe, (size_t)xsize * ysize * sizeof(uint32_t));
        return;
    }

    unsigned int blocks_x = xsize / bsizex;
    unsigned int blocks_y = ysize / bsizey;
    unsigned int xrest = xsize - blocks_x * bsizex;
    unsigned int yrest = ysize - blocks_y * bsizey;

    unsigned int area_full = bsizex * bsizey;
    uint64_t magic_full = (area_full <= 11000000u) ? fast_div_magic(area_full) : 0;
    unsigned int area_xr = xrest * bsizey;
    uint64_t magic_xr = (xrest > 0 && area_xr <= 11000000u) ? fast_div_magic(area_xr) : 0;

    for (unsigned int yi = 0; yi < blocks_y; ++yi)
    {
        size_t offset = (size_t)yi * bsizey * xsize;
        for (unsigned int xi = 0; xi < blocks_x; ++xi)
        {
            uint32_t col = average_block(inframe + offset, bsizex, bsizey, xsize, area_full, magic_full);
            fill_block_fast(outframe + offset, inframe + offset, bsizex, bsizey, xsize, col, pass_alpha);
            offset += bsizex;
        }
        if (xrest > 0)
        {
            uint32_t col = average_block(inframe + offset, xrest, bsizey, xsize, area_xr, magic_xr);
            fill_block_fast(outframe + offset, inframe + offset, xrest, bsizey, xsize, col, pass_alpha);
        }
    }
    if (yrest > 0)
    {
        unsigned int area_yr = bsizex * yrest;
        uint64_t magic_yr = (area_yr <= 11000000u) ? fast_div_magic(area_yr) : 0;
        size_t offset = (size_t)blocks_y * bsizey * xsize;
        for (unsigned int xi = 0; xi < blocks_x; ++xi)
        {
            uint32_t col = average_block(inframe + offset, bsizex, yrest, xsize, area_yr, magic_yr);
            fill_block_fast(outframe + offset, inframe + offset, bsizex, yrest, xsize, col, pass_alpha);
            offset += bsizex;
        }
        if (xrest > 0)
        {
            unsigned int area_xyr = xrest * yrest;
            uint64_t magic_xyr = (area_xyr <= 11000000u) ? fast_div_magic(area_xyr) : 0;
            uint32_t col = average_block(inframe + offset, xrest, yrest, xsize, area_xyr, magic_xyr);
            fill_block_fast(outframe + offset, inframe + offset, xrest, yrest, xsize, col, pass_alpha);
        }
    }
}
