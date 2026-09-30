#include "bundle-registry-contract.h"

int f0r_bundle_emscripten_run(void)
{
  return bundle_registry_contract_run();
}

unsigned int f0r_bundle_emscripten_output_digest(void)
{
  return bundle_registry_contract_output_digest();
}

unsigned int f0r_bundle_emscripten_application_frame_count(void)
{
  return bundle_registry_contract_application_frame_count();
}

unsigned int f0r_bundle_emscripten_parameter_change_count(void)
{
  return bundle_registry_contract_parameter_change_count();
}

size_t f0r_bundle_emscripten_output_count(void)
{
  return bundle_registry_contract_output_count();
}

unsigned int f0r_bundle_emscripten_output_digest_at(size_t index)
{
  return bundle_registry_contract_output_digest_at(index);
}

const char *f0r_bundle_emscripten_output_id_at(size_t index)
{
  return bundle_registry_contract_output_id_at(index);
}

const char *f0r_bundle_emscripten_failure_id(void)
{
  return bundle_registry_contract_failure_id();
}

const char *f0r_bundle_emscripten_failure_stage(void)
{
  return bundle_registry_contract_failure_stage();
}
