#pragma once
#include <Arduino.h>

typedef void (*MqttMessageHandler)(const String &topic, const String &payload);

void mqttManagerBegin(MqttMessageHandler handler);
void mqttManagerUpdate();
bool mqttIsConnected();
bool mqttPublish(const String &topic, const String &payload, bool retained = false);
