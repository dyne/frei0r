/*
 * motiondiff0r - temporal frame differencing
 * Copyright (C) 2026 phoebus-84
 *
 * The current frame is compared with a frame taken from the recent past:
 * the delayed frame is inverted and laid over the current one, so that
 * everything that stays still turns into neutral gray while moving or
 * changing regions show up as light and dark (positive / negative) shapes.
 *
 * Modes:
 *   - Negative overlay:     R = (1 - b) * F(t) + b * (255 - F(t - dt))
 *                           with b = 0.5 static areas become mid gray
 *   - Absolute difference:  R = |F(t) - F(t - dt)|, static areas are black
 *   - Motion mask:          current frame with the amount of change written
 *                           into the alpha channel, for use with other filters
 *
 * The delayed frame is chosen from a small history using the timestamps
 * given by the host, so the time offset is independent of the frame rate.
 * Memory use grows with the time offset: one frame buffer is kept for every
 * frame received during the offset interval.
 *
 * This source code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This source code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  Please refer
 * to the GNU Public License for more details.
 */

#include "frei0r.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <deque>
#include <vector>

namespace {

const double MAX_OFFSET_SECONDS = 2.0;
const double MAX_GAIN = 16.0;
// Safety limit for hosts sending very dense timestamps.
const size_t MAX_HISTORY_FRAMES = 512;

enum Mode { MODE_OVERLAY = 0, MODE_ABSDIFF = 1, MODE_MASK = 2 };

inline uint8_t clamp8(double v)
{
  if (v <= 0.0) return 0;
  if (v >= 255.0) return 255;
  return (uint8_t)(v + 0.5);
}

inline int luma(const uint8_t* p)
{
  // Rec. 601 weights on R, G, B (RGBA8888 byte order)
  return (299 * p[0] + 587 * p[1] + 114 * p[2] + 500) / 1000;
}

struct Frame {
  double time;
  std::vector<uint32_t> pixels;
};

class motiondiff0r : public frei0r::filter
{
public:
  motiondiff0r(unsigned int, unsigned int)
  {
    offset = 0.1;      // 200 ms
    mode = 0.0;        // negative overlay
    blend = 0.5;
    contrast = 0.0;    // gain 1x
    threshold = 0.0;
    monochrome = false;

    register_param(offset, "Time Offset",
                   "Time between the two frames (0..1 = 0..2 seconds; 0 = previous frame)");
    register_param(mode, "Mode",
                   "0: negative overlay, 0.5: absolute difference, 1: motion mask");
    register_param(blend, "Blend",
                   "Opacity of the inverted delayed frame in negative overlay mode");
    register_param(contrast, "Contrast",
                   "Amplification of the differences (0..1 = 1x..16x)");
    register_param(threshold, "Threshold",
                   "Ignore differences smaller than this to suppress noise");
    register_param(monochrome, "Monochrome",
                   "Compare luminance only and produce a grayscale result");
  }

  virtual void update(double time, uint32_t* out, const uint32_t* in)
  {
    // No exception may cross the plugin API. If memory runs out the frame
    // is compared with itself, which gives the neutral "no motion" output,
    // and the history keeps only complete frames so later updates recover.
    try {
      store(time, in);
    } catch (...) {
      process(in, in, out, size);
      return;
    }
    process(&history.back().pixels[0], &history.front().pixels[0], out, size);
  }

private:
  double offset;
  double mode;
  double blend;
  double contrast;
  double threshold;
  bool monochrome;

  std::deque<Frame> history;
  std::vector<uint32_t> spare;

  void store(double time, const uint32_t* in)
  {
    const double target = time - std::min(std::max(offset, 0.0), 1.0) * MAX_OFFSET_SECONDS;

    // Copy the current frame before touching the history, so that a failed
    // allocation leaves the history unchanged. The copy also keeps the
    // history intact when the host renders in place (in == out).
    std::vector<uint32_t> pixels;
    pixels.swap(spare);
    pixels.assign(in, in + size);

    // Forget frames at or after the current time: the host seeked backwards
    // or is rendering the same time again.
    while (!history.empty() && history.back().time >= time) {
      recycle(history.back().pixels);
      history.pop_back();
    }

    // Keep the newest frame not later than the target as the oldest one.
    while (history.size() >= 2 && history[1].time <= target) {
      recycle(history.front().pixels);
      history.pop_front();
    }
    while (history.size() >= MAX_HISTORY_FRAMES) {
      recycle(history.front().pixels);
      history.pop_front();
    }

    history.push_back(Frame());
    history.back().time = time;
    history.back().pixels.swap(pixels);
  }

  void recycle(std::vector<uint32_t>& pixels)
  {
    // One spare buffer avoids reallocating every frame during playback.
    if (spare.capacity() < pixels.capacity())
      spare.swap(pixels);
  }

  int current_mode() const
  {
    if (mode < 1.0 / 3.0) return MODE_OVERLAY;
    if (mode < 2.0 / 3.0) return MODE_ABSDIFF;
    return MODE_MASK;
  }

  void process(const uint32_t* in, const uint32_t* delayed, uint32_t* out, size_t n) const
  {
    const int m = current_mode();
    const double b = std::min(std::max(blend, 0.0), 1.0);
    const double gain = std::pow(MAX_GAIN, std::min(std::max(contrast, 0.0), 1.0));
    const int thr = (int)(std::min(std::max(threshold, 0.0), 1.0) * 255.0 + 0.5);

    const uint8_t* src = reinterpret_cast<const uint8_t*>(in);
    const uint8_t* old = reinterpret_cast<const uint8_t*>(delayed);
    uint8_t* dst = reinterpret_cast<uint8_t*>(out);

    for (size_t i = 0; i < n; ++i, src += 4, old += 4, dst += 4) {
      int cur[3], d[3];
      if (monochrome) {
        const int lc = luma(src);
        const int ld = luma(old);
        cur[0] = cur[1] = cur[2] = lc;
        d[0] = d[1] = d[2] = lc - ld;
      } else {
        for (int c = 0; c < 3; ++c) {
          cur[c] = src[c];
          d[c] = src[c] - old[c];
        }
      }

      int mag = std::max(std::abs(d[0]), std::max(std::abs(d[1]), std::abs(d[2])));
      if (mag < thr) {
        d[0] = d[1] = d[2] = 0;
        mag = 0;
      }

      switch (m) {
      case MODE_OVERLAY:
        // (1 - b) * current + b * inverted delayed frame, where the
        // delayed frame is current - gain * difference.
        for (int c = 0; c < 3; ++c)
          dst[c] = clamp8((1.0 - b) * cur[c] + b * (255.0 - cur[c] + gain * d[c]));
        dst[3] = src[3];
        break;
      case MODE_ABSDIFF:
        for (int c = 0; c < 3; ++c)
          dst[c] = clamp8(gain * std::abs(d[c]));
        dst[3] = src[3];
        break;
      default: // MODE_MASK
        for (int c = 0; c < 3; ++c)
          dst[c] = (uint8_t)cur[c];
        dst[3] = clamp8(gain * mag * src[3] / 255.0);
        break;
      }
    }
  }
};

} // namespace

static frei0r::construct<motiondiff0r> plugin("motiondiff0r",
                                       "Temporal frame differencing: overlays an inverted delayed frame to reveal motion",
                                       "phoebus-84",
                                       0, 1,
                                       F0R_COLOR_MODEL_RGBA8888);
