# PicoVideo

PicoVideo is an expansion board for the Timex/Sinclair TS2068 that uses a
Raspberry Pi Pico to generate VGA video. It gives a clearer picture than the
stock video output and adds new video modes: four-color, 64 columns with
individual attributes, and 80 columns with a font you can load.

> **Still in development. Use at your own risk.**

## Which firmware should I use?

| Version | Status | Get it |
|---|---|---|
| **V4** | Stable. Shipped on production boards. Fixes stray "rogue" pixels on boards with the faster Atmel PLDs. | [`V4/picoTsVid.uf2`](V4/picoTsVid.uf2) |
| **4.3** | Beta. Everything in 4.2 (Speccy Black, DF2 writes through DF1, video modes 3, 5 and 7: see [features](docs/features.md)), plus bug fixes and the photo-matched palette. Not yet tested on hardware. | [Releases](https://github.com/timex-sinclair-projects/PicoVideo/releases/tag/v4.3) |
| **4.4** | Beta. Everything in 4.3, plus support for the [TS-2068 Megabank](https://github.com/timex-sinclair-projects/TS-2068-Megabank): it ignores screen writes that go to a Megabank bank. Not yet tested on hardware. | [Releases](https://github.com/timex-sinclair-projects/PicoVideo/releases/tag/v4.4) |

Older builds (V2.0, V3.0 RC1, 4.2, 4.3) are on the [Releases page](https://github.com/timex-sinclair-projects/PicoVideo/releases) too.

To flash the board, follow [V4/readme.md](V4/readme.md). In short: remove the
board, hold **BOOTSEL** on the Pico while you plug in USB, then drag the
`.uf2` file onto the `RPI-RP2` drive that appears.

## What's in this repository

| Folder | Contents |
|---|---|
| [`firmware/`](firmware/) | Pico C source (Pico SDK, CMake). Core 0 handles the TS2068 bus and core 1 generates VGA. |
| [`pld/`](pld/) | WinCUPL source and JEDEC file for the GAL16V8/ATF16V8 bus-decode PLD. |
| [`hardware/`](hardware/) | Current SMD board in EasyEDA Pro (schematic, PCB, BOM), plus the earlier Altium design and its Gerbers. |
| [`docs/`](docs/) | Programmer's reference for the 4.2 features. |
| [`examples/`](examples/) | 80-column text routines for the TS2068 (z88dk C and Z80 assembler). |
| [`V4/`](V4/) | The V4 firmware image and flashing guide, kept at this path so existing links keep working. |
| `PicoVideo manual.pdf` | User manual. |

## History

Earlier, each release was uploaded as a zip in its own folder. Those zips are
now unpacked, and the git history replays them in order:
V2.0 → V3.0 RC1 → V4 → 4.2. Run `git log -p -- firmware/` to see exactly what
changed between releases. The original zips are still in the history:
`git log --diff-filter=D --name-only -- "*.zip"`.

## Credits

Hardware, PLD and firmware by J. Burrell ([@jburrell7](https://github.com/jburrell7)).
Published by Timex Sinclair Projects under the [MIT License](LICENSE).
