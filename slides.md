---
theme: seriph
themeConfig:
  primary: "#FF9900"
title: IoT Is Everywhere
info: |
  IoT Is Everywhere: How One Device Talks to AWS
  High-level AWS community talk (~20 min).
drawings:
  persist: false
transition: slide-left
mdc: true
favicon: /favicon.svg
seoMeta:
  ogTitle: "IoT Is Everywhere: How One Device Talks to AWS"
  ogDescription: "High-level AWS community talk on everyday IoT and one device path to AWS IoT Core."
  ogImage: /og-image.png
  twitterCard: summary_large_image
fonts:
  sans: DM Sans
  serif: Gelasio
  mono: Fira Code
colorSchema: dark
layout: cover-photo
image: /cover.png
class: text-center
---

<div class="absolute bottom-10 left-0 right-0 text-center">
  <span @click="$slidev.nav.next" class="px-3 py-1.5 rounded cursor-pointer bg-black bg-opacity-40 text-sm tracking-wide" hover="bg-opacity-60">
    Press Space <carbon:arrow-right class="inline"/>
  </span>
</div>

<!--
Before the talk: boards powered and publishing; Amplify dashboard open in a second tab; AWS console closed.
Flow: four sections (see Agenda), ~20 min. Appendix after Thanks holds the deploy commands — only open it for questions.
Icons: official AWS Architecture Icons (aws-icons.johna.kiwi, AWS trademark guidelines).
-->

---
layout: default
---

# About me

<div class="about">
  <div class="about-copy">
    <h2 class="about-name">John Ajera</h2>
    <p class="about-role">
      Platform engineer · Earth Sciences New Zealand (GeoNet)
      <span class="about-seismic" title="Geohazards monitoring" aria-label="Seismic monitoring">
        <svg class="about-seismic-svg" viewBox="0 0 64 24" aria-hidden="true">
          <g class="about-seismic-trace">
            <path
              d="M0 12 H6 L9 12 L11 4 L13 20 L15 8 L17 16 L19 12 H26 L29 12 L31 3 L33 21 L35 7 L37 17 L39 12 H46 L49 12 L51 5 L53 19 L55 9 L57 15 L59 12 H64"
              fill="none"
              stroke="currentColor"
              stroke-width="1.75"
              stroke-linecap="round"
              stroke-linejoin="round"
            />
            <path
              d="M64 12 H70 L73 12 L75 4 L77 20 L79 8 L81 16 L83 12 H90 L93 12 L95 3 L97 21 L99 7 L101 17 L103 12 H110 L113 12 L115 5 L117 19 L119 9 L121 15 L123 12 H128"
              fill="none"
              stroke="currentColor"
              stroke-width="1.75"
              stroke-linecap="round"
              stroke-linejoin="round"
            />
          </g>
        </svg>
      </span>
    </p>
    <ul class="about-lines">
      <li>AWS Community Builder · Network &amp; Content Delivery</li>
      <li>AWS UG Leader · Wellington</li>
    </ul>
  </div>
  <aside class="about-site">
    <img class="about-qr" src="/qr-johna-kiwi.png" alt="QR code linking to https://johna.kiwi/" />
    <a class="about-site-url" href="https://johna.kiwi/">https://johna.kiwi/</a>
  </aside>
</div>

<!--
Speaker: brief hello (~30 s). Point at the QR for johna.kiwi — labs and writing live there.
The trace after GeoNet stands for seismic / geohazards monitoring —
a sensor network that follows the same pattern as this talk, at a very different scale.
-->

---
layout: default
---

# Agenda

<div class="agenda">
  <div class="agenda-item">
    <span class="agenda-num">1</span>
    <div><strong>IoT around us</strong><span>what the word means, and where it already is</span></div>
  </div>
  <div class="agenda-item">
    <span class="agenda-num">2</span>
    <div><strong>How one device talks to AWS</strong><span>IoT Core, identity, topics, the message</span></div>
  </div>
  <div class="agenda-item">
    <span class="agenda-num">3</span>
    <div><strong>What sits behind it</strong><span>architecture, six boards, what changed as it grew</span></div>
  </div>
  <div class="agenda-item">
    <span class="agenda-num">4</span>
    <div><strong>Live dashboard</strong><span>the fleet publishing right now, then where to try it</span></div>
  </div>
</div>

<!--
Speaker: ~15 seconds. Four sections, roughly five minutes each.
Section 1: What IoT means, IoT around us.
Section 2: AWS IoT Core, Thing · certificate · policy, Topics and rules, What the message looks like.
Section 3: Architecture, One path, six boards, What changed as it grew, Deploy.
Section 4: Live dashboard, Try it later.
If slides are added or moved, update this mapping.
-->

---
layout: default
---

# What IoT means

<p class="acronym-line">
  <strong>IoT</strong> = <strong>I</strong>nternet <strong>o</strong>f <strong>T</strong>hings —
  everyday objects that measure something and send it on.
</p>

<div class="path-grid">
  <div class="path-step">
    <ph-waveform class="path-icon" />
    <span>Sense</span>
    <span class="path-desc">temperature, motion, signal strength</span>
  </div>
  <div class="path-step">
    <ph-wifi-high class="path-icon" />
    <span>Connect</span>
    <span class="path-desc">joins a network and sends the reading</span>
  </div>
  <div class="path-step">
    <ph-cloud class="path-icon" />
    <span>Cloud</span>
    <span class="path-desc">checks who sent it, then stores or routes it</span>
  </div>
  <div class="path-step">
    <ph-eye class="path-icon" />
    <span>Act or observe</span>
    <span class="path-desc">a dashboard, an alert, or a device responding</span>
  </div>
</div>

<!--
Speaker: spell out the acronym once, then walk the four steps left to right. Every later slide sits on one of them.
Icons are generic here on purpose; nothing is AWS-specific yet.
-->

---
layout: default
---

# IoT around us

<div class="iot-grid">
  <div class="iot-item"><img class="aws-icon" src="/aws-icons/thing-lightbulb.svg" alt="" /><span>Smart lights</span></div>
  <div class="iot-item"><img class="aws-icon" src="/aws-icons/thing-generic.svg" alt="" /><span>Smart speaker</span></div>
  <div class="iot-item"><img class="aws-icon" src="/aws-icons/thing-thermostat.svg" alt="" /><span>Thermostat</span></div>
  <div class="iot-item"><img class="aws-icon" src="/aws-icons/thing-door-lock.svg" alt="" /><span>Smart lock</span></div>
  <div class="iot-item"><img class="aws-icon" src="/aws-icons/thing-camera.svg" alt="" /><span>Doorbell camera</span></div>
  <div class="iot-item"><img class="aws-icon" src="/aws-icons/thing-temp-sensor.svg" alt="" /><span>Weather station</span></div>
  <div class="iot-item"><img class="aws-icon" src="/aws-icons/thing-utility.svg" alt="" /><span>Smart meter</span></div>
  <div class="iot-item"><img class="aws-icon" src="/aws-icons/thing-car.svg" alt="" /><span>Connected car</span></div>
</div>

<!--
Speaker: devices people already own. The grid reads outward: room → house → doorstep → garden → street → road.
Icons are the AWS IoT "Thing" resource icons — the same word IoT Core uses for a device identity, next section.
-->

---
layout: default
---

# AWS IoT Core

<div class="core-hero">
  <img class="aws-icon aws-icon--hero" src="/aws-icons/iot-core.svg" alt="" />
  <p class="core-lead">
    A managed service that devices connect to and that routes their messages. Devices talk to it over
    <strong>MQTT</strong> (Message Queuing Telemetry Transport): small publish/subscribe messages addressed by topic.
  </p>
</div>

<div class="path-grid path-grid--two">
  <div class="path-step">
    <img class="aws-icon aws-icon--lg" src="/aws-icons/iot-certificate.svg" alt="" />
    <span>Who is connecting</span>
    <span class="path-desc">a Thing, a certificate and a policy decide which device may connect and what it may publish</span>
  </div>
  <div class="path-step">
    <img class="aws-icon aws-icon--lg" src="/aws-icons/iot-rule.svg" alt="" />
    <span>Where the message goes</span>
    <span class="path-desc">topics address each message; rules match topics and pass messages to other AWS services</span>
  </div>
</div>

<!--
Speaker: this is the "cloud" step from the four-step pattern. Two jobs; the next two slides take them in order.
-->

---
layout: default
---

# Thing · certificate · policy

<div class="path-grid path-grid--three">
  <div class="path-step">
    <img class="aws-icon aws-icon--lg" src="/aws-icons/thing-generic.svg" alt="" />
    <span>Thing</span>
    <span class="path-desc">registry entry for one device, e.g. <code>esp32-c61-01</code></span>
  </div>
  <div class="path-step">
    <img class="aws-icon aws-icon--lg" src="/aws-icons/iot-certificate.svg" alt="" />
    <span>Certificate</span>
    <span class="path-desc">X.509, attached to the Thing; presented on every connect</span>
  </div>
  <div class="path-step">
    <img class="aws-icon aws-icon--lg" src="/aws-icons/iot-policy.svg" alt="" />
    <span>Policy</span>
    <span class="path-desc">attached to the certificate; allows <code>iot:Connect</code> and <code>iot:Publish</code> on its own topics only</span>
  </div>
</div>

<p class="iot-tease">
  <strong>mTLS</strong> (mutual TLS): the device and IoT Core each present a certificate. The device holds a private key, not a password.
</p>

<!--
Speaker: the chain is what trips people up — policy attaches to the certificate, certificate attaches to the Thing.
A Thing is the logical device in IoT Core, not the circuit board.
One Thing + one certificate per board: a leaked key affects one device, and it can be revoked alone.
If asked: IoT Core also supports other auth paths (SigV4, custom authorizers) — acknowledge and move on.
-->

---
layout: default
---

# Topics and rules

<div class="topic-flow">
  <div class="topic-col">
    <span class="topic-label">Device publishes to</span>
    <code class="topic-line">devices/esp32-c61-01/telemetry</code>
    <code class="topic-line">devices/esp32-c61-01/events</code>
    <code class="topic-line">fleet/ideaspark-oled-02/telemetry</code>
  </div>
  <ph-arrow-right class="build-arrow text-orange-400" />
  <div class="topic-col">
    <span class="topic-label">Rule selects</span>
    <code class="topic-line">SELECT * FROM 'devices/+/telemetry'</code>
    <code class="topic-line">SELECT * FROM 'fleet/+/telemetry'</code>
    <span class="path-desc"><code>+</code> matches one level: any Thing name</span>
  </div>
  <ph-arrow-right class="build-arrow text-orange-400" />
  <div class="topic-col topic-col--narrow">
    <span class="topic-label">Action</span>
    <div class="topic-action">
      <img class="aws-icon aws-icon--md" src="/aws-icons/lambda.svg" alt="" />
      <span>ingest Lambda</span>
    </div>
  </div>
</div>

<p class="iot-tease">
  The device only knows its own topics. Where the data goes next is decided in the cloud, without reflashing the board.
</p>

<!--
Speaker: a topic is a slash-separated address; the Thing name sits in the middle so the policy can limit each device to its own.
Rules are SQL over topics. One rule per topic pattern here, all feeding one ingest Lambda.
Two prefixes: devices/ is the talk firmware, fleet/ is the shared multi-board firmware. Same cloud path.
-->

---
layout: default
---

# What the message looks like

<p class="payload-topic">
  <img class="aws-icon aws-icon--inline" src="/aws-icons/iot-topic.svg" alt="" />
  <code>devices/esp32-c61-01/telemetry</code> · every 15 s
</p>

```json
{
  "device_id": "esp32-c61-01",
  "ts": 1700000000,
  "type": "connectivity",
  "rssi": -67,
  "uptime_s": 3600,
  "heap_free": 180000,
  "chip_temp_c": 41.2,
  "chip_model": "ESP32-C61"
}
```

<p class="iot-tease">
  Every message carries <code>device_id</code>, <code>ts</code> and <code>type</code>. A button press uses the same fields on <code>…/events</code>.
</p>

<!--
Speaker: topic is the address, JSON is the payload. The dashboard reads exactly these fields.
rssi = Wi-Fi signal strength in dBm; uptime resets tell us about reboots; heap_free is free memory.
Full contract (extra Wi-Fi fields, events): demo/PAYLOAD.md.
-->

---
layout: default
---

# Architecture

