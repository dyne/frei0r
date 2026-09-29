#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <frei0r/bundle.h>

static int fail(const char *id, const char *stage)
{
  fprintf(stderr, "registry contract: %s: %s\n", id ? id : "<unknown>", stage);
  return 1;
}

static int fail_parameter(const char *id, int index, const char *stage)
{
  fprintf(stderr, "registry contract: %s: parameter %d: %s\n", id, index, stage);
  return 1;
}

/* Round-trip each plugin's own valid default.  Some parameters form a
 * coordinated set (Curves' control points are one example), so assigning a
 * synthetic value to every parameter independently is not a valid contract
 * check.  This mirrors the ordinary module runner's default-value checks. */
static int round_trip_parameter(const f0r_plugin_descriptor_t *descriptor,
                                f0r_instance_t instance,
                                const f0r_param_info_t *parameter, int index)
{
  if (parameter->type == F0R_PARAM_BOOL || parameter->type == F0R_PARAM_DOUBLE) {
    double value = NAN;
    descriptor->get_param_value(instance, &value, index);
    if (!isfinite(value) || value < 0.0 || value > 1.0)
      return fail_parameter(descriptor->id, index, "range");
    descriptor->set_param_value(instance, &value, index);
    descriptor->get_param_value(instance, &value, index);
    if (!isfinite(value) || value < 0.0 || value > 1.0)
      return fail_parameter(descriptor->id, index, "round trip");
  } else if (parameter->type == F0R_PARAM_COLOR) {
    f0r_param_color_t value = { NAN, NAN, NAN };
    descriptor->get_param_value(instance, &value, index);
    if (!isfinite(value.r) || !isfinite(value.g) || !isfinite(value.b) ||
        value.r < 0.0 || value.r > 1.0 || value.g < 0.0 || value.g > 1.0 ||
        value.b < 0.0 || value.b > 1.0)
      return fail_parameter(descriptor->id, index, "range");
    descriptor->set_param_value(instance, &value, index);
  } else if (parameter->type == F0R_PARAM_POSITION) {
    f0r_param_position_t value = { NAN, NAN };
    descriptor->get_param_value(instance, &value, index);
    if (!isfinite(value.x) || !isfinite(value.y) || value.x < 0.0 ||
        value.x > 1.0 || value.y < 0.0 || value.y > 1.0)
      return fail_parameter(descriptor->id, index, "range");
    descriptor->set_param_value(instance, &value, index);
  } else if (parameter->type == F0R_PARAM_STRING) {
    char *value = NULL;
    descriptor->get_param_value(instance, &value, index);
    if (!value) return fail_parameter(descriptor->id, index, "range");
    descriptor->set_param_value(instance, &value, index);
  }
  return 0;
}

static int exercise(const f0r_plugin_descriptor_t *descriptor)
{
  f0r_plugin_info_t info;
  f0r_instance_t instance;
  uint32_t input[64] = {0};
  uint32_t output[64] = {0};
  int index;

  fprintf(stderr, "registry contract: %s: begin\n",
          descriptor && descriptor->id ? descriptor->id : "<unknown>");

  if (!descriptor || descriptor->descriptor_size != F0R_PLUGIN_DESCRIPTOR_SIZE ||
      descriptor->descriptor_version != F0R_PLUGIN_DESCRIPTOR_VERSION ||
      !descriptor->id || !descriptor->init || !descriptor->deinit ||
      !descriptor->get_plugin_info || !descriptor->get_param_info ||
      !descriptor->construct || !descriptor->destruct ||
      !descriptor->set_param_value || !descriptor->get_param_value)
    return fail(descriptor ? descriptor->id : NULL, "descriptor");
  if (f0r_bundle_plugin_by_id(descriptor->id) != descriptor)
    return fail(descriptor->id, "registry lookup");
  descriptor->init();
  memset(&info, 0, sizeof(info));
  descriptor->get_plugin_info(&info);
  if (!info.name || info.num_params < 0)
    return fail(descriptor->id, "metadata");
  instance = descriptor->construct(8, 8);
  if (!instance) return fail(descriptor->id, "construct");
  for (index = 0; index < info.num_params; ++index) {
    f0r_param_info_t parameter;
    memset(&parameter, 0, sizeof(parameter));
    descriptor->get_param_info(&parameter, index);
    if (parameter.type < F0R_PARAM_BOOL || parameter.type > F0R_PARAM_STRING) {
      descriptor->destruct(instance); descriptor->deinit();
      return fail(descriptor->id, "parameter metadata");
    }
    if (round_trip_parameter(descriptor, instance, &parameter, index)) {
      descriptor->destruct(instance); descriptor->deinit();
      return 1;
    }
  }
  fprintf(stderr, "registry contract: %s: update\n", descriptor->id);
  if ((info.plugin_type == F0R_PLUGIN_TYPE_FILTER ||
       info.plugin_type == F0R_PLUGIN_TYPE_SOURCE) && descriptor->update)
    descriptor->update(instance, 0.0, input, output);
  else if ((info.plugin_type == F0R_PLUGIN_TYPE_MIXER2 ||
            info.plugin_type == F0R_PLUGIN_TYPE_MIXER3) && descriptor->update2)
    descriptor->update2(instance, 0.0, input, input, input, output);
  else {
    descriptor->destruct(instance); descriptor->deinit();
    return fail(descriptor->id, "missing update entry point");
  }
  descriptor->destruct(instance);
  descriptor->deinit();
  return 0;
}

static int exercise_concurrently(const f0r_plugin_descriptor_t *first,
                                 const f0r_plugin_descriptor_t *second)
{
  uint32_t input[64] = {0};
  uint32_t first_output[64] = {0};
  uint32_t second_output[64] = {0};
  f0r_instance_t first_instance;
  f0r_instance_t second_instance;

  if (!first || !second || first == second || !first->update || !second->update)
    return fail("<registry>", "concurrent descriptors");
  first->init();
  second->init();
  first_instance = first->construct(8, 8);
  second_instance = second->construct(8, 8);
  if (!first_instance || !second_instance) {
    if (first_instance) first->destruct(first_instance);
    if (second_instance) second->destruct(second_instance);
    second->deinit();
    first->deinit();
    return fail("<registry>", "concurrent construct");
  }
  first->update(first_instance, 0.0, input, first_output);
  second->update(second_instance, 0.0, input, second_output);
  first->destruct(first_instance);
  second->destruct(second_instance);
  second->deinit();
  first->deinit();
  return 0;
}

int main(int argc, char **argv)
{
  size_t count = f0r_bundle_plugin_count();
  size_t index;
  if (!count || f0r_bundle_plugin_by_index(count) != NULL) return 1;
  for (index = 0; index < count; ++index) {
    const f0r_plugin_descriptor_t *descriptor = f0r_bundle_plugin_by_index(index);
    if (argc == 2 && strcmp(argv[1], descriptor->id) != 0) continue;
    if (exercise(descriptor)) return 1;
  }
  if (argc == 1 && exercise_concurrently(f0r_bundle_plugin_by_index(0),
                                         f0r_bundle_plugin_by_index(1)))
    return 1;
  return 0;
}
