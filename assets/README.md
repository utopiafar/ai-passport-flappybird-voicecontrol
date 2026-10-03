<p align="right">
  <a href="README.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Assets

This directory stores reusable fonts, images, music, and sound effects, organized by asset type.

Keep each asset in the matching subdirectory and document its destination, naming, integration method, and source/license. Do not mix binary assets with Markdown documentation.

## Fonts

Store reusable font files and generated font sources in `fonts/`.

- Use descriptive names that include the family, weight, size, and format when relevant.
- Document the source, license, character range, conversion command, and expected destination.
- Check Flash and internal-RAM impact before adding a font; the ESP32-C3 has no PSRAM.
- Do not commit fonts whose license does not permit redistribution.

### Voice Flappy font

`fonts/NotoSansSC-Flappy-subset.ttf` is the 199-character application subset
of [Noto Sans SC](https://github.com/google/fonts/tree/main/ofl/notosanssc),
instantiated at weight 500 from the variable upstream TTF (SHA-256
`a3041811a78c361b1de50f953c805e0244951c21c5bd412f7232ef0d899af0da`).
The font is redistributed under the SIL Open Font License in
`fonts/NotoSansSC-OFL.txt`. The subset family is named Noto Sans SC Flappy.
`fonts/flappy_font_16.c` contains uncompressed 4-bpp LVGL glyphs at 16 px and
is compiled directly into `main`. `fonts/flappy_chars.txt` records printable
ASCII plus every fixed Chinese UI character; `fonts/flappy_glyphs.c` supports
startup coverage checking. All application labels bind this font explicitly.

Regenerate with Python/fonttools 4.60.1 and the official `lv_font_conv` 1.5.3
(pinned in the script; requires Node/npm):

```bash
python3 tools/generate_flappy_font.py assets/fonts/NotoSansSC-Flappy-subset.ttf
python3 tests/test_flappy_font.py
```

When adding a character absent from the subset, supply the original licensed
TTF instead. Conversion asserts source coverage before producing the subset.
The game sprite, scenery, and 16 kHz / 16-bit mono synthesized effects are
original code under the repository MIT license; no commercial game assets or
sound recordings are embedded.

## Images

Store reusable source images and generated display assets in `images/`.

| File | Dimensions and format | Use and source |
| --- | --- | --- |
| [`images/home.jpg`](images/home.jpg) | 3840 × 2160, JPEG | Product hero image embedded in both project README files to foreground AI Passport and its open, maker-oriented identity. |
| [`images/readme-hardware-specs.png`](images/readme-hardware-specs.png) | 2172 × 724, PNG RGBA | Optional technical infographic retained as a reference asset; it is no longer used as the homepage hero. Generated for this repository with the built-in image generation tool on 2026-09-17; the six labels and values were checked against the documented hardware contract. |
| [`images/logo-wordmark.png`](images/logo-wordmark.png) | 1648 × 336, PNG RGBA | Transparent black wordmark extracted from the repository's original `images/logo.png`; embedded in both project README files for light backgrounds. |
| [`images/logo-wordmark-dark.png`](images/logo-wordmark-dark.png) | 1648 × 336, PNG RGBA | White version of the extracted wordmark, used by the README `<picture>` element when GitHub is in dark mode. |

- Use descriptive names and document dimensions, pixel format, conversion steps, and destination.
- Prefer formats suitable for the 240 × 320 RGB565 display and account for Flash and internal RAM.
- Preserve editable sources where licensing permits, and record the source and license.
- Never commit device QR secrets, credentials, or personal data in images.

## Music and sound effects

Store reusable music and sound-effect sources in `music/`.

- Document the source, license, sample rate, bit depth, channels, conversion command, and destination.
- Prefer 16 kHz, 16-bit mono PCM when it matches the current BSP audio path.
- Check Flash and internal-RAM cost before embedding audio; stream or chunk long recordings.
- Do not commit media without redistribution permission.
