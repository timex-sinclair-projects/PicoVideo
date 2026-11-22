#!/usr/bin/env python3
"""Build picovideo-demo.tap.

    python3 build.py

Needs sjasmplus, zmakebas and z88dk's z88dk-zx0 (on PATH or in ~/z88dk).
"""
import hashlib
import os
import shutil
import subprocess
from pathlib import Path

import screens

HERE = Path(__file__).resolve().parent
BUILD = HERE / 'build'

# The running order, with each scene's length in seconds. The narration's
# timecodes follow from this: changing a length moves every cue after it.
SCENES = (('Opening', 16), ('Intro', 22), ('Colors', 20), ('Speccy', 18), ('Border', 20),
          ('Mode1', 16), ('Mode2', 20), ('Mode6', 16), ('Mode7', 18), ('Mode5', 22),
          ('Font', 18), ('Quad', 22), ('Ula', 32), ('Mirror', 18), ('Pinned', 18),
          ('Checks', 24), ('Sound', 14), ('Ending', 24))
FPS = 60.1145


def tool(name):
    found = shutil.which(name)
    local = Path.home() / 'z88dk/z88dk/bin' / name
    if found or local.exists():
        return found or str(local)
    raise SystemExit(f'{name} not found')


def packed(data):
    path = BUILD / hashlib.sha1(data).hexdigest()[:16]
    out = path.with_suffix('.zx0')
    if not out.exists():
        path.write_bytes(data)
        subprocess.run([tool('z88dk-zx0'), '-f', str(path), str(out)], check=True, capture_output=True)
        path.unlink()
    return out


def block(flag, body):
    data = bytes([flag]) + body
    check = 0
    for byte in data:
        check ^= byte
    return (len(data) + 1).to_bytes(2, 'little') + data + bytes([check])


def main():
    BUILD.mkdir(exist_ok=True)

    at, lines = 0, [f'SCENES equ {len(SCENES)}', 'timeline:']
    for name, seconds in SCENES:
        lines.append(f'        dw {round(at * FPS)},s{name}')
        print(f'{at // 60}:{at % 60:02d}  {name}')
        at += seconds
    lines.append('        dw 65535')
    print(f'{at // 60}:{at % 60:02d}  (end card holds)')
    (BUILD / 'timeline.inc').write_text('\n'.join(lines) + '\n')

    (BUILD / 'font.raw').write_bytes(screens.FONT)
    lines = ['fontData:', '        incbin "build/font.raw"']
    for name, data in screens.tables().items():
        lines.append(f'{name}:')
        for i in range(0, len(data), 16):
            lines.append('        db ' + ','.join(str(v) for v in data[i:i + 16]))
    for name, data in screens.all_screens().items():
        lines += [f'img_{name}:', f'        incbin "build/{packed(bytes(data)).name}"']
    (BUILD / 'data.inc').write_text('\n'.join(lines) + '\n')

    subprocess.run([tool('sjasmplus'), '--nologo', '--msg=war', '--sym=build/demo.sym', 'demo.asm'],
                   cwd=HERE, check=True)

    (BUILD / 'loader.bas').write_text(
        '10 REM PicoVideo 4.5 feature demo\n'
        '20 BORDER 0: PAPER 0: INK 7: CLEAR 32767\n'
        '30 PRINT "Loading the PicoVideo demo..."\n'
        '40 LOAD "" CODE 32768\n'
        '50 RANDOMIZE USR 32768\n')
    subprocess.run([tool('zmakebas'), '-a', '10', '-n', 'pvdemo', '-o', 'build/loader.tap', 'build/loader.bas'],
                   cwd=HERE, check=True)
    code = (BUILD / 'demo.bin').read_bytes()
    header = bytes([3]) + b'pvdemo    ' + len(code).to_bytes(2, 'little') + b'\x00\x80\x00\x80'
    tap = (BUILD / 'loader.tap').read_bytes() + block(0, header) + block(255, code)
    (HERE / 'picovideo-demo.tap').write_bytes(tap)
    print(f'code {len(code)} bytes, to {0x8000 + len(code):04X}h; tap {len(tap)} bytes')


if __name__ == '__main__':
    main()
