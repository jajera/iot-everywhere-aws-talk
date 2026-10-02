#include "event_publisher.h"

#include "debounce_utils.h"
#include "logger.h"

namespace {
constexpr int kBootButtonPin = 0;
constexpr uint32_t kDebounceMs = 300;
constexpr size_t kPayloadMaxBytes = 128;
}  // namespace

EventPublisher* EventPublisher::instance_ = nullptr;

void EventPublisher::begin(const char* thing_name) {
  thing_name_ = thing_name;
  snprintf(topic_, sizeof(topic_), "devices/%s/events", thing_name_);
  pinMode(kBootButtonPin, INPUT_PULLUP);
  instance_ = this;
  attachInterrupt(digitalPinToInterrupt(kBootButtonPin), onFallingEdge, FALLING);
}

void IRAM_ATTR EventPublisher::onFallingEdge() {
  if (instance_ != nullptr) {
    instance_->pending_press_ = true;
  }
}

int EventPublisher::buildPayload(char* buffer, size_t size, const NTPSync& ntp) const {
  const EventData data = {
      .device_id = thing_name_,
      .ts = ntp.isSynced() ? ntp.getEpoch() : 0,
  };
  return serializeEventJson(data, buffer, size);
}

void EventPublisher::handlePress(MQTTManager& mqtt, const NTPSync& ntp) {
  const uint32_t now = millis();
  if (!DebounceUtils::shouldAcceptPress(now, last_press_ms_, kDebounceMs)) {
    return;
  }
  last_press_ms_ = now;

  if (!mqtt.isConnected()) {
    Logger::log("event", "MQTT unavailable");
    return;
  }

  char payload[kPayloadMaxBytes] = {0};
  const int payload_len = buildPayload(payload, sizeof(payload), ntp);
  if (payload_len <= 0 || payload_len > static_cast<int>(kPayloadMaxBytes)) {
    Logger::log("event", "payload serialization failed or overflow");
    return;
  }

  if (mqtt.publish(topic_, payload, 1)) {
    Logger::logf("event", "published topic=%s payload=%s", topic_, payload);
  } else {
    Logger::log("event", "publish failed");
  }
}

void EventPublisher::loop(MQTTManager& mqtt, const NTPSync& ntp) {
  if (!pending_press_) {
    return;
  }
  noInterrupts();
  pending_press_ = false;
  interrupts();

  handlePress(mqtt, ntp);
}
