#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <Arduino.h>
#include <MQTT.h>
#include <WiFiClientSecure.h>

class MQTTManager {
 public:
  MQTTManager();

  void begin(const char* endpoint, const char* thing_name, const char* ca, const char* cert, const char* key);
  bool loop(bool wifi_connected);
  bool isConnected() const;
  bool publish(const char* topic, const char* payload, int qos);

 private:
  const char* endpoint_ = nullptr;
  const char* thing_name_ = nullptr;
  uint32_t backoff_ms_ = 1000;
  uint32_t last_attempt_ms_ = 0;
  bool last_connected_ = false;

  WiFiClientSecure secure_client_;
  MQTTClient client_;
};

#endif  // MQTT_MANAGER_H
