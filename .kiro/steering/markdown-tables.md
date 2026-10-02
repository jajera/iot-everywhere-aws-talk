---
inclusion: fileMatch
fileMatchPattern: ["**/*.md", "**/*.mdx"]
---

# Markdown tables

When using GFM pipe tables:

- Keep a separator row (`| --- | --- |`)
- Escape literal pipes in cells as `\|`
- Prefer short header labels

The `format-markdown-tables` hook repairs dirty Markdown tables on save.
