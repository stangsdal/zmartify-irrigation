/**
 * @file version.h
 * @brief Firmware version constants for Zmartify Irrigation Controller v5.0
 */
#pragma once

#define ZIC_FW_VERSION_MAJOR    5
#define ZIC_FW_VERSION_MINOR    0
#define ZIC_FW_VERSION_PATCH    0
#define ZIC_FW_VERSION_STR      "5.0.0"

#define ZIC_FW_BUILD_DATE       __DATE__
#define ZIC_FW_BUILD_TIME       __TIME__

/** Controller model identifier */
#define ZIC_CONTROLLER_MODEL    "ZIC-S3-RevB"

/** MQTT client ID prefix (append unique suffix in production) */
#define ZIC_MQTT_CLIENT_PREFIX  "zmartify_zic_"
