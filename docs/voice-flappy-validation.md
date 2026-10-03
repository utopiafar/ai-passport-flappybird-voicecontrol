[简体中文](voice-flappy-validation.zh_CN.md)

# Voice Flappy validation

## Scope

The implementation is based on upstream `0b9e4c81`, developed on
`codex/voice-flappy-bird`, and intended for the user's fork `main`.
No BSP drivers, pin assignments, or partition layout were changed. Radio stacks
are disabled. All active application screens are implemented in `flappy_ui.c`;
the hardware-test menu is excluded from the application's CMake sources.

## Acceptance on the board

Device tests have not run. Flashing requires the user's approval of the exact
validated image and target. A `0x0` merged flash can reset NVS; no full-chip erase
is required by this application.

1. Check every Chinese label, rounded screen corners, battery indicator, pipe,
   bird, score, and lift meter; confirm no clipping, missing glyphs, or tearing.
2. Keep quiet during calibration; speak softly, normally, and loudly at several
   distances. Check monotonic lift, responsiveness, falling in silence, all three
   sensitivity levels, and rejection of an excessively noisy calibration.
3. Confirm home, calibration, countdown, flight, pause, resume, result, retry,
   practice, and long-OK navigation. Check a long press returns home once and
   does not start another game on release.
4. Check point and crash sounds, muting, and absence of speaker-induced lift.
   Retry with sound disabled in a reflective room.
5. Pass a pipe once, collide with each boundary, earn medals, and verify that
   practice scores do not update the classic best.
6. Restart to verify best score and settings. Confirm no boot loop, watchdog,
   stack overflow, growing heap use, or memory allocation failure in a 10-minute
   session. Measure actual frame rate and audio latency.

## Automated results

Results and exact firmware identity are recorded after the complete gate.
Build and host tests are separate from the on-device checklist above.

Validated on 2026-10-03 with ESP-IDF 5.5.3 and CMake 3.30.5:

- Build: PASS — full `tools/validate.sh`, 8 MB default layout, merged offsets and payload verification.
- Host tests: PASS — baseline gate, game model, 199 glyphs including a negative coverage case, and AddressSanitizer/UndefinedBehaviorSanitizer model run.
- UI host rendering: PASS — nine states using actual LVGL 9.5.0, the shipped font, and a 24 KB LVGL pool; a ground-pixel assertion catches missing canvases. Images were visually inspected for Chinese coverage and clipping.
- Device tests: NOT RUN — USB enumeration showed only system virtual serial ports. No flash was performed.
- Unverified: the board acceptance checklist above, including microphone response, acoustic suppression, real display performance, and NVS after reboot.

The final content-addressed bundle and `manifest.json` under `build/firmware/`
identify the delivered merged image and its matching ELF/MAP; artifacts are not
committed. Generate UI QA images with `tools/render_flappy_ui.sh` after building.
