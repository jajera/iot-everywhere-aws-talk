---
inclusion: always
---

# Structure

```plaintext
.github/
  workflows/             # pages + markdown-lint + commitmsg-conform
  dependabot.yml
.kiro/
  settings/mcp.json      # aws-docs on, aws-api off
  steering/              # always-on product/tech/tone + scoped files
  hooks/                 # mutation guard, table format, cite reminders
  specs/                 # optional design notes for this talk
layouts/                 # Slidev custom layouts (e.g. cover-photo)
styles/                  # theme overrides
public/                  # favicon, cover, og-image
demo/
  README.md
  PAYLOAD.md
  firmware/              # PlatformIO feeder + C61 hardware baseline (README.md)
  aws/                   # CLI/console cloud path (no Terraform)
  web/                   # Amplify-hosted dashboard (stage visual)
slides.md                # talk content
README.md
package.json
```

## Rules

- One job per slide: short title, one visual idea, one supporting line.
- Speaker notes in HTML comments under the slide body.
- Do not bake board model names into artwork unless the image is verified for that model.
- Keep `demo/` aligned with https://aws-iot-walkthrough.johna.kiwi/ where contracts overlap.
- Prefer editing `slides.md` + `styles/index.css` over adding new frameworks.
- Live beat is Amplify UI; firmware/aws are operator prep, not on-stage walkthroughs.
