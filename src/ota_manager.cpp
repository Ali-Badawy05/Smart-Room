#include "ota_manager.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ElegantOTA.h>
#include "esp_ota_ops.h"
#include "esp_task_wdt.h"
#include "config.h"
#include "logger.h"

static WiFiClientSecure otaClient;
static WebServer webServer(80);

extern "C" bool verifyRollbackLater() {
  return true;
}

void otaWebBegin() {
  if (MDNS.begin(OTA_HOSTNAME)) {
    LOG_I("OTA", String("mDNS ready: http://") + OTA_HOSTNAME + ".local/update");
  } else {
    LOG_W("OTA", "mDNS failed - use the IP address instead.");
  }

  webServer.on("/", []() {
    webServer.sendHeader("Location", "/update");
    webServer.send(302, "text/plain", "");
  });

  ElegantOTA.begin(&webServer, OTA_WEB_USER, OTA_WEB_PASS);

  ElegantOTA.onStart([]() {
    LOG_I("OTA", "Web upload started.");
    esp_task_wdt_delete(NULL);
  });
  ElegantOTA.onEnd([](bool success) {
    if (success) {
      LOG_I("OTA", "Web upload finished OK - rebooting.");
    } else {
      LOG_E("OTA", "Web upload FAILED - staying on current firmware.");
      esp_task_wdt_add(NULL);
    }
  });

  webServer.begin();
  LOG_I("OTA", String("Web OTA ready: http://") + WiFi.localIP().toString() + "/update");
}

void otaWebUpdate() {
  webServer.handleClient();
  ElegantOTA.loop();
}

void otaManagerBegin() {
  otaClient.setInsecure();
}

void otaMarkFirmwareValid() {
  esp_err_t err = esp_ota_mark_app_valid_cancel_rollback();
  if (err == ESP_OK) {
    LOG_I("OTA", "Firmware marked valid (rollback cancelled).");
  }
}

static String extractJsonString(const String &json, const String &key) {
  String pattern = "\"" + key + "\"";
  int keyIdx = json.indexOf(pattern);
  if (keyIdx < 0) return "";
  int colonIdx = json.indexOf(':', keyIdx);
  if (colonIdx < 0) return "";
  int firstQuote = json.indexOf('"', colonIdx + 1);
  int secondQuote = json.indexOf('"', firstQuote + 1);
  if (firstQuote < 0 || secondQuote < 0) return "";
  return json.substring(firstQuote + 1, secondQuote);
}

void otaCheckForUpdate() {
  LOG_I("OTA", "Checking for update at " OTA_VERSION_CHECK_URL " ...");

  HTTPClient http;
  http.begin(otaClient, OTA_VERSION_CHECK_URL);
  int httpCode = http.GET();

  if (httpCode != HTTP_CODE_OK) {
    LOG_W("OTA", "Version check failed, HTTP code: " + String(httpCode));
    http.end();
    return;
  }

  String body = http.getString();
  http.end();

  String remoteVersion = extractJsonString(body, "version");
  String firmwareUrl = extractJsonString(body, "url");

  if (remoteVersion.isEmpty() || firmwareUrl.isEmpty()) {
    LOG_W("OTA", "Malformed version-check response: " + body);
    return;
  }

  if (remoteVersion == FW_VERSION) {
    LOG_I("OTA", "Already up to date (v" FW_VERSION ").");
    return;
  }

  LOG_I("OTA", "New version available: " + remoteVersion + " (current: " FW_VERSION "). Downloading...");

  httpUpdate.rebootOnUpdate(true);

  esp_task_wdt_delete(NULL);
  t_httpUpdate_return result = httpUpdate.update(otaClient, firmwareUrl);
  esp_task_wdt_add(NULL);

  switch (result) {
    case HTTP_UPDATE_FAILED:
      LOG_E("OTA", "Update FAILED: " + httpUpdate.getLastErrorString());
      break;
    case HTTP_UPDATE_NO_UPDATES:
      LOG_I("OTA", "Server reported no update available.");
      break;
    case HTTP_UPDATE_OK:
      LOG_I("OTA", "Update OK - rebooting into new firmware now.");
      break;
  }
}
