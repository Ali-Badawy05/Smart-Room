#include "mqtt_manager.h"
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "config.h"
#include "logger.h"
#include "wifi_manager.h"

static WiFiClientSecure secureClient;
static PubSubClient mqttClient(secureClient);
static MqttMessageHandler userHandler = nullptr;

static unsigned long lastAttemptMs = 0;
static unsigned long currentBackoffMs = MQTT_RECONNECT_BASE_MS;

static void rawMqttCallback(char *topic, byte *payload, unsigned int length) {
  String msg;
  msg.reserve(length);
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];

  LOG_I("MQTT", "RX [" + String(topic) + "] " + msg);

  if (userHandler) userHandler(String(topic), msg);
}

static void subscribeAll() {
  mqttClient.subscribe(TOPIC_RELAY_SET_SUB, 1);
  mqttClient.subscribe(TOPIC_AWAY_MODE_SET, 1);
  mqttClient.subscribe(TOPIC_OTA_TRIGGER, 1);
  LOG_I("MQTT", "Subscribed to command topics.");
}

static const char *mqttStateName(int state) {
  switch (state) {
    case -4: return "TIMEOUT - server didn't respond in time (broker unreachable, wrong host/port, or handshake too slow)";
    case -3: return "CONNECTION_LOST - TCP/TLS connection dropped mid-handshake";
    case -2: return "CONNECT_FAILED - couldn't open a connection to MQTT_HOST:MQTT_PORT at all (check host spelling / port 8883 / WiFi)";
    case -1: return "DISCONNECTED";
    case 1:  return "BAD_PROTOCOL";
    case 2:  return "BAD_CLIENT_ID";
    case 3:  return "UNAVAILABLE - broker reachable but not accepting connections right now";
    case 4:  return "BAD_CREDENTIALS - MQTT_USER or MQTT_PASS is wrong";
    case 5:  return "UNAUTHORIZED - user/pass correct but not allowed to connect (check broker's access list / permissions)";
    default: return "UNKNOWN";
  }
}

void mqttManagerBegin(MqttMessageHandler handler) {
  userHandler = handler;

  secureClient.setInsecure();

  secureClient.setTimeout(15000);
#if defined(ARDUINO_ARCH_ESP32)
  secureClient.setHandshakeTimeout(15);
#endif

  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCallback(rawMqttCallback);
  mqttClient.setBufferSize(512);
  mqttClient.setKeepAlive(20);
  mqttClient.setSocketTimeout(15);
}

static void attemptConnect() {
  LOG_I("MQTT", "Connecting to broker...");

  String cId = String(DEVICE_ID) + "-" + String(random(0xffff), HEX);

  bool ok = mqttClient.connect(
      cId.c_str(),
      MQTT_USER, MQTT_PASS,
      TOPIC_STATUS, 1, true,
      "offline"
  );

  if (ok) {
    LOG_I("MQTT", "Connected as " + cId);
    currentBackoffMs = MQTT_RECONNECT_BASE_MS;
    mqttClient.publish(TOPIC_STATUS, "online", true);
    subscribeAll();
  } else {
    int rc = mqttClient.state();
    LOG_W("MQTT", "Connect failed, rc=" + String(rc) + " (" + mqttStateName(rc) +
                       "). Retrying in " + String(currentBackoffMs / 1000) + "s");
    currentBackoffMs = min(currentBackoffMs * 2, (unsigned long)MQTT_RECONNECT_MAX_MS);
  }
}

void mqttManagerUpdate() {
  if (!wifiIsConnected()) return;

  if (!mqttClient.connected()) {
    if (millis() - lastAttemptMs >= currentBackoffMs) {
      lastAttemptMs = millis();
      attemptConnect();
    }
    return;
  }

  mqttClient.loop();
}

bool mqttIsConnected() {
  return mqttClient.connected();
}

bool mqttPublish(const String &topic, const String &payload, bool retained) {
  if (!mqttClient.connected()) {
    LOG_W("MQTT", "Publish skipped (not connected): " + topic);
    return false;
  }
  bool ok = mqttClient.publish(topic.c_str(), payload.c_str(), retained);
  if (!ok) LOG_W("MQTT", "Publish FAILED: " + topic);
  return ok;
}