<div class="arch-rows">
  <div class="arch-row">
    <span class="arch-row-label">Publish</span>
    <div class="build-flow build-flow--arch">
      <div class="build-node">
        <img class="aws-icon aws-icon--md" src="/aws-icons/thing-generic.svg" alt="" />
        <strong>Device</strong>
        <span>ESP32 · Thing</span>
      </div>
      <ph-arrow-right class="build-arrow text-orange-400" />
      <div class="build-node">
        <img class="aws-icon aws-icon--md" src="/aws-icons/iot-core.svg" alt="" />
        <strong>IoT Core</strong>
        <span>MQTT · mTLS</span>
      </div>
      <ph-arrow-right class="build-arrow text-orange-400" />
      <div class="build-node">
        <img class="aws-icon aws-icon--md" src="/aws-icons/iot-rule.svg" alt="" />
        <strong>Rule</strong>
        <span>topic SQL</span>
      </div>
      <ph-arrow-right class="build-arrow text-orange-400" />
      <div class="build-node">
        <img class="aws-icon aws-icon--md" src="/aws-icons/lambda.svg" alt="" />
        <strong>Lambda</strong>
        <span>ingest</span>
      </div>
      <ph-arrow-right class="build-arrow text-orange-400" />
      <div class="build-node">
        <img class="aws-icon aws-icon--md" src="/aws-icons/dynamodb.svg" alt="" />
        <strong>DynamoDB</strong>
        <span>raw + 1-min rollups</span>
      </div>
      <div class="build-node build-node--side">
        <img class="aws-icon aws-icon--md" src="/aws-icons/s3.svg" alt="" />
        <strong>S3</strong>
        <span>camera frames</span>
      </div>
    </div>
  </div>

  <div class="arch-row">
    <span class="arch-row-label">Show</span>
    <div class="build-flow build-flow--arch">
      <div class="build-node">
        <img class="aws-icon aws-icon--md" src="/aws-icons/amplify.svg" alt="" />
        <strong>Amplify</strong>
        <span>hosts the page</span>
      </div>
      <ph-arrow-right class="build-arrow text-orange-400" />
      <div class="build-node">
        <img class="aws-icon aws-icon--md" src="/aws-icons/lambda.svg" alt="" />
        <strong>Lambda</strong>
        <span>query · Function URL</span>
      </div>
      <ph-arrow-right class="build-arrow text-orange-400" />
      <div class="build-node">
        <img class="aws-icon aws-icon--md" src="/aws-icons/dynamodb.svg" alt="" />
        <strong>DynamoDB</strong>
        <span>read rollups</span>
      </div>
      <div class="build-node build-node--side">
        <img class="aws-icon aws-icon--md" src="/aws-icons/eventbridge.svg" alt="" />
        <strong>EventBridge</strong>
        <span>every 1 min · warm</span>
      </div>
    </div>
  </div>
</div>

<p class="iot-tease">
  The browser polls the query API; Amplify only serves static files. No servers to patch on either row.
</p>

<!--
Speaker: top row fills the tables, bottom row reads them.
Ingest writes each message as-is and also adds it to a one-minute summary row per device (the rollup).
Camera frames go to S3 instead of DynamoDB; the table keeps a pointer.
EventBridge calls the query Lambda once a minute so its cache is warm when someone switches the time window.
All of it is created by two shell scripts with the AWS CLI — no console clicks, no Terraform in this repo.
-->

---
layout: default
---

# One path, six boards

<div class="board-grid">
  <div class="board-card">
    <ph-lightbulb class="board-icon" />
    <strong>ESP32-C61</strong>
    <span>RGB status LED · talk board</span>
  </div>
  <div class="board-card">
    <ph-lightbulb class="board-icon" />
    <strong>ESP32-C3</strong>
    <span>RGB status LED</span>
  </div>
  <div class="board-card">
    <ph-lightbulb class="board-icon" />
    <strong>ESP32-S3</strong>
    <span>RGB status LED</span>
  </div>
  <div class="board-card">
    <ph-monitor class="board-icon" />
    <strong>Ideaspark 0.96″</strong>
    <span>monochrome OLED status</span>
  </div>
  <div class="board-card">
    <ph-monitor class="board-icon" />
    <strong>Ideaspark 1.14″</strong>
    <span>colour TFT status</span>
  </div>
  <div class="board-card">
    <ph-camera class="board-icon" />
    <strong>ESP32-CAM</strong>
    <span>camera + microSD</span>
  </div>
</div>

<p class="iot-tease">
  New board → new Thing and certificate. Cloud side unchanged.
</p>

