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
 * History is preallocated at construction: at most 1 GiB of pixels and
 * history metadata per instance, including the current frame. The effective offset
 * is shortened when the requested history does not fit. Rendering allocates
 * no buffers and reuses released slots before touching unused buffers;
 * dimensions that cannot fit two frames fail construction.
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
#include <memory>

namespace {

const double MAX_OFFSET_SECONDS = 2.0;
const double MAX_GAIN = 16.0;
#ifdef MOTIONDIFF_TEST_HISTORY_BYTES
const size_t MAX_HISTORY_BYTES = MOTIONDIFF_TEST_HISTORY_BYTES;
#else
const size_t MAX_HISTORY_BYTES = 1024u * 1024u * 1024u;
#endif

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

class motiondiff0r : public frei0r::filter
{
  struct FrameInfo {
    double timestamp;
    size_t previous;
    size_t next;
  };

public:
  motiondiff0r(unsigned int frame_width, unsigned int frame_height)
    : frame_pixels(0), capacity(0), head(0), tail(0), count(0),
      free_head(0), next_unused(0)
  {
    offset = 0.1;      // 200 ms
    mode = 0.0;        // negative overlay
    blend = 0.5;
    contrast = 0.0;    // gain 1x
    threshold = 0.0;
    monochrome = false;

    register_param(offset, "Time Offset",
                   "0..1 = 0..2 seconds, limited to 1 GiB of history; 0 = previous frame");
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

    // The registration helper creates a zero-sized metadata-only instance.
    if (!frame_width || !frame_height) return;
    const size_t max_pixels = (MAX_HISTORY_BYTES / 2 - sizeof(FrameInfo)) / sizeof(uint32_t);
    if ((size_t)frame_width > max_pixels / frame_height) return;
    frame_pixels = (size_t)frame_width * frame_height;
    capacity = MAX_HISTORY_BYTES / (frame_pixels * sizeof(uint32_t) + sizeof(FrameInfo));
    storage.reset(new (std::nothrow) uint32_t[capacity * frame_pixels]);
    if (!storage) return;
    frames.reset(new (std::nothrow) FrameInfo[capacity]);
    // capacity is the sentinel for empty links. Initialize entries on use so
    // short offsets touch only the pixel buffers and metadata they need.
    head = tail = free_head = capacity;
  }

  virtual bool is_valid() const { return storage.get() != 0 && frames.get() != 0; }

  virtual void update(double time, uint32_t* out, const uint32_t* in)
  {
    const size_t n = frame_pixels;
    const double target = time - std::min(std::max(offset, 0.0), 1.0) * MAX_OFFSET_SECONDS;

    // Forget frames at or after the current time: the host seeked backwards
    // or is rendering the same time again.
    while (count && frames[tail].timestamp >= time)
      release(tail);

    // Keep the newest frame not later than the target as the oldest one.
    while (count >= 2 && frames[frames[head].next].timestamp <= target)
      release(head);
    if (count == capacity)
      release(head);

    // Store a copy of the current frame before rendering, so that the
    // history stays intact when the host renders in place (in == out).
    size_t current;
    if (free_head != capacity) {
      current = free_head;
      free_head = frames[current].next;
    } else {
      current = next_unused++;
    }
    uint32_t* pixels = storage.get() + current * n;
    std::copy(in, in + n, pixels);
    frames[current].timestamp = time;
    frames[current].previous = tail;
    frames[current].next = capacity;
    if (count) frames[tail].next = current;
    else head = current;
    tail = current;
    ++count;

    // With no older frame yet, the current frame is compared with itself.
    process(pixels, storage.get() + head * n, out, n);
  }

private:
  double offset;
  double mode;
  double blend;
  double contrast;
  double threshold;
  bool monochrome;

  size_t frame_pixels;
  size_t capacity;
  size_t head;
  size_t tail;
  size_t count;
  size_t free_head;
  size_t next_unused;
  std::unique_ptr<FrameInfo[]> frames;
  std::unique_ptr<uint32_t[]> storage;

  void release(size_t index)
  {
    const size_t previous = frames[index].previous;
    const size_t next = frames[index].next;
    if (previous != capacity) frames[previous].next = next;
    else head = next;
    if (next != capacity) frames[next].previous = previous;
    else tail = previous;
    // The next link doubles as a free list while this slot is inactive.
    frames[index].next = free_head;
    free_head = index;
    --count;
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
