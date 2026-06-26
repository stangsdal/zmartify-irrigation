/*
 * et_engine.c
 * ET Engine v5.0 – Hargreaves simplified FAO-56 evapotranspiration
 *
 * Formula (Hargreaves simplified):
 *   ETo (mm/day) = 0.0023 × (Tmean + 17.8) × (Rs + 1)
 *   where Rs = solar radiation in MJ/m²/day
 *
 * This simplified model is appropriate for Version 5.0 and calibrated
 * for irrigated grass. Full Penman-Monteith is a future enhancement.
 *
 * Effective rainfall = observed_mm × 0.75 (infiltration factor)
 * Net requirement    = max(0, ETo - effective_rain)
 *
 * Runtime adjustment:
 *   adjusted = base × (net_mm / reference_mm) × Kc × (seasonal_pct / 100)
 *   Clamped to [20%, 200%] of base.
 */

#include "et_engine.h"
#include "config_manager.h"
#include "event_bus.h"
#include "esp_log.h"
#include <string.h>
#include <math.h>

static const char *TAG = "et_engine";

#define RAIN_INFILTRATION_FACTOR  0.75f
#define REFERENCE_ET_MM_DAY       5.0f   /**< Typical warm-season ETo baseline */
#define RUNTIME_MIN_FACTOR        0.20f  /**< Floor: 20% of base */
#define RUNTIME_MAX_FACTOR        2.00f  /**< Ceiling: 200% of base */
#define ROLLING_WINDOW_DAYS       7

static et_output_t  s_last;
static float        s_daily_history[ROLLING_WINDOW_DAYS];
static uint8_t      s_history_idx  = 0;
static bool         s_initialized  = false;

bool et_engine_init(void)
{
    if (s_initialized)
    {
        return true;
    }
    memset(&s_last, 0, sizeof(s_last));
    memset(s_daily_history, 0, sizeof(s_daily_history));
    s_initialized = true;
    ESP_LOGI(TAG, "ET Engine initialized");
    return true;
}

void et_engine_compute(const et_input_t *input, et_output_t *output)
{
    if (!input || !output)
    {
        return;
    }

    /* Hargreaves simplified ETo */
    float t_mean = input->temperature_c;
    float rs     = (input->solar_radiation_mj_m2 > 0.0f)
                 ? input->solar_radiation_mj_m2
                 : 15.0f;  /* Default mid-latitude summer */

    float eto = 0.0023f * (t_mean + 17.8f) * (rs + 1.0f);
    if (eto < 0.0f) eto = 0.0f;

    float effective_rain = input->rain_mm * RAIN_INFILTRATION_FACTOR;
    float net_req = eto - effective_rain;
    if (net_req < 0.0f) net_req = 0.0f;

    /* Update rolling 7-day history */
    s_daily_history[s_history_idx % ROLLING_WINDOW_DAYS] = eto;
    s_history_idx++;

    float weekly_sum = 0.0f;
    uint8_t cnt = (s_history_idx < ROLLING_WINDOW_DAYS) ? s_history_idx : ROLLING_WINDOW_DAYS;
    for (int i = 0; i < cnt; i++)
    {
        weekly_sum += s_daily_history[i];
    }

    output->daily_et_mm        = eto;
    output->weekly_et_mm       = weekly_sum;
    output->effective_rain_mm  = effective_rain;
    output->net_requirement_mm = net_req;
    output->confidence         = (input->solar_radiation_mj_m2 > 0.0f) ? 90u : 60u;

    s_last = *output;

    ESP_LOGI(TAG, "ETo=%.2f mm/day  net_req=%.2f mm  weekly=%.1f mm",
             eto, net_req, weekly_sum);

    /* Publish daily ET event */
    uint8_t payload[4];
    uint32_t eto_x100 = (uint32_t)(eto * 100.0f);
    payload[0] = (uint8_t)(eto_x100 >> 24);
    payload[1] = (uint8_t)(eto_x100 >> 16);
    payload[2] = (uint8_t)(eto_x100 >>  8);
    payload[3] = (uint8_t)(eto_x100);
    event_bus_publish(EVENT_ET_CALCULATED, 0, EVENT_PRIORITY_NORMAL,
                      0, payload, sizeof(payload));
}

uint32_t et_engine_adjust_runtime(uint32_t base_runtime_s,
                                   float    et_mm_day,
                                   float    reference_mm_day,
                                   float    kc,
                                   uint8_t  seasonal_pct)
{
    if (base_runtime_s == 0)
    {
        return 0;
    }

    float ref = (reference_mm_day > 0.0f) ? reference_mm_day : REFERENCE_ET_MM_DAY;
    float et_factor = (et_mm_day > 0.0f) ? (et_mm_day / ref) : 1.0f;
    float kc_factor = (kc > 0.0f) ? kc : 1.0f;
    float seasonal  = (float)seasonal_pct / 100.0f;

    float multiplier = et_factor * kc_factor * seasonal;

    /* Clamp to valid range */
    if (multiplier < RUNTIME_MIN_FACTOR) multiplier = RUNTIME_MIN_FACTOR;
    if (multiplier > RUNTIME_MAX_FACTOR) multiplier = RUNTIME_MAX_FACTOR;

    uint32_t adjusted = (uint32_t)((float)base_runtime_s * multiplier);
    ESP_LOGD(TAG, "Runtime adjust: %lu s × %.2f = %lu s",
             (unsigned long)base_runtime_s, multiplier, (unsigned long)adjusted);
    return adjusted;
}

bool et_engine_get_last(et_output_t *out)
{
    if (!out || !s_initialized)
    {
        return false;
    }
    *out = s_last;
    return true;
}
