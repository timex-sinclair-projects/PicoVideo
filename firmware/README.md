# PicoVideo firmware

This is the Raspberry Pi Pico (RP2040) firmware for PicoVideo. `macros.h`
sets `SOFTWARE_VER` (currently `0x44`; read it back from ULA+ register `$7F`).

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

This produces `build/picoTsVid.uf2`.

Released images were built on Windows with Pico SDK 1.5.1 in a Debug build.
The source also builds cleanly with Pico SDK 2.1.1 and GCC 13.3. A build with
a different SDK, compiler or build type will not match the released `.uf2`
byte for byte, so test it on real hardware before you share it.

### Build options

| Option | Default | Effect |
|---|---|---|
| `PICOVIDEO_PHOTO_PALETTE` | `ON` | Uses the camera-matched normal colors and starts with Speccy Black on ([details](../docs/photo-palette.md)). Set it to `OFF` for the original stock colors. |

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
