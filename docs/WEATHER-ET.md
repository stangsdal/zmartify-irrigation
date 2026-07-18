# Weather and ET Runtime Adjustment

Step 7 accepts provider-neutral weather snapshots, caches the latest valid snapshot in NVS,
and applies weather adjustments only to automatically scheduled irrigation.

## Weather ingestion

Send a snapshot to `POST /weather` as JSON:

```json
{
  "timestamp": 1767225600,
  "temperature_c": 25.0,
  "humidity_pct": 55.0,
  "wind_speed_mps": 3.0,
  "solar_radiation_mj_m2": 18.0,
  "rain_mm_last_24h": 0.0,
  "rain_probability_pct": 20.0,
  "uv_index": 5.0
}
```

The timestamp may be omitted after SNTP synchronization. Required percentages must be in
the range 0-100, and rain, wind and radiation values must be non-negative. A successful
request returns HTTP 200 with `{"status":"accepted"}`.

The endpoint is an adapter boundary for NWS, OpenWeatherMap, Home Assistant or another
provider. Provider credentials and polling do not live in the controller firmware.

## Cache and fallback

Snapshots are stored as a versioned, CRC-protected NVS record. A cached snapshot remains
usable for 24 hours, including after reboot or provider failure. Missing, invalid, future or
older weather is never used to skip irrigation. In that state the configured program and
zone seasonal factors still apply, while ET and rain adjustments are omitted.

## Runtime policy

Reference ET uses the daily FAO-56 Penman-Monteith equation with provider values in SI
units. The current implementation assumes standard atmospheric pressure at sea level and
estimates net radiation as `0.77 * solar_radiation_mj_m2`; the provider radiation value must
therefore be a daily total, not an instantaneous W/m2 reading.

For each scheduled zone:

1. Crop demand is `ETo * Kc`, using the zone crop coefficient.
2. Effective rainfall is `rain_mm_last_24h * 0.8`.
3. Effective rainfall is deducted from crop demand.
4. The program and zone seasonal factors are combined.
5. Runtime is clamped to 20-200% of base runtime.
6. Irrigation is skipped when effective rain covers demand or weather policy blocks it.

Manual zone commands are not weather-adjusted. Existing configured zone and global runtime
limits remain the final authority for every start.