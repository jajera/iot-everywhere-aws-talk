# Demo backbone

Prep for the live talk visual. On stage you **show the Amplify dashboard**; boards publish in the background.

Full lab (deeper): https://aws-iot-walkthrough.johna.kiwi/

## Layout

| Path | Purpose |
| --- | --- |
| [`aws/`](aws/) | Thing/cert, ingest, query API, Amplify host notes |
| [`firmware/`](firmware/) | PlatformIO feeder — **C61 hardware baseline** |
| `web/` | Static dashboard (build + host per [`aws/README.md`](aws/README.md)) |
| [`PAYLOAD.md`](PAYLOAD.md) | MQTT topics + JSON contract |
| [`../docs/`](../docs/) | Architecture diagram (draw.io + exports) |

## Prep (before the talk)

1. **Cloud** — follow [`aws/README.md`](aws/README.md) (identity → ingest/query → note query URL).
2. **Firmware** — PlatformIO Core (`pio`), C61 on USB, gitignored `config.h` / `certs.h`. Follow the **C61 baseline** in [`firmware/README.md`](firmware/README.md) (ACM port, `huge_app` erase, LED SPI+DMA). Stage checklist: [`aws/README.md` §3](aws/README.md#3-firmware).
3. **Dashboard** — build/host `web/` with `VITE_API_URL` (steps in [`aws/README.md`](aws/README.md#amplify-hosting-console)).
4. **Stage** — boards on Wi‑Fi; Amplify tab open; slides in another tab; no live console ops.

If Wi‑Fi drops, the verbal story still stands; the dashboard may show Stale.

## Hardware

- **ESP32-C61-DevKitC-1** (primary) + USB **data** cable → typically `/dev/ttyACM0` (Espressif `303a`). CH340 `/dev/ttyUSB*` is usually classic ESP32 — wrong env for this talk repo (use fleet hub `ideaspark-oled` / Thing `ideaspark-oled-01`).
- This talk demo stays **one device** (`esp32-c61-01` on `devices/…`). Extra boards belong in the fleet hub: https://github.com/jajera/esp32-aws-iot-fleet.
- Optional desk board: `pio run -e esp32-s3-n16r8 -t upload` (same payload contract; has RMT).
- Tooling: PlatformIO Core (`pio --version`); see [`aws/README.md` §3](aws/README.md#3-firmware).
- **C61 quirks (baseline):** no RMT → `led_strip` SPI+DMA on GPIO8; hybrid image needs `partitions/huge_app.csv` + erase on first/layout change; USB power LED is hardwired red — ignore it. Full detail: [`firmware/README.md`](firmware/README.md).

## Deploy walkthrough (slideshow)

Deploy beats live in the **main Slidev deck** after **Architecture** (`slides.md`).

```bash
# from repo root
npm run dev
```

Full command detail stays in [`aws/README.md`](aws/README.md).

## Related

- [`firmware/README.md`](firmware/README.md) — C61 first-flash baseline (reuse for other ESP32 work)
- [`aws/README.md`](aws/README.md) — CLI scripts, console alternatives, Amplify hosting, tear-down
- [`PAYLOAD.md`](PAYLOAD.md) — telemetry / events shape
- Fleet hub (multi-model): https://github.com/jajera/esp32-aws-iot-fleet — Ideaspark, S3, C3, CAM (+ fleet-side C61 on `fleet/…`)
- Upstream pattern: https://github.com/jajera/esp32-aws-iot-demo
