#include "storage_manager.h"

#include <stdio.h>
#include <string.h>

#define STORAGE_MAGIC 0x5A4C4F47u
#define STORAGE_VERSION 1u

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t count;
    uint32_t crc32;
    zic_log_entry_t entries[ZIC_LOG_PERSIST_CAPACITY];
} storage_snapshot_t;

static storage_snapshot_t s_snapshot;
static zic_log_entry_t s_retained[ZIC_LOG_PERSIST_CAPACITY];

static uint32_t crc32_compute(const void *data, size_t length)
{
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t index = 0; index < length; ++index) {
        crc ^= bytes[index];
        for (uint8_t bit = 0; bit < 8; ++bit) {
            crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
        }
    }
    return ~crc;
}

bool storage_manager_init(storage_manager_t *manager, zic_log_entry_t *backing, size_t capacity)
{
    if (manager == 0 || backing == 0 || capacity == 0) {
        return false;
    }

    manager->entries = backing;
    manager->capacity = capacity;
    manager->count = 0;
    manager->head = 0;
    manager->dirty = false;
    manager->last_flush_timestamp = 0;
    manager->load = 0;
    manager->save = 0;
    manager->persistence_context = 0;
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
    manager->dirty = true;
}

size_t storage_manager_count(const storage_manager_t *manager)
{
    if (manager == 0) {
        return 0;
    }

    return manager->count;
}

bool storage_manager_get(const storage_manager_t *manager,
                         size_t chronological_index,
                         zic_log_entry_t *out)
{
    if (manager == 0 || out == 0 || chronological_index >= manager->count) {
        return false;
    }
    size_t oldest = (manager->head + manager->capacity - manager->count) % manager->capacity;
    *out = manager->entries[(oldest + chronological_index) % manager->capacity];
    return true;
}

void storage_manager_set_persistence(storage_manager_t *manager,
                                     storage_load_fn load,
                                     storage_save_fn save,
                                     void *context)
{
    if (manager == 0) {
        return;
    }
    manager->load = load;
    manager->save = save;
    manager->persistence_context = context;
}

bool storage_manager_restore(storage_manager_t *manager)
{
    if (manager == 0 || manager->load == 0) {
        return false;
    }

    memset(&s_snapshot, 0, sizeof(s_snapshot));
    size_t length = sizeof(s_snapshot);
    if (!manager->load(manager->persistence_context, &s_snapshot, &length) ||
        length != sizeof(s_snapshot) || s_snapshot.magic != STORAGE_MAGIC ||
        s_snapshot.version != STORAGE_VERSION || s_snapshot.count > ZIC_LOG_PERSIST_CAPACITY) {
        return false;
    }

    uint32_t stored_crc = s_snapshot.crc32;
    s_snapshot.crc32 = 0;
    if (crc32_compute(&s_snapshot, sizeof(s_snapshot)) != stored_crc) {
        return false;
    }

    manager->count = 0;
    manager->head = 0;
    size_t first = s_snapshot.count > manager->capacity ? s_snapshot.count - manager->capacity : 0;
    for (size_t index = first; index < s_snapshot.count; ++index) {
        storage_manager_append(manager, s_snapshot.entries[index].timestamp,
                               s_snapshot.entries[index].type,
                               s_snapshot.entries[index].message);
    }
    manager->dirty = false;
    return true;
}

bool storage_manager_flush(storage_manager_t *manager, uint32_t timestamp)
{
    if (manager == 0 || manager->save == 0) {
        return false;
    }
    if (!manager->dirty) {
        manager->last_flush_timestamp = timestamp;
        return true;
    }

    memset(&s_snapshot, 0, sizeof(s_snapshot));
    s_snapshot.magic = STORAGE_MAGIC;
    s_snapshot.version = STORAGE_VERSION;
    size_t first = manager->count > ZIC_LOG_PERSIST_CAPACITY
        ? manager->count - ZIC_LOG_PERSIST_CAPACITY
        : 0;
    for (size_t index = first; index < manager->count; ++index) {
        (void)storage_manager_get(manager, index, &s_snapshot.entries[s_snapshot.count++]);
    }
    s_snapshot.crc32 = 0;
    s_snapshot.crc32 = crc32_compute(&s_snapshot, sizeof(s_snapshot));
    if (!manager->save(manager->persistence_context, &s_snapshot, sizeof(s_snapshot))) {
        return false;
    }
    manager->dirty = false;
    manager->last_flush_timestamp = timestamp;
    return true;
}

