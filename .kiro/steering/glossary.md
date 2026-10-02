---
inclusion: fileMatch
fileMatchPattern: ["demo/**", "slides.md", "README.md"]
---

# Glossary conventions

Use plain language on slides. When a term repeats across `demo/` and slides, prefer these
meanings:

| Term | Meaning here |
| --- | --- |
| MQTT | Publish/subscribe messaging used from the device to AWS IoT Core |
| AWS IoT Core | Managed broker / device gateway for MQTT (and related IoT features) |
| Thing | IoT Core registry identity for a device |
| Onboard | Create Thing, attach certificate and policy so the device can connect |
| Telemetry | Periodic device readings (for this demo: connectivity / chip temp style fields) |
| Desk board | Stationary board that should stay online during the talk |
| Pass-around board | Handheld board that may drop Wi-Fi when moved |

Do not invent alternate definitions mid-deck.
