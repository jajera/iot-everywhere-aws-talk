---
inclusion: fileMatch
fileMatchPattern: ["demo/**", "README.md", "slides.md"]
---

# Lab and demo safety

This repository is **talk material**, not a place for the agent to provision AWS resources.
Demo devices and certs are operator-owned.

## Never run AWS mutations unprompted

The agent does not create, update, or delete AWS resources on its own. Author the command
or document it; the operator runs it. The `guard-aws-mutations` hook blocks accidental
execution from shell tools.

Opt in only for a deliberate operator session:

```bash
export IOT_TALK_ALLOW_AWS=1
```

## Conventions when documenting AWS

| Convention | Value |
| --- | --- |
| Profile example | `sandbox` |
| Region example | `ap-southeast-2` |
| Mutation guard | `IOT_TALK_ALLOW_AWS=1` |
| ARN samples | Account `123456789012` |
| Secrets | Never commit device certs, private keys, Wi-Fi passwords, or `config.h` / `certs.h` |

## Stage and demo

- Do not instruct live AWS console clicking during the talk flow.
- Pass-around board may disconnect; desk board is the reliability path.
- Firmware under `demo/firmware/` targets ESP32-C61 by default; say so in text, not as a
  misleading label on unrelated artwork. Onboarding baseline: `demo/firmware/README.md`.

## Cost callouts

Any page or note that creates IoT Things, rules, or always-on resources should state that
usage can be billable and point at current AWS pricing pages rather than hardcoding rates.
