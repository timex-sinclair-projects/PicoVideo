# PicoVideo firmware

This is the Raspberry Pi Pico (RP2040) firmware for PicoVideo. `macros.h`
sets `SOFTWARE_VER` (currently `0x46`; read it back from ULA+ register `$7F`).

| File | Role |
|---|---|
| `picoTsVid.c` | Entry point. Sets the system clock to 250 MHz, starts VGA on core 1, then runs the bus handler on core 0. |
| `coreB/busMain.c` | Core 0. Snoops the TS2068 bus, mirrors video memory writes and handles the I/O ports (ULA+, mode `$FF`, PicoVideo control registers). Skips a screen write when port `$F4` pages the chunk away or a TS-2068 Megabank bank owns it (its registers `OWN2`/`OWN3`, written through ports `$FD`/`$FC`). |
| `coreV/vidMain.c` | Core 1. Runs the VGA timing and DMA. |
| `coreV/vgaPioSm.pio` | PIO state machines for the VGA output. |
| `coreV/displayProc80.c` | Scanline renderer for all modes, including 80 columns. This is the file the build uses. |
| `globals.c`, `globals.h` | Shared state, color tables and the built-in font (CP437 8×8). Every file gets the shared variables from `globals.h`, so don't declare them with a hand-written `extern`. |

## Building

You need the [Pico SDK](https://github.com/raspberrypi/pico-sdk), an
`arm-none-eabi` GCC toolchain, CMake and Ninja.

```bash
cmake -S firmware -B build -G Ninja -DPICO_SDK_PATH=/path/to/pico-sdk
```

```bash
ninja -C build
```

This produces `build/picoTsVid.uf2`, a Release build unless you set
`CMAKE_BUILD_TYPE`. In every build type, `CMakeLists.txt` compiles
`coreB/busMain.c` with `-Og`, as V4 was. **Don't remove that setting:** the bus
loop's repeated pin reads are its delays, and higher optimization packs them
too tightly to work on a board.

What `-Og` produces can change with the compiler, so check the linked bus
handler before you flash or share a build:

```bash
python3 firmware/tests/check_arm.py build/picoTsVid.elf
```

How the released images were built:

| Release | Pico SDK | Build type | Note |
|---|---|---|---|
| V4 and 4.2 | 1.5.1, on Windows | Debug | |
| 4.3 and 4.4 | 2.1.1, GCC 13.3 | Release | Test builds, not released publicly. They read the bus too soon, and lose border, video mode and ULAplus writes. |
| 4.5 | 2.1.1, GCC 13.3 | Release | `busMain.c` at `-Og`. The released image was tested on one board; its reads of `$46`–`$48` are too slow. |
| 4.6 | 2.1.1, GCC 13.3 | Release | Register reads answered in time. A build of this change was tested on one board. |

4.3 and 4.4 have two separate faults, both fixed in the source:

- **Bus timing.** In those Release builds, the address high byte is last
  sampled 12 ns after the buffers switch, against 44 ns in V4 and 4.2, so it
  reads as the low byte. Building `busMain.c` with `-Og` restores V4's timing.
- **Lost writes.** `busMain()` never returns, so without a memory barrier the
  compiler dropped its stores to globals that only the video core reads. The
  barrier at `endoFunc` keeps them.

On 22 Nov 2026 a Release build of 4.5 with only the barrier did not work on a
board, and the same build with the `-Og` setting added did (Pico SDK 2.1.1,
GCC 13.3, photo palette, one board). Other build types, palettes and compilers
have only been checked in simulation.

The build embeds the date, so to reproduce an image byte for byte use the same
SDK, compiler and build type, and set `SOURCE_DATE_EPOCH` to the day it was
built. For 4.5 and 4.6, build tag `v4.5` or `v4.6` with
`SOURCE_DATE_EPOCH=1795348800` (22 Nov 2026). The checks here are not hardware tests: try an image on a real
board before you share it.

### Build options

| Option | Default | Effect |
|---|---|---|
| `PICOVIDEO_PHOTO_PALETTE` | `ON` | Uses the photo palette (normal colors at about half the light of BRIGHT) and starts with Speccy Black on ([details](../docs/photo-palette.md)). Set it to `OFF` for the original stock colors. |

## Tests

Host-side regression tests compile the real bus handler, renderer and video
interrupt code with a desktop C compiler, using stub Pico headers:

```bash
python3 firmware/tests/run.py
```

```bash
python3 firmware/tests/run.py --photo
```

The first command tests the stock palette and the second tests the photo
palette. On Windows, `firmware\tests\run_windows.cmd` runs the same tests
with MSVC.

These tests can't show what the ARM compiler did to the bus loop, because
their stubbed GPIO calls force every global to be stored. After each firmware
build, also run:

```bash
python3 firmware/tests/check_arm.py build/picoTsVid.elf
```

It runs the linked `busMain()` in a small Cortex-M0+ interpreter and feeds it
bus cycles. It checks what the video core would find in RAM and what the Z80
reads back. It fails if the gaps between the bus accesses are shorter than in
the V4 image that shipped, if a register read is answered later than 516 ns, or
if a screen write keeps the loop busy for more than 760 ns. The 516 ns limit
comes from a board test of 4.5: its reads of registers `$46`–`$48` took up to
604 ns and often returned wrong values.
It needs only Python. It doesn't model the PLD, the Z80's signal timing or
bus contention, so a pass is no substitute for a test on a board.
