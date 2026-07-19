#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "config_types.h"

bool config_validate_hydraulics(const config_hydraulic_t *config);
bool config_validate_alarms(const config_alarms_t *config);
bool config_validate_zone_safety(const config_zone_t *zone,
                                 const config_system_t *system);
bool config_validate_safety(const zic_config_t *config);
bool config_migrate_v1(zic_config_t *config);
bool config_migrate_v2(zic_config_t *config);
bool config_migrate_to_current(zic_config_t *config);
bool config_apply_zone_commissioning(config_zone_t *zone,
                                     uint16_t observed_flow_lpm_x10,
                                     uint16_t observed_pressure_mbar);