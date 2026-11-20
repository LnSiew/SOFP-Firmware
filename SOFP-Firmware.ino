#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include <esp_camera.h>
#include "config.h"

// ---------------- Network clients ----------------
WiFiClient mqttNetwork;
PubSubClient mqtt(mqttNetwork);

unsigned long lastTelemetryMs = 0;
unsigned long lastCameraMs = 0;
unsigned long lastWifiAttemptMs = 0;
unsigned long lastMqttAttemptMs = 0;

bool cameraReady = false;

// ---------------- Helpers ----------------
bool isConfigured(const char* value) {
  return value && value[0] != '\0' &&
         strstr(value, "YOUR_") == nullptr;
}

void setStatusLed(bool on) {
#if STATUS_LED_PIN >= 0
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, on ? HIGH : LOW);
#endif
}

void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  unsigned long now = millis();
  if (now - lastWifiAttemptMs < WIFI_RECONNECT_INTERVAL_MS) return;
  lastWifiAttemptMs = now;

  Serial.printf("[WiFi] Connecting to %s\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void connectMQTT() {
  if (mqtt.connected() || WiFi.status() != WL_CONNECTED) return;

  unsigned long now = millis();
  if (now - lastMqttAttemptMs < MQTT_RECONNECT_INTERVAL_MS) return;
  lastMqttAttemptMs = now;

  if (!isConfigured(TB_HOST) || !isConfigured(TB_DEVICE_TOKEN)) {
    Serial.println("[MQTT] Configure TB_HOST and TB_DEVICE_TOKEN in config.h");
    return;
  }

  Serial.printf("[MQTT] Connecting to %s:%d ...\n", TB_HOST, TB_PORT);

  // ThingsBoard device access token is the MQTT username.
  if (mqtt.connect(SOFP_DEVICE_ID, TB_DEVICE_TOKEN, nullptr)) {
    Serial.println("[MQTT] Connected to ThingsBoard");
  } else {
    Serial.printf("[MQTT] Failed, state=%d\n", mqtt.state());
  }
}

// ---------------- Camera ----------------
bool beginCamera() {
#if !CAMERA_ENABLED
  Serial.println("[Camera] Disabled in config.h");
  return false;
#else
  camera_config_t cfg = {};
  cfg.ledc_channel = LEDC_CHANNEL_0;
  cfg.ledc_timer = LEDC_TIMER_0;

  cfg.pin_d0 = CAM_PIN_D0;
  cfg.pin_d1 = CAM_PIN_D1;
  cfg.pin_d2 = CAM_PIN_D2;
  cfg.pin_d3 = CAM_PIN_D3;
  cfg.pin_d4 = CAM_PIN_D4;
  cfg.pin_d5 = CAM_PIN_D5;
  cfg.pin_d6 = CAM_PIN_D6;
  cfg.pin_d7 = CAM_PIN_D7;

  cfg.pin_xclk = CAM_PIN_XCLK;
  cfg.pin_pclk = CAM_PIN_PCLK;
  cfg.pin_vsync = CAM_PIN_VSYNC;
  cfg.pin_href = CAM_PIN_HREF;
  cfg.pin_sccb_sda = CAM_PIN_SIOD;
  cfg.pin_sccb_scl = CAM_PIN_SIOC;
  cfg.pin_pwdn = CAM_PIN_PWDN;
  cfg.pin_reset = CAM_PIN_RESET;

  cfg.xclk_freq_hz = 20000000;
  cfg.pixel_format = PIXFORMAT_JPEG;
  cfg.frame_size = CAMERA_FRAME_SIZE;
  cfg.jpeg_quality = CAMERA_JPEG_QUALITY;
  cfg.fb_count = psramFound() ? 2 : 1;
  cfg.grab_mode = CAMERA_GRAB_LATEST;
  cfg.fb_location = psramFound() ? CAMERA_FB_IN_PSRAM : CAMERA_FB_IN_DRAM;

  Serial.printf("[Camera] PSRAM: %s\n", psramFound() ? "found" : "not found");
  esp_err_t err = esp_camera_init(&cfg);
  if (err != ESP_OK) {
    Serial.printf("[Camera] Init failed: 0x%x\n", err);
    return false;
  }

  sensor_t* sensor = esp_camera_sensor_get();
  if (sensor) {
    // OV5640 supports JPEG mode. Actual sensor detection is handled by driver.
    sensor->set_quality(sensor, CAMERA_JPEG_QUALITY);
  }

  Serial.println("[Camera] Initialized");
  return true;
#endif
}

