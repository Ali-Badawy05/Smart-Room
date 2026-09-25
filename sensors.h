#pragma once
#include <Arduino.h>

void sensorsBegin();
void sensorsUpdate();

bool dhtHasNewReading();
float dhtGetTemperature();
float dhtGetHumidity();

bool presenceIsOccupied();
bool presenceChanged();
