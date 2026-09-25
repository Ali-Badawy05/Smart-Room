#include <Arduino.h>
#include <Arduino.h>
#include "config.h"
#include "logger.h"
#include "wifi_manager.h"
#include "mqtt_manager.h"
#include "sensors.h"
#include "relay_control.h"
#include "ota_manager.h"
#include "buzzer.h"
#include <esp_task_wdt.h>
#include <WiFi.h>

static bool awayMode = false;
static bool firmwareMarkedValid = false;
static unsigned long lastHeartbeatMs = 0;

static void onMqttMessage(const String &topic, const String &payload) {
  if (relayHandleMqttMessage(topic, payload)) return;

  if (topic == TOPIC_AWAY_MODE_SET) {
    String p = payload;
    p.trim();
    p.toUpperCase();
    awayMode = (p == "ON" || p == "1" || p == "TRUE");
    LOG_I("MAIN", String("Away mode set to ") + (awayMode ? "ON" : "OFF"));
    mqttPublish(TOPIC_AWAY_MODE_STATE, awayMode ? "ON" : "OFF", true);
    return;
  }

  if (topic == TOPIC_OTA_TRIGGER) {
    LOG_I("MAIN", "OTA check requested via MQTT.");
    otaCheckForUpdate();
    return;
  }
}

static void publishUpdates() {
  if (dhtHasNewReading()) {
    char payload[64];
    snprintf(payload, sizeof(payload), "{\"temp\":%.1f,\"hum\":%.1f}",
              dhtGetTemperature(), dhtGetHumidity());
    mqttPublish(TOPIC_DHT, payload, true);
  }

  if (presenceChanged()) {
    bool occ = presenceIsOccupied();
    mqttPublish(TOPIC_PRESENCE, occ ? "occupied" : "empty", true);

    if (occ && awayMode) {
      LOG_W("MAIN", "Motion detected while AWAY MODE is active - publishing alert.");
      mqttPublish(TOPIC_MOTION_ALERT, "{\"event\":\"motion_detected\"}", false);
      buzzerTrigger();
    }
  }

  for (uint8_t i = 0; i < NUM_RELAYS; i++) {
    if (relayConsumeChanged(i)) {
      char topic[64];
      snprintf(topic, sizeof(topic), TOPIC_RELAY_STATE_FMT, i);
      mqttPublish(String(topic), relayGetState(i) ? "ON" : "OFF", true);
    }
  }

  if (millis() - lastHeartbeatMs >= HEARTBEAT_INTERVAL_MS) {
    lastHeartbeatMs = millis();
    mqttPublish(TOPIC_STATUS, "online", true);
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n==================================================");
  Serial.println("  SMART ROOM FIRMWARE  v" FW_VERSION "  (" DEVICE_ID ")");
  Serial.println("==================================================\n");

  relayControlBegin();

  sensorsBegin();
  buzzerBegin();
  wifiManagerBegin();
  mqttManagerBegin(onMqttMessage);
  otaManagerBegin();
  otaWebBegin();

  esp_task_wdt_init(WATCHDOG_TIMEOUT_S, true);
  esp_task_wdt_add(NULL);

  LOG_I("MAIN", "Setup complete. Entering main loop.");
}

void loop() {
  esp_task_wdt_reset();

  wifiManagerUpdate();
  mqttManagerUpdate();

  relayControlUpdate();
  sensorsUpdate();
  buzzerUpdate();
  otaWebUpdate();

  if (mqttIsConnected()) {
    publishUpdates();

    if (!firmwareMarkedValid) {
      otaMarkFirmwareValid();
      firmwareMarkedValid = true;

      mqttPublish(TOPIC_OTA_INFO,
                  String("{\"version\":\"") + FW_VERSION + "\",\"web\":\"http://" +
                  WiFi.localIP().toString() + "/update\"}",
                  true);
    }
  }
}
