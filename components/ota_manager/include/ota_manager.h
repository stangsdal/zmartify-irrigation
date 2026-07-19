#pragma once

#include <stdbool.h>
#include "ota_policy.h"

typedef struct {
    const char *firmware_url;
    const char *cert_pem;
} ota_manager_config_t;

void ota_manager_init(void);
ota_state_t ota_manager_get_state(void);
bool ota_manager_transition(ota_state_t state);
bool ota_manager_perform(const ota_manager_config_t *config);
