#pragma once
#include <Arduino.h>

void otaManagerBegin();

void otaWebBegin();
void otaWebUpdate();

void otaMarkFirmwareValid();
void otaCheckForUpdate();
