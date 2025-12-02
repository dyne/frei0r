/* This file is part of frei0r (https://frei0r.dyne.org)
 *
 * Copyright (C) 2024-2025 Dyne.org foundation
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#ifndef TEST_PATTERN_H
#define TEST_PATTERN_H

#include <stdint.h>

/**
 * Generate an animated test pattern for video filter testing
 * 
 * This creates a comprehensive test pattern that includes:
 * - Animated radial gradient background (tests color interpolation and smooth transitions)
 * - Bouncing circle (tests circular features and motion)
 * - Rotating square (tests corners, rotation, and sharp edges)
 * - Moving horizontal bar (tests horizontal motion and rectangular shapes)
 * - Zone plate patch (tests spatial frequency response and aliasing)
 * - Frame counter (tests text/detail preservation)
 * 
 * @param frame        Output buffer (32-bit per pixel)
 * @param width        Frame width in pixels
 * @param height       Frame height in pixels
 * @param frame_num    Current frame number (for animation)
 * @param color_model  Color model (F0R_COLOR_MODEL_BGRA8888, F0R_COLOR_MODEL_RGBA8888, or F0R_COLOR_MODEL_PACKED32)
 */
void generate_animated_test_pattern(uint32_t* frame, int width, int height, int frame_num, int color_model);

#endif // TEST_PATTERN_H
