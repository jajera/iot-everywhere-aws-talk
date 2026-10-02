#ifndef TELEMETRY_PUBLISHER_H
#define TELEMETRY_PUBLISHER_H

#include <Arduino.h>

#include "mqtt_manager.h"
#include "ntp_sync.h"
#include "payload_codec.h"

class StatusLed;

class TelemetryPublisher {
 public:
  void begin(const char* thing_name, uint16_t interval_sec);
  void setStatusLed(StatusLed* led);
  void loop(MQTTManager& mqtt, const NTPSync& ntp);
  int buildPayload(char* buffer, size_t size, const NTPSync& ntp) const;

 private:
  const char* thing_name_ = nullptr;
  uint16_t interval_sec_ = 60;
  uint32_t last_publish_ms_ = 0;
  char topic_[160] = {0};
};

#endif  // TELEMETRY_PUBLISHER_H
