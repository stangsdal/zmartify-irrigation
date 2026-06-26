/*
 * irrigation_engine.c
 * Irrigation Engine v5.0 – state machine, safety checks, zone sequencing
 */

#include "irrigation_engine.h"
#include "zone_manager.h"
#include "relay_manager.h"
#include "config_manager.h"
#include "event_bus.h"
#include "weather_manager.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <string.h>

static const char *TAG = "irr_engine";

/* Forward declarations */
static void execute_emergency(const char *reason);

/* ─── Engine state ──────────────────────────────────────────────────────── */

typedef enum
{
    CMD_START_ZONE = 0,
    CMD_STOP_ZONE,
    CMD_STOP_ALL,
    CMD_EMERGENCY_STOP,
} engine_cmd_type_t;

typedef struct
{
    engine_cmd_type_t type;
    irrigation_request_t req;
    char reason[32];
} engine_cmd_t;

#define ENGINE_QUEUE_DEPTH     8
#define MASTER_VALVE_DELAY_MS  2000u
#define ENGINE_TICK_MS         1000u

static QueueHandle_t  s_cmd_queue   = NULL;
static TaskHandle_t   s_task_handle = NULL;
static engine_state_t s_state       = ENGINE_STATE_IDLE;
static uint8_t        s_active_zone = 0;
static bool           s_initialized = false;

/* ─── Hydraulic fault callback ──────────────────────────────────────── */

static void on_hydraulic_fault(const zic_event_t *event, void *ctx)
{
    (void)ctx;
    if (s_state != ENGINE_STATE_ZONE_RUNNING &&
        s_state != ENGINE_STATE_MASTER_OPEN)
    {
        return;
    }
    const char *reason = (event->event_id == EVENT_FLOW_ANOMALY)
                       ? "Flow anomaly"
                       : "Pressure out of bounds";
    ESP_LOGW(TAG, "Hydraulic fault: %s", reason);
    execute_emergency(reason);
}

static void set_state(engine_state_t ns)
{
    if (s_state == ns) return;
    ESP_LOGI(TAG, "State %d -> %d", (int)s_state, (int)ns);
    s_state = ns;
    uint8_t p = (uint8_t)ns;
    uint32_t ev = (ns == ENGINE_STATE_FAULT || ns == ENGINE_STATE_EMERGENCY)
                ? EVENT_IRRIGATION_FAULT
                : (ns == ENGINE_STATE_IDLE ? EVENT_IRRIGATION_COMPLETED
                                           : EVENT_IRRIGATION_STARTED);
    event_bus_publish(ev, 0, EVENT_PRIORITY_HIGH, 0, &p, 1);
}

static bool pre_flight_checks(uint8_t zone_id)
{
    if (s_state == ENGINE_STATE_EMERGENCY) return false;

    config_zone_t zcfg;
    if (config_get_zone(zone_id - 1, &zcfg) != CFG_OK) return false;
    if (!zcfg.enabled)
    {
        ESP_LOGW(TAG, "Zone %u disabled", zone_id);
        return false;
    }

    config_system_t sys;
    if (config_get_system(&sys) == CFG_OK && sys.operational_mode == CONFIG_MODE_OFF)
    {
        ESP_LOGW(TAG, "Controller in OFF mode");
        return false;
    }

    /* 3. Weather / rain delay check */
    if (!weather_irrigation_allowed())
    {
        ESP_LOGW(TAG, "Pre-flight: weather blocks irrigation for zone %u", zone_id);
        return false;
    }

    return true;
}

static void execute_start_zone(const irrigation_request_t *req)
{
    uint8_t zid = req->zone_id;
    if (!pre_flight_checks(zid)) return;

    if (s_active_zone != 0)
    {
        zone_stop(s_active_zone);
        s_active_zone = 0;
    }

    set_state(ENGINE_STATE_PREPARING);

    if (relay_master_open() != RELAY_OK)
    {
        ESP_LOGE(TAG, "Master valve open failed");
        set_state(ENGINE_STATE_FAULT);
        return;
    }

    set_state(ENGINE_STATE_MASTER_OPEN);
    vTaskDelay(pdMS_TO_TICKS(MASTER_VALVE_DELAY_MS));

    if (!zone_start(zid, req->runtime_s))
    {
        relay_master_close();
        set_state(ENGINE_STATE_FAULT);
        return;
    }

    s_active_zone = zid;
    set_state(ENGINE_STATE_ZONE_RUNNING);
}

