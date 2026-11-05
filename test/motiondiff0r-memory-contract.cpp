#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <new>

extern "C" {
#include <frei0r.h>
}

namespace {
#ifdef MOTIONDIFF_TEST_HISTORY_BYTES
const size_t budget = MOTIONDIFF_TEST_HISTORY_BYTES;
#else
const size_t budget = 1024u * 1024u * 1024u;
#endif
size_t allocation_calls = 0;
size_t largest_request = 0;
size_t fail_size = 0;
bool deny_allocations = false;
bool capture_storage = false;
unsigned char *captured_storage = 0;
const size_t small_frame_bytes = 8 * 8 * sizeof(uint32_t);
size_t captured_bytes = 0;
size_t array_calls = 0;
size_t array_bytes = 0;
size_t pixel_bytes = 0;

void *allocate(size_t size, bool throwing, bool array = false)
{
  ++allocation_calls;
  if (size > largest_request) largest_request = size;
  void *p = (deny_allocations || size == fail_size || size > budget) ? 0 : std::malloc(size ? size : 1);
  if (!p && throwing) throw std::bad_alloc();
  if (array) {
    array_bytes += size;
    if (array_calls++ == 0) {
      pixel_bytes = size;
      if (p && capture_storage) {
        captured_storage = static_cast<unsigned char*>(p);
        // A bounded prefix detects cycling without touching the whole reservation.
        captured_bytes = size < 1024 * small_frame_bytes ? size : 1024 * small_frame_bytes;
        std::memset(p, 0xa5, captured_bytes);
      }
    }
  }
  return p;
}

void parameter(f0r_instance_t instance, int index, double value)
{
  f0r_set_param_value(instance, &value, index);
}
} // namespace

void *operator new(size_t size) { return allocate(size, true); }
void *operator new[](size_t size) { return allocate(size, true, true); }
void *operator new(size_t size, const std::nothrow_t&) noexcept { return allocate(size, false); }
void *operator new[](size_t size, const std::nothrow_t&) noexcept { return allocate(size, false, true); }
void operator delete(void *p) noexcept { std::free(p); }
void operator delete[](void *p) noexcept { std::free(p); }
void operator delete(void *p, size_t) noexcept { std::free(p); }
void operator delete[](void *p, size_t) noexcept { std::free(p); }
void operator delete(void *p, const std::nothrow_t&) noexcept { std::free(p); }
void operator delete[](void *p, const std::nothrow_t&) noexcept { std::free(p); }

static int construction_budget(unsigned int width, unsigned int height)
{
  const size_t frame_bytes = (size_t)width * height * sizeof(uint32_t);
  largest_request = 0;
  array_calls = array_bytes = pixel_bytes = 0;
  f0r_instance_t instance = f0r_construct(width, height);
  if (frame_bytes > budget / 2) {
    if (!instance && array_bytes == 0) return 0;
  } else if (instance && array_calls == 2 && array_bytes <= budget &&
             pixel_bytes % frame_bytes == 0) {
    const size_t frames = pixel_bytes / frame_bytes;
    // Account for observed metadata too; another complete frame must not fit.
    if (frames >= 2 && budget - array_bytes < frame_bytes + (array_bytes - pixel_bytes) / frames) {
      f0r_destruct(instance);
      return 0;
    }
  }
  std::fprintf(stderr, "%ux%u: unexpected buffer allocation %zu bytes\n",
               width, height, largest_request);
  if (instance) f0r_destruct(instance);
  return 1;
}

static int construction_failure()
{
  deny_allocations = true;
  f0r_instance_t instance = f0r_construct(8, 8);
  deny_allocations = false;
  if (instance) {
    f0r_destruct(instance);
    std::fprintf(stderr, "Failed instance allocation was accepted\n");
    return 1;
  }
  f0r_instance_t existing = f0r_construct(8, 8);
  if (!existing) return 1;
  // Fail the per-instance parameter-pointer allocation inside the constructor.
  fail_size = sizeof(void*);
  instance = f0r_construct(8, 8);
  fail_size = 0;
  f0r_plugin_info_t info;
  f0r_get_plugin_info(&info);
  parameter(existing, 0, 0.25);
  double value = -1.0;
  f0r_get_param_value(existing, &value, 0);
  f0r_destruct(existing);
  if (instance || info.num_params != 6 || value != 0.25) {
    if (instance) f0r_destruct(instance);
    std::fprintf(stderr, "Constructor failure corrupted existing parameter metadata\n");
    return 1;
  }
  array_calls = array_bytes = pixel_bytes = 0;
  instance = f0r_construct(8, 8);
  if (!instance) return 1;
  const size_t storage_size = pixel_bytes;
  const size_t metadata_size = array_bytes - pixel_bytes;
  f0r_destruct(instance);
  fail_size = storage_size;
  array_calls = array_bytes = pixel_bytes = 0;
  instance = f0r_construct(8, 8);
  fail_size = 0;
  if (instance || array_calls != 1) {
    if (instance) f0r_destruct(instance);
    std::fprintf(stderr, "Failed frame allocation was accepted\n");
    return 1;
  }
  fail_size = metadata_size;
  array_calls = array_bytes = pixel_bytes = 0;
  instance = f0r_construct(8, 8);
  fail_size = 0;
  if (instance || array_calls != 2) {
    if (instance) f0r_destruct(instance);
    std::fprintf(stderr, "Failed history metadata allocation was accepted\n");
    return 1;
  }
  largest_request = 0;
  // Two pixel frames alone fill the budget, leaving no room for metadata.
  instance = f0r_construct(8, (unsigned int)(budget / (2 * 8 * sizeof(uint32_t))));
  if (instance || largest_request > budget) {
    if (instance) f0r_destruct(instance);
    std::fprintf(stderr, "Dimensions exceeding the two-frame budget were accepted\n");
    return 1;
  }
  return 0;
}

