/*
 * kaleid0sc0pe.cpp
 * Copyright (C) 2020-2023 Brendan Hack (github@bendys.com)
 * This file is part of a Frei0r plugin that applies a kaleidoscope
 * effect.
 * Version 1.1	july 2023
 *
 * The kaleidoscope class implementation.
 *
 * This source code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This source code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. Please refer
 * to the GNU Public License for more details.
 *
 * You should have received a copy of the GNU Public License along
 * with this source code; if not, write to: Free Software Foundation,
 * Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
#include "kaleid0sc0pe.h"
#include <memory>
#include <cstring>
#ifndef NO_FUTURE
#include <future>
#endif

#ifdef __SSE2__
#define USE_SSE2
#include "sse_mathfun_extension.h"
#undef USE_SSE2
#ifdef HAS_INTEL_INTRINSICS
#include <immintrin.h>
#endif
#ifdef HAS_SIN_INTRINSIC
#define _mm_call_sin_ps _mm_sin_ps
#else
#define _mm_call_sin_ps sin_ps
#endif
#ifdef HAS_COS_INTRINSIC
#define _mm_call_cos_ps _mm_cos_ps
#else
#define _mm_call_cos_ps cos_ps
#endif
#ifdef HAS_ATAN2_INTRINSIC
#define _mm_call_atan2_ps _mm_atan2_ps
#else
#define _mm_call_atan2_ps atan2_ps
#endif
#endif

#ifndef M_PI
#define M_PI  3.14159265358979323846
#endif
#ifndef M_2PI
#define M_2PI 6.28318530717958647693
#endif
#ifndef MF_PI
#define MF_PI  3.14159265358979323846f
#endif
#ifndef MF_2PI
#define MF_2PI 6.28318530717958647693f
#endif

namespace std
{

void default_delete<libkaleid0sc0pe::IKaleid0sc0pe>::operator()(libkaleid0sc0pe::IKaleid0sc0pe *p)
{
    delete p;
}

} // namespace std

namespace libkaleid0sc0pe {

IKaleid0sc0pe *IKaleid0sc0pe::create(std::uint32_t width, std::uint32_t height, std::uint32_t component_size, std::uint32_t num_components, std::uint32_t stride)
{
    return new Kaleid0sc0pe(width, height, component_size, num_components, stride);
}

Kaleid0sc0pe::Kaleid0sc0pe(std::uint32_t width, std::uint32_t height, std::uint32_t component_size, std::uint32_t num_components, std::uint32_t stride):
m_width(width),
m_height(height),
m_component_size(component_size),
m_num_components(num_components),
m_stride(stride ? stride : width * component_size * num_components),
m_pixel_size(component_size * num_components),
m_aspect(width/static_cast<float>(height)),
m_origin_x(0.5f),
m_origin_y(0.5f),
m_origin_native_x(m_origin_x * width),
m_origin_native_y(m_origin_y * height),
m_segmentation(16),
m_segment_direction(Direction::NONE),
m_preferred_corner(Corner::BR),
m_preferred_search_dir(Direction::CLOCKWISE),
m_edge_reflect(true),
m_background_colour(nullptr),
m_edge_threshold(0),
m_source_segment_angle(-1),
m_n_segments(0),
m_start_angle(0),
m_segment_width(0),
m_n_threads(0)
{
#ifdef __SSE2__
    m_sse_width = _mm_set1_ps(static_cast<float>(m_width));
    m_sse_height = _mm_set1_ps(static_cast<float>(m_height));
    m_sse_aspect = _mm_set1_ps(m_width / static_cast<float>(m_height));
    m_sse_ps_0 = _mm_set1_ps(0.0f);
    m_sse_ps_1 = _mm_set1_ps(1.0f);
    m_sse_ps_1 = _mm_set1_ps(1.0f);
    m_sse_ps_2 = _mm_set1_ps(2.0f);
    m_sse_epi32_1 = _mm_set1_epi32(1);
    m_sse_epi32_2 = _mm_set1_epi32(2);
    m_sse_shift_1 = _mm_cvtsi32_si128(1);
#endif
}

std::int32_t Kaleid0sc0pe::set_origin(float x, float y)
{
    if (x < 0 || y < 0 || x > 1 || y > 1) {
        return -2;
    }
    m_origin_x = x;
    m_origin_y = y;

    m_origin_native_x = m_origin_x * m_width;
    m_origin_native_y = m_origin_y * m_height;

    m_n_segments = 0;

    return 0;
}

float Kaleid0sc0pe::get_origin_x() const
{
    return m_origin_x;
}

float Kaleid0sc0pe::get_origin_y() const
{
    return m_origin_y;
}

std::int32_t Kaleid0sc0pe::set_segmentation(std::uint32_t segmentation)
{
    if (segmentation == 0) {
        return -2;
    }
    m_segmentation = segmentation;
    m_n_segments = 0;

    return 0;
}

std::uint32_t Kaleid0sc0pe::get_segmentation() const
{
    return m_segmentation;
}

std::int32_t Kaleid0sc0pe::set_edge_threshold(std::uint32_t threshold)
{
    m_edge_threshold = threshold;
    return 0;
}

std::uint32_t Kaleid0sc0pe::get_edge_threshold() const
{
    return m_edge_threshold;
}

std::int32_t Kaleid0sc0pe::set_preferred_corner(Corner corner)
{
    m_preferred_corner = corner;
    m_n_segments = 0;
    return 0;
}

Kaleid0sc0pe::Corner Kaleid0sc0pe::get_preferred_corner() const
{
    return m_preferred_corner;
}

std::int32_t Kaleid0sc0pe::set_preferred_corner_search_direction(Direction direction)
{
    if (direction == Direction::NONE) {
        return -2;
    }
    m_preferred_search_dir = direction;
    m_n_segments = 0;
    return 0;
}

Kaleid0sc0pe::Direction Kaleid0sc0pe::get_preferred_corner_search_direction() const
{
    return m_preferred_search_dir;
}

std::int32_t Kaleid0sc0pe::set_reflect_edges(bool reflect)
{
    m_edge_reflect = reflect;
    return 0;
}

bool Kaleid0sc0pe::get_reflect_edges() const
{
    return m_edge_reflect;
}

std::int32_t Kaleid0sc0pe::set_background_colour(void* colour)
{
    m_background_colour = colour;
    return 0;
}

void* Kaleid0sc0pe::get_background_colour() const
{
    return m_background_colour;
}

std::int32_t Kaleid0sc0pe::set_source_segment(float angle)
{
    m_source_segment_angle = angle;
    return 0;
}

float Kaleid0sc0pe::get_source_segment() const
{
    return m_source_segment_angle;
}

static double distance_sq(double x1, double y1, double x2, double y2)
{
    return std::pow(x1 - x2, 2) + std::pow(y1 - y2, 2);
}

std::int32_t inc_idx(std::int32_t start_idx, std::int32_t inc, std::int32_t max)
{
    start_idx += inc;
    return (start_idx < 0) ? max - 1 : start_idx % max;
}

void Kaleid0sc0pe::init()
{
    m_n_segments = m_segmentation * 2;
    m_segment_width = MF_PI * 2 / m_n_segments;

    if (m_source_segment_angle < 0) {
        // find origin rotation
        std::uint32_t corners[4][2] = {
            { 0, 0 },
            { 1, 0 },
            { 1, 1 },
            { 0, 1 }
        };
        std::int32_t start_idx(0);
        switch (m_preferred_corner) {
        case Corner::TL: start_idx = 0; break;
        case Corner::TR: start_idx = 1; break;
        case Corner::BR: start_idx = 2; break;
        case Corner::BL: start_idx = 3; break;
        }
        std::int32_t dir = m_preferred_search_dir == Direction::CLOCKWISE ? 1 : -1;
        std::uint32_t idx = start_idx;
        float origin_x = m_origin_x;
        float origin_y = m_origin_y;
        double dist = distance_sq(origin_x, origin_y, corners[idx][0], corners[idx][1]);
        std::int32_t corner = idx;
        idx = inc_idx(idx, dir, 4);
        while (idx != start_idx) {
            double d = distance_sq(origin_x, origin_y, corners[idx][0], corners[idx][1]);
            if (d > dist) {
                dist = d;
                corner = idx;
            }
            idx = inc_idx(idx, dir, 4);
        }

        float start_line_x = corners[corner][0] - origin_x;
        float start_line_y = corners[corner][1] - origin_y;
        m_start_angle = std::atan2(start_line_y, start_line_x) - (m_segment_direction == Direction::NONE ?
                                                                    0 :
                                                                    (m_segment_width / (m_segment_direction == Direction::CLOCKWISE ? -2 : 2)));
    } else {
        m_start_angle = -m_source_segment_angle;
    }
#ifdef __SSE2__
    m_sse_origin_native_x = _mm_set1_ps(m_origin_x * m_width);
    m_sse_origin_native_y = _mm_set1_ps(m_origin_y * m_height);
    m_sse_start_angle = _mm_set1_ps(m_start_angle);
    m_sse_segment_width = _mm_set1_ps(m_segment_width);
    m_sse_half_segment_width = _mm_set1_ps(m_segment_width/2);
#endif
}

#ifdef __SSE2__
Kaleid0sc0pe::Reflect_info Kaleid0sc0pe::calculate_reflect_info(__m128i* x, __m128i* y)
{
    Reflect_info info;

    to_screen(&info.screen_x, &info.screen_y, x, y);

    info.angle = _mm_sub_ps(_mm_call_atan2_ps(info.screen_y, info.screen_x), m_sse_start_angle);
    info.reference_angle = _mm_add_ps(_mm_and_ps(info.angle, *(v4sf*)_ps_inv_sign_mask), m_sse_half_segment_width);
    // we do a max with 0 since atan2_ps will return nan for atan2(0,0) which ends up with a negative reference angle.
    info.segment_number_i = _mm_cvttps_epi32(_mm_max_ps(_mm_div_ps(info.reference_angle, m_sse_segment_width), m_sse_ps_0));
    info.segment_number = _mm_cvtepi32_ps(info.segment_number_i);

    return info;
}

void Kaleid0sc0pe::to_screen(__m128* x, __m128* y, __m128i* sx, __m128i* sy)
{
    *x = _mm_cvtepi32_ps(*sx);
    *x = _mm_sub_ps(*x, m_sse_origin_native_x);
    *y = _mm_cvtepi32_ps(*sy);
    *y = _mm_sub_ps(*y, m_sse_origin_native_y);
    *y = _mm_mul_ps(*y, m_sse_aspect);
}

void Kaleid0sc0pe::from_screen(__m128* x, __m128* y)
{
    *x = _mm_add_ps(*x, m_sse_origin_native_x);
    *y = _mm_div_ps(*y, m_sse_aspect);
    *y = _mm_add_ps(*y, m_sse_origin_native_y);
}

void Kaleid0sc0pe::rotate(int x, int y, __m128 *source_x, __m128 *source_y)
{
    ALIGN16_BEG int ALIGN16_END mx[4] = { x, x + 1, x + 2, x + 3 };
    ALIGN16_BEG int ALIGN16_END my[4] = { y, y, y, y };

    Reflect_info info = calculate_reflect_info((__m128i*)mx, (__m128i*)my);

    __m128 reflection_angle = _mm_mul_ps(info.segment_number, m_sse_segment_width);

    __m128i segi_p1 = _mm_add_epi32(info.segment_number_i, m_sse_epi32_1);
    __m128 refl_factor = _mm_cvtepi32_ps(_mm_sub_epi32(_mm_srl_epi32(segi_p1, m_sse_shift_1), _mm_srl_epi32(info.segment_number_i, m_sse_shift_1)));

    reflection_angle = _mm_sub_ps(reflection_angle, _mm_mul_ps(refl_factor, _mm_sub_ps(m_sse_segment_width, _mm_mul_ps(m_sse_ps_2, _mm_sub_ps(info.reference_angle, reflection_angle)))));

    reflection_angle = _mm_mul_ps(reflection_angle, _mm_sub_ps(m_sse_ps_0, _mm_or_ps(_mm_and_ps(info.angle, *(v4sf*)_ps_sign_mask), m_sse_ps_1)));

    reflection_angle = _mm_mul_ps(reflection_angle, _mm_and_ps(_mm_cmpge_ps(info.segment_number, m_sse_ps_1), m_sse_ps_1));

    __m128 cos_angle = _mm_call_cos_ps(reflection_angle);
    __m128 sin_angle = _mm_call_sin_ps(reflection_angle);
    *source_x = _mm_sub_ps(_mm_mul_ps(info.screen_x, cos_angle), _mm_mul_ps(info.screen_y, sin_angle));
    *source_y = _mm_add_ps(_mm_mul_ps(info.screen_y, cos_angle), _mm_mul_ps(info.screen_x, sin_angle));

    from_screen(source_x, source_y);
}

#else
Kaleid0sc0pe::Reflect_info Kaleid0sc0pe::calculate_reflect_info(std::uint32_t x, std::uint32_t y)
{
    Reflect_info info;

    to_screen(&(info.screen_x), &(info.screen_y), x, y);

    info.angle = std::atan2(info.screen_y, info.screen_x) - m_start_angle;
    info.reference_angle = std::fabs(info.angle) + m_segment_width / 2;
    info.segment_number = std::uint32_t(info.reference_angle / m_segment_width);

    return info;
}
void Kaleid0sc0pe::to_screen(float *x, float *y, std::uint32_t sx, std::uint32_t sy)
{
    *x = sx - m_origin_native_x;
    *y = (sy - m_origin_native_y) * m_aspect;
}

void Kaleid0sc0pe::from_screen(float *x, float *y)
{
    *x = *x + m_origin_native_x;
    *y = *y / m_aspect + m_origin_native_y;
}
#endif

const std::uint8_t *Kaleid0sc0pe::lookup(const std::uint8_t* p, std::uint32_t x, std::uint32_t y)
{
    return p + m_stride * static_cast<std::size_t>(y) + m_pixel_size * static_cast<std::size_t>(x);
}

std::uint8_t* Kaleid0sc0pe::lookup(std::uint8_t* p, std::uint32_t x, std::uint32_t y)
{
    return p + m_stride * static_cast<std::size_t>(y) + m_pixel_size * static_cast<std::size_t>(x);
}

void Kaleid0sc0pe::process_bg(float x, float y, const std::uint8_t* in, std::uint8_t* out)
{
    if (x < 0 && -x <= m_edge_threshold) {
        x = 0;
    }
    else if (x >= m_width && x < m_width + m_edge_threshold) {
        x = m_width - 1.0f;
    }
    if (y < 0 && -y <= m_edge_threshold) {
        y = 0;
    }
    else if (y >= m_height && y < m_height + m_edge_threshold) {
        y = m_height - 1.0f;
    }
    if (static_cast<std::uint32_t>(x) >= 0 && static_cast<std::uint32_t>(x) < m_width &&
        static_cast<std::uint32_t>(y) >= 0 && static_cast<std::uint32_t>(y) < m_height) {
        std::memcpy(out, lookup(in, static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y)), m_pixel_size);
    }
    else if (m_background_colour) {
        std::memcpy(out, reinterpret_cast<const std::uint8_t*>(m_background_colour), m_pixel_size);
    }
}

#ifdef __SSE2__
void Kaleid0sc0pe::process_block(Block* block)
{
    for (std::int32_t y = block->y_start; y <= static_cast<std::int32_t>(block->y_end); ++y) {
        for (std::int32_t x = block->x_start; x <= static_cast<std::int32_t>(block->x_end); x += 4) {
            std::uint8_t* out = lookup(block->out_frame, x, y);
            __m128 source_x;
            __m128 source_y;

            // rotate points to source_x,source_y
            rotate(x, y, &source_x, &source_y);

            // reflect back into image if necessary

            source_x = _mm_and_ps(source_x, *(v4sf*)_ps_inv_sign_mask);
            __m128 ge_width = _mm_cmpge_ps(source_x, m_sse_width);
            source_x = _mm_or_ps(_mm_and_ps(_mm_sub_ps(m_sse_width, _mm_sub_ps(source_x, m_sse_width)), ge_width), _mm_andnot_ps(ge_width, source_x));

            // same for y
            source_y = _mm_and_ps(source_y, *(v4sf*)_ps_inv_sign_mask);
            __m128 ge_height = _mm_cmpge_ps(source_y, m_sse_height);
            source_y = _mm_or_ps(_mm_and_ps(_mm_sub_ps(m_sse_height, _mm_sub_ps(source_y, m_sse_height)), ge_height), _mm_andnot_ps(ge_height,source_y));

            __m128i source_xi = _mm_cvttps_epi32(_mm_min_ps(source_x, _mm_sub_ps(m_sse_width, m_sse_ps_1)));
            __m128i source_yi = _mm_cvttps_epi32(_mm_min_ps(source_y, _mm_sub_ps(m_sse_height, m_sse_ps_1)));

            std::int32_t* sx = reinterpret_cast<std::int32_t*>(&source_xi);
            std::int32_t* sy = reinterpret_cast<std::int32_t*>(&source_yi);
            std::memcpy(out, lookup(block->in_frame, sx[0], sy[0]), m_pixel_size);
            out += m_pixel_size;
            std::memcpy(out, lookup(block->in_frame, sx[1], sy[1]), m_pixel_size);
            out += m_pixel_size;
            std::memcpy(out, lookup(block->in_frame, sx[2], sy[2]), m_pixel_size);
            out += m_pixel_size;
            std::memcpy(out, lookup(block->in_frame, sx[3], sy[3]), m_pixel_size);
        }
    }
}

void Kaleid0sc0pe::process_block_bg(Block* block)
{
    for (std::int32_t y = block->y_start; y <= static_cast<std::int32_t>(block->y_end); ++y) {
        for (std::int32_t x = block->x_start; x <= static_cast<std::int32_t>(block->x_end); x += 4) {
            std::uint8_t* out = lookup(block->out_frame, x, y);
            __m128 source_x;
            __m128 source_y;

            // rotate points to source_x,source_y
            rotate(x, y, &source_x, &source_y);

            float* sx = reinterpret_cast<float*>(&source_x);
            float* sy = reinterpret_cast<float*>(&source_y);
            process_bg(sx[0], sy[0], block->in_frame, out);
            out += m_pixel_size;
            process_bg(sx[1], sy[1], block->in_frame, out);
            out += m_pixel_size;
            process_bg(sx[2], sy[2], block->in_frame, out);
            out += m_pixel_size;
            process_bg(sx[3], sy[3], block->in_frame, out);
        }
    }
}


#else
void Kaleid0sc0pe::process_block(Block *block)
{
    for (std::uint32_t y = block->y_start; y <= block->y_end; ++y) {
        for (std::uint32_t x = block->x_start; x <= block->x_end; ++x) {
            std::uint8_t* out = lookup(block->out_frame, x, y);

            Reflect_info info = calculate_reflect_info(x, y);

            if (info.segment_number) {
                float reflection_angle = (info.segment_number * m_segment_width);
                reflection_angle -= info.segment_number % 2 ? (m_segment_width - 2 * (info.reference_angle - reflection_angle)) : 0;

                reflection_angle *= std::signbit(info.angle) ? 1 : -1;
                float cos_angle = std::cos(reflection_angle);
                float sin_angle = std::sin(reflection_angle);
                float source_x = info.screen_x * cos_angle - info.screen_y * sin_angle;
                float source_y = info.screen_y * cos_angle + info.screen_x * sin_angle;

                from_screen(&source_x, &source_y);

                if (m_edge_reflect) {
                    if (source_x < 0) {
                        source_x = -source_x;
                    } else if (source_x > m_width - 10e-4f) {
                        source_x = m_width - (source_x - m_width + 10e-4f);
                    } if (source_y < 0) {
                        source_y = -source_y;
                    } else if (source_y > m_height - 10e-4f) {
                        source_y = m_height - (source_y - m_height + 10e-4f);
                    }
                    std::memcpy(out, lookup(block->in_frame, static_cast<std::uint32_t>(source_x), static_cast<std::uint32_t>(source_y)), m_pixel_size);
                } else {
                    process_bg(source_x, source_y, block->in_frame, out);
                }
            } else {
                std::memcpy(out, lookup(block->in_frame, x, y), m_pixel_size);
            }
        }
    }
}
#endif

std::uint8_t colours[63][3] = {
    { 0x00, 0xFF, 0x00 },
    { 0x00, 0x00, 0xFF },
    { 0xFF, 0x00, 0x00 },
    { 0x01, 0xFF, 0xFE },
    { 0xFF, 0xA6, 0xFE },
    { 0xFF, 0xDB, 0x66 },
    { 0x00, 0x64, 0x01 },
    { 0x01, 0x00, 0x67 },
    { 0x95, 0x00, 0x3A },
    { 0x00, 0x7D, 0xB5 },
    { 0xFF, 0x00, 0xF6 },
    { 0xFF, 0xEE, 0xE8 },
    { 0x77, 0x4D, 0x00 },
    { 0x90, 0xFB, 0x92 },
    { 0x00, 0x76, 0xFF },
    { 0xD5, 0xFF, 0x00 },
    { 0xFF, 0x93, 0x7E },
    { 0x6A, 0x82, 0x6C },
    { 0xFF, 0x02, 0x9D },
    { 0xFE, 0x89, 0x00 },
    { 0x7A, 0x47, 0x82 },
    { 0x7E, 0x2D, 0xD2 },
    { 0x85, 0xA9, 0x00 },
    { 0xFF, 0x00, 0x56 },
    { 0xA4, 0x24, 0x00 },
    { 0x00, 0xAE, 0x7E },
    { 0x68, 0x3D, 0x3B },
    { 0xBD, 0xC6, 0xFF },
    { 0x26, 0x34, 0x00 },
    { 0xBD, 0xD3, 0x93 },
    { 0x00, 0xB9, 0x17 },
    { 0x9E, 0x00, 0x8E },
    { 0x00, 0x15, 0x44 },
    { 0xC2, 0x8C, 0x9F },
    { 0xFF, 0x74, 0xA3 },
    { 0x01, 0xD0, 0xFF },
    { 0x00, 0x47, 0x54 },
    { 0xE5, 0x6F, 0xFE },
    { 0x78, 0x82, 0x31 },
    { 0x0E, 0x4C, 0xA1 },
    { 0x91, 0xD0, 0xCB },
    { 0xBE, 0x99, 0x70 },
    { 0x96, 0x8A, 0xE8 },
    { 0xBB, 0x88, 0x00 },
    { 0x43, 0x00, 0x2C },
    { 0xDE, 0xFF, 0x74 },
    { 0x00, 0xFF, 0xC6 },
    { 0xFF, 0xE5, 0x02 },
    { 0x62, 0x0E, 0x00 },
    { 0x00, 0x8F, 0x9C },
    { 0x98, 0xFF, 0x52 },
    { 0x75, 0x44, 0xB1 },
    { 0xB5, 0x00, 0xFF },
    { 0x00, 0xFF, 0x78 },
    { 0xFF, 0x6E, 0x41 },
    { 0x00, 0x5F, 0x39 },
    { 0x6B, 0x68, 0x82 },
    { 0x5F, 0xAD, 0x4E },
    { 0xA7, 0x57, 0x40 },
    { 0xA5, 0xFF, 0xD2 },
    { 0xFF, 0xB1, 0x67 },
    { 0x00, 0x9B, 0xFF },
    { 0xE8, 0x5E, 0xBE }
};

std::int32_t Kaleid0sc0pe::set_segment_direction(Direction direction)
{
    m_segment_direction = direction;
    m_n_segments = 0;
    return 0;
}

libkaleid0sc0pe::Kaleid0sc0pe::Direction Kaleid0sc0pe::get_segment_direction() const
{
    return m_segment_direction;
}

std::int32_t Kaleid0sc0pe::process(const void* in_frame, void* out_frame)
{
    if (in_frame == nullptr || out_frame == nullptr) {
        return -2;
    }
#ifdef __SSE2__
    if (m_width % 4 != 0) {
        return -2;
    }
#endif
    if (m_n_segments == 0) {
        init();
    }
#ifdef NO_FUTURE
    Block block(reinterpret_cast<const std::uint8_t*>(in_frame),
        reinterpret_cast<std::uint8_t*>(out_frame),
        0, 0,
        m_width - 1, m_height - 1);
#ifdef __SSE2__
    if (m_edge_reflect) {
        process_block(&block);
    } else {
        process_block_bg(&block);
    }
#else
    process_block(&block);
#endif
#else
    if (m_n_threads == 1) {
        Block block(reinterpret_cast<const std::uint8_t*>(in_frame),
            reinterpret_cast<std::uint8_t*>(out_frame),
            0, 0,
            m_width - 1, m_height - 1);
#ifdef __SSE2__
        if (m_edge_reflect) {
            process_block(&block);
        } else {
            process_block_bg(&block);
        }
#else
        process_block(&block);
#endif
    } else {
        std::uint32_t n_threads = m_n_threads == 0 ? std::thread::hardware_concurrency() : m_n_threads;

        std::vector<std::future<void>> futures;
        std::vector<std::unique_ptr<Block>> blocks;

        std::uint32_t block_height = m_height / n_threads;
        std::uint32_t y_start = 0;
        std::uint32_t y_end = m_height - block_height * (n_threads - 1) - 1;

        for (std::uint32_t i = 0; i < n_threads; ++i) {
            blocks.emplace_back(new Block(
                reinterpret_cast<const std::uint8_t*>(in_frame),
                reinterpret_cast<std::uint8_t*>(out_frame),
                0, y_start,
                m_width - 1, y_end));

#ifdef __SSE2__
            futures.push_back(std::async(std::launch::async, m_edge_reflect ? &Kaleid0sc0pe::process_block : &Kaleid0sc0pe::process_block_bg, this, blocks[i].get()));
#else
            futures.push_back(std::async(std::launch::async, &Kaleid0sc0pe::process_block, this, blocks[i].get()));
#endif
            y_start = y_end + 1;
            y_end += block_height;
        }
        for (auto& f : futures) {
            f.wait();
        }
    }
#endif

    return 0;
}

std::int32_t Kaleid0sc0pe::set_threading(std::uint32_t threading)
{
    m_n_threads = threading;
    return 0;
}

std::uint32_t Kaleid0sc0pe::get_threading() const
{
    return m_n_threads;
}

std::int32_t Kaleid0sc0pe::visualise(void* out_frame)
{
    if (out_frame == nullptr) {
        return -2;
    }
#ifdef __SSE2__
    if (m_width % 4 != 0) {
        return -2;
    }
#endif
    if (m_n_segments == 0) {
        init();
    }

    for (std::uint32_t y = 0; y < m_height; ++y) {
#ifdef __SSE2__
        for (std::uint32_t x = 0; x < m_width; x+=4) {
#else
        for (std::uint32_t x = 0; x < m_width; ++x) {
#endif
            std::uint8_t* out = lookup(reinterpret_cast<std::uint8_t*>(out_frame), x, y);
#ifdef __SSE2__
            ALIGN16_BEG int ALIGN16_END mx[4] = { static_cast<int>(x), static_cast<int>(x) + 1, static_cast<int>(x) + 2, static_cast<int>(x) + 3};
            ALIGN16_BEG int ALIGN16_END my[4] = { static_cast<int>(y), static_cast<int>(y), static_cast<int>(y), static_cast<int>(y) };

            Reflect_info info = calculate_reflect_info((__m128i*)mx, (__m128i*)my);
            //float* segment_number = reinterpret_cast<float*>(&info.segment_number);
            std::int32_t* segment_number = reinterpret_cast<std::int32_t*>(&info.segment_number_i);
            std::uint32_t col_idx = (*segment_number) % 63;
            out[0] = colours[col_idx][0];
            out[1] = colours[col_idx][1];
            out[2] = colours[col_idx][2];
            if (m_num_components > 3) {
                out[3] = 0xff;
                out++;
            }
            segment_number++;
            out += 3;

            col_idx = (*segment_number) % 63;
            out[0] = colours[col_idx][0];
            out[1] = colours[col_idx][1];
            out[2] = colours[col_idx][2];
            if (m_num_components > 3) {
                out[3] = 0xff;
                out++;
            }
            segment_number++;
            out += 3;

            col_idx = (*segment_number) % 63;
            out[0] = colours[col_idx][0];
            out[1] = colours[col_idx][1];
            out[2] = colours[col_idx][2];
            if (m_num_components > 3) {
                out[3] = 0xff;
                out++;
            }
            segment_number++;
            out += 3;

            col_idx = (*segment_number) % 63;
            out[0] = colours[col_idx][0];
            out[1] = colours[col_idx][1];
            out[2] = colours[col_idx][2];
            if (m_num_components > 3) {
                out[3] = 0xff;
                out++;
            }

#else
            Reflect_info info = calculate_reflect_info(x, y);
            std::uint32_t col_idx = info.segment_number % 63;
            out[0] = colours[col_idx][0];
            out[1] = colours[col_idx][1];
            out[2] = colours[col_idx][2];
            if (m_num_components > 3) {
                out[3] = 0xff;
            }
#endif

        }
    }
    return 0;
}

} // namespace libkaleid0sc0pe