<!--
Speaker: "how one device talks to AWS" scales by repeating the identity step, not by redesigning the cloud.
Different chips, displays and USB adapters; same payload shape and rules. Topic prefix differs:
devices/ for this talk's C61 firmware, fleet/ for the shared multi-board repo (esp32-aws-iot-fleet).
The query Lambda keeps a list of known Things so a device that goes quiet still shows as stale rather than disappearing.
-->

---
layout: default
---

# What changed as it grew

<div class="path-grid path-grid--three">
  <div class="path-step path-step--left">
    <ph-timer class="path-icon" />
    <span>Reading a day of data</span>
    <span class="path-desc"><strong>Before:</strong> the 24 h view read every raw message, about 13 s per switch.</span>
    <span class="path-desc"><strong>Now:</strong> ingest keeps 1-minute rollups; the dashboard reads those.</span>
  </div>
  <div class="path-step path-step--left">
    <ph-clock-clockwise class="path-icon" />
    <span>Cold first request</span>
    <span class="path-desc"><strong>Before:</strong> the first window switch after a quiet period was slow.</span>
    <span class="path-desc"><strong>Now:</strong> EventBridge calls the query Lambda every minute to keep its cache warm.</span>
  </div>
  <div class="path-step path-step--left">
    <ph-image class="path-icon" />
    <span>Bigger messages</span>
    <span class="path-desc"><strong>Before:</strong> camera frames did not fit the board's MQTT buffer.</span>
    <span class="path-desc"><strong>Now:</strong> a larger buffer on that board only, under IoT Core's 128 KB limit; images land in S3.</span>
  </div>
</div>

<!--
Speaker: practical lessons from growing one board into six. Keep it factual, ~1 minute.
Rollups: one DynamoDB item per device per minute (sample count, sums for averages, latest heap and uptime), written by ingest.
Hardware aside if asked: one "OLED" board turned out to have an SPI colour TFT, not an I2C OLED — check the hardware before choosing a driver.
-->

---
layout: default
---

# Deploy

<div class="build-flow">
  <div class="build-node">
    <img class="aws-icon aws-icon--md" src="/aws-icons/iot-certificate.svg" alt="" />
    <strong>1 · Identity</strong>
    <span><code>provision-thing.sh</code></span>
    <span>Thing, certificate, policy</span>
  </div>
  <ph-arrow-right class="build-arrow text-orange-400" />
  <div class="build-node">
    <img class="aws-icon aws-icon--md" src="/aws-icons/dynamodb.svg" alt="" />
    <strong>2 · Ingest + query</strong>
    <span><code>deploy-stack.sh</code></span>
    <span>tables, Lambdas, rules, URL</span>
  </div>
  <ph-arrow-right class="build-arrow text-orange-400" />
  <div class="build-node">
    <ph-cpu class="build-icon text-orange-300" />
    <strong>3 · Firmware</strong>
    <span><code>pio run -t upload</code></span>
    <span>Wi-Fi, endpoint, certificate</span>
  </div>
  <ph-arrow-right class="build-arrow text-orange-400" />
  <div class="build-node">
    <img class="aws-icon aws-icon--md" src="/aws-icons/amplify.svg" alt="" />
    <strong>4 · Dashboard</strong>
    <span><code>npm run build</code></span>
    <span>upload to Amplify</span>
  </div>
</div>

<p class="iot-tease">
  AWS CLI only, region <code>ap-southeast-2</code>. Each script ends with its own validation. Commands are in the appendix and <code>demo/aws/README.md</code>.
</p>

<!--
Speaker: one breath per stage. Stages 1–2 need no hardware; stage 3 is the only one with a USB cable.
Do not step through commands here — the appendix after Thanks has them if someone asks.
-->

---
layout: cover-photo
image: /amplify-dashboard.png
class: dashboard-slide
title: Live dashboard
---

<div class="dashboard-caption">
  Live dashboard · switch to the open tab
</div>

<!--
Speaker: switch to the pre-opened Amplify tab now. This slide is the backup if Wi-Fi or the venue network fails.
Point at: hosts live, health per host, delivery rate against the 15 s interval, last seen.
Click one host at a time — charts follow that row. Camera panel only shows for the CAM board; C61 has no camera.
Switch 1h → 6h → 24h to show the rollups doing their job. Press BOOT on a board to show an event arrive.
The pass-around board may drop and reconnect when moved; that shows up as a gap, which is the point.
-->

---
layout: default
---

# Try it later

