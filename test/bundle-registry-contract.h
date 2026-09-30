#ifndef FREI0R_BUNDLE_REGISTRY_CONTRACT_H
#define FREI0R_BUNDLE_REGISTRY_CONTRACT_H

#include <stddef.h>
#include <stdint.h>

int bundle_registry_contract_run(void);
unsigned int bundle_registry_contract_output_digest(void);
size_t bundle_registry_contract_output_count(void);
unsigned int bundle_registry_contract_application_frame_count(void);
unsigned int bundle_registry_contract_parameter_change_count(void);
uint32_t bundle_registry_contract_output_digest_at(size_t index);
const char *bundle_registry_contract_output_id_at(size_t index);
const char *bundle_registry_contract_failure_id(void);
const char *bundle_registry_contract_failure_stage(void);

#endif
