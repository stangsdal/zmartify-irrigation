#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "storage_manager.h"

static uint8_t persisted[4096];
static size_t persisted_length;

static bool load_snapshot(void *context, void *data, size_t *length)
{
    (void)context;
    if (persisted_length == 0 || *length < persisted_length) {
        return false;
    }
    memcpy(data, persisted, persisted_length);
    *length = persisted_length;
    return true;
}

static bool save_snapshot(void *context, const void *data, size_t length)
{
    (void)context;
    assert(length <= sizeof(persisted));
    memcpy(persisted, data, length);
    persisted_length = length;
    return true;
}

static void test_rotation_and_chronological_read(void)
{
    zic_log_entry_t backing[3];
    storage_manager_t manager;
    assert(storage_manager_init(&manager, backing, 3));
    storage_manager_append(&manager, 1, ZIC_LOG_AUDIT, "one");
    storage_manager_append(&manager, 2, ZIC_LOG_AUDIT, "two");
    storage_manager_append(&manager, 3, ZIC_LOG_AUDIT, "three");
    storage_manager_append(&manager, 4, ZIC_LOG_AUDIT, "four");

    zic_log_entry_t entry;
    assert(storage_manager_get(&manager, 0, &entry));
    assert(entry.timestamp == 2);
    assert(storage_manager_get(&manager, 2, &entry));
    assert(entry.timestamp == 4);
}

static void test_persistence_and_crc(void)
{
    zic_log_entry_t source_backing[3];
    zic_log_entry_t restored_backing[3];
    storage_manager_t source;
    storage_manager_t restored;
    persisted_length = 0;
    assert(storage_manager_init(&source, source_backing, 3));
    storage_manager_set_persistence(&source, load_snapshot, save_snapshot, 0);
    storage_manager_append(&source, 10, ZIC_LOG_IRRIGATION, "start");
    storage_manager_append(&source, 20, ZIC_LOG_ALARM, "stop");
    assert(storage_manager_flush(&source, 20));
    assert(!source.dirty);

    assert(storage_manager_init(&restored, restored_backing, 3));
    storage_manager_set_persistence(&restored, load_snapshot, save_snapshot, 0);
    assert(storage_manager_restore(&restored));
    assert(storage_manager_count(&restored) == 2);

    persisted[persisted_length - 1] ^= 1u;
    assert(!storage_manager_restore(&restored));
}

static void test_retention_and_json_escape(void)
{
    zic_log_entry_t backing[4];
    storage_manager_t manager;
    char json[256];
    char csv[256];
    assert(storage_manager_init(&manager, backing, 4));
    storage_manager_append(&manager, 100, ZIC_LOG_AUDIT, "old");
    storage_manager_append(&manager, 200, ZIC_LOG_AUDIT, "quote \" and \\ slash");
    assert(storage_manager_prune_before(&manager, 150) == 1);

    zic_log_entry_t entry;
    assert(storage_manager_get(&manager, 0, &entry));
    assert(storage_manager_export_json(&entry, json, sizeof(json)) > 0);
    assert(strstr(json, "quote \\\" and \\\\ slash") != 0);
    assert(storage_manager_export_csv(&entry, csv, sizeof(csv)) > 0);
    assert(strstr(csv, "\"quote \"\" and \\ slash\"") != 0);
}

int main(void)
{
    test_rotation_and_chronological_read();
    test_persistence_and_crc();
    test_retention_and_json_escape();
    return 0;
}