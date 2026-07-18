#pragma once

#include <stdbool.h>

typedef struct {
    const char *firmware_url;
    const char *cert_pem;
} ota_manager_config_t;

bool ota_manager_perform(const ota_manager_config_t *config);
