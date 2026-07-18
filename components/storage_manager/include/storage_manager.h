#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ZIC_LOG_RETENTION_TARGET 10000
#define ZIC_LOG_MESSAGE_MAX 96
#define ZIC_LOG_PERSIST_CAPACITY 32
#define ZIC_LOG_RETENTION_SECONDS (30u * 24u * 60u * 60u)

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

typedef bool (*storage_load_fn)(void *context, void *data, size_t *length);
typedef bool (*storage_save_fn)(void *context, const void *data, size_t length);

typedef struct {
    zic_log_entry_t *entries;
    size_t capacity;
    size_t count;
    size_t head;
    bool dirty;
    uint32_t last_flush_timestamp;
    storage_load_fn load;
    storage_save_fn save;
    void *persistence_context;
} storage_manager_t;

bool storage_manager_init(storage_manager_t *manager, zic_log_entry_t *backing, size_t capacity);
void storage_manager_append(storage_manager_t *manager, uint32_t timestamp, zic_log_type_t type, const char *message);
size_t storage_manager_count(const storage_manager_t *manager);
bool storage_manager_get(const storage_manager_t *manager, size_t chronological_index, zic_log_entry_t *out);
void storage_manager_set_persistence(storage_manager_t *manager,
                                     storage_load_fn load,
                                     storage_save_fn save,
                                     void *context);
bool storage_manager_restore(storage_manager_t *manager);
bool storage_manager_flush(storage_manager_t *manager, uint32_t timestamp);
bool storage_manager_flush_due(const storage_manager_t *manager,
                               uint32_t timestamp,
                               uint32_t interval_seconds);
size_t storage_manager_prune_before(storage_manager_t *manager, uint32_t minimum_timestamp);
size_t storage_manager_export_csv(const zic_log_entry_t *entry, char *out, size_t out_len);
size_t storage_manager_export_json(const zic_log_entry_t *entry, char *out, size_t out_len);
