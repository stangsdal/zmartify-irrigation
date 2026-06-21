#pragma once

#include <stdbool.h>
#include <stdint.h>

bool persistent_store_init(void);
bool persistent_store_set_u32(const char *key, uint32_t value);
bool persistent_store_get_u32(const char *key, uint32_t *value_out, uint32_t default_value);
bool persistent_store_set_i32(const char *key, int32_t value);
bool persistent_store_get_i32(const char *key, int32_t *value_out, int32_t default_value);