static void execute_stop_all(void)
{
    zone_stop_all();
    relay_master_close();
    s_active_zone = 0;
    set_state(ENGINE_STATE_IDLE);
}

static void execute_emergency(const char *reason)
{
    ESP_LOGE(TAG, "EMERGENCY: %s", reason ? reason : "");
    relay_close_all();
    zone_stop_all();
    s_active_zone = 0;
    set_state(ENGINE_STATE_EMERGENCY);
}

static void engine_task(void *arg)
{
    (void)arg;
    engine_cmd_t cmd;
    ESP_LOGI(TAG, "Engine task running");

    for (;;)
    {
        if (xQueueReceive(s_cmd_queue, &cmd, pdMS_TO_TICKS(ENGINE_TICK_MS)) == pdTRUE)
        {
            switch (cmd.type)
            {
                case CMD_START_ZONE:     execute_start_zone(&cmd.req); break;
                case CMD_STOP_ZONE:
                    if (cmd.req.zone_id == s_active_zone || cmd.req.zone_id == 0)
                        execute_stop_all();
                    else
                        zone_stop(cmd.req.zone_id);
                    break;
                case CMD_STOP_ALL:       execute_stop_all(); break;
                case CMD_EMERGENCY_STOP: execute_emergency(cmd.reason); break;
            }
        }

        zone_manager_tick();

        if (s_state == ENGINE_STATE_ZONE_RUNNING && s_active_zone != 0)
        {
            zone_runtime_t rt;
            if (zone_get_runtime(s_active_zone, &rt) && rt.state == ZONE_STATE_IDLE)
            {
                ESP_LOGI(TAG, "Zone %u completed naturally", s_active_zone);
                s_active_zone = 0;
                relay_master_close();
                set_state(ENGINE_STATE_IDLE);
            }
        }
    }
}

bool irrigation_engine_init(void)
{
    if (s_initialized) return true;

    s_cmd_queue = xQueueCreate(ENGINE_QUEUE_DEPTH, sizeof(engine_cmd_t));
    if (!s_cmd_queue)
    {
        ESP_LOGE(TAG, "Queue create failed");
        return false;
    }

    if (xTaskCreate(engine_task, "irr_engine", 4096, NULL, 8, &s_task_handle) != pdPASS)
    {
        vQueueDelete(s_cmd_queue);
        s_cmd_queue = NULL;
        ESP_LOGE(TAG, "Task create failed");
        return false;
    }

    s_state       = ENGINE_STATE_IDLE;
    s_active_zone = 0;
    s_initialized = true;

    /* Subscribe to hydraulic fault events from flow/pressure managers */
    event_bus_subscribe(EVENT_FLOW_ANOMALY,             on_hydraulic_fault, NULL);
    event_bus_subscribe(EVENT_PRESSURE_OUT_OF_BOUNDS,   on_hydraulic_fault, NULL);

    ESP_LOGI(TAG, "Irrigation Engine initialized");
    return true;
}

bool irrigation_start_zone(const irrigation_request_t *req)
{
    if (!s_initialized || !req || req->zone_id < 1 || req->zone_id > 15) return false;
    engine_cmd_t cmd = { .type = CMD_START_ZONE, .req = *req };
    return xQueueSend(s_cmd_queue, &cmd, 0) == pdTRUE;
}

bool irrigation_stop_zone(uint8_t zone_id)
{
    if (!s_initialized) return false;
    engine_cmd_t cmd = { .type = CMD_STOP_ZONE, .req = { .zone_id = zone_id } };
    return xQueueSend(s_cmd_queue, &cmd, 0) == pdTRUE;
}

bool irrigation_stop_all(void)
{
    if (!s_initialized) return false;
    engine_cmd_t cmd = { .type = CMD_STOP_ALL };
    return xQueueSend(s_cmd_queue, &cmd, 0) == pdTRUE;
}

void irrigation_emergency_stop(const char *reason)
{
    execute_emergency(reason);
}

engine_state_t irrigation_get_state(void)  { return s_state; }
uint8_t        irrigation_active_zone(void) { return s_active_zone; }
bool           irrigation_is_idle(void)     { return s_state == ENGINE_STATE_IDLE; }
