#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ZIC_LOG_RETENTION_TARGET 10000
#define ZIC_LOG_MESSAGE_MAX 96

typedef enum {
    ZIC_LOG_IRRIGATION = 0,
    ZIC_LOG_ALARM,
    ZIC_LOG_AUDIT,
    ZIC_LOG_WEATHER
} zic_log_type_t;

typedef struct {
    uint32_t timestamp;
    zic_log_type_t type;
    char message[ZIC_LOG_MESSAGE_MAX];
} zic_log_entry_t;

typedef struct {
    zic_log_entry_t *entries;
    size_t capacity;
    size_t count;
    size_t head;
} storage_manager_t;

bool storage_manager_init(storage_manager_t *manager, zic_log_entry_t *backing, size_t capacity);
void storage_manager_append(storage_manager_t *manager, uint32_t timestamp, zic_log_type_t type, const char *message);
size_t storage_manager_count(const storage_manager_t *manager);
size_t storage_manager_export_csv(const zic_log_entry_t *entry, char *out, size_t out_len);
size_t storage_manager_export_json(const zic_log_entry_t *entry, char *out, size_t out_len);
