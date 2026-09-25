#include "sensors.h"
#include <DHT.h>
#include "config.h"
#include "logger.h"

static DHT dht(DHT_PIN, DHT_TYPE);

static unsigned long lastDhtReadMs = 0;
static float lastTemp = NAN;
static float lastHum  = NAN;
static bool newDhtReading = false;

static unsigned long lastPirPollMs = 0;
static unsigned long lastMotionMs  = 0;
static bool occupied = false;
static bool occupiedChanged = false;
static uint8_t motionConsecutiveReads = 0;

void sensorsBegin() {
  dht.begin();
  pinMode(PIR_PIN, INPUT);
  LOG_I("SENSOR", "DHT11 on GPIO" + String(DHT_PIN) + ", PIR on GPIO" + String(PIR_PIN) + " initialised.");
}

static void updateDht() {
  newDhtReading = false;
  if (millis() - lastDhtReadMs < DHT_READ_INTERVAL_MS) return;
  lastDhtReadMs = millis();

  float t = dht.readTemperature();
  float h = dht.readHumidity();

  bool commsFailure = isnan(t) || isnan(h);
  bool valid = !commsFailure &&
               t >= DHT_T_MIN && t <= DHT_T_MAX &&
               h >= DHT_H_MIN && h <= DHT_H_MAX;

  if (valid) {
    lastTemp = t;
    lastHum = h;
    newDhtReading = true;
    LOG_I("SENSOR", "DHT11 OK: " + String(t, 1) + "C, " + String(h, 1) + "%");
  } else if (commsFailure) {
    LOG_W("SENSOR", "DHT11 read FAILED: no response / checksum error from sensor - "
                     "check DATA wiring, the pull-up resistor, and power. Keeping last good reading.");
  } else {
    LOG_W("SENSOR", "DHT11 read out of physical range (t=" + String(t, 1) + "C, h=" + String(h, 1) +
                     "%) - rejecting. Keeping last good reading.");
  }
}

static void updatePir() {
  occupiedChanged = false;
  if (millis() - lastPirPollMs < PIR_POLL_INTERVAL_MS) return;
  lastPirPollMs = millis();

  bool rawHigh = digitalRead(PIR_PIN) == HIGH;
  if (rawHigh) {
    if (motionConsecutiveReads < PIR_CONFIRM_READS) motionConsecutiveReads++;
  } else {
    motionConsecutiveReads = 0;
  }
  bool motionNow = motionConsecutiveReads >= PIR_CONFIRM_READS;
  if (motionNow) lastMotionMs = millis();

  bool shouldBeOccupied = (millis() - lastMotionMs) < PIR_HOLD_TIME_MS;

  if (shouldBeOccupied != occupied) {
    occupied = shouldBeOccupied;
    occupiedChanged = true;
    LOG_I("SENSOR", occupied ? "Presence: room OCCUPIED" : "Presence: room EMPTY");
  }
}

void sensorsUpdate() {
  updateDht();
  updatePir();
}

bool dhtHasNewReading() { return newDhtReading; }
float dhtGetTemperature() { return lastTemp; }
float dhtGetHumidity() { return lastHum; }

bool presenceIsOccupied() { return occupied; }
bool presenceChanged() { return occupiedChanged; }
