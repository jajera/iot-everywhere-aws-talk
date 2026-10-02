# Cloud path (no Terraform)

Thin AWS path for the talk demo: Thing identity → IoT rule → ingest → DynamoDB → query Function URL → Amplify UI.

Default region: `ap-southeast-2` (`AWS_REGION` overrides). Use `AWS_PROFILE` as needed (example: `sandbox`).

Generated files under `out/` and `firmware/include/certs.h` / `config.h` are gitignored — they hold **sensitive content** (device private keys, Wi‑Fi password, cert PEMs). Do not force-add them. Do not commit private keys or device PEMs.

## Stages

1. [Identity](#1-identity)
2. [Ingest + query](#2-ingest--query)
3. [Firmware](#3-firmware) (timing in [../README.md](../README.md))
4. [Amplify UI](#4-amplify-ui)

In the talk deck (`slides.md`): one **Deploy** overview slide in the main flow, and slides A1–A4 in the appendix after **Thanks**.

---

## 1. Identity

```bash
cd demo/aws
export AWS_PROFILE=sandbox
export AWS_REGION=ap-southeast-2
export THING_NAME=esp32-c61-01
./provision-thing.sh
```

Optional: `POLICY_NAME`, `OUT_DIR`.

### Sample output

```text
Region=ap-southeast-2 Thing=esp32-c61-01 Out=.../demo/aws/out/esp32-c61-01
Created Thing esp32-c61-01
Created certificate arn:aws:iot:ap-southeast-2:<account-id>:cert/<cert-id>
Created policy iot-talk-esp32-c61-01-policy
Wrote .../demo/aws/out/esp32-c61-01/certs.h
Copied certs.h → .../demo/firmware/include/certs.h

Next:
  1. Copy config.example.h → config.h and set WIFI_* , THING_NAME=esp32-c61-01,
     AWS_IOT_ENDPOINT=<iot-data-ats-endpoint>
  2. Run ./deploy-stack.sh (ingest + query)
  3. Flash firmware and open the Amplify dashboard
```

### Files written (`out/esp32-c61-01/`, all gitignored)

| File | Notes |
| --- | --- |
| `device.pem.crt` | Device certificate |
| `device.private.key` | Secret — never commit |
| `device.public.key` | Device public key |
| `AmazonRootCA1.pem` | Amazon Root CA 1 |
| `certificate-arn.txt` | `arn:aws:iot:ap-southeast-2:<account-id>:cert/<cert-id>` |
| `iot-endpoint.txt` | Data-ATS endpoint for `config.h` |
| `certs.h` | Also copied to `firmware/include/certs.h` |

Re-run is safe if `out/<thing>/` is kept (reuses the same certificate).

<details>
<summary>Console alternative (identity)</summary>

1. IoT Core → Manage → Things → Create thing.
2. Auto-generate certificate → Activate → Attach a policy that allows `iot:Connect` for the client id and `iot:Publish` on `devices/<thing>/telemetry` and `…/events`.
3. Download device cert, private key, and Amazon Root CA 1.
4. Build `firmware/include/certs.h` from those PEMs (same shape as `certs.example.h`), or re-run `provision-thing.sh` after deleting `aws/out/<thing>/` if you want the script to own the files.

</details>

---

## 2. Ingest + query

Creates DynamoDB tables, ingest/query Lambdas (`python3.14`), IoT rules, and a public query Function URL. Ends with built-in validation (tables, GSI, runtime, rules, HTTP, ingest smoke → query 200).

```bash
cd demo/aws
export AWS_PROFILE=sandbox
export AWS_REGION=ap-southeast-2
export PREFIX=iot-talk
export THING_NAME=esp32-c61-01
./deploy-stack.sh
```

Override runtime if needed: `PYTHON_RUNTIME=python3.14` (default; latest GA — not preview `python3.15`).

### Sample output

```text
Region=ap-southeast-2 Prefix=iot-talk Runtime=python3.14
Table iot-talk-telemetry exists
Table iot-talk-events exists
Role iot-talk-lambda-role exists
Updated Lambda iot-talk-ingest
Updated Lambda iot-talk-query
Updated rule iot_talk_telemetry
Updated rule iot_talk_events

=== Validation ===
OK  table iot-talk-telemetry ACTIVE
OK  table iot-talk-events ACTIVE
OK  GSI device_ts_idx on iot-talk-telemetry
OK  GSI device_ts_idx on iot-talk-events
OK  Lambda iot-talk-ingest runtime python3.14
OK  Lambda iot-talk-query runtime python3.14
OK  IoT rule iot_talk_telemetry
OK  IoT rule iot_talk_events
OK  query Function URL OPTIONS (204)
OK  query telemetry route (200)
OK  ingest smoke invoke
OK  query returns smoke telemetry for esp32-c61-01

Validation PASSED

Query API base URL:
  https://<function-url-id>.lambda-url.ap-southeast-2.on.aws

Dashboard routes:
  GET https://<function-url-id>.lambda-url.ap-southeast-2.on.aws/fleet/analytics?window=3600
  GET https://<function-url-id>.lambda-url.ap-southeast-2.on.aws/devices
  GET https://<function-url-id>.lambda-url.ap-southeast-2.on.aws/devices/{Thing}/telemetry/latest
  GET https://<function-url-id>.lambda-url.ap-southeast-2.on.aws/devices/{Thing}/events?limit=10

Next: stage 3 firmware (see demo/aws/README.md)
```

Query URL is also written to `out/query-url.txt` (gitignored). Use it as `VITE_API_URL` in stage 4.

**Console trap:** DynamoDB `describe-table` **ItemCount** can show `0` while items exist. Prefer the deploy validation above, a query on GSI `device_ts_idx` for `device_id = esp32-c61-01`, or the Amplify dashboard.

---

## 3. Firmware

Short overview: [../README.md](../README.md).  
**C61 hardware baseline (ports, partitions, LED, erase):** [../firmware/README.md](../firmware/README.md).  
Payload contract: [../PAYLOAD.md](../PAYLOAD.md).

### Prerequisites (do not skip)

Stage 3 is **local device work**. Stages 1–2 alone are not enough.

Documented and verified path for now: **Ubuntu**. Windows, Mac and Fedora are planned later (serial ports, drivers, and group names differ — do not assume these Ubuntu steps apply unchanged).

| Need | Why | How to confirm (Ubuntu) |
| --- | --- | --- |
| **PlatformIO Core** (`pio`) | Build + upload; this repo does **not** install it | `pio --version` (expect Core 6.x). If missing: [platformio.org/install/cli](https://platformio.org/install/cli) (or VS Code/Cursor PlatformIO IDE). First run may download the Espressif platform + toolchain (large). |
| **Python 3** | PlatformIO’s env / installer | `python3 --version` |
| **ESP32-C61-DevKitC-1** + USB data cable | Primary env `esp32-c61` | Board powered; cable carries data (not charge-only) |
| **C61 on `/dev/ttyACM*`** | Native USB-Serial/JTAG (`303a:`). CH340 `/dev/ttyUSB*` is Ideaspark OLED / classic ESP32 — use fleet `ideaspark-oled` | `ls /dev/ttyACM*`; `lsusb` shows Espressif. See [firmware README](../firmware/README.md#boards) |
| **Serial permission** | Otherwise “Permission denied” on port | User in `dialout` (often `plugdev` too): `groups`. After `sudo usermod -aG dialout $USER`, **re-login**. |
| **Stage 1 done** | Thing + `certs.h` + IoT endpoint | `demo/aws/out/<thing>/iot-endpoint.txt` and `demo/firmware/include/certs.h` exist |
| **Stage 2 done** | Rules + ingest so MQTT lands in DynamoDB | Stage 2 validation passed earlier |
| **Local `config.h`** | Wi‑Fi + endpoint + thing name | Copy from `config.example.h`; fill `WIFI_*`, `THING_NAME`, `AWS_IOT_ENDPOINT` |
| **Wi‑Fi the board can join** | Device must reach the IoT data endpoint on the internet | SSID/password correct; AP allows client internet (captive portal / client isolation will break MQTT) |

**Sensitive / gitignored:** `firmware/include/config.h` (Wi‑Fi password + endpoint) and `firmware/include/certs.h` (device key material) are gitignored on purpose. Only the `*.example.h` templates are tracked. Never force-add the real files.

**Not assumed:** board plugged in, PlatformIO already installed, or Wi‑Fi reachable from the device. Confirm the table above before upload.

### C61 first flash (baseline)

The hybrid Arduino+ESP-IDF image needs the **`huge_app`** partition table. On a **new board**, or whenever `partitions/` / that setting changes, **erase before upload**. Full rationale (LED SPI+DMA, CDC, ACM vs CH340): [../firmware/README.md](../firmware/README.md).

```bash
cd demo/firmware
cp include/config.example.h include/config.h
# edit config.h: WIFI_SSID / WIFI_PASSWORD, THING_NAME, AWS_IOT_ENDPOINT
#   (endpoint from ../aws/out/<thing>/iot-endpoint.txt; certs.h from stage 1)

pio run -e esp32-c61                 # build only (no board required)
pio run -e esp32-c61 -t erase        # first flash / after partition change
pio run -e esp32-c61 -t upload       # needs /dev/ttyACM*
pio device monitor -e esp32-c61      # 115200 — expect LED white→R→G→B then Wi‑Fi/MQTT
```

Optional desk board: `-e esp32-s3-n16r8` (same payload contract; has RMT — different LED path).

### Sample output

Ubuntu, ESP32-C61 on `/dev/ttyACM0` (Espressif USB JTAG/serial). First build on a clean machine can take several minutes (Espressif platform + C61 Arduino IDF libs).

Upload (trimmed):

```text
Writing at 0x0012b260 [██████████████████████████████] 100.0% …
Wrote … bytes (…) at 0x00010000 in … seconds.
Hard resetting via RTS pin...
========================= [SUCCESS] Took 18.03 seconds =========================
Environment    Status    Duration
-------------  --------  ------------
esp32-c61      SUCCESS   00:00:18.032
```

Serial (`pio device monitor` / ACM @ 115200) after USB CDC flags in `platformio.ini`:

```text
ESP-ROM:esp32c61-eco3-20250228
…
entry 0x4083bd70
[513][main] boot
[515][led] rgb ready (led_strip SPI v3 gpio=8, dma=1)
[515][led] boot WHITE 2s — addressable LED near GPIO8
[2515][led] boot RED 2s
[4515][led] boot GREEN 2s
[6515][led] boot BLUE 2s
[8586][wifi] attempting connect (backoff=1000ms)
[9768][wifi] connected ip=192.170.32.191
[9768][ntp] sync started server=pool.ntp.org
[9769][ntp] synced utc=2026-09-22T19:08:41Z offset_ms=0
[9769][mqtt] connect attempt (backoff=1000ms)
[12002][mqtt] connected
[23646][telemetry] published topic=devices/esp32-c61-01/telemetry bytes=345
```

**Pass criteria:** `dma=1` in the LED line; addressable LED shows white→R→G→B (not the hardwired USB red); MQTT connected; telemetry publish; stage 2 query URL returns live JSON for the Thing.

Cloud check: `GET …/devices/esp32-c61-01/telemetry/latest` → 200 with current `wifi_ssid` / `uptime_s`.

If LED is dark or blue-only, or boot fails after upload, see [firmware troubleshooting](../firmware/README.md#troubleshooting).

Next: stage 4 Amplify UI.

---

## 4. Amplify UI

```bash
cd demo/web
npm ci
VITE_API_URL="$(cat ../aws/out/query-url.txt)" npm run build
```

The dashboard reads one route, `GET /fleet/analytics?window=<seconds>`, where the window is `3600`, `21600` or `86400`. For every device it returns:

- The latest telemetry and a health score out of 100
- Delivery rate against the expected 15 s interval, gap count and longest gap
- Reboots (detected when `uptime_s` resets) with the `reset_reason` code
- RSSI minimum, maximum, average, median, worst-10% (p10), standard deviation and percent of samples below -75 dBm
- Chip temperature minimum, average, maximum and trend per hour
- Free heap now, minimum, low-water mark and trend in bytes per hour
- Bucketed series for charts: 1 min buckets for 1 h, 5 min for 6 h, 15 min for 24 h
- Optional latest camera frame URL (fleet CAM only)

It also returns all devices' events for the window, newest first. Warm Lambda
instances cache each window (`ANALYTICS_CACHE_TTL_*`, defaults 25 s / 60 s / 120 s
for 1 h / 6 h / 24 h). The page polls every 30 s, 60 s or 120 s depending on the
window.

UI behaviour: one host selected at a time (row click). Charts and events follow that host. The camera panel appears only when the selected host has a frame (e.g. `esp32-cam-01`); C61 and other boards leave it hidden.

Cost note: each refresh queries every device's telemetry for the whole window. A 24 h window at 15 s intervals reads about 5,760 items per device per refresh, and DynamoDB on-demand read charges apply. For windows longer than 24 h, pre-aggregate into another store (for example Timestream or S3 with Athena) rather than widening this route. Check current [DynamoDB pricing](https://aws.amazon.com/dynamodb/pricing/).

Then host `dist/` with Amplify Hosting (manual zip deploy, no Git):

```bash
# create app + branch once
aws amplify create-app --name iot-talk-dashboard --platform WEB --region ap-southeast-2
aws amplify create-branch --app-id <app-id> --branch-name main --region ap-southeast-2

# zip dist contents (files at zip root), upload, start job
(cd dist && zip -r /tmp/iot-talk-amplify.zip .)
# create-deployment → PUT zip to zipUploadUrl → start-deployment → wait SUCCEED
```

Console alternative: **Amplify** → **Host web app** → **Deploy without Git** → upload `demo/web/dist`.

On stage: Amplify tab open; do not operate the AWS console live. URL is also written to `out/amplify-url.txt` (gitignored).

### Sample output

```text
vite v5.4.x building for production...
✓ 5 modules transformed.
dist/index.html                 0.52 kB │ gzip: 0.32 kB
dist/assets/index-….css         ~3 kB
dist/assets/index-….js          ~6 kB
✓ built in ~100ms

Amplify job status: SUCCEED
Dashboard: https://main.<app-id>.amplifyapp.com

Smoke checks:
  GET https://main.<app-id>.amplifyapp.com/ → 200
  GET <query-url>/devices/esp32-c61-01/telemetry/latest → 200 (live board data)
```

---

## Tear-down (optional)

Delete IoT rules, Lambdas, DynamoDB tables, IAM role/policies, Thing/cert/policy, and the Amplify app (`out/amplify-app-id.txt`) when finished. Prefix defaults to `iot-talk`.
