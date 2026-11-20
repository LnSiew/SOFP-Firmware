#pragma once

// ============================================================
// SOFP Firmware configuration
// Board: ESP32-S3 WROOM N16R8
// Camera: OV5640 (DVP / 8-bit parallel assumed)
// IMPORTANT: GPIO values below are PLACEHOLDERS, not verified
// for your PCB. Confirm every pin with the hardware designer
// before connecting power or enabling the camera.
// ============================================================

// ---------------- Wi-Fi ----------------
#define WIFI_SSID       "YOUR_WIFI_NAME"
#define WIFI_PASSWORD   "YOUR_WIFI_PASSWORD"

// ---------------- Device identity ----------------
// Must match the Device ThingsBoardId used by SOFP Website
// and the device_id sent to SOFP Backend.
#define SOFP_DEVICE_ID  "YOUR_DEVICE_ID"

// ---------------- ThingsBoard MQTT ----------------
// For ThingsBoard Cloud, use the host shown in your instance.
// Port 1883 is unencrypted MQTT. Use 8883 + TLS for production.
#define TB_HOST         "YOUR_THINGSBOARD_HOST"
#define TB_PORT         1883
#define TB_DEVICE_TOKEN "YOUR_THINGSBOARD_DEVICE_ACCESS_TOKEN"

// ---------------- SOFP Backend ----------------
// Must be the current Cloudflare HTTPS URL, without trailing slash.
// Quick Tunnel URLs change when the backend restarts.
// Example: "https://example-name.trycloudflare.com"
#define BACKEND_BASE_URL "https://YOUR-CLOUDFLARE-URL"

// ---------------- Camera ----------------
#define CAMERA_ENABLED       1
#define CAMERA_FRAME_INTERVAL_MS 5000UL
#define CAMERA_JPEG_QUALITY  12       // 10-15 is a reasonable starting range
#define CAMERA_FRAME_SIZE    FRAMESIZE_VGA

// OV5640 DVP pin map - PLACEHOLDER VALUES ONLY.
// Replace with the actual PCB wiring.
#define CAM_PIN_XCLK   10
#define CAM_PIN_SIOD   40
#define CAM_PIN_SIOC   39

#define CAM_PIN_D0     15
#define CAM_PIN_D1     17
#define CAM_PIN_D2     18
#define CAM_PIN_D3     16
#define CAM_PIN_D4     14
#define CAM_PIN_D5     12
#define CAM_PIN_D6     11
#define CAM_PIN_D7     48

#define CAM_PIN_VSYNC  38
#define CAM_PIN_HREF   47
#define CAM_PIN_PCLK   13

// Set to -1 if the camera module does not connect these signals.
#define CAM_PIN_PWDN   -1
#define CAM_PIN_RESET  -1

// ---------------- Telemetry intervals ----------------
#define TELEMETRY_INTERVAL_MS 10000UL
#define MQTT_RECONNECT_INTERVAL_MS 5000UL
#define WIFI_RECONNECT_INTERVAL_MS 5000UL

// ---------------- Optional sensors ----------------
// Keep disabled until the actual sensor and wiring are known.
#define BATTERY_SENSOR_ENABLED  0
#define TEMPERATURE_SENSOR_ENABLED 0
#define PEOPLE_SENSOR_ENABLED   0

// Battery ADC placeholder. Requires a suitable voltage divider
// and calibration for the board's battery circuit.
#define BATTERY_ADC_PIN  4

// Temperature I2C placeholder. Set pins and sensor driver later.
#define TEMP_SDA_PIN     8
#define TEMP_SCL_PIN     9

// People sensor placeholder (e.g. digital output sensor).
#define PEOPLE_SENSOR_PIN 5

// Optional status LED. Set to -1 if not connected.
#define STATUS_LED_PIN   2
