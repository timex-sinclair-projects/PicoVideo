# PicoVideo firmware

This is the Raspberry Pi Pico (RP2040) firmware for PicoVideo. `macros.h`
sets `SOFTWARE_VER` (currently `0x45`; read it back from ULA+ register `$7F`).

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
`CMAKE_BUILD_TYPE`. Before you flash or share it, check the linked bus handler:

```bash
python3 firmware/tests/check_arm.py build/picoTsVid.elf
```

How the released images were built:

| Release | Pico SDK | Build type | Note |
|---|---|---|---|
| V4 and 4.2 | 1.5.1, on Windows | Debug | |
| 4.3 and 4.4 | 2.1.1, GCC 13.3 | Release | **Broken: don't use these images.** Border, video mode and ULAplus changes never reach the video core. |

4.3 and 4.4 were built without a memory barrier in `busMain()`. That function
never returns, so the compiler dropped its stores to globals that only the
video core reads. Building those tags as Debug avoids the fault.

The build embeds the date, so to reproduce an image byte for byte use the same
SDK, compiler and build type, and set `SOURCE_DATE_EPOCH` to the day it was
built. The checks here are not hardware tests: try an image on a real board
before you share it.

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

It runs the linked `busMain()` in a small Cortex-M0+ interpreter, feeds it bus
cycles and checks what the video core would find in RAM. It needs only Python.
It doesn't model the PLD or signal timing.
