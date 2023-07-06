/*
 * kaleid0sc0pe.h
 * Copyright (C) 2020-2023 Brendan Hack (github@bendys.com)
 * This file is part of a Frei0r plugin that applies a kaleidoscope
 * effect.
 * Version 1.1	july 2023
 *
 * The kaleidoscope class definition.
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
#ifndef SRC_FILTER_KALEID0SC0PE_KALEID0SC0PE_H_
#define SRC_FILTER_KALEID0SC0PE_KALEID0SC0PE_H_ 1

#include "ikaleid0sc0pe.h"

#include <vector>
#include <cmath>
#include <functional>

#ifdef NO_SSE2
#ifdef __SSE2__
#undef __SSE2__
#endif
#else
// set sse2 flag on windows
#if _M_IX86_FP == 2 || _M_X64 == 100
#ifndef _M_ARM
#define __SSE2__
#endif
#endif
#endif

#ifdef __SSE2__
#include <emmintrin.h>
#endif

namespace libkaleid0sc0pe {

/**
 * Class which implements the kaleid0sc0pe effect
 */
class Kaleid0sc0pe: public IKaleid0sc0pe {
public:
    /**
     * Constructor
     * @param width the frame width
     * @param height the frame height
     * @param component_size the byte size of each frame pixel component
     * @param num_components the number of components per pixel
     * @param stride the image stride, if \c 0 then calculated as \p width * \p component_size * \p num_components
     */
    Kaleid0sc0pe(std::uint32_t width, std::uint32_t height, std::uint32_t component_size, std::uint32_t num_components, std::uint32_t stride = 0);

    virtual std::int32_t set_origin(float x, float y);

    virtual float get_origin_x() const;

    virtual float get_origin_y() const;

    virtual std::int32_t set_segmentation(std::uint32_t segmentation);

    virtual std::uint32_t get_segmentation() const;

    virtual std::int32_t set_edge_threshold(std::uint32_t threshold);

    virtual std::uint32_t get_edge_threshold() const;

    virtual std::int32_t set_segment_direction(Direction direction);

    virtual Direction get_segment_direction() const;

    virtual std::int32_t set_preferred_corner(Corner corner);

    virtual Corner get_preferred_corner() const;

    virtual std::int32_t set_preferred_corner_search_direction(Direction direction);

    virtual Direction get_preferred_corner_search_direction() const;

    virtual std::int32_t set_reflect_edges(bool reflect);

    virtual bool get_reflect_edges() const;

    virtual std::int32_t set_background_colour(void* colour);

    virtual void *get_background_colour() const;

    virtual std::int32_t set_source_segment(float angle);

    virtual float get_source_segment() const;

    virtual std::int32_t process(const void* in_frame, void* out_frame);

    virtual std::int32_t set_threading(std::uint32_t threading);

    virtual std::uint32_t get_threading() const;

    virtual std::int32_t visualise(void* out_frame);

private:
    void init();

#ifdef __SSE2__
    /// Defines reflection information for a given point in the frame
    struct Reflect_info {
        __m128 screen_x;                 ///< x coordinate in screen space (range -0.5->0.5, left negative)
        __m128 screen_y;                 ///< y coordinate in screen space (range -0.5->0.5, top negative)
        __m128 angle;                    ///< angle from this point to the start of the source segment
        __m128 segment_number;           ///< segment number the point resides in
        __m128i segment_number_i;
        __m128 reference_angle;          ///< positive angle to start of source segment
    };

    Reflect_info calculate_reflect_info(__m128i *x, __m128i *y);

    /// Converts coordinates to screen space
    /// @param x x coordinate
    /// @param y y coordinate
    /// @param sx source x coordinate
    /// @param sy source y coordinate
    void to_screen(__m128 *x, __m128 *y, __m128i *sx, __m128i *sy);

    /// Converts coordinates from screen space in place
    /// @param x x coordinate
    /// @param y y coordinate
    void from_screen(__m128 *x, __m128 *y);

    /// Rotate the four coordinates from <tt>x,y</tt> to <tt>x+4,y</tt> and store results in
    /// <tt>source_x,source_y</tt>
    /// @param x x coordinate to start rotate from
    /// @param y y coordinate to rotate
    /// @param source_x receives the x coordiante results
    /// @param source_y receives the y coordinate results
    inline void rotate(int x, int y, __m128 *source_x, __m128 *source_y);
#else
    /// Defines reflection information for a given point in the frame
    struct Reflect_info {
        float screen_x;                 ///< x coordinate in screen space (range -0.5->0.5, left negative)
        float screen_y;                 ///< y coordinate in screen space (range -0.5->0.5, top negative)
        float angle;                    ///< angle from this point to the start of the source segment
        std::uint32_t segment_number;   ///< segment number the point resides in
        float reference_angle;          ///< positive angle to start of source segment
    };

