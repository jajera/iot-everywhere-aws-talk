#ifndef EVENT_PUBLISHER_H
#define EVENT_PUBLISHER_H

#include <Arduino.h>

#include "mqtt_manager.h"
#include "ntp_sync.h"
#include "payload_codec.h"

class EventPublisher {
 public:
  void begin(const char* thing_name);
  void loop(MQTTManager& mqtt, const NTPSync& ntp);
  int buildPayload(char* buffer, size_t size, const NTPSync& ntp) const;

 private:
  static void IRAM_ATTR onFallingEdge();
  void handlePress(MQTTManager& mqtt, const NTPSync& ntp);

  static EventPublisher* instance_;
  volatile bool pending_press_ = false;
  uint32_t last_press_ms_ = 0;
  const char* thing_name_ = nullptr;
  char topic_[160] = {0};
};

#endif  // EVENT_PUBLISHER_H
