/*
 * zone_manager.c
 * Zone Manager v5.0 - per-zone state machine and runtime tracking
 */

#include "zone_manager.h"
#include "relay_manager.h"
#include "config_manager.h"
#include "event_bus.h"
#include "esp_timer.h"
#include "esp_log.h"
#include <string.h>
#include <time.h>

static const char *TAG = "zone_mgr";

#define MAX_ZONES 15

static zone_runtime_t s_zones[MAX_ZONES];

/* ─── Internal helpers ─────────────────────────────────────────────── */

static zone_runtime_t *get_zone(uint8_t zone_id)
{
    if (zone_id < 1 || zone_id > MAX_ZONES)
    {
        return NULL;
    }
    return &s_zones[zone_id - 1];
}

static void publish_zone_event(uint32_t event_id, uint8_t zone_id, zone_state_t state)
{
    uint8_t payload[2] = { zone_id, (uint8_t)state };
    event_bus_publish(event_id, 0, EVENT_PRIORITY_HIGH, 0, payload, sizeof(payload));
}

static uint32_t current_epoch(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000000ULL);
}

/* ─── API ──────────────────────────────────────────────────────────── */

void zone_manager_init(void)
{
    memset(s_zones, 0, sizeof(s_zones));
    for (int i = 0; i < MAX_ZONES; i++)
    {
        s_zones[i].zone_id = (uint8_t)(i + 1);
        s_zones[i].state   = ZONE_STATE_IDLE;
    }
    ESP_LOGI(TAG, "Zone Manager initialized (%d zones)", MAX_ZONES);
}

bool zone_start(uint8_t zone_id, uint32_t runtime_seconds)
{
    zone_runtime_t *z = get_zone(zone_id);
    if (z == NULL)
    {
        return false;
    }
    if (z->state != ZONE_STATE_IDLE)
    {
        ESP_LOGW(TAG, "Zone %u already active (state=%d)", zone_id, z->state);
        return false;
    }

    /* Determine relay index (zone_id == relay_index for zones 1-15) */
    uint8_t relay = zone_id;

    /* Determine runtime – use config default if not specified */
    if (runtime_seconds == 0)
    {
        config_zone_t cfg;
        if (config_get_zone(zone_id - 1, &cfg) == CFG_OK)
        {
            runtime_seconds = cfg.default_runtime_s;
        }
        else
        {
            runtime_seconds = 600;  /* fallback: 10 minutes */
        }
    }

    /* Open zone relay */
    relay_result_t r = relay_zone_open(relay);
    if (r != RELAY_OK)
    {
        ESP_LOGE(TAG, "Zone %u: relay_zone_open failed: %d", zone_id, r);
        z->state = ZONE_STATE_FAULT;
        publish_zone_event(EVENT_ZONE_FAULT, zone_id, ZONE_STATE_FAULT);
        return false;
    }

    z->state              = ZONE_STATE_RUNNING;
    z->requested_runtime_s = runtime_seconds;
    z->elapsed_s          = 0;
    z->start_epoch        = current_epoch();
    z->run_count++;

    ESP_LOGI(TAG, "Zone %u started (relay=%u, runtime=%lus)",
             zone_id, relay, (unsigned long)runtime_seconds);
    publish_zone_event(EVENT_ZONE_STARTED, zone_id, ZONE_STATE_RUNNING);
    return true;
}

bool zone_stop(uint8_t zone_id)
{
    zone_runtime_t *z = get_zone(zone_id);
    if (z == NULL)
    {
        return false;
    }
    if (z->state == ZONE_STATE_IDLE)
    {
        return true;  /* already stopped */
    }

    uint8_t relay = zone_id;
    relay_zone_close(relay);

    z->total_runtime_s += z->elapsed_s;
    z->last_run_epoch   = current_epoch();
    z->elapsed_s        = 0;
    z->state            = ZONE_STATE_IDLE;

    ESP_LOGI(TAG, "Zone %u stopped (total runtime: %lus)", zone_id,
             (unsigned long)z->total_runtime_s);
    publish_zone_event(EVENT_ZONE_STOPPED, zone_id, ZONE_STATE_IDLE);
    return true;
}

void zone_stop_all(void)
{
    for (int i = 1; i <= MAX_ZONES; i++)
    {
        zone_runtime_t *z = get_zone(i);
        if (z && z->state != ZONE_STATE_IDLE)
        {
            zone_stop((uint8_t)i);
        }
    }
}

void zone_manager_tick(void)
{
    for (int i = 0; i < MAX_ZONES; i++)
    {
        zone_runtime_t *z = &s_zones[i];
        if (z->state != ZONE_STATE_RUNNING)
        {
            continue;
        }

        z->elapsed_s++;

        if (z->elapsed_s >= z->requested_runtime_s)
        {
            ESP_LOGI(TAG, "Zone %u runtime complete (%lu s)", z->zone_id,
                     (unsigned long)z->elapsed_s);
            zone_stop(z->zone_id);
        }
    }
}

bool zone_get_runtime(uint8_t zone_id, zone_runtime_t *out)
{
    zone_runtime_t *z = get_zone(zone_id);
    if (z == NULL || out == NULL)
    {
        return false;
    }
    *out = *z;
    return true;
}

bool zone_any_active(void)
{
    for (int i = 0; i < MAX_ZONES; i++)
    {
        if (s_zones[i].state == ZONE_STATE_RUNNING ||
            s_zones[i].state == ZONE_STATE_OPENING)
        {
            return true;
        }
    }
    return false;
}

uint8_t zone_active_count(void)
{
    uint8_t n = 0;
    for (int i = 0; i < MAX_ZONES; i++)
    {
        if (s_zones[i].state == ZONE_STATE_RUNNING ||
            s_zones[i].state == ZONE_STATE_OPENING)
        {
            n++;
        }
    }
    return n;
}
