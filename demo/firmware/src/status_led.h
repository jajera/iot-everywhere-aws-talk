#ifndef STATUS_LED_H
#define STATUS_LED_H

#include <Arduino.h>
#include <stdint.h>

// Onboard WS2812 (PIN_RGB_LED / RGB_BUILTIN from board variant).
// Chips with RMT: Arduino rgbLedWrite().
// Chips without RMT (ESP32-C61): espressif/led_strip SPI backend (app dependency).

#if defined(PIN_RGB_LED) || defined(RGB_BUILTIN)
#define STATUS_LED_HAS_RGB 1
#endif

class StatusLed {
 public:
  void begin();
  void loop();
  void flashTelemetrySent();

 private:
#ifdef STATUS_LED_HAS_RGB
  bool initialized_ = false;
  bool active_ = false;
  uint32_t off_at_ms_ = 0;
#endif
};

#endif  // STATUS_LED_H