bool storage_manager_flush_due(const storage_manager_t *manager,
                               uint32_t timestamp,
                               uint32_t interval_seconds)
{
    return manager != 0 && manager->dirty &&
        (manager->last_flush_timestamp == 0 ||
         timestamp - manager->last_flush_timestamp >= interval_seconds);
}

size_t storage_manager_prune_before(storage_manager_t *manager, uint32_t minimum_timestamp)
{
    if (manager == 0 || manager->count == 0) {
        return 0;
    }
    size_t retained_count = 0;
    size_t removed = 0;
    for (size_t index = 0; index < manager->count; ++index) {
        zic_log_entry_t entry;
        (void)storage_manager_get(manager, index, &entry);
        if (entry.timestamp != 0 && entry.timestamp < minimum_timestamp) {
            ++removed;
        } else if (retained_count < ZIC_LOG_PERSIST_CAPACITY) {
            s_retained[retained_count++] = entry;
        }
    }
    if (removed == 0) {
        return 0;
    }
    manager->count = 0;
    manager->head = 0;
    for (size_t index = 0; index < retained_count; ++index) {
        storage_manager_append(manager, s_retained[index].timestamp,
                       s_retained[index].type, s_retained[index].message);
    }
    manager->dirty = true;
    return removed;
}

static bool append_json_escaped(char *out, size_t out_len, size_t *offset, const char *text)
{
    while (*text != '\0') {
        const char *escape = 0;
        char escaped_control[7];
        switch (*text) {
        case '\\': escape = "\\\\"; break;
        case '"': escape = "\\\""; break;
        case '\n': escape = "\\n"; break;
        case '\r': escape = "\\r"; break;
        case '\t': escape = "\\t"; break;
        default:
            if ((unsigned char)*text < 0x20u) {
                (void)snprintf(escaped_control, sizeof(escaped_control), "\\u%04x",
                               (unsigned char)*text);
                escape = escaped_control;
            }
            break;
        }
        const char *source = escape != 0 ? escape : text;
        size_t count = escape != 0 ? strlen(escape) : 1;
        if (*offset + count >= out_len) {
            return false;
        }
        memcpy(out + *offset, source, count);
        *offset += count;
        ++text;
    }
    out[*offset] = '\0';
    return true;
}

size_t storage_manager_export_csv(const zic_log_entry_t *entry, char *out, size_t out_len)
{
    if (entry == 0 || out == 0 || out_len == 0) {
        return 0;
    }

    int written = snprintf(out, out_len, "%lu,%d,\"", (unsigned long)entry->timestamp,
                           (int)entry->type);
    if (written < 0 || (size_t)written >= out_len) {
        return 0;
    }
    size_t offset = (size_t)written;
    for (const char *cursor = entry->message; *cursor != '\0'; ++cursor) {
        size_t required = *cursor == '"' ? 2 : 1;
        if (offset + required + 1 >= out_len) {
            return 0;
        }
        out[offset++] = *cursor;
        if (*cursor == '"') {
            out[offset++] = '"';
        }
    }
    out[offset++] = '"';
    out[offset] = '\0';
    return offset;
}

size_t storage_manager_export_json(const zic_log_entry_t *entry, char *out, size_t out_len)
{
    if (entry == 0 || out == 0 || out_len == 0) {
        return 0;
    }

    int written = snprintf(out, out_len, "{\"timestamp\":%lu,\"type\":%d,\"message\":\"",
                           (unsigned long)entry->timestamp, (int)entry->type);
    if (written < 0 || (size_t)written >= out_len) {
        return 0;
    }
    size_t offset = (size_t)written;
    if (!append_json_escaped(out, out_len, &offset, entry->message) || offset + 2 >= out_len) {
        return 0;
    }
    out[offset++] = '"';
    out[offset++] = '}';
    out[offset] = '\0';
    return offset;
}
