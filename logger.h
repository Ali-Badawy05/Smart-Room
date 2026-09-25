#pragma once
#include <Arduino.h>

enum LogLevel { LOG_INFO, LOG_WARN, LOG_ERROR };

inline void logMsg(LogLevel level, const char *tag, const String &msg) {
  const char *levelStr;
  switch (level) {
    case LOG_INFO:  levelStr = "INFO "; break;
    case LOG_WARN:  levelStr = "WARN "; break;
    case LOG_ERROR: levelStr = "ERROR"; break;
    default:        levelStr = "?????"; break;
  }
  Serial.printf("[%8lu][%s][%-8s] %s\n", millis(), levelStr, tag, msg.c_str());
}

#define LOG_I(tag, msg) logMsg(LOG_INFO, tag, msg)
#define LOG_W(tag, msg) logMsg(LOG_WARN, tag, msg)
#define LOG_E(tag, msg) logMsg(LOG_ERROR, tag, msg)
