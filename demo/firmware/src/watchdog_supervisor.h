#ifndef WATCHDOG_SUPERVISOR_H
#define WATCHDOG_SUPERVISOR_H

#include <Arduino.h>

class WatchdogSupervisor {
 public:
  void begin(uint32_t timeout_sec);
  void feed();
  void noteDualConnected();
  void checkConnectivityTimeout(bool wifi_ok, bool mqtt_ok);
  void logResetReason() const;

 private:
  uint32_t timeout_ms_ = 30000;
  uint32_t last_dual_connected_ms_ = 0;
  bool initialized_ = false;
};

#endif  // WATCHDOG_SUPERVISOR_H
