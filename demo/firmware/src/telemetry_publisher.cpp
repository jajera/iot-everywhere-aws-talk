#include "telemetry_publisher.h"

#include <WiFi.h>

#include "interval_utils.h"
#include "logger.h"
#include "status_led.h"

namespace {
StatusLed* g_status_led = nullptr;
}  // namespace

void TelemetryPublisher::setStatusLed(StatusLed* led) { g_status_led = led; }

namespace {
constexpr size_t kPayloadMaxBytes = 512;
constexpr uint32_t kToleranceSec = 5;

void copyIp(char* dst, size_t len, const String& ip) { ip.toCharArray(dst, len); }
}  // namespace

void TelemetryPublisher::begin(const char* thing_name, uint16_t interval_sec) {
  thing_name_ = thing_name;
  interval_sec_ = interval_sec;
  last_publish_ms_ = millis();
  snprintf(topic_, sizeof(topic_), "devices/%s/telemetry", thing_name_);
}

int TelemetryPublisher::buildPayload(char* buffer, size_t size, const NTPSync& ntp) const {
  TelemetryData data {};
  data.device_id = thing_name_;
  data.ts = ntp.isSynced() ? ntp.getEpoch() : 0;
  data.rssi = static_cast<int>(WiFi.RSSI());
  data.uptime_s = millis() / 1000;
  data.heap_free = ESP.getFreeHeap();
  data.chip_temp_c = temperatureRead();
  copyField(data.chip_model, sizeof(data.chip_model), ESP.getChipModel());
  data.cpu_mhz = static_cast<uint16_t>(ESP.getCpuFreqMHz());
  data.flash_bytes = ESP.getFlashChipSize();
  copyField(data.wifi_ssid, sizeof(data.wifi_ssid), WiFi.SSID().c_str());
  data.wifi_status = static_cast<int8_t>(WiFi.status());
  data.wifi_channel = static_cast<int8_t>(WiFi.channel());
  copyIp(data.wifi_ip, sizeof(data.wifi_ip), WiFi.localIP().toString());
  copyIp(data.wifi_gateway, sizeof(data.wifi_gateway), WiFi.gatewayIP().toString());
  copyIp(data.wifi_dns, sizeof(data.wifi_dns), WiFi.dnsIP().toString());
  data.clock_offset_ms = ntp.getClockOffsetMs();

  return serializeTelemetryJson(data, buffer, size);
}

void TelemetryPublisher::loop(MQTTManager& mqtt, const NTPSync& ntp) {
  const uint32_t now = millis();
  if (!IntervalUtils::shouldPublish(now, last_publish_ms_, interval_sec_)) {
    return;
  }

  const uint32_t elapsed = now - last_publish_ms_;
  if (!IntervalUtils::withinTolerance(elapsed, interval_sec_, kToleranceSec) &&
      elapsed < (interval_sec_ + kToleranceSec) * 1000UL) {
    return;
  }

  if (!mqtt.isConnected()) {
    Logger::log("telemetry", "mqtt unavailable, skipping publish");
    last_publish_ms_ = now;
    return;
  }

  char payload[kPayloadMaxBytes] = {0};
  const int payload_len = buildPayload(payload, sizeof(payload), ntp);
  if (payload_len <= 0 || payload_len > static_cast<int>(kPayloadMaxBytes)) {
    Logger::log("telemetry", "payload serialization failed or overflow");
    last_publish_ms_ = now;
    return;
  }

  if (mqtt.publish(topic_, payload, 1)) {
    Logger::logf("telemetry", "published topic=%s bytes=%d", topic_, payload_len);
    if (g_status_led != nullptr) {
      g_status_led->flashTelemetrySent();
    }
  } else {
    Logger::log("telemetry", "publish failed");
  }
  last_publish_ms_ = now;
}
