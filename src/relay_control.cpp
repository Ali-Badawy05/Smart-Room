#include "relay_control.h"
#include <Preferences.h>
#include "logger.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

static Preferences prefs;

static bool relayState[NUM_RELAYS]   = {false, false, false};
static bool relayChangedFlag[NUM_RELAYS] = {false, false, false};

static bool     rawLastReading[NUM_RELAYS];
static bool     stableState[NUM_RELAYS];
static unsigned long lastBounceMs[NUM_RELAYS];

static void checkSwitches();

static SemaphoreHandle_t relayMutex = nullptr;
static TaskHandle_t switchTaskHandle = nullptr;

static void switchPollTask(void *pvParameters) {
  (void)pvParameters;
  for (;;) {
    checkSwitches();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

static void writeRelayPin(uint8_t index) {
  bool on = relayState[index];
  int level = RELAY_ACTIVE_LOW ? (on ? LOW : HIGH) : (on ? HIGH : LOW);
  digitalWrite(RELAY_PINS[index], level);
}

void relayControlBegin() {
  prefs.begin("relay", false);
  relayMutex = xSemaphoreCreateMutex();

  for (uint8_t i = 0; i < NUM_RELAYS; i++) {
    pinMode(RELAY_PINS[i], OUTPUT);
    pinMode(SWITCH_PINS[i], INPUT_PULLUP);

    String key = "r" + String(i);
    relayState[i] = prefs.getBool(key.c_str(), false);
    writeRelayPin(i);

    bool initialRead = digitalRead(SWITCH_PINS[i]);
    rawLastReading[i] = initialRead;
    stableState[i] = initialRead;
    lastBounceMs[i] = millis();

    LOG_I("RELAY", "Relay " + String(i) + " restored to " + String(relayState[i] ? "ON" : "OFF") +
                       " (pin " + String(RELAY_PINS[i]) + ", switch pin " + String(SWITCH_PINS[i]) + ")");
  }

  xTaskCreatePinnedToCore(
      switchPollTask,
      "SwitchPoll",
      4096,
      nullptr,
      2,
      &switchTaskHandle,
      0
  );
  LOG_I("RELAY", "Physical-switch polling task started on core 0 (network-independent).");
}

void setRelay(uint8_t index, bool on, const char *source) {
  if (index >= NUM_RELAYS) {
    LOG_E("RELAY", "setRelay called with invalid index " + String(index));
    return;
  }

  if (relayMutex) xSemaphoreTake(relayMutex, portMAX_DELAY);
  bool changed = (relayState[index] != on);
  if (changed) {
    relayState[index] = on;
    writeRelayPin(index);
    relayChangedFlag[index] = true;

    String key = "r" + String(index);
    prefs.putBool(key.c_str(), on);
  }
  if (relayMutex) xSemaphoreGive(relayMutex);

  if (changed) {
    LOG_I("RELAY", "Relay " + String(index) + " -> " + String(on ? "ON" : "OFF") +
                       " (source: " + String(source) + ")");
  } else {
    LOG_I("RELAY", "Relay " + String(index) + " already " + String(on ? "ON" : "OFF") +
                       " (source: " + String(source) + ") - no pin change needed");
  }
}

bool relayGetState(uint8_t index) {
  if (index >= NUM_RELAYS) return false;
  if (relayMutex) xSemaphoreTake(relayMutex, portMAX_DELAY);
  bool s = relayState[index];
  if (relayMutex) xSemaphoreGive(relayMutex);
  return s;
}

bool relayConsumeChanged(uint8_t index) {
  if (index >= NUM_RELAYS) return false;
  if (relayMutex) xSemaphoreTake(relayMutex, portMAX_DELAY);
  bool c = relayChangedFlag[index];
  relayChangedFlag[index] = false;
  if (relayMutex) xSemaphoreGive(relayMutex);
  return c;
}

static void checkSwitches() {
  unsigned long now = millis();

  for (uint8_t i = 0; i < NUM_RELAYS; i++) {
    bool reading = digitalRead(SWITCH_PINS[i]);

    if (reading != rawLastReading[i]) {
      lastBounceMs[i] = now;
      rawLastReading[i] = reading;
    }

    if ((now - lastBounceMs[i]) > SWITCH_DEBOUNCE_MS && reading != stableState[i]) {
      stableState[i] = reading;
      setRelay(i, !relayState[i], "switch");
    }
  }
}

void relayControlUpdate() {
}

bool relayHandleMqttMessage(const String &topic, const String &payload) {
  int relayIdx = topic.indexOf("/relay/");
  if (relayIdx < 0) return false;

  int startIdx = relayIdx + 7;
  int endIdx = topic.indexOf("/set", startIdx);
  if (endIdx < 0) return false;

  String idxStr = topic.substring(startIdx, endIdx);
  int idx = idxStr.toInt();
  if (idx < 0 || idx >= NUM_RELAYS) {
    LOG_W("RELAY", "MQTT command for out-of-range relay index: " + idxStr);
    return false;
  }

  String p = payload;
  p.trim();
  p.toUpperCase();
  bool wantOn = (p == "ON" || p == "1" || p == "TRUE");
  bool wantOff = (p == "OFF" || p == "0" || p == "FALSE");

  if (!wantOn && !wantOff) {
    LOG_W("RELAY", "Unrecognised relay payload: \"" + payload + "\"");
    return false;
  }

  setRelay((uint8_t)idx, wantOn, "mqtt");
  return true;
}
