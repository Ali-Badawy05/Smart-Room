#pragma once

// ============================================================
// FIRMWARE IDENTITY
// ============================================================
#define FW_VERSION "1.1.0"
#define DEVICE_ID  "smart_room_01"

// ============================================================
// WIFI
// ============================================================
#define WIFI_SSID     "hazemm"
#define WIFI_PASSWORD "Marrr28091979loly15022005"

#define WIFI_BOOT_TIMEOUT_MS 45000
#define WIFI_BOOT_RETRY_MS   15000
#define WIFI_HARD_RETRY_MS   30000

// ============================================================
// MQTT BROKER  (HiveMQ Cloud or your own broker)
// ============================================================
#define MQTT_HOST "e219d0db021949a3bc4670fad9b7d2c3.s1.eu.hivemq.cloud"
#define MQTT_PORT 8883          // 8883 = TLS (required by HiveMQ Cloud)
#define MQTT_USER "alybadawy"
#define MQTT_PASS "aly3573332"

// MQTT TOPICS
#define TOPIC_STATUS        "smart_room/" DEVICE_ID "/status"
#define TOPIC_DHT           "smart_room/" DEVICE_ID "/sensor/dht"
#define TOPIC_PRESENCE      "smart_room/" DEVICE_ID "/sensor/presence"
#define TOPIC_MOTION_ALERT  "smart_room/" DEVICE_ID "/alert/motion"
#define TOPIC_AWAY_MODE_SET "smart_room/" DEVICE_ID "/away_mode/set"
#define TOPIC_AWAY_MODE_STATE "smart_room/" DEVICE_ID "/away_mode/state"

#define TOPIC_RELAY_STATE_FMT "smart_room/" DEVICE_ID "/relay/%d/state"
#define TOPIC_RELAY_SET_FMT   "smart_room/" DEVICE_ID "/relay/%d/set"
#define TOPIC_RELAY_SET_SUB   "smart_room/" DEVICE_ID "/relay/+/set"

#define TOPIC_OTA_TRIGGER   "smart_room/" DEVICE_ID "/ota/check"
#define TOPIC_OTA_INFO      "smart_room/" DEVICE_ID "/ota/info"

// ============================================================
// HARDWARE PIN MAP  (ESP32-WROOM-32)
// ============================================================
#define NUM_RELAYS 3

const uint8_t RELAY_PINS[NUM_RELAYS]  = {26, 27, 14};
const uint8_t SWITCH_PINS[NUM_RELAYS] = {32, 33, 25};

#define DHT_PIN  4

// DHT11: if you swap to a DHT22, change BOTH this line and
// DHT_T_MIN/MAX + DHT_H_MIN/MAX below.
#define DHT_TYPE DHT11

#define DHT_T_MIN  -2.0f
#define DHT_T_MAX  52.0f
#define DHT_H_MIN  15.0f
#define DHT_H_MAX  95.0f

#define PIR_PIN  13

#define BUZZER_PIN 21
// Flip to true if your buzzer module beeps when the pin is driven LOW.
#define BUZZER_ACTIVE_LOW false

// Flip to false if your relay module is active-high.
#define RELAY_ACTIVE_LOW true

// ============================================================
// TIMING (all in milliseconds unless noted)
// ============================================================
#define DHT_READ_INTERVAL_MS       10000
#define PIR_POLL_INTERVAL_MS       200
#define PIR_CONFIRM_READS          3
#define PIR_HOLD_TIME_MS           2000
#define SWITCH_DEBOUNCE_MS         50
#define HEARTBEAT_INTERVAL_MS      30000
#define MQTT_RECONNECT_BASE_MS     2000
#define MQTT_RECONNECT_MAX_MS      60000
#define WIFI_RECONNECT_CHECK_MS    5000

#define BUZZER_ALERT_BEEP_COUNT   3
#define BUZZER_ALERT_BEEP_ON_MS   150
#define BUZZER_ALERT_BEEP_OFF_MS  150

#define WATCHDOG_TIMEOUT_S 15

// ============================================================
// OTA (remote firmware updates)
// ============================================================
#define OTA_VERSION_CHECK_URL "https://your-backend.example.com/firmware/latest.json"

#define OTA_HOSTNAME  "smart-room"
#define OTA_WEB_USER  "alybadawy"
#define OTA_WEB_PASS  "aly3573332"       // CHANGE THIS
