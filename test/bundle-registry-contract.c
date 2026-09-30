#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <frei0r/bundle.h>

#include "bundle-registry-contract.h"

enum {
  contract_width = 320,
  contract_height = 240,
  contract_pixels = contract_width * contract_height,
  contract_frames = 3
};

static const char *failure_id = "<none>";
static const char *failure_stage = "<none>";
static unsigned int output_digest;
static uint32_t *plugin_digests;
static size_t plugin_digest_count;
static unsigned int application_frame_count;
static unsigned int parameter_change_count;

static uint32_t digest_frame(const uint32_t *frame)
{
  const unsigned char *bytes = (const unsigned char *)frame;
  uint32_t digest = 2166136261u;
  size_t index;

  for (index = 0; index < contract_pixels * sizeof(*frame); ++index)
    digest = (digest ^ bytes[index]) * 16777619u;
  return digest;
}

static uint32_t digest_text(const char *text)
{
  uint32_t digest = 2166136261u;

  while (*text)
    digest = (digest ^ (unsigned char)*text++) * 16777619u;
  return digest;
}

static uint32_t next_noise(uint32_t *state)
{
  uint32_t value = *state;

  value ^= value << 13;
  value ^= value >> 17;
  value ^= value << 5;
  *state = value;
  return value;
}

static void fill_noise(uint32_t *frame, uint32_t seed)
{
  size_t index;
  uint32_t state = seed ? seed : UINT32_C(0x6d2b79f5);

  for (index = 0; index < contract_pixels; ++index)
    frame[index] = UINT32_C(0xff000000) | (next_noise(&state) & UINT32_C(0x00ffffff));
}

static int fail(const char *id, const char *stage)
{
  failure_id = id ? id : "<unknown>";
  failure_stage = stage;
  return 1;
}

