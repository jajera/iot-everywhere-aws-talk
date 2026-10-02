# Payload contract

Aligned with the AWS IoT walkthrough so the full lab remains a valid “try later”.

## Topics

- Telemetry: `devices/{Thing_Name}/telemetry`
- Events: `devices/{Thing_Name}/events`

## Base fields

Present on every message:

- `device_id` (string) — matches Thing name
- `ts` (integer) — UTC epoch seconds, or `0` if NTP unavailable
- `type` (string) — message discriminator

## Telemetry (`type`: `connectivity`)

```json
{
  "device_id": "esp32-c61-01",
  "ts": 1700000000,
  "type": "connectivity",
  "rssi": -67,
  "uptime_s": 3600,
  "heap_free": 180000,
  "chip_temp_c": 41.2,
  "chip_model": "ESP32-C61",
  "cpu_mhz": 160,
  "flash_bytes": 4194304,
  "wifi_ssid": "demo-ssid",
  "wifi_status": 3,
  "wifi_channel": 6,
  "wifi_ip": "192.168.1.42",
  "wifi_gateway": "192.168.1.1",
  "wifi_dns": "8.8.8.8",
  "clock_offset_ms": 125
}
```

Dashboard tiles use: `rssi`, `chip_temp_c`, `heap_free`, `uptime_s`. Extra fields feed the detail panel.

## Event (`type`: `button`)

```json
{
  "device_id": "esp32-c61-01",
  "ts": 1700000001,
  "type": "button",
  "event": "press"
}
```

Boot-button presses publish when firmware is flashed with the event publisher enabled (GPIO0).
