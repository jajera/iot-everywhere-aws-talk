#include "mqtt_manager.h"

#include "backoff_utils.h"
#include "logger.h"

namespace {
constexpr uint32_t kInitialBackoffMs = 1000;
constexpr uint32_t kMaxBackoffMs = 120000;
constexpr uint32_t kConnectTimeoutMs = 30000;
constexpr uint32_t kPublishAckTimeoutMs = 10000;
constexpr int kMqttPort = 8883;
constexpr int kMqttBufferSize = 512;

const char* describeMqttError(int err) {
  switch (err) {
    case 0:
      return "no error reported";
    default:
      return "transport/ack failure or broker rejection";
  }
}
}  // namespace

MQTTManager::MQTTManager() : client_(kMqttBufferSize) {}

void MQTTManager::begin(const char* endpoint, const char* thing_name, const char* ca, const char* cert, const char* key) {
  endpoint_ = endpoint;
  thing_name_ = thing_name;
  backoff_ms_ = kInitialBackoffMs;
  last_attempt_ms_ = 0;
  last_connected_ = false;

  secure_client_.setCACert(ca);
  secure_client_.setCertificate(cert);
  secure_client_.setPrivateKey(key);

  client_.begin(endpoint_, kMqttPort, secure_client_);
  client_.setKeepAlive(60);
  client_.setCleanSession(true);
  client_.setTimeout(kPublishAckTimeoutMs);
  // Attempt first connect immediately after Wi-Fi is available.
  last_attempt_ms_ = millis() - kInitialBackoffMs;
}

bool MQTTManager::loop(bool wifi_connected) {
  if (!wifi_connected) {
    if (client_.connected()) {
      client_.disconnect();
    }
    if (last_connected_) {
      Logger::log("mqtt", "disconnected (wifi unavailable)");
      last_connected_ = false;
    }
    return false;
  }

  if (client_.connected()) {
    client_.loop();
    if (!last_connected_) {
      Logger::log("mqtt", "connected");
      last_connected_ = true;
      backoff_ms_ = kInitialBackoffMs;
    }
    return true;
  }

  if (last_connected_) {
    Logger::log("mqtt", "disconnected");
    last_connected_ = false;
  }

  const uint32_t now = millis();
  if ((now - last_attempt_ms_) < backoff_ms_) {
    return false;
  }

  Logger::logf("mqtt", "connect attempt (backoff=%lums)", backoff_ms_);
  last_attempt_ms_ = now;

  bool connected = false;
  const uint32_t attempt_start = millis();
  while ((millis() - attempt_start) < kConnectTimeoutMs) {
    connected = client_.connect(thing_name_);
    if (connected) {
      break;
    }
    delay(200);
  }

  if (!connected) {
    const int err = client_.lastError();
    Logger::logf("mqtt", "connect failed err=%d desc=%s", err, describeMqttError(err));
    backoff_ms_ = BackoffUtils::next(backoff_ms_, kInitialBackoffMs, kMaxBackoffMs);
    return false;
  }

  Logger::log("mqtt", "connected");
  last_connected_ = true;
  backoff_ms_ = kInitialBackoffMs;
  return true;
}

bool MQTTManager::isConnected() const {
  return const_cast<MQTTClient&>(client_).connected();
}

bool MQTTManager::publish(const char* topic, const char* payload, int qos) {
  if (!client_.connected()) {
    return false;
  }

  if (!client_.publish(topic, payload, false, qos)) {
    const int err = client_.lastError();
    Logger::logf("mqtt", "publish failed err=%d desc=%s", err, describeMqttError(err));
    return false;
  }

  // The MQTT client publish call is bounded by setTimeout(10s), satisfying QoS1 ack timeout handling.
  return true;
}
