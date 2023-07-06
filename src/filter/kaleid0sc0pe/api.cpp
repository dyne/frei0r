/*
 * api.cpp
 * Copyright (C) 2020-2023 Brendan Hack (github@bendys.com)
 * This file is part of a Frei0r plugin that applies a kaleidoscope
 * effect.
 * Version 1.1	july 2023
 *
 * Implements the Frei0r interface
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
#include "frei0r.hpp"
#include "ikaleid0sc0pe.h"

class kaleid0sc0pe : public frei0r::filter
{
public:
    kaleid0sc0pe(unsigned int width, unsigned int height):
        m_kaleid0sc0pe(libkaleid0sc0pe::IKaleid0sc0pe::factory(width,height,1,4)),
        m_origin_x(0.5),
        m_origin_y(0.5),
        m_segmentation(16/128.0),
        m_direction(1.0),
        m_corner(0),
        m_corner_direction(true),
        m_reflect_edges(true),
        m_edge_threshold(0),
        m_bg_alpha(1),
        m_specify_source(false),
        m_source_segment(0),
        m_multithreaded(true),
        m_threads(0)
    {
        m_bg_colour.r = 1.0;
        m_bg_colour.g = 0.0;
        m_bg_colour.b = 1.0;

        register_param(m_origin_x,
                                "origin_x",
                                "origin of the kaleid0sc0pe in x. default 0.5");
        register_param(m_origin_y,
                                "origin_y",
                                "origin of the kaleid0sc0pe in y. default 0.5");
        register_param(m_segmentation,
                                "segmentation",
                                "kaleid0sc0pe segmentation / 128, segmentations of 1, 2 or multiples of 4 work best. default 16/128");
        register_param(m_specify_source,
                                "specify_source",
                                "if true then source angle is read from source_segment, otherwise auto-calculated");
        register_param(m_source_segment,
                                "source_segment",
                                "centre of source segment if specify_source is true. 0 is in +x and rotates counter clockwise");
        register_param(m_direction,
                                "segmentation_direction",
                                "segmentation direction, < 1/3 is none, < 2/3 is counter clockwise, otherwise clockwise");
        register_param(m_reflect_edges,
                                "reflect_edges",
                                "if true then reflections that end up outside the source reflect back into it, otherwise the specified background colour is used.");
        register_param(m_edge_threshold,
                                "edge_threshold",
                                "edge threshold / 4, reflections outside the image but within this distance clamp to the edge. default 0");
        register_param(m_corner,
                                "preferred_corner",
                                "preferred corner, 0 is top right, 0.25 top left, 0.5 bottom left, 0.75 bottom right");
        register_param(m_corner_direction,
                                "corner_search",
                                "if true search clockwise for furthest corner, otherwise counter clockwise");
        register_param(m_bg_colour,
                                "bg_color",
                                "colour to use if reflection lies outside of source image and not reflecting back in. default 1,0,1");
        register_param(m_bg_alpha,
                                "bg_alpha",
                                "alpha to use if reflection lies outside of source image and not reflecting back in. default 1");
        register_param(m_multithreaded,
                                "multithreaded",
                                "set to true to enable multithreaded calculation. default true");
        register_param(m_threads,
                                "n_threads",
                                "the number of threads to use, if 0 then autocalculate otherwise value * 32. default 0");


        m_kaleid0sc0pe->set_background_colour(m_background);
    }

    virtual void update(double time,
        uint32_t* out,
        const uint32_t* in) {

        update_params();
        m_kaleid0sc0pe->process(in, out);
    }

private:
    void update_params()
    {
        m_kaleid0sc0pe->set_origin(static_cast<float>(m_origin_x), static_cast<float>(m_origin_y));
        m_kaleid0sc0pe->set_segmentation(static_cast<std::uint32_t>(m_segmentation * 128));
        if (m_direction < 1/3.0) {
            m_kaleid0sc0pe->set_segment_direction(libkaleid0sc0pe::IKaleid0sc0pe::Direction::NONE);
        } else if (m_direction < 2/3.0) {
            m_kaleid0sc0pe->set_segment_direction(libkaleid0sc0pe::IKaleid0sc0pe::Direction::ANTICLOCKWISE);
        } else {
            m_kaleid0sc0pe->set_segment_direction(libkaleid0sc0pe::IKaleid0sc0pe::Direction::CLOCKWISE);
        }
        if (m_corner < 0.25) {
            m_kaleid0sc0pe->set_preferred_corner(libkaleid0sc0pe::IKaleid0sc0pe::Corner::TR);
        } else if (m_corner < 0.5) {
            m_kaleid0sc0pe->set_preferred_corner(libkaleid0sc0pe::IKaleid0sc0pe::Corner::TL);
        } else if (m_corner < 0.75) {
            m_kaleid0sc0pe->set_preferred_corner(libkaleid0sc0pe::IKaleid0sc0pe::Corner::BL);
        } else {
            m_kaleid0sc0pe->set_preferred_corner(libkaleid0sc0pe::IKaleid0sc0pe::Corner::BR);
        }
        if (m_corner_direction) {
            m_kaleid0sc0pe->set_preferred_corner_search_direction(libkaleid0sc0pe::IKaleid0sc0pe::Direction::CLOCKWISE);
        } else {
            m_kaleid0sc0pe->set_preferred_corner_search_direction(libkaleid0sc0pe::IKaleid0sc0pe::Direction::ANTICLOCKWISE);
        }
        m_kaleid0sc0pe->set_reflect_edges(m_reflect_edges);
        m_kaleid0sc0pe->set_edge_threshold(static_cast<std::uint32_t>(m_edge_threshold * 4));

        if (m_specify_source) {
            m_kaleid0sc0pe->set_source_segment(static_cast<float>(m_source_segment) * 3.141592654f * 2);
        } else {
            m_kaleid0sc0pe->set_source_segment(-1);
        }
        if (m_multithreaded) {
            m_kaleid0sc0pe->set_threading(static_cast<std::uint32_t>(m_threads * 32));
        } else {
            m_kaleid0sc0pe->set_threading(1);
        }
        m_background[0] = static_cast<std::uint8_t>(m_bg_colour.r * 255);
        m_background[1] = static_cast<std::uint8_t>(m_bg_colour.g * 255);
        m_background[2] = static_cast<std::uint8_t>(m_bg_colour.b * 255);
        m_background[3] = static_cast<std::uint8_t>(m_bg_alpha * 255);
    }

    double m_origin_x;
    double m_origin_y;

    double m_segmentation;
    double m_direction;

    double m_corner;
    bool m_corner_direction;

    bool m_reflect_edges;

    double m_edge_threshold;

    f0r_param_color m_bg_colour;
    double m_bg_alpha;

    bool m_specify_source;
    double m_source_segment;

    bool m_multithreaded;
    double m_threads;

    std::uint8_t m_background[4];

    std::unique_ptr<libkaleid0sc0pe::IKaleid0sc0pe> m_kaleid0sc0pe;
};

frei0r::construct<kaleid0sc0pe> plugin("Kaleid0sc0pe", "Applies a kaleid0sc0pe effect", "Brendan Hack", 1, 1, F0R_COLOR_MODEL_RGBA8888);
