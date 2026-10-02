#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include <stdarg.h>
#include <stdio.h>

#include "log_format.h"

namespace Logger {

inline void begin(unsigned long baud = 115200) {
#if ARDUINO_USB_CDC_ON_BOOT
  Serial.begin(baud);
  delay(500);
#else
  Serial.begin(baud, SERIAL_8N1);
  delay(50);
#endif
}

inline void log(const char* tag, const char* message) {
  char buffer[256];
  formatLogLine(buffer, sizeof(buffer), millis(), tag, message);
  Serial.println(buffer);
}

inline void logf(const char* tag, const char* fmt, ...) {
  char buffer[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);
  log(tag, buffer);
}

}  // namespace Logger

#endif  // LOGGER_H
