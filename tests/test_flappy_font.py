"""Check the shipped LVGL cmap against every UI character, with a negative case."""
import re
from pathlib import Path
root = Path(__file__).resolve().parent.parent
font = (root / 'assets/fonts/flappy_font_16.c').read_text()
# Converter emits FORMAT0_TINY ASCII and SPARSE_TINY CJK maps.
blocks = re.findall(r'\.range_start = (\d+), \.range_length = (\d+), \.glyph_id_start = (\d+),\s*\.unicode_list = (\w+),.*?\.list_length = (\d+), \.type = (\w+)', font, re.S)
coverage = set()
assert blocks, 'No generated font maps found'
for start, length, gid, name, count, kind in blocks:
    start, length = int(start), int(length)
    if name == 'NULL':
        assert kind.endswith('FORMAT0_TINY')
        coverage.update(range(start, start + length))
    else:
        values = re.search(r'\b' + name + r'\[\]\s*=\s*\{([^}]+)\}', font).group(1)
        offsets = [int(v, 0) for v in re.findall(r'0x[0-9a-fA-F]+|\b\d+\b', values)]
        assert len(offsets) == int(count)
        coverage.update(start + offset for offset in offsets)
ui = (root / 'main/flappy_ui.c').read_text()
required = set(map(ord, re.findall(r'[\u3400-\u9fff]', ui))) | set(range(32,127))
assert required <= coverage, f'Missing glyphs: {required-coverage}'
assert 0x9F98 not in coverage, 'Negative case unexpectedly covered'
assert '&flappy_font_16' in ui and 'lv_font_montserrat' not in ui
print(f'Voice Flappy font coverage: PASS ({len(required)} characters)')
