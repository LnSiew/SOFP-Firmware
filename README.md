# SOFP Firmware - ESP32-S3 WROOM N16R8 + OV5640

## Important hardware warning
The camera GPIO numbers in `config.h` are placeholders. They are not a verified
pinout for your specific PCB. Do not power/enable the camera with these values
until the hardware schematic or PCB pin mapping has been checked.

This sketch assumes OV5640 uses an 8-bit DVP/parallel interface. If your camera
is MIPI CSI, this firmware/ESP32-S3 camera interface is not suitable as-is.

## Arduino IDE setup
1. Install Espressif ESP32 board package using Boards Manager.
2. Select the exact ESP32-S3 board if known; otherwise ESP32S3 Dev Module is
   only a provisional selection.
3. Enable PSRAM in Tools if the board menu exposes that option. N16R8 modules
   typically have 16 MB flash and 8 MB PSRAM, but the carrier board matters.
4. Install library: PubSubClient by Nick O'Leary.
5. `WiFi`, `HTTPClient`, `WiFiClientSecure`, and `esp_camera` are supplied by
   the ESP32 Arduino core.
6. Put `SOFP-Firmware.ino` and `config.h` in the same Arduino sketch folder.

## Configure
Edit `config.h`:
- WIFI_SSID / WIFI_PASSWORD
- SOFP_DEVICE_ID: use the same ID that the website uses for the camera.
- TB_HOST / TB_DEVICE_TOKEN: ThingsBoard host and device access token.
- BACKEND_BASE_URL: current Cloudflare HTTPS URL.
- Camera GPIOs: replace all placeholders with actual PCB pin mapping.

The backend upload is multipart/form-data with field name `file`, matching:
POST /api/devices/{device_id}/frame

ThingsBoard telemetry is published to:
v1/devices/me/telemetry
Keys are RSSI, People, Battery, Temperature. Optional sensor keys are omitted
until their ENABLE flag is set and their driver implementation is completed.

## Initial limitations / safety
- Battery, temperature, and people sensor readers are placeholders.
- Battery reading requires the actual voltage divider and ADC calibration.
- Temperature sensor needs its actual model/library and bus wiring.
- People sensor logic needs the actual sensor model.
- MQTT currently uses port 1883 without TLS for initial testing. For production,
  configure TLS on port 8883 and certificate validation.
- HTTPS upload currently uses setInsecure() for initial Cloudflare testing.
  Replace with certificate validation for production.
- Cloudflare Quick Tunnel URLs change when restarted. Update BACKEND_BASE_URL
  after each restart, or later switch to a named tunnel / device-discoverable URL.
- Never commit Wi-Fi passwords or ThingsBoard access tokens to a public repo.