// Send a JPEG frame as multipart/form-data.
// Backend endpoint expects POST /api/devices/{device_id}/frame
// with multipart field name "file".
bool sendFrameToBackend(camera_fb_t* fb) {
  if (!fb || !fb->buf || fb->len == 0) return false;
  if (WiFi.status() != WL_CONNECTED) return false;
  if (!isConfigured(BACKEND_BASE_URL)) {
    Serial.println("[HTTP] Configure BACKEND_BASE_URL in config.h");
    return false;
  }
  if (!isConfigured(SOFP_DEVICE_ID)) {
    Serial.println("[HTTP] Configure SOFP_DEVICE_ID in config.h");
    return false;
  }

  String url = String(BACKEND_BASE_URL) +
               "/api/devices/" + String(SOFP_DEVICE_ID) + "/frame";

  // Cloudflare Quick Tunnel uses HTTPS. This skips certificate validation.
  // Suitable for initial testing only; use certificate validation in production.
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  if (!http.begin(client, url)) {
    Serial.println("[HTTP] Could not start request");
    return false;
  }

  String boundary = "----SOFPBoundary7MA4YWxkTrZu0gW";
  String head = "--" + boundary +
                "\r\nContent-Disposition: form-data; name=\"file\"; filename=\"frame.jpg\"" 
                "\r\nContent-Type: image/jpeg\r\n\r\n";
  String tail = "\r\n--" + boundary + "--\r\n";

  // Combine multipart pieces for the initial prototype.
  // VGA JPEG frames should generally fit in available heap; monitor free heap.
  size_t totalLen = head.length() + fb->len + tail.length();
  uint8_t* body = (uint8_t*) malloc(totalLen);
  if (!body) {
    Serial.println("[HTTP] Not enough heap for multipart frame");
    http.end();
    return false;
  }

  memcpy(body, head.c_str(), head.length());
  memcpy(body + head.length(), fb->buf, fb->len);
  memcpy(body + head.length() + fb->len, tail.c_str(), tail.length());

  http.addHeader("Content-Type", "multipart/form-data; boundary=" + boundary);
  http.setTimeout(15000);
  int status = http.POST(body, totalLen);

  free(body);
  String response = http.getString();
  http.end();

  Serial.printf("[HTTP] Frame upload status=%d, bytes=%u\n",
                status, (unsigned int)fb->len);
  if (response.length()) Serial.println(response);
  return status >= 200 && status < 300;
}

void captureAndSendFrame() {
#if CAMERA_ENABLED
  if (!cameraReady || WiFi.status() != WL_CONNECTED) return;

  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("[Camera] Capture failed");
    return;
  }

  bool ok = sendFrameToBackend(fb);
  esp_camera_fb_return(fb);

  if (ok) Serial.println("[Camera] Frame delivered to Backend");
#endif
}

// ---------------- Optional sensor placeholders ----------------
float readBatteryVoltage() {
#if BATTERY_SENSOR_ENABLED
  // TODO: configure ADC attenuation, voltage divider ratio and calibration
  // for the actual PCB. Do not assume raw ADC voltage equals battery voltage.
  int raw = analogRead(BATTERY_ADC_PIN);
  return (raw / 4095.0f) * 3.3f; // Placeholder only; add divider multiplier.
#else
  return -1.0f;
#endif
}

float readTemperatureC() {
#if TEMPERATURE_SENSOR_ENABLED
  // TODO: add the actual sensor library and model (e.g. BME280 / DS18B20).
  return -999.0f;
#else
  return -999.0f;
#endif
}

int readPeopleCount() {
#if PEOPLE_SENSOR_ENABLED
  // TODO: replace with actual sensor logic.
  return 0;
#else
  return -1; // -1 means not configured / unavailable.
#endif
}

// ---------------- ThingsBoard telemetry ----------------
void publishTelemetry() {
  if (!mqtt.connected()) return;

  String payload = "{";
  payload += "\"RSSI\":" + String(WiFi.RSSI());

  int people = readPeopleCount();
  if (people >= 0) {
    payload += ",\"People\":" + String(people);
  }

  float battery = readBatteryVoltage();
  if (battery >= 0.0f) {
    payload += ",\"Battery\":" + String(battery, 2);
  }

  float temperature = readTemperatureC();
  if (temperature > -100.0f) {
    payload += ",\"Temperature\":" + String(temperature, 1);
  }

  payload += "}";

  bool ok = mqtt.publish("v1/devices/me/telemetry", payload.c_str());
  Serial.printf("[MQTT] Telemetry %s: %s\n", ok ? "sent" : "failed", payload.c_str());
}

// ---------------- Arduino setup / loop ----------------
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("=================================");
  Serial.println(" SOFP Firmware - ESP32-S3 N16R8 ");
  Serial.println(" Camera: OV5640 (DVP assumed)   ");
  Serial.println("=================================");

  setStatusLed(false);

  if (!isConfigured(SOFP_DEVICE_ID)) {
    Serial.println("[CONFIG] Set SOFP_DEVICE_ID in config.h");
  }

  mqtt.setServer(TB_HOST, TB_PORT);
  mqtt.setBufferSize(512);

  connectWiFi();

  // Camera init can be done before Wi-Fi connects.
  cameraReady = beginCamera();
  setStatusLed(cameraReady);
}

void loop() {
  connectWiFi();
  connectMQTT();

  if (mqtt.connected()) {
    mqtt.loop();
  }

  unsigned long now = millis();

  if (mqtt.connected() && now - lastTelemetryMs >= TELEMETRY_INTERVAL_MS) {
    lastTelemetryMs = now;
    publishTelemetry();
  }

  if (now - lastCameraMs >= CAMERA_FRAME_INTERVAL_MS) {
    lastCameraMs = now;
    captureAndSendFrame();
  }

  delay(5);
}
