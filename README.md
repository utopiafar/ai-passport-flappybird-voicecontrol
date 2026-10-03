[简体中文](README.zh_CN.md)

# Voice Flappy — AI Passport

An offline, microphone-controlled arcade game for FoloToy AI Passport. Speak,
hum, or call out to climb; louder sound produces a faster climb, and silence
lets the bird fall. Fly through green pipes, earn one point per pipe, and avoid
the pipes, ceiling, and ground. Pixel scenery, an animated bird, medals, short
original synthesized sound effects, and an on-screen lift meter complete the
240 × 320 experience. Normal speech is sufficient; shouting is optional.

Based on the complete [FoloToy AI Passport](https://gitee.com/FoloToy/ai-passport)
project at `0b9e4c81ee4421c0bac39ca3561d65a8285acd4a`. Reuses the current
BSP without driver changes. The startup and navigation use a new application UI;
the baseline hardware demos remain available as reference source only.

## Play and controls

| Screen | UP | DOWN | OK | Hold OK |
| --- | --- | --- | --- | --- |
| Home | Toggle effects | Toggle classic / practice | Start calibration | Home |
| Calibration / countdown | — | — | — | Cancel to home |
| Flight | Increase sensitivity | Decrease sensitivity | Pause | Home |
| Pause | Increase sensitivity | Decrease sensitivity | Resume after one second | Home |
| Result / microphone error | — | — | Retry calibration | Home |

Keep quiet during the approximately 1.2-second ambient calibration, then follow
the three-second countdown. Control lift using short sounds or sustained speech.
The three sensitivity levels adjust how much sound is needed. Speaker output
and its short decay interval are excluded from microphone input; lift holds its
previous value briefly during effects. Switch effects off on Home if acoustic
feedback affects play in your environment. Calibration above the noise limit
asks for a quieter location; failed or stale PCM freezes play into a retry page.

Classic mode uses 84-pixel gaps, gradually increases speed, and awards bronze,
silver, and gold at 5, 15, and 30 points. Practice uses wider 104-pixel gaps and a
slower fixed speed; it does not affect the classic best score. Classic best score,
sensitivity, mode, and sound preference persist in the application's own NVS
namespace. Storage failure leaves the session playable but records are not saved.
The battery indicator displays `--%` when unavailable.

## Build and firmware

ESP32-C3, 8 MB Flash, no PSRAM, ESP-IDF **5.5.3**. Activate that environment,
then run the complete shared gate:

```bash
./tools/validate.sh
```

The gate runs the baseline tests plus game logic and Chinese glyph coverage,
builds in an isolated directory from `sdkconfig.defaults`, and verifies the
merged image's offsets, partition table, payloads, and debug identities.
The delivered image is `build/FoloToy-AI-Passport-full.bin`, for writing at
**`0x0`**. Its matching ELF/MAP and manifest are retained under
`build/firmware/<full-image-sha256>/`. Build outputs are Git-ignored.

A merged flash replaces firmware and may reset NVS records. It requires separate
user approval; no flashing is performed by build or validation. See the
[firmware/data policy](docs/development/engineering/firmware-layout.md).
No Wi-Fi, Bluetooth, cloud account, or additional wiring is required.

## UI assets and verification

The fixed Chinese UI uses a licensed Noto Sans SC subset at 16 px. The subset
includes printable ASCII and every Chinese UI character; host cmap checks and a
startup glyph check include a deliberately absent negative case. Every label
explicitly uses this font. Source, converter versions, regeneration, and license
are in the [asset inventory](assets/README.md). The sprite, scenery, and sound
synthesis are original code in this repository.

Automated tests cover loudness/DC rejection, proportional lift, quiet falling,
calibration timeout and noise rejection, collisions, one-time scoring, pipe
recycling, best-score isolation, pause/resume, speaker suppression, and input
bounds. Compilation and host rendering do not establish device performance.
Real microphone gain, acoustic feedback, speaker quality, physical keys, display
refresh, Chinese display, runtime heap/stack, and persistence after reboot require
on-device acceptance. See [validation results](docs/voice-flappy-validation.md).
