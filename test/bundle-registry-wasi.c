#include "bundle-registry-contract.h"

int f0r_bundle_wasi_run(void)
{
  return bundle_registry_contract_run();
}

unsigned int f0r_bundle_wasi_output_digest(void)
{
  return bundle_registry_contract_output_digest();
}

const char *f0r_bundle_wasi_failure_id(void)
{
  return bundle_registry_contract_failure_id();
}

const char *f0r_bundle_wasi_failure_stage(void)
{
  return bundle_registry_contract_failure_stage();
}
