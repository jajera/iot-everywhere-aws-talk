---
inclusion: fileMatch
fileMatchPattern: ["slides.md", "layouts/**", "styles/**", "public/**"]
---

# Slides pattern

## Slide shape

1. One H1 title
2. One visual block (grid, flow, or short list)
3. At most one supporting paragraph or tease line
4. HTML comment speaker notes under the body

## Visual bias

- Prefer icons + short labels over dense bullets
- Match charcoal + quiet teal/amber from `styles/index.css`
- Cover uses `layout: cover-photo` + `public/cover.png`
- Do not stretch soft raster art as a theme background with dim overlays
- Prefer official AWS Architecture Icons from `public/aws-icons/` for AWS services and IoT resources
  (browser: [aws-icons.johna.kiwi](https://aws-icons.johna.kiwi/))

## Wording

Follow `.kiro/steering/neutral-language.md`. Everyday IoT examples must be clearly connected
devices. Spell out acronyms (IoT, MQTT, mTLS) the first time they appear. Keep the live beat
on the Amplify dashboard.

## Agenda

Keep the agenda in sync when adding or renaming slides.
