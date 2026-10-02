#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>

class WiFiManager {
 public:
  void begin(const char* ssid, const char* password);
  bool loop();
  bool isConnected() const;
  int8_t getRSSI() const;
  String getIP() const;

 private:
  enum class State { kDisconnected, kConnecting, kConnected };

  const char* ssid_ = nullptr;
  const char* password_ = nullptr;
  State state_ = State::kDisconnected;
  uint32_t backoff_ms_ = 1000;
  uint32_t last_attempt_ms_ = 0;
  uint32_t boot_start_ms_ = 0;
  bool boot_timeout_logged_ = false;
};

#endif  // WIFI_MANAGER_H
