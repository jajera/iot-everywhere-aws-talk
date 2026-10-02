---
inclusion: always
---

# Product

`iot-everywhere-aws-talk` is talk material for a ~20 minute high-level AWS community session:

**IoT Is Everywhere: How One Device Talks to AWS**

## The story

Everyday connected devices share one idea: **sense → connect → cloud → act or observe**.
On stage we show that pattern with ESP32 boards already publishing over MQTT to **AWS IoT Core**,
and the room watches a pre-opened **Amplify** dashboard. The full hands-on lab lives elsewhere;
this repo is slides plus a thin demo backbone.

```text
Audience
   │
   ▼
Slidev deck (GitHub Pages)
   │
   ├── everyday IoT examples
   ├── one path to the cloud
   ├── identity + message shape
   └── live look → Amplify UI
            │
            ▼
     MQTT → IoT Core → thin API → Amplify
     (boards feed in background; no live console ops)
```

## Audience

AWS community attendees who may know cloud better than embedded. Keep the talk accessible
without talking down to people who already build devices.

## In scope

- Slidev slides + theme/styling
- Favicon / OG assets
- `demo/` firmware, thin AWS CLI cloud path, and Amplify-hosted dashboard UI
- GitHub Pages publish + markdown / commit-message hygiene workflows

## Out of scope

- Terraform stacks in this repository (prefer AWS CLI / console docs)
- Live AWS console operation during the talk
- Claiming a specific DevKit photo is an ESP32-C61 unless the asset is known accurate
- Deep seismic / GNSS / geomag how-tos

## Stage rules

- Show the Amplify dashboard for the live beat. Boards keep publishing in the background.
- Keep AWS console closed on stage (no live console ops).
- Desk board stays online; pass-around board may drop and reconnect.
- If Wi-Fi fails, the verbal story still stands.

## Related

Full lab: https://aws-iot-walkthrough.johna.kiwi/
