#include "wifi_manager.h"
#include <WiFi.h>
#include "lwip/dns.h"
#include "config.h"
#include "logger.h"

static unsigned long lastCheckMs = 0;
static unsigned long offlineSinceMs = 0;
static unsigned long lastHardRetryMs = 0;
static bool offline = false;

static const char *reasonText(uint8_t r) {
  switch (r) {
    case 2:   return "AUTH_EXPIRE (router dropped the session)";
    case 5:   return "ASSOC_TOOMANY (router client limit reached)";
    case 8:   return "ASSOC_LEAVE (we left, or the router kicked us)";
    case 15:  return "4WAY_HANDSHAKE_TIMEOUT (weak signal or WPA3/PMF issue)";
    case 200: return "BEACON_TIMEOUT (signal lost - weak signal / interference)";
    case 201: return "NO_AP_FOUND (network not visible - 5GHz only? out of range?)";
    case 202: return "AUTH_FAIL (wrong password or WPA3/PMF mismatch)";
    case 203: return "ASSOC_FAIL";
    case 204: return "HANDSHAKE_TIMEOUT";
    case 205: return "CONNECTION_FAIL";
    default:  return "other";
  }
}

static void onWifiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED: {
      uint8_t reason = info.wifi_sta_disconnected.reason;
      LOG_W("WIFI", "Disconnected, reason " + String(reason) + " = " + reasonText(reason));
      break;
    }
    case ARDUINO_EVENT_WIFI_STA_GOT_IP: {
      ip_addr_t d1, d2;
      IP_ADDR4(&d1, 8, 8, 8, 8);
      IP_ADDR4(&d2, 8, 8, 4, 4);
      dns_setserver(0, &d1);
      dns_setserver(1, &d2);

      LOG_I("WIFI", "Got IP " + WiFi.localIP().toString() +
                    "  RSSI " + String(WiFi.RSSI()) + " dBm" +
                    "  channel " + String(WiFi.channel()));
      break;
    }
    default:
      break;
  }
}

void wifiManagerBegin() {
  LOG_I("WIFI", "Connecting to \"" WIFI_SSID "\"...");
  WiFi.onEvent(onWifiEvent);
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long start = millis();
  unsigned long lastTry = start;
  while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_BOOT_TIMEOUT_MS) {
    delay(250);
    Serial.print(".");
    if (millis() - lastTry >= WIFI_BOOT_RETRY_MS) {
      lastTry = millis();
      Serial.println();
      LOG_W("WIFI", "Still not connected - retrying the attempt...");
      WiFi.disconnect(false);
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    }
  }
  Serial.println();

  if (WiFi.status() != WL_CONNECTED) {
    LOG_E("WIFI", "Could not connect within boot timeout. Rebooting to retry...");
    delay(1000);
    ESP.restart();
  }
}

void wifiManagerUpdate() {
  if (millis() - lastCheckMs < WIFI_RECONNECT_CHECK_MS) return;
  lastCheckMs = millis();

  if (WiFi.status() == WL_CONNECTED) {
    offline = false;
    return;
  }

  if (!offline) {
    offline = true;
    offlineSinceMs = millis();
    lastHardRetryMs = millis();
    LOG_W("WIFI", "Connection lost - waiting for auto-reconnect...");
    return;
  }

  if (millis() - lastHardRetryMs >= WIFI_HARD_RETRY_MS) {
    lastHardRetryMs = millis();
    LOG_W("WIFI", "Offline for " + String((millis() - offlineSinceMs) / 1000) +
                  "s - restarting connection attempt");
    WiFi.disconnect(false);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  }
}

bool wifiIsConnected() {
  return WiFi.status() == WL_CONNECTED;
}
