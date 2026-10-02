# Firmware (talk demo feeder)

PlatformIO sketch that publishes MQTT telemetry/events to AWS IoT Core for the
live Amplify dashboard. **Primary board: ESP32-C61-DevKitC-1.**

Cloud / Thing / Amplify steps: [`../aws/README.md`](../aws/README.md).  
Payload contract: [`../PAYLOAD.md`](../PAYLOAD.md).

This file is the **C61 hardware baseline** for onboarding and for other jajera
ESP32 work that needs the same board quirks.

---

## Boards

| Env | Board | Port (typical) | Notes |
| --- | --- | --- | --- |
| `esp32-c61` (**default**) | ESP32-C61-DevKitC-1 | `/dev/ttyACM0` | Espressif USB-Serial/JTAG; hybrid Arduino+ESP-IDF |
| `esp32-s3-n16r8` | ESP32-S3-DevKitC-1 (16MB) | often `/dev/ttyACM0` | Arduino only; has RMT → `rgbLedWrite` |

**Do not confuse ports:**

| You see | Likely board |
| --- | --- |
| `/dev/ttyACM*` + `303a:` (Espressif) | C61 / S3 native USB |
| `/dev/ttyUSB*` + CH340 (`1a86:7523`) | Ideaspark ESP32 OLED-0.96 V3.0 (or similar classic ESP32) — **not** this talk env; use fleet `ideaspark-oled` / Thing `ideaspark-oled-01` in `esp32-aws-iot-fleet` |

Confirm:

```bash
ls -l /dev/ttyACM* /dev/ttyUSB* 2>/dev/null
lsusb | rg -i 'espressif|303a|1a86|cp210|ch340'
# optional:
# pio device list
# ~/.platformio/penv/bin/python ~/.platformio/packages/tool-esptoolpy/esptool.py --port /dev/ttyACM0 chip-id
```

---

## C61 baseline (must follow)

These are not optional footnotes — they are required for a working first flash.

### 1. Framework and LED

- C61 has **no RMT**. Stock Arduino `rgbLedWrite` / NeoPixel paths **do not** drive the onboard WS2812.
- Env uses `framework = arduino, espidf` plus `src/idf_component.yml` → `espressif/led_strip` **^3** on **SPI2 + DMA**, GPIO8, GRB.
- **SPI2 is reserved for the LED** in this sketch (do not also attach SPI devices on SPI2).
- Hardwired **USB power LED is red and always on** — ignore it. Watch the **addressable** LED near GPIO8.
- Without DMA, USB/Wi‑Fi IRQs can insert gaps mid-frame → LED often shows **blue only** (early G/R bytes dropped). Keep `with_dma = true` in `status_led.cpp`.

### 2. Partition table (`huge_app`)

- Hybrid Arduino+IDF image is **>1MB**. Default factory slot is too small.
- Env pins `board_build.partitions = partitions/huge_app.csv` (~3MB app0).
- **First flash** (or any time you change the partition table): erase, then upload:

```bash
cd demo/firmware
pio run -e esp32-c61 -t erase
pio run -e esp32-c61 -t upload
```

Skipping erase after a partition change can brick boots or leave a stale layout.

### 3. USB CDC serial

`platformio.ini` sets `ARDUINO_USB_CDC_ON_BOOT` / `ARDUINO_USB_MODE` so `Serial` appears on `/dev/ttyACM*`. Without those flags, logs go to UART pins and the ACM port looks “dead”.

### 4. Expected boot (visual + serial)

On reset, addressable LED should show roughly:

**white 2s → red 2s → green 2s → blue 2s** → off, then Wi‑Fi / MQTT.

Serial @ 115200 (trimmed):

```text
[…][main] boot
[…][led] rgb ready (led_strip SPI v3 gpio=8, dma=1)
[…][led] boot WHITE 2s — addressable LED near GPIO8
[…][led] boot RED 2s
[…][led] boot GREEN 2s
[…][led] boot BLUE 2s
[…][wifi] connected ip=…
[…][ntp] synced …
[…][mqtt] connected
[…][telemetry] published topic=devices/esp32-c61-01/telemetry bytes=…
```

Each successful telemetry publish flashes **blue** briefly on the addressable LED.

### 5. Secrets (gitignored)

```bash
cp include/config.example.h include/config.h
# WIFI_*, THING_NAME, AWS_IOT_ENDPOINT from stage 1
# certs.h from demo/aws/provision-thing.sh (never commit)
```

---

## Commands

```bash
cd demo/firmware

pio run -e esp32-c61                 # build only
pio run -e esp32-c61 -t erase        # first flash / after partition change
pio run -e esp32-c61 -t upload
pio device monitor -e esp32-c61      # 115200 on ACM
```

Optional S3 desk board (same MQTT contract):

```bash
pio run -e esp32-s3-n16r8 -t upload
```

---

## Troubleshooting

| Symptom | Check |
| --- | --- |
| No `/dev/ttyACM*` | Data cable, board power, `lsusb` for Espressif `303a` |
| Permission denied | User in `dialout`; re-login after `usermod` |
| Upload OK, no serial logs | CDC flags; open the ACM port at 115200 |
| Boot loop / app won’t start after upload | Erase + reflash (`huge_app`); do not use default 1MB factory |
| LED dark or “blue only” | Confirm `dma=1` in log; watch addressable LED not USB red; SPI2 free |
| Wi‑Fi OK, MQTT fail | Endpoint + certs + policy; AP must allow internet (no captive portal) |
| Cloud empty | Stage 2 rules/ingest; query URL; Thing name matches `device_id` |

---

## Layout

| Path | Role |
| --- | --- |
| `platformio.ini` | Envs, CDC flags, `huge_app`, lib deps |
| `partitions/huge_app.csv` | Large app0 for hybrid image |
| `src/idf_component.yml` | `espressif/led_strip` |
| `src/status_led.*` | C61 SPI+DMA LED; S3 uses `rgbLedWrite` |
| `include/config.example.h` | Wi‑Fi / endpoint template |
| `include/certs.example.h` | Cert placeholders |

---

## Related

- Fleet hub (multi-model): https://github.com/jajera/esp32-aws-iot-fleet
  - Desk C61 on `fleet/…`: env `esp32-c61`, Thing `esp32-c61-01`
  - Ideaspark OLED / TFT, S3, C3, CAM — see that repo’s `docs/boards/`
- Full lab docs: https://aws-iot-walkthrough.johna.kiwi/
