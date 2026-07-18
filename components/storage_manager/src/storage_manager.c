#include "storage_manager.h"

#include <stdio.h>
#include <string.h>

bool storage_manager_init(storage_manager_t *manager, zic_log_entry_t *backing, size_t capacity)
{
    if (manager == 0 || backing == 0 || capacity == 0) {
        return false;
    }

    manager->entries = backing;
    manager->capacity = capacity;
    manager->count = 0;
    manager->head = 0;
    return true;
}

void storage_manager_append(storage_manager_t *manager, uint32_t timestamp, zic_log_type_t type, const char *message)
{
    if (manager == 0 || manager->entries == 0 || manager->capacity == 0 || message == 0) {
        return;
    }

    zic_log_entry_t *entry = &manager->entries[manager->head];
    entry->timestamp = timestamp;
    entry->type = type;
    strncpy(entry->message, message, ZIC_LOG_MESSAGE_MAX - 1);
    entry->message[ZIC_LOG_MESSAGE_MAX - 1] = '\0';

    manager->head = (manager->head + 1) % manager->capacity;
    if (manager->count < manager->capacity) {
        manager->count++;
    }
}

size_t storage_manager_count(const storage_manager_t *manager)
{
    if (manager == 0) {
        return 0;
    }

    return manager->count;
}

size_t storage_manager_export_csv(const zic_log_entry_t *entry, char *out, size_t out_len)
{
    if (entry == 0 || out == 0 || out_len == 0) {
        return 0;
    }

    int written = snprintf(out, out_len, "%lu,%d,%s", (unsigned long)entry->timestamp, (int)entry->type, entry->message);
    if (written < 0 || (size_t)written >= out_len) {
        return 0;
    }

    return (size_t)written;
}

size_t storage_manager_export_json(const zic_log_entry_t *entry, char *out, size_t out_len)
{
    if (entry == 0 || out == 0 || out_len == 0) {
        return 0;
    }

    int written = snprintf(out,
                           out_len,
                           "{\"timestamp\":%lu,\"type\":%d,\"message\":\"%s\"}",
                           (unsigned long)entry->timestamp,
                           (int)entry->type,
                           entry->message);
    if (written < 0 || (size_t)written >= out_len) {
        return 0;
    }

    return (size_t)written;
}
