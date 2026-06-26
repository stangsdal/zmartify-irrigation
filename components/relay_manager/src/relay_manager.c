/*
 * relay_manager.c
 * Relay Manager – safe relay control with HL-58S active-low compensation
 *
 * The HL-58S relay board has active-low control inputs.
 * The hardware chain is: MCP23017 → ULN2803A → HL-58S
 * The ULN2803A is an open-collector (active-low) driver, so:
 *   MCP23017 output HIGH → ULN2803A sinks current → HL-58S input LOW → Relay ON
 *
 * hal_relay_on() already sets MCP23017 output HIGH, which produces relay ON.
 * No additional inversion is needed here; hal_relay is semantically correct.
 *
 * Published events (via event_bus):
 *   EVENT_RELAY_CHANGED  (payload: relay index + state byte)
 */

#include "relay_manager.h"
#include "hal.h"
#include "event_bus.h"
#include "esp_log.h"

static const char *TAG = "relay_mgr";

static bool    s_initialized      = false;
static uint8_t s_max_simultaneous = 1;
static uint16_t s_zone_open_mask  = 0;   /* bit i set → zone relay (i+1) open */
static bool    s_master_open      = false;

/* ─── Helpers ──────────────────────────────────────────────────────── */

static void publish_relay_event(uint8_t relay, bool state)
{
    uint8_t payload[2] = { relay, state ? 1u : 0u };
    uint32_t ev = state ? EVENT_RELAY_ACTIVATED : EVENT_RELAY_DEACTIVATED;
    event_bus_publish(ev, 0, EVENT_PRIORITY_HIGH, 0, payload, sizeof(payload));
}

static uint8_t count_open_zones(void)
{
    uint8_t n = 0;
    uint16_t mask = s_zone_open_mask;
    while (mask)
    {
        n += (uint8_t)(mask & 1u);
        mask >>= 1;
    }
    return n;
}

/* ─── Lifecycle ────────────────────────────────────────────────────── */

relay_result_t relay_manager_init(uint8_t max_simultaneous)
{
    if (s_initialized)
    {
        return RELAY_OK;
    }

    s_max_simultaneous = (max_simultaneous == 0) ? 1u : max_simultaneous;
    s_zone_open_mask   = 0;
    s_master_open      = false;

    /* Guarantee all hardware relays are de-energised */
    hal_result_t r = hal_relay_all_off();
    if (r != HAL_OK)
    {
        ESP_LOGE(TAG, "hal_relay_all_off failed: %d", r);
        return RELAY_HAL_ERROR;
    }

    s_initialized = true;
    ESP_LOGI(TAG, "Relay Manager ready (max simultaneous zones: %u)", s_max_simultaneous);
    return RELAY_OK;
}

/* ─── Master valve ─────────────────────────────────────────────────── */

relay_result_t relay_master_open(void)
{
    if (!s_initialized)
    {
        return RELAY_NOT_INITIALIZED;
    }
    if (s_master_open)
    {
        return RELAY_OK;  /* idempotent */
    }

    hal_result_t r = hal_relay_on(RELAY_MASTER_VALVE);
    if (r != HAL_OK)
    {
        ESP_LOGE(TAG, "Failed to open master valve: %d", r);
        return RELAY_HAL_ERROR;
    }

    s_master_open = true;
    ESP_LOGI(TAG, "Master valve OPEN");
    publish_relay_event(RELAY_MASTER_VALVE, true);
    return RELAY_OK;
}

relay_result_t relay_master_close(void)
{
    if (!s_initialized)
    {
        return RELAY_NOT_INITIALIZED;
    }

    hal_result_t r = hal_relay_off(RELAY_MASTER_VALVE);
    if (r != HAL_OK)
    {
        ESP_LOGE(TAG, "Failed to close master valve: %d", r);
        return RELAY_HAL_ERROR;
    }

    s_master_open = false;
    ESP_LOGI(TAG, "Master valve CLOSED");
    publish_relay_event(RELAY_MASTER_VALVE, false);
    return RELAY_OK;
}

/* ─── Zone valves ──────────────────────────────────────────────────── */

relay_result_t relay_zone_open(uint8_t relay)
{
    if (!s_initialized)
    {
        return RELAY_NOT_INITIALIZED;
    }
    if (relay < RELAY_ZONE_FIRST || relay > RELAY_ZONE_LAST)
    {
        return RELAY_INVALID_INDEX;
    }
    if (!s_master_open)
    {
        ESP_LOGW(TAG, "Relay %u: cannot open zone without master valve", relay);
        return RELAY_MASTER_NOT_OPEN;
    }
    if (count_open_zones() >= s_max_simultaneous)
    {
        ESP_LOGW(TAG, "Relay %u: max simultaneous limit (%u) reached", relay, s_max_simultaneous);
        return RELAY_MAX_CONCURRENT;
    }

    hal_result_t r = hal_relay_on(relay);
    if (r != HAL_OK)
    {
        ESP_LOGE(TAG, "Failed to open zone relay %u: %d", relay, r);
        return RELAY_HAL_ERROR;
    }

    s_zone_open_mask |= (1u << (relay - 1));
    ESP_LOGI(TAG, "Zone relay %u OPEN", relay);
    publish_relay_event(relay, true);
    return RELAY_OK;
}

relay_result_t relay_zone_close(uint8_t relay)
{
    if (!s_initialized)
    {
        return RELAY_NOT_INITIALIZED;
    }
    if (relay < RELAY_ZONE_FIRST || relay > RELAY_ZONE_LAST)
    {
        return RELAY_INVALID_INDEX;
    }

    hal_result_t r = hal_relay_off(relay);
    if (r != HAL_OK)
    {
        ESP_LOGE(TAG, "Failed to close zone relay %u: %d", relay, r);
        return RELAY_HAL_ERROR;
    }

    s_zone_open_mask &= ~(1u << (relay - 1));
    ESP_LOGI(TAG, "Zone relay %u CLOSED", relay);
    publish_relay_event(relay, false);
    return RELAY_OK;
}

relay_result_t relay_close_all(void)
{
    hal_result_t r = hal_relay_all_off();
    s_zone_open_mask = 0;
    s_master_open    = false;

    ESP_LOGW(TAG, "ALL relays closed");
    event_bus_publish(EVENT_SYSTEM_FAULT, 0, EVENT_PRIORITY_CRITICAL, 0, NULL, 0);

    return (r == HAL_OK) ? RELAY_OK : RELAY_HAL_ERROR;
}

/* ─── State queries ────────────────────────────────────────────────── */

bool relay_master_is_open(void)
{
    return s_master_open;
}

bool relay_zone_is_open(uint8_t relay)
{
    if (relay < RELAY_ZONE_FIRST || relay > RELAY_ZONE_LAST)
    {
        return false;
    }
    return (s_zone_open_mask & (1u << (relay - 1))) != 0;
}

uint8_t relay_open_zone_count(void)
{
    return count_open_zones();
}