static int rendering_without_allocations()
{
  alignas(16) uint32_t input[64], output[64];
  array_calls = array_bytes = pixel_bytes = 0;
  f0r_instance_t instance = f0r_construct(8, 8);
  if (!instance) return 1;
  const size_t capacity = pixel_bytes / sizeof(input);
  parameter(instance, 0, 1.0);
  parameter(instance, 1, 0.5);
  const size_t before = allocation_calls;
  bool failed = false;
  deny_allocations = true;
  try {
    // Retain more than 128 dense frames; the small-budget build also fills
    // the pool and checks eviction at its byte-derived capacity.
    for (unsigned int frame = 0; frame < 3000; ++frame) {
      const unsigned int by_time = frame < 2048 ? 0 : frame - 2048;
      const unsigned int by_capacity = frame < capacity ? 0 : (unsigned int)(frame + 1 - capacity);
      const unsigned int oldest = by_time > by_capacity ? by_time : by_capacity;
      const unsigned int difference = (unsigned int)std::abs((int)(frame % 251) - (int)(oldest % 251));
      for (size_t i = 0; i < 64; ++i) {
        uint8_t *p = reinterpret_cast<uint8_t*>(&input[i]);
        p[0] = (uint8_t)(frame % 251); p[1] = 7; p[2] = 9; p[3] = 200;
      }
      f0r_update(instance, frame / 1024.0, input, output);
      const uint8_t *p = reinterpret_cast<const uint8_t*>(&output[0]);
      if (p[0] != difference || p[1] != 0 || p[2] != 0 || p[3] != 200) failed = true;
    }
    // Parameter changes, seeks, and same-time rendering must also allocate nothing.
    for (unsigned int frame = 0; frame < 200; ++frame) {
      parameter(instance, 0, (frame % 3) * 0.5);
      parameter(instance, 1, (frame % 3) * 0.5);
      f0r_update(instance, (frame % 17) / 30.0, input, input);
    }
  } catch (const std::bad_alloc&) {
    failed = true;
  }
  deny_allocations = false;
  if (allocation_calls != before) failed = true;
  f0r_destruct(instance);
  if (failed) std::fprintf(stderr, "Allocation-free rendering or bounded history failed\n");
  return failed ? 1 : 0;
}

static int working_set(double offset, size_t expected_slots)
{
  captured_storage = 0;
  array_calls = array_bytes = pixel_bytes = 0;
  capture_storage = true;
  f0r_instance_t instance = f0r_construct(8, 8);
  capture_storage = false;
  if (!instance || !captured_storage) {
    if (instance) f0r_destruct(instance);
    return 1;
  }
  alignas(16) uint32_t input[64], output[64];
  std::memset(input, 0x11, sizeof(input));
  parameter(instance, 0, offset);
  // Binary-exact timestamps make the expected history length unambiguous.
  // Repeated rendering must reuse released buffers, leaving unused ones intact.
  for (unsigned int frame = 0; frame < 512; ++frame)
    f0r_update(instance, frame / 32.0, input, output);
  // Replacing the current frame and seeking must release their slots as well.
  for (unsigned int frame = 0; frame < 512; ++frame) {
    const double time = (frame % 16) / 32.0;
    f0r_update(instance, time, input, output);
    f0r_update(instance, time, input, output);
  }
  size_t touched = 0;
  for (size_t slot = 0; slot < captured_bytes / small_frame_bytes; ++slot) {
    for (size_t byte = 0; byte < small_frame_bytes; ++byte) {
      if (captured_storage[slot * small_frame_bytes + byte] != 0xa5) {
        ++touched;
        break;
      }
    }
  }
  f0r_destruct(instance);
  captured_storage = 0;
  if (touched != expected_slots) {
    std::fprintf(stderr, "Offset %g touched %zu buffers, expected %zu\n",
                 offset, touched, expected_slots);
    return 1;
  }
  return 0;
}

int main()
{
  if (!f0r_init()) return 1;
  int errors = construction_budget(8, 8);
  errors += construction_budget(1920, 1080);
  errors += construction_budget(3840, 2160);
  errors += construction_budget(4096, 4096);
  errors += construction_budget(8192, 16383); // Two frames plus metadata fit in 1 GiB.
  errors += construction_failure();
  errors += rendering_without_allocations();
  errors += working_set(0.0, 2);
  errors += working_set(0.0625, 5); // 125 ms at 32 fps, including the current frame.
  f0r_deinit();
  return errors ? 1 : 0;
}