    /// Calculates the reflection information for a given point.
    /// NB: init() must have already been called.
    /// @param x the x coordinate
    /// @param x the y coordinate
    /// @return the reflection information for the point
    Reflect_info calculate_reflect_info(std::uint32_t x, std::uint32_t y);

    /// Converts coordinates to screen space
    /// @param x recieves x coordinate
    /// @param y recieves y coordinate
    /// @param sx source x coordinate
    /// @param sy source y coordinate
    void to_screen(float *x, float *y, std::uint32_t sx, std::uint32_t sy);

    /// Converts coordinates from screen space in place
    /// @param x x coordinate
    /// @param y y coordinate
    void from_screen(float *x, float *y);
#endif
    /// A block of data to process
    struct Block {
        const std::uint8_t* in_frame;
        std::uint8_t* out_frame;
        std::uint32_t x_start;
        std::uint32_t y_start;
        std::uint32_t x_end;
        std::uint32_t y_end;

        /// \param in_frame the input frame
        /// \param out_frame the output frame
        /// \param x_start start x coordinate of block to process
        /// \param y_start start y coordinate of block to process
        /// \param x_end end x coordinate of block to process (inclusive)
        /// \param y_end end y coordinate of block to process (inclusive)
        Block(const std::uint8_t* _in_frame, std::uint8_t* _out_frame, std::uint32_t _x_start, std::uint32_t _y_start, std::uint32_t _x_end, std::uint32_t _y_end):
            in_frame(_in_frame),
            out_frame(_out_frame),
            x_start(_x_start),
            y_start(_y_start),
            x_end(_x_end),
            y_end(_y_end)
        {}
    };

    /// Process a block
    void process_block(Block *block);

    /// Copy pixel <tt>x,y</tt> from \p in to \p out using the background colour
    /// if the pixel is out of range
    /// @param x x coordinate to copy
    /// @param y y coordinate to copy
    /// @param in the first pixel in the source image
    /// @param out destination
    void process_bg(float x, float y, const std::uint8_t* in, std::uint8_t* out);


#ifdef __SSE2__
    // Process a block using background colour copy
    void process_block_bg(Block* block);
#endif

    std::uint8_t *lookup(std::uint8_t *p, std::uint32_t x, std::uint32_t y);

    const std::uint8_t* lookup(const std::uint8_t* p, std::uint32_t x, std::uint32_t y);

    std::uint32_t m_width;
    std::uint32_t m_height;
    std::uint32_t m_component_size;
    std::uint32_t m_num_components;
    std::uint32_t m_stride;
    std::uint32_t m_pixel_size;

    float m_aspect;

    float m_origin_x;
    float m_origin_y;
    float m_origin_native_x;
    float m_origin_native_y;

    std::uint32_t m_segmentation;
    Direction m_segment_direction;

    Corner m_preferred_corner;
    Direction m_preferred_search_dir;

    bool m_edge_reflect;

    void* m_background_colour;
    std::uint32_t m_edge_threshold;

    float m_source_segment_angle;

    std::uint32_t m_n_segments;
    float m_start_angle;
    float m_segment_width;

    std::uint32_t m_n_threads;

#ifdef __SSE2__
    __m128 m_sse_aspect;
    __m128 m_sse_origin_native_x;
    __m128 m_sse_origin_native_y;
    __m128 m_sse_start_angle;
    __m128 m_sse_segment_width;
    __m128 m_sse_half_segment_width;
    __m128 m_sse_ps_0;
    __m128 m_sse_ps_1;
    __m128 m_sse_ps_2;
    __m128i m_sse_epi32_1;
    __m128i m_sse_epi32_2;
    __m128i m_sse_shift_1;
    __m128 m_sse_width;
    __m128 m_sse_height;
    __m128 m_sse_width_m1;
    __m128 m_sse_height_m1;
#endif
};

} // namespace libkaleid0sc0pe

#endif // SRC_FILTER_KALEID0SC0PE_KALEID0SC0PE_H_