<div class="try-later">
  <img class="path-qr" src="/qr-talk-demo.png" alt="QR code linking to https://github.com/jajera/iot-everywhere-aws-talk" />
  <p class="try-later-url">
    <a href="https://github.com/jajera/iot-everywhere-aws-talk">https://github.com/jajera/iot-everywhere-aws-talk</a>
  </p>
  <p class="path-desc">C61 firmware, cloud scripts, Amplify dashboard</p>
</div>

<p class="iot-tease">
  An ESP32 dev board, a USB data cable and an AWS account are enough to start.
  Other boards: <a href="https://github.com/jajera/esp32-aws-iot-fleet">https://github.com/jajera/esp32-aws-iot-fleet</a>
  · Lab: <a href="https://aws-iot-walkthrough.johna.kiwi/">https://aws-iot-walkthrough.johna.kiwi/</a>
</p>

<!--
Speaker: one QR — this talk's repo. Fleet and the full walkthrough stay as text links.
Costs stay small — delete the stack after trying (teardown in demo/aws/README.md).
-->

---
layout: center
class: text-center
---

# Thanks

Questions?

<img class="thanks-qr" src="/qr-iot-is-everywhere.png" alt="Feedback QR — IoT Is Everywhere, AWS UG Wellington Meetup October 2026" />

<!--
Speaker: QR is meetup feedback (feedbackfeijoa). Leave it up during Q&A.
Audience talk ends here. Appendix follows for questions about deploy commands.
-->

---
layout: center
class: text-center
---

# Appendix

Deploy commands, stage by stage

<!--
Only open during Q&A. Source of truth: demo/aws/README.md.
-->

---
layout: default
---

# A1 · Identity

```bash
cd demo/aws
export AWS_PROFILE=sandbox AWS_REGION=ap-southeast-2
export THING_NAME=esp32-c61-01
./provision-thing.sh
```

**Done when:** Thing, certificate and policy exist, and `certs.h` is copied into the firmware.

```text
Created Thing esp32-c61-01
Created certificate arn:aws:iot:…:cert/…
Created policy iot-talk-esp32-c61-01-policy
Copied certs.h → …/firmware/include/certs.h
```

<!--
No Wi-Fi or board needed. certs.h and the PEM files are gitignored — never commit them.
-->

---
layout: default
---

# A2 · Ingest + query

```bash
cd demo/aws
export AWS_PROFILE=sandbox AWS_REGION=ap-southeast-2
export PREFIX=iot-talk THING_NAME=esp32-c61-01
./deploy-stack.sh
```

**Done when:** validation passes and `out/query-url.txt` is written.

```text
=== Validation ===
OK  table iot-talk-telemetry ACTIVE
OK  IoT rule iot_talk_telemetry
OK  query Function URL OPTIONS (204)
OK  query returns smoke telemetry for esp32-c61-01
Validation PASSED
```

<!--
Creates DynamoDB tables (telemetry, events, metrics), ingest and query Lambdas, IoT rules for devices/ and fleet/,
the query Function URL, the EventBridge warmer, and the camera bucket. Safe to re-run.
-->

---
layout: default
---

# A3 · Firmware

```bash
cd demo/firmware
cp include/config.example.h include/config.h   # Wi-Fi, THING_NAME, AWS_IOT_ENDPOINT
pio run -e esp32-c61 -t erase                  # first flash only
pio run -e esp32-c61 -t upload
pio device monitor -e esp32-c61
```

**Done when:** serial shows Wi-Fi, MQTT and a telemetry publish.

```text
[wifi] connected ip=…
[mqtt] connected
[telemetry] published topic=devices/esp32-c61-01/telemetry bytes=345
```

<!--
C61 is on /dev/ttyACM* (native USB). Needs PlatformIO and dialout group membership on Ubuntu.
config.h holds the Wi-Fi password — gitignored.
-->

---
layout: default
---

# A4 · Dashboard

```bash
cd demo/web
npm ci
VITE_API_URL="$(cat ../aws/out/query-url.txt)" npm run build
# upload dist/ to Amplify Hosting: console "Deploy without Git", or CLI create-deployment
```

**Done when:** the Amplify URL returns 200 and hosts show as live.

```text
Amplify job status: SUCCEED
Dashboard: https://main.<app-id>.amplifyapp.com
```

<!--
The page calls one route, GET /fleet/analytics?window=3600|21600|86400, and polls every 30–120 s depending on the window.
-->
