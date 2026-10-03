#!/usr/bin/env python3
"""Generate the licensed, fixed UI subset. Needs fonttools 4.60.1 and npx."""
import hashlib
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
source = Path(sys.argv[1])
text = (ROOT / 'main/flappy_ui.c').read_text()
chars = ''.join(sorted(set(chr(c) for c in range(32, 127)) | set(re.findall(r'[\u3400-\u9fff]', text))))
assets = ROOT / 'assets/fonts'
(assets / 'flappy_chars.txt').write_text(chars + '\n')
from fontTools.ttLib import TTFont
from fontTools import subset
from fontTools.varLib.instancer import instantiateVariableFont
font = TTFont(source)
missing = set(map(ord, chars)) - set(font.getBestCmap())
assert not missing, missing
if 'fvar' in font:
    font = instantiateVariableFont(font, {'wght': 500}, inplace=True)
subsetter = subset.Subsetter()
subsetter.populate(text=chars)
subsetter.subset(font)
output = assets / 'NotoSansSC-Flappy-subset.ttf'
for record in font['name'].names:
    replacement = {1: 'Noto Sans SC Flappy', 4: 'Noto Sans SC Flappy Medium',
                   6: 'NotoSansSCFlappy-Medium', 16: 'Noto Sans SC Flappy',
                   17: 'Medium'}.get(record.nameID)
    if replacement:
        record.string = replacement.encode(record.getEncoding())
font.save(output)
subprocess.run(['npx', '--yes', 'lv_font_conv@1.5.3', '--font', str(output),
                '--range', '0x20-0x7E', '--symbols', chars, '--size', '16',
                '--bpp', '4', '--format', 'lvgl', '--no-compress',
                '--lv-font-name', 'flappy_font_16', '--lv-include', 'lvgl.h',
                '--output', str(assets / 'flappy_font_16.c')], check=True)
points = ', '.join(hex(ord(c)) for c in chars)
(assets / 'flappy_glyphs.c').write_text('#include <stdint.h>\nconst uint32_t flappy_codepoints[] = {' + points + '};\nconst unsigned flappy_codepoint_count = sizeof(flappy_codepoints)/sizeof(flappy_codepoints[0]);\n')
print('Font coverage:', len(chars), 'glyphs; source SHA256:', hashlib.sha256(source.read_bytes()).hexdigest())
