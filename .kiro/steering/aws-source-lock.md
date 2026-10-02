---
inclusion: always
---

# AWS source lock

Training data about AWS IoT can be incomplete or stale. **Behavioural claims about AWS
services in this repository must trace to a source below** (or to a page linked from the
claim). If it is not sourced and not verified in an account, do not state it as fact.

## Canonical sources

| Topic | Source |
| --- | --- |
| AWS IoT Core | https://docs.aws.amazon.com/iot/latest/developerguide/what-is-aws-iot.html |
| MQTT on IoT Core | https://docs.aws.amazon.com/iot/latest/developerguide/mqtt.html |
| Device certificates | https://docs.aws.amazon.com/iot/latest/developerguide/x509-client-certs.html |
| Thing registry | https://docs.aws.amazon.com/iot/latest/developerguide/iot-thing-management.html |
| IoT policies | https://docs.aws.amazon.com/iot/latest/developerguide/iot-policies.html |
| IoT rules | https://docs.aws.amazon.com/iot/latest/developerguide/iot-rules.html |
| Device Shadow | https://docs.aws.amazon.com/iot/latest/developerguide/iot-device-shadows.html |
| IoT Core endpoints | https://docs.aws.amazon.com/general/latest/gr/iot-core.html |
| IoT pricing | https://aws.amazon.com/iot-core/pricing/ |
| ESP-IDF MQTT (device side) | https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/mqtt.html |

Use the `aws-docs` MCP server to re-read these pages rather than recalling them.

## Safe high-level facts for this talk

1. Devices can publish and subscribe with **MQTT** to **AWS IoT Core** using mutual TLS
   (X.509) when configured with a Thing, certificate, and policy.
2. Topic design and rules can fan messages into other AWS services; exact rule SQL and
   targets belong in the lab docs, not as improvised slide claims.
3. Point learners to https://aws-iot-walkthrough.johna.kiwi/ for the full procedure.

## Do not invent

- Throughput, latency, or pricing numbers without a cited page
- Region availability of niche IoT features without checking docs
- That the on-stage board “is” a seismic / GNSS instrument
