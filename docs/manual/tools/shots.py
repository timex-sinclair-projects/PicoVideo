import emu, sys
from pathlib import Path
E = Path('../examples'); I = Path('../images')
pal = list(Path('demo_pal.bin').read_bytes())
which = sys.argv[1:]
def go(name, wait):
    m, ocr = emu.run_tap(E / f'{name}.tap', wait)
    print(f'== {name}'); print('\n'.join(l for l in ocr if l.strip()))
    Path(f'tmp/{name}.bin').write_bytes(m); return m
if 'detect' in which: go('detect', 8)
if 'mirror' in which: go('mirror', 10)
if 'font80' in which: go('font80', 10)
if 'spblack' in which:
    m = go('spblack', 10)
    emu.render(m, I / 'spblack_off.png', border=0, sb=0)
    emu.render(m, I / 'spblack_on.png', border=0, sb=1)
if 'ulaplus' in which:
    m = go('ulaplus', 25)
    emu.render(m, I / 'ulaplus.png', border=0, ula=1, pal=pal)
if 'text80' in which:
    m = go('text80', 120)
    emu.render(m, I / 'text80_basic.png', mode=5)
