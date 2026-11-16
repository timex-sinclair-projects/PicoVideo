# PicoVideo manual

The user and programmer's manual for PicoVideo firmware 4.3.

| File | What it is |
|---|---|
| `PicoVideo Manual.html` | The manual for reading on screen. One self-contained file. |
| `PicoVideo Manual (half-letter).pdf` | The printed edition, 5.5 × 8.5 in, 28 pages. |
| `PicoVideo Manual (booklet, print duplex).pdf` | The same pages imposed for a booklet. Print on letter paper, double-sided, flipping on the short edge, then fold and staple. |
| `examples/` | Every program in the manual, as source and `.tap`: BASIC (`.bas`, built with zmakebas), C (`c/`, z88dk) and Z80 (`asm/`). |
| `images/` | Screenshots and the board photo. |
| `manual_src.html`, `zine_src.html` | Sources. `{{file:...}}` tokens pull in the example listings. |
| `tools/` | Build and screenshot scripts. |

## Rebuilding

`tools/make.py` expands the listings and runs the `ts-program-manual` build
scripts (Python 3, Pillow, pypdf, Google Chrome, poppler):

```bash
python3 docs/manual/tools/make.py
```

## How the screenshots are made

Each example runs in ZEsarUX as a TS2068. `tools/emu.py` reads video memory
(`4000h`–`7FFFh`) from the emulator, and `tools/render` passes it through the
firmware's own `makeScanLine()` from `firmware/coreV/displayProc80.c`, built
for the host with the test stubs, using the photo palette. The pictures are
what the firmware draws for that memory. Build the renderer with:

```bash
cc -O1 -DPICOVIDEO_PHOTO_PALETTE=1 -Ifirmware/tests/stubs -Ifirmware docs/manual/tools/render.c firmware/coreV/displayProc80.c firmware/globals.c -o docs/manual/tools/render
```

`tools/shots.py` and `tools/emu.py` show how each screenshot was taken.

## The 3D board render

`images/board_render.jpg` is the current PicoVideo SMD board (v3.1), made
from the EasyEDA Pro project. `tools/epro2kicad.py` converts the `.epro2`
into a KiCad board (outline, copper, pours, silkscreen) and places KiCad
library 3D models on the parts by fitting each one's pads to the EasyEDA
pads. KiCad has no PJ-320B jack, so `tools/models/PJ-320B.wrl` is a simple
model built from the footprint's dimensions. Render with KiCad 10:

```bash
python3 docs/manual/tools/epro2kicad.py hardware/easyeda/PicoVideo-SMD.epro2 picovideo.kicad_pcb
kicad-cli pcb render -o board.png -w 2400 -h 1500 --rotate '-52,0,-28' --perspective --use-board-stackup-colors --quality high --floor picovideo.kicad_pcb
```

The solder mask color is set to blue in the script; the EasyEDA file doesn't
record one.
