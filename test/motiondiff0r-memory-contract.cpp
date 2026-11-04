#include <cstdlib>
#include <cstdio>
#include <new>

extern "C" {
#include <frei0r.h>
}

namespace {
const size_t budget = 128u * 1024u * 1024u;
size_t allocation_calls = 0;
size_t largest_request = 0;
size_t fail_size = 0;
bool deny_allocations = false;

void *allocate(size_t size, bool throwing)
{
  ++allocation_calls;
  if (size > largest_request) largest_request = size;
  void *p = (deny_allocations || size == fail_size || size > budget) ? 0 : std::malloc(size ? size : 1);
  if (!p && throwing) throw std::bad_alloc();
  return p;
}

void parameter(f0r_instance_t instance, int index, double value)
{
  f0r_set_param_value(instance, &value, index);
}
} // namespace

void *operator new(size_t size) { return allocate(size, true); }
void *operator new[](size_t size) { return allocate(size, true); }
void *operator new(size_t size, const std::nothrow_t&) noexcept { return allocate(size, false); }
void *operator new[](size_t size, const std::nothrow_t&) noexcept { return allocate(size, false); }
void operator delete(void *p) noexcept { std::free(p); }
void operator delete[](void *p) noexcept { std::free(p); }
void operator delete(void *p, size_t) noexcept { std::free(p); }
void operator delete[](void *p, size_t) noexcept { std::free(p); }
void operator delete(void *p, const std::nothrow_t&) noexcept { std::free(p); }
void operator delete[](void *p, const std::nothrow_t&) noexcept { std::free(p); }

static int construction_budget(unsigned int width, unsigned int height)
{
  const size_t frame_bytes = (size_t)width * height * sizeof(uint32_t);
  const size_t frames = budget / frame_bytes < 128 ? budget / frame_bytes : 128;
  largest_request = 0;
  f0r_instance_t instance = f0r_construct(width, height);
  if (!instance || largest_request != frames * frame_bytes || largest_request > budget) {
    std::fprintf(stderr, "%ux%u: unexpected buffer allocation %zu bytes\n",
                 width, height, largest_request);
    if (instance) f0r_destruct(instance);
    return 1;
  }
  f0r_destruct(instance);
  return 0;
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
  fail_size = 128 * 8 * 8 * sizeof(uint32_t);
  instance = f0r_construct(8, 8);
  fail_size = 0;
  if (instance) {
    f0r_destruct(instance);
    std::fprintf(stderr, "Failed frame allocation was accepted\n");
    return 1;
  }
  largest_request = 0;
  instance = f0r_construct(8192, 8192);
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
  f0r_instance_t instance = f0r_construct(8, 8);
  if (!instance) return 1;
  parameter(instance, 0, 1.0);
  parameter(instance, 1, 0.5);
  const size_t before = allocation_calls;
  bool failed = false;
  deny_allocations = true;
  try {
    // Dense timestamps reach the frame-count limit before the two-second offset.
    for (unsigned int frame = 0; frame < 1000; ++frame) {
      const unsigned int oldest = frame < 128 ? 0 : frame - 127;
      const unsigned int difference = (unsigned int)std::abs((int)(frame % 251) - (int)(oldest % 251));
      for (size_t i = 0; i < 64; ++i) {
        uint8_t *p = reinterpret_cast<uint8_t*>(&input[i]);
        p[0] = (uint8_t)(frame % 251); p[1] = 7; p[2] = 9; p[3] = 200;
      }
      f0r_update(instance, frame / 1000.0, input, output);
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

int main()
{
  if (!f0r_init()) return 1;
  int errors = construction_budget(8, 8);
  errors += construction_budget(1920, 1080);
  errors += construction_budget(3840, 2160);
  errors += construction_budget(4096, 4096);
  errors += construction_failure();
  errors += rendering_without_allocations();
  f0r_deinit();
  return errors ? 1 : 0;
}
