/*
 * Regression test for motiondiff0r: a failed allocation inside f0r_update
 * must not let an exception cross the plugin API, must produce a defined
 * output and must leave the frame history usable for the next frames.
 *
 * The plugin source is compiled into this program, so replacing the global
 * operator new below also covers the plugin's allocations.
 */

#include <stdint.h>

#include <cstdio>
#include <cstdlib>
#include <new>

extern "C" {
#include <frei0r.h>
}

namespace {

enum { width = 8, height = 8, pixels = width * height };
enum { p_offset, p_mode };

std::size_t fail_size = 0;  // fail the next allocation of exactly this size
bool fail_all = false;      // fail every allocation
int failures = 0;

void fill(uint32_t* frame, uint8_t value)
{
  for (int i = 0; i < pixels; ++i) {
    uint8_t* p = reinterpret_cast<uint8_t*>(&frame[i]);
    p[0] = p[1] = p[2] = value;
    p[3] = 255;
  }
}

int update(f0r_instance_t instance, double time, const uint32_t* in, uint32_t* out,
           const char* what)
{
  try {
    f0r_update(instance, time, in, out);
  } catch (...) {
    std::fprintf(stderr, "%s: exception escaped f0r_update\n", what);
    return 1;
  }
  return 0;
}

int expect(const uint32_t* frame, unsigned int value, const char* what)
{
  for (int i = 0; i < pixels; ++i) {
    const uint8_t* p = reinterpret_cast<const uint8_t*>(&frame[i]);
    if (p[0] != value || p[1] != value || p[2] != value || p[3] != 255) {
      std::fprintf(stderr, "%s: pixel %d is (%u,%u,%u,%u), expected (%u,%u,%u,255)\n",
                   what, i, p[0], p[1], p[2], p[3], value, value, value);
      return 1;
    }
  }
  return 0;
}

int expect_failures(int count, const char* what)
{
  if (failures == count) return 0;
  std::fprintf(stderr, "%s: %d allocation failures injected, expected %d\n",
               what, failures, count);
  return 1;
}

}  // namespace

void* operator new(std::size_t n)
{
  if (fail_all || (fail_size != 0 && n == fail_size)) {
    fail_size = 0;
    ++failures;
    throw std::bad_alloc();
  }
  void* p = std::malloc(n ? n : 1);
  if (!p) throw std::bad_alloc();
  return p;
}

void operator delete(void* p) noexcept
{
  std::free(p);
}

void operator delete(void* p, std::size_t) noexcept
{
  std::free(p);
}

int main()
{
  static uint32_t in[pixels], out[pixels];
  int errors = 0;
  double value;

  if (!f0r_init()) return 1;
  f0r_instance_t instance = f0r_construct(width, height);
  if (!instance) return 1;

  // Compare with the previous frame and show the absolute difference.
  value = 0.0;
  f0r_set_param_value(instance, &value, p_offset);
  value = 0.5;
  f0r_set_param_value(instance, &value, p_mode);

  // The copy of the first frame fails: the output shows no motion.
  fill(in, 100);
  fill(out, 77);
  fail_size = sizeof(in);
  errors += update(instance, 0.0, in, out, "failed first frame");
  errors += expect_failures(1, "failed first frame");
  errors += expect(out, 0, "failed first frame");

  // The next frames render normally from a consistent history.
  fill(in, 150);
  errors += update(instance, 1 / 30.0, in, out, "frame after failure");
  errors += expect(out, 0, "frame after failure");
  fill(in, 200);
  errors += update(instance, 2 / 30.0, in, out, "recovered history");
  errors += expect(out, 50, "recovered history");

  // Every allocation fails while older frames are in the history.
  fill(in, 210);
  fail_all = true;
  errors += update(instance, 3 / 30.0, in, out, "failed frame with history");
  fail_all = false;
  errors += expect_failures(2, "failed frame with history");
  errors += expect(out, 0, "failed frame with history");

  // The frame that could not be stored is skipped: compare with 200.
  fill(in, 230);
  errors += update(instance, 4 / 30.0, in, out, "history after failure");
  errors += expect(out, 30, "history after failure");

  f0r_destruct(instance);

  // In-place rendering in the default negative overlay mode falls back to
  // the neutral mid gray.
  instance = f0r_construct(width, height);
  if (!instance) return 1;
  fill(in, 240);
  fail_size = sizeof(in);
  errors += update(instance, 0.0, in, in, "failed in-place frame");
  errors += expect_failures(3, "failed in-place frame");
  errors += expect(in, 128, "failed in-place frame");
  fill(in, 240);
  errors += update(instance, 1 / 30.0, in, in, "in-place frame after failure");
  errors += expect(in, 128, "in-place frame after failure");

  f0r_destruct(instance);
  f0r_deinit();
  return errors ? 1 : 0;
}
