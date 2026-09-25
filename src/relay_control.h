#pragma once
#include <Arduino.h>
#include "config.h"

void relayControlBegin();
void relayControlUpdate();

void setRelay(uint8_t index, bool on, const char *source);

bool relayGetState(uint8_t index);
bool relayConsumeChanged(uint8_t index);

bool relayHandleMqttMessage(const String &topic, const String &payload);
