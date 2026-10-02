#ifndef PAYLOAD_CODEC_H
#define PAYLOAD_CODEC_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <ArduinoJson.h>

struct TelemetryData {
  const char* device_id;
  uint32_t ts;
  int rssi;
  uint32_t uptime_s;
  uint32_t heap_free;
  float chip_temp_c;
  char chip_model[16];
  uint16_t cpu_mhz;
  uint32_t flash_bytes;
  char wifi_ssid[33];
  int8_t wifi_status;
  int8_t wifi_channel;
  char wifi_ip[16];
  char wifi_gateway[16];
  char wifi_dns[16];
  int32_t clock_offset_ms;
};

struct EventData {
  const char* device_id;
  uint32_t ts;
};

inline void copyField(char* dst, size_t dst_size, const char* src) {
  if (dst_size == 0) {
    return;
  }
  if (src == nullptr) {
    dst[0] = '\0';
    return;
  }
  strncpy(dst, src, dst_size - 1);
  dst[dst_size - 1] = '\0';
}

inline int serializeTelemetryJson(const TelemetryData& data, char* out, size_t out_size) {
  JsonDocument doc;
  doc["device_id"] = data.device_id;
  doc["ts"] = data.ts;
  doc["type"] = "connectivity";
  doc["rssi"] = data.rssi;
  doc["uptime_s"] = data.uptime_s;
  doc["heap_free"] = data.heap_free;
  doc["chip_temp_c"] = data.chip_temp_c;
  doc["chip_model"] = data.chip_model;
  doc["cpu_mhz"] = data.cpu_mhz;
  doc["flash_bytes"] = data.flash_bytes;
  doc["wifi_ssid"] = data.wifi_ssid;
  doc["wifi_status"] = data.wifi_status;
  doc["wifi_channel"] = data.wifi_channel;
  doc["wifi_ip"] = data.wifi_ip;
  doc["wifi_gateway"] = data.wifi_gateway;
  doc["wifi_dns"] = data.wifi_dns;
  doc["clock_offset_ms"] = data.clock_offset_ms;
  return static_cast<int>(serializeJson(doc, out, out_size));
}

inline int serializeEventJson(const EventData& data, char* out, size_t out_size) {
  JsonDocument doc;
  doc["device_id"] = data.device_id;
  doc["ts"] = data.ts;
  doc["type"] = "button";
  doc["event"] = "press";
  return static_cast<int>(serializeJson(doc, out, out_size));
}

#endif  // PAYLOAD_CODEC_H
