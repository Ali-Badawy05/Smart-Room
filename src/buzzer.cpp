#include "buzzer.h"
#include "config.h"
#include "logger.h"

static bool buzzing = false;
static bool buzzerOn = false;
static uint8_t stepsRemaining = 0;
static unsigned long lastToggleMs = 0;

static void writeBuzzer(bool on) {
  bool physicalOn = BUZZER_ACTIVE_LOW ? !on : on;
  digitalWrite(BUZZER_PIN, physicalOn ? HIGH : LOW);
}

void buzzerBegin() {
  pinMode(BUZZER_PIN, OUTPUT);
  writeBuzzer(false);
  LOG_I("BUZZER", "Buzzer on GPIO" + String(BUZZER_PIN) + " initialised.");
}

void buzzerTrigger() {
  stepsRemaining = BUZZER_ALERT_BEEP_COUNT * 2;
  buzzing = true;
  buzzerOn = false;
  writeBuzzer(false);
  lastToggleMs = millis() - BUZZER_ALERT_BEEP_OFF_MS;
  LOG_I("BUZZER", "Away-mode motion alert - starting beep pattern.");
}

void buzzerUpdate() {
  if (!buzzing) return;

  unsigned long interval = buzzerOn ? BUZZER_ALERT_BEEP_ON_MS : BUZZER_ALERT_BEEP_OFF_MS;
  if (millis() - lastToggleMs < interval) return;

  lastToggleMs = millis();
  buzzerOn = !buzzerOn;
  writeBuzzer(buzzerOn);
  stepsRemaining--;

  if (stepsRemaining == 0) {
    buzzing = false;
    writeBuzzer(false);
  }
}
