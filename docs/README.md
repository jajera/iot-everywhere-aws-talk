# Architecture diagrams

## Talk demo path

ESP32 → IoT Core → Rule → ingest Lambda → DynamoDB; Amplify-hosted UI **polls** query Lambda → DynamoDB.

| Asset | Use |
| --- | --- |
| [`../public/architecture/iot-talk-demo-path.svg`](../public/architecture/iot-talk-demo-path.svg) / [`.png`](../public/architecture/iot-talk-demo-path.png) | Standalone poster (dark, real AWS icons) — source of truth |
| [`iot-talk-demo-path.svg`](iot-talk-demo-path.svg) / [`.png`](iot-talk-demo-path.png) | Copy kept in `docs/` for browsing without `public/` |
| [`iot-talk-demo-architecture.drawio`](iot-talk-demo-architecture.drawio) | Editable draw.io source (export only when needed) |
| Slide **Architecture** | Native dark icon rows in `slides.md` (not the poster) |
| Slide **Deploy** | Inlined in `slides.md` (identity → Amplify) |

### Regenerate poster PNG

From repo root (needs `cairosvg`):

```bash
python3 - <<'PY'
import cairosvg
cairosvg.svg2png(
  url="public/architecture/iot-talk-demo-path.svg",
  write_to="public/architecture/iot-talk-demo-path.png",
  output_width=1920, output_height=1080,
)
PY
install -m 644 public/architecture/iot-talk-demo-path.png docs/iot-talk-demo-path.png
install -m 644 public/architecture/iot-talk-demo-path.svg docs/iot-talk-demo-path.svg
```

**Alt text:** Dark architecture poster — publish path Device to DynamoDB; show path Amplify polls query Lambda to DynamoDB.
