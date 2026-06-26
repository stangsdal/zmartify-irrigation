/**
 * @file storage_manager.h
 * @brief Storage Manager v5 - event logger with circular RAM buffer
 *
 * Singleton. Automatically logs system events by subscribing to event bus.
 * Provides 512-entry circular RAM buffer with NVS flush capability.
 * Architecture ref: MEP v5.0 Volume 2, Chapter 15
 */
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define LOG_BUFFER_SIZE    512
#define LOG_MSG_MAX        96

typedef enum {
    LOG_CAT_SYSTEM     = 0,
    LOG_CAT_IRRIGATION = 1,
    LOG_CAT_ALARM      = 2,
    LOG_CAT_WEATHER    = 3,
    LOG_CAT_NETWORK    = 4,
    LOG_CAT_HYDRAULIC  = 5,
} log_category_t;

typedef struct {
    uint32_t       timestamp_epoch;
    log_category_t category;
    uint8_t        severity;     /* 0=info 1=warning 2=critical */
    uint8_t        zone_id;
    uint32_t       value;
    char           message[LOG_MSG_MAX];
} log_entry_t;

bool    storage_manager_init(void);
void    storage_manager_log(log_category_t cat, uint8_t severity, uint8_t zone_id,
                            uint32_t value, const char *message);
size_t  storage_manager_count(void);
bool    storage_manager_get(size_t index, log_entry_t *out);
size_t  storage_manager_export_json(char *buf, size_t buf_len, size_t max_entries);
void    storage_manager_clear(void);

/* Legacy compat shim */
typedef struct {
    log_entry_t *entries;
    size_t capacity;
    size_t count;
    size_t head;
} storage_manager_t;
static inline bool storage_manager_init_legacy(storage_manager_t *m,
    log_entry_t *backing, size_t cap) { (void)m; (void)backing; (void)cap; return storage_manager_init(); }
static inline void storage_manager_append(storage_manager_t *m, uint32_t ts,
    int type, const char *msg) { (void)m; storage_manager_log((log_category_t)type, 0, 0, ts, msg); }
static inline size_t storage_manager_count_legacy(const storage_manager_t *m)
{ (void)m; return storage_manager_count(); }
