---
inclusion: always
---

# Neutral language

All words in this repository — slides, README, demo docs, speaker notes, commit messages,
and agent replies while working here — stay **neutral**.

## Required tone

- Precise and calm. State what something does; do not sell it.
- Inclusive and respectful of every audience skill level.
- Vendor-neutral where a generic term works; name a product only when it matters
  (for example AWS IoT Core, MQTT, ESP32).
- Prefer observable facts over adjectives.

## Avoid

| Pattern | Prefer |
| --- | --- |
| Hype (`revolutionary`, `game-changing`, `blazing fast`, `best`) | Measured wording or omit |
| Absolutes (`always`, `never`, `guarantees`) unless documented | Qualified claims |
| Dismissing other tools, clouds, or boards | Neutral comparison or no comparison |
| Talking down (`obviously`, `simply`, `just`, `anyone can`) | Direct instruction |
| Fear or urgency (`don't get left behind`) | Practical next step |
| Unverified model labels on photos | Generic “dev board” or verified name |
| Marketing voice in slides | One clear sentence |

## Talk-specific

- Everyday examples must be **clearly connected** devices (smart speaker, thermostat),
  not ambiguous appliances.
- Seismic / GNSS / geomag is a **pattern parallel**, not a claim that the demo board
  is that class of instrument.
- On stage: describe failure calmly (“pass-around may drop”) without drama.

## When unsure

Rewrite toward shorter, factual language. If a claim needs AWS behaviour, check
`.kiro/steering/aws-source-lock.md` or the aws-docs MCP server before asserting it.
