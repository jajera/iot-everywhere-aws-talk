---
inclusion: always
---

# Tech

## Slides

- **Slidev** (`@slidev/cli`) with `@slidev/theme-seriph`
- Entry: `slides.md`
- Custom layouts: `layouts/`
- Global styles: `styles/index.css` (charcoal + quiet teal/amber atmosphere)
- Static assets: `public/` (`favicon.svg`, `cover.png`, `og-image.png`)
- Icons: prefer collections already installed (`ph`, `carbon`); do not invent MDI names
  without `@iconify-json/mdi` present and verified

## Commands

```bash
npm ci
npm run dev                                          # local Slidev
npm run build -- --base /iot-everywhere-aws-talk/    # GitHub Pages base path
```

## Demo backbone

- `demo/firmware/` — PlatformIO feeder; **primary board ESP32-C61-DevKitC-1**
- C61 hardware baseline (ports, `huge_app` erase, LED SPI+DMA, CDC): `demo/firmware/README.md`
- Stage 3 checklist + sample serial: `demo/aws/README.md` §3
- `demo/PAYLOAD.md` — telemetry / event contract
- Secrets stay gitignored (`config.h`, `certs.h`, `certs/`)

## CI / Pages

- `.github/workflows/pages.yml` — build Slidev, deploy `dist/`
- `.github/workflows/markdown-lint.yml` — `actionsforge` reusable
- `.github/workflows/commitmsg-conform.yml` — `actionsforge` reusable

## Defaults when discussing AWS

- Profile example: `sandbox`
- Region example: `ap-southeast-2`
- Do not run mutating AWS CLI from the agent unless `IOT_TALK_ALLOW_AWS=1`
  (see `.kiro/steering/lab-safety.md`)