static int fail_parameter(const char *id, int index, const char *stage)
{
  (void)index;
  return fail(id, stage);
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

/* Exercise the same normalized parameter domain used by the ordinary module
 * runner.  Position and string parameters retain their valid current values:
 * positions can form coordinated sets, while strings have plugin-specific
 * grammars. */
static int vary_parameter(const f0r_plugin_descriptor_t *descriptor,
                          f0r_instance_t instance,
                          const f0r_param_info_t *parameter, int index,
                          unsigned int frame)
{
  if (parameter->type == F0R_PARAM_BOOL || parameter->type == F0R_PARAM_DOUBLE) {
    double current = NAN;
    double value;

    descriptor->get_param_value(instance, &current, index);
    if (!isfinite(current) || current < 0.0 || current > 1.0)
      return fail_parameter(descriptor->id, index, "application parameter range");
    if (parameter->type == F0R_PARAM_BOOL)
      value = current < 0.5 ? 1.0 : 0.0;
    else {
      value = (double)(((frame + (unsigned int)index) % 3) + 1) / 4.0;
      if (fabs(value - current) < 0.000001)
        value = value < 0.5 ? 0.75 : 0.25;
    }
    descriptor->set_param_value(instance, &value, index);
    ++parameter_change_count;
  } else if (parameter->type == F0R_PARAM_COLOR) {
    f0r_param_color_t value = {
      (double)((frame + 1) % 3) / 2.0,
      (double)((frame + 2) % 3) / 2.0,
      (double)(frame % 3) / 2.0
    };
    descriptor->set_param_value(instance, &value, index);
    ++parameter_change_count;
  } else if (parameter->type == F0R_PARAM_POSITION) {
    f0r_param_position_t value = { NAN, NAN };
    descriptor->get_param_value(instance, &value, index);
    if (!isfinite(value.x) || !isfinite(value.y) || value.x < 0.0 ||
        value.x > 1.0 || value.y < 0.0 || value.y > 1.0)
      return fail_parameter(descriptor->id, index, "application position range");
    descriptor->set_param_value(instance, &value, index);
  } else if (parameter->type == F0R_PARAM_STRING) {
    char *value = NULL;
    descriptor->get_param_value(instance, &value, index);
    if (!value)
      return fail_parameter(descriptor->id, index, "application string range");
    descriptor->set_param_value(instance, &value, index);
  }
  return 0;
}

static int exercise(const f0r_plugin_descriptor_t *descriptor, size_t digest_index)
{
  f0r_plugin_info_t info;
  f0r_instance_t instance = NULL;
  uint32_t *input = malloc(contract_pixels * sizeof(*input));
  uint32_t *input2 = malloc(contract_pixels * sizeof(*input2));
  uint32_t *input3 = malloc(contract_pixels * sizeof(*input3));
  uint32_t *output = calloc(contract_pixels, sizeof(*output));
  uint32_t plugin_digest = 2166136261u;
  uint32_t plugin_seed;
  int initialized = 0;
  int status = 1;
  unsigned int frame;
  int index;

  if (!input || !input2 || !input3 || !output) {
    status = fail(descriptor ? descriptor->id : NULL, "frame allocation");
    goto cleanup;
  }
  if (!descriptor || descriptor->descriptor_size != F0R_PLUGIN_DESCRIPTOR_SIZE ||
      descriptor->descriptor_version != F0R_PLUGIN_DESCRIPTOR_VERSION ||
      !descriptor->id || !descriptor->init || !descriptor->deinit ||
      !descriptor->get_plugin_info || !descriptor->get_param_info ||
      !descriptor->construct || !descriptor->destruct ||
      !descriptor->set_param_value || !descriptor->get_param_value) {
    status = fail(descriptor ? descriptor->id : NULL, "descriptor");
    goto cleanup;
  }
  if (f0r_bundle_plugin_by_id(descriptor->id) != descriptor) {
    status = fail(descriptor->id, "registry lookup");
    goto cleanup;
  }
  descriptor->init();
  initialized = 1;
  memset(&info, 0, sizeof(info));
  descriptor->get_plugin_info(&info);
  if (!info.name || info.num_params < 0) {
    status = fail(descriptor->id, "metadata");
    goto cleanup;
  }
  instance = descriptor->construct(contract_width, contract_height);
  if (!instance) {
    status = fail(descriptor->id, "construct");
    goto cleanup;
  }
  for (index = 0; index < info.num_params; ++index) {
    f0r_param_info_t parameter;
    memset(&parameter, 0, sizeof(parameter));
    descriptor->get_param_info(&parameter, index);
    if (parameter.type < F0R_PARAM_BOOL || parameter.type > F0R_PARAM_STRING) {
      status = fail(descriptor->id, "parameter metadata");
      goto cleanup;
    }
    if (round_trip_parameter(descriptor, instance, &parameter, index)) {
      status = 1;
      goto cleanup;
    }
  }

  plugin_seed = digest_text(descriptor->id);
  for (frame = 0; frame < contract_frames; ++frame) {
    uint32_t input_digest;
    uint32_t input2_digest;
    uint32_t input3_digest;

    fill_noise(input, plugin_seed ^ (UINT32_C(0x9e3779b9) * (frame + 1)));
    fill_noise(input2, plugin_seed ^ (UINT32_C(0x85ebca6b) * (frame + 1)));
    fill_noise(input3, plugin_seed ^ (UINT32_C(0xc2b2ae35) * (frame + 1)));
    input_digest = digest_frame(input);
    input2_digest = digest_frame(input2);
    input3_digest = digest_frame(input3);
    memset(output, 0, contract_pixels * sizeof(*output));

    for (index = 0; index < info.num_params; ++index) {
      f0r_param_info_t parameter;
      memset(&parameter, 0, sizeof(parameter));
      descriptor->get_param_info(&parameter, index);
      if (vary_parameter(descriptor, instance, &parameter, index, frame)) {
        status = 1;
        goto cleanup;
      }
    }

    if (info.plugin_type == F0R_PLUGIN_TYPE_SOURCE && descriptor->update)
      descriptor->update(instance, (double)frame / 30.0, NULL, output);
    else if (info.plugin_type == F0R_PLUGIN_TYPE_FILTER && descriptor->update)
      descriptor->update(instance, (double)frame / 30.0, input, output);
    else if (info.plugin_type == F0R_PLUGIN_TYPE_MIXER2 && descriptor->update2)
      descriptor->update2(instance, (double)frame / 30.0,
                          input, input2, NULL, output);
    else if (info.plugin_type == F0R_PLUGIN_TYPE_MIXER3 && descriptor->update2)
      descriptor->update2(instance, (double)frame / 30.0,
                          input, input2, input3, output);
    else {
      status = fail(descriptor->id, "missing update entry point");
      goto cleanup;
    }

    if ((info.plugin_type == F0R_PLUGIN_TYPE_FILTER ||
         info.plugin_type == F0R_PLUGIN_TYPE_MIXER2 ||
         info.plugin_type == F0R_PLUGIN_TYPE_MIXER3) &&
        (digest_frame(input) != input_digest ||
         (info.plugin_type != F0R_PLUGIN_TYPE_FILTER &&
          digest_frame(input2) != input2_digest) ||
         (info.plugin_type == F0R_PLUGIN_TYPE_MIXER3 &&
          digest_frame(input3) != input3_digest))) {
      status = fail(descriptor->id, "modified application input");
      goto cleanup;
    }
    plugin_digest = (plugin_digest ^ digest_frame(output)) * 16777619u;
    ++application_frame_count;
  }

  plugin_digests[digest_index] = plugin_digest;
  output_digest = (output_digest ^ plugin_digests[digest_index]) * 16777619u;
  status = 0;

cleanup:
  if (instance)
    descriptor->destruct(instance);
  if (initialized)
    descriptor->deinit();
  free(input);
  free(input2);
  free(input3);
  free(output);
  return status;
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

int bundle_registry_contract_run(void)
{
  size_t count = f0r_bundle_plugin_count();
  size_t index;
  failure_id = "<none>";
  failure_stage = "<none>";
  output_digest = 2166136261u;
  application_frame_count = 0;
  parameter_change_count = 0;
  free(plugin_digests);
  plugin_digests = calloc(count, sizeof(*plugin_digests));
  plugin_digest_count = plugin_digests ? count : 0;
  if (!plugin_digests)
    return fail("<registry>", "digest allocation");
  srand(1);
  if (!count || f0r_bundle_plugin_by_index(count) != NULL)
    return fail("<registry>", "index");
  for (index = 0; index < count; ++index) {
    const f0r_plugin_descriptor_t *descriptor = f0r_bundle_plugin_by_index(index);
    if (exercise(descriptor, index)) return 1;
  }
  if (exercise_concurrently(f0r_bundle_plugin_by_index(0),
                            f0r_bundle_plugin_by_index(1)))
    return 1;
  return 0;
}

unsigned int bundle_registry_contract_output_digest(void)
{
  return output_digest;
}

size_t bundle_registry_contract_output_count(void)
{
  return plugin_digest_count;
}

unsigned int bundle_registry_contract_application_frame_count(void)
{
  return application_frame_count;
}

unsigned int bundle_registry_contract_parameter_change_count(void)
{
  return parameter_change_count;
}

uint32_t bundle_registry_contract_output_digest_at(size_t index)
{
  return index < plugin_digest_count ? plugin_digests[index] : 0;
}

const char *bundle_registry_contract_output_id_at(size_t index)
{
  const f0r_plugin_descriptor_t *descriptor;

  if (index >= plugin_digest_count)
    return NULL;
  descriptor = f0r_bundle_plugin_by_index(index);
  return descriptor ? descriptor->id : NULL;
}

const char *bundle_registry_contract_failure_id(void)
{
  return failure_id;
}

const char *bundle_registry_contract_failure_stage(void)
{
  return failure_stage;
}

#ifndef BUNDLE_REGISTRY_CONTRACT_NO_MAIN
int main(void)
{
  int status = bundle_registry_contract_run();
  if (status)
    fprintf(stderr, "registry contract: %s: %s\n", failure_id, failure_stage);
  return status;
}
#endif
