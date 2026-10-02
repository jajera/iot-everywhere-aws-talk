# IoT Is Everywhere: How One Device Talks to AWS

Talk materials for a ~20 minute high-level AWS community session.

- **Slides:** Slidev (this repo) → GitHub Pages
- **Live visual:** Amplify dashboard in [`demo/web/`](demo/web/) (boards keep publishing in the background)
- **Demo source:** [`demo/`](demo/) — C61 firmware + thin AWS CLI path + Amplify UI
- **Full lab (later):** https://aws-iot-walkthrough.johna.kiwi/
- **Multi-board fleet:** https://github.com/jajera/esp32-aws-iot-fleet (Ideaspark, S3, C3, CAM — not this repo)

## Develop slides

```bash
npm ci
npm run dev              # full talk; deploy commands sit in the appendix after Thanks
```

## Build

```bash
npm run build -- --base /
```

Output: `dist/` (published by `.github/workflows/pages.yml` to
https://iot-everywhere-aws-talk.johna.kiwi/).

Deck shape (~20 min): IoT around us → how one device talks to AWS → what sits behind it → live dashboard. A one-slide **Deploy** overview stays in the main flow; command-level slides are in the **Appendix** after **Thanks**.

## Repo layout

| Path | Purpose |
| --- | --- |
| `slides.md` | Slide source (main talk + deploy appendix) |
| `public/` | Favicon, OG image, architecture exports, AWS icons |
| `docs/` | Architecture poster + editable `.drawio` |
| `demo/` | Firmware, AWS CLI path, Amplify dashboard |
| `.github/workflows/` | Pages deploy + markdown / commitmsg hygiene |

## Notes

- On stage: show the pre-opened Amplify UI. Touch hardware only if needed; do not live-operate the AWS console.
- Desk board stays online; pass-around board may disconnect and reconnect.
- Operator prep: see [`demo/README.md`](demo/README.md).
