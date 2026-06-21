#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "zic_types.h"

typedef struct {
    zic_zone_t zones[ZIC_MAX_ZONES];
} zone_manager_t;

void zone_manager_init(zone_manager_t *manager);
bool zone_manager_start(zone_manager_t *manager, uint8_t zone_id);
bool zone_manager_stop(zone_manager_t *manager, uint8_t zone_id);
bool zone_manager_stop_all(zone_manager_t *manager);
const zic_zone_t *zone_manager_get(const zone_manager_t *manager, uint8_t zone_id);
