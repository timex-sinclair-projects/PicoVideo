# Native video and ULA+ display updates

PicoVideo's palette and extended display modes make it possible to explore a
wide range of TS2068 software and graphics. This proposed series builds on
those features, with a focus on native mode transitions while a palette is
active, consistent color selection, and scanline buffer handling.

The source is distributed in `v4.2/picoTsVid4_2.zip`, so these changes are
provided as small, readable patches against that archive. The archive and
existing firmware images are unchanged. Each patch can be reviewed separately.
This proposal is source-tested; board validation is still pending.

## Patch series

| Patch | Change |
|---|---|
| 0001 | Native mode tracking with ULA+; CLUT0 PAPER border; native 64-column color decoding. |
| 0002 | ISR flag visibility; end-of-image free-buffer selection; 32-frame native FLASH interval; initial border consistency. |
| 0003 | Keep port F4 bank writes separate from extended video address writes. |
| 0004 | Quad-color linear row selection and bounded scanline output, in both renderer copies. |
| 0005 | Consistent 48 KiB display-RAM declarations and bounded experimental extended writes. |
| 0006 | Accept `PICO_SDK_PATH` through CMake or the environment. |
| 0007 | Host assertions using the actual bus, renderer, controller and interrupt code. |

## Register compatibility: please review before adoption

The [ULA+ specification](https://zxdesign.itch.io/ulaplus) identifies native
port `$FF` as the primary video-mode control on Timex-capable implementations.
Allowing native mode changes with an active palette is consistent with that
behavior. PicoVideo's separate `$41` mode-control register is an extension,
however: using `$41=0` for native-follow and `$41=$80` for fixed normal are
PicoVideo-specific policies, not standard ULA+ commands. This series preserves
that extended interface; it does not implement the standard's optional
mode-group selector mirroring or claim complete ULA+ specification compliance.

With ULA+ enabled, register `$41=0` currently selects fixed normal mode.
This proposal changes zero to follow the native `$FF` mode bits, allowing a
palette loader to leave subsequent native software in control of its modes.
For example, a program's `$FF=2` transition then selects DF2 scanline attributes
for ECM rather than continuing with ordinary DF1 cell attributes.

`$41=$80` provides explicit fixed normal mode instead. Other nonzero values
retain their existing low-nibble mode selection, including `$02` ECM, `$05`
80-column text and `$07` attributed high resolution. Native selection with
ULA+ disabled is unchanged. This is an observable interface change: callers
that intentionally used zero to hold normal mode must use `$80`. An opt-in
native-tracking control is also a possible alternative if compatibility with
those callers is preferred. The choice remains open for maintainer review.

Palette contents are unchanged in the core series. ULA+ still uses the attribute FLASH/BRIGHT bits
for palette-bank selection. The 64-column update does not add native BRIGHT.

## Applying the proposal

From the repository root, with Python 3 and Git available:

```sh
python v4.2/patches/native-video/apply.py /path/to/new/work-directory
```

The script verifies SHA-256 of the published archive, extracts source without
its cached build/IDE directories, normalizes source text to LF, and applies
all seven patches. It refuses an existing destination. The reviewed archive
SHA-256 is `9ff9ccacd694e35cda72ed3443e35e4f911a6af729f16375415bc07cc24f2a7b`.
The patches can also be applied manually, in `series.txt` order, from the
extraction root containing `picoCode/`; normalize CRLF source files to LF first.

## Host tests

From the extracted work directory, using a native C compiler:

```sh
python tests/run.py --cc cc
```

On Windows, `tests\run_windows.cmd` locates Visual Studio C++ using `vswhere`.
Alternatively run `python tests/run.py --cc cl` from a developer command prompt.
Assertions are enabled; the Pico SDK is not needed for these host tests.

For the expected-failure control, supply a separate unmodified `picoCode`
directory extracted from the same archive:

```sh
python tests/run.py --cc cc --upstream-dir /path/to/original/picoCode
```

Success means the unchanged source reaches and fails the native ECM transition
assertion. It does not mean the unchanged source passes the proposed behavior.
The controller/ISR test extracts their actual bodies and omits SDK setup; it
does not model RP2040 hardware timing. See [VALIDATION.md](VALIDATION.md).

## Firmware build

Use the project's usual Pico SDK/toolchain setup and a fresh build directory:

```sh
cmake -S picoCode -B build -DPICO_SDK_PATH=/path/to/pico-sdk -DPICO_BOARD=pico -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

An appropriate CMake generator and Pico ARM compiler must be installed. SDK
1.5.1 and arm-none-eabi GCC 12.3.0 were used for the local Debug/Release checks.
No compiler, SDK, generated firmware image or version-register change is
included in this proposal.

## Scope

The core series does not change the resistor DAC, native palette, GPIO drive,
PIO timing or CPU clock. It does not include Amiga artwork or the gallery cartridge. The separate
recommended optional photo-palette patch is described below. The experimental mode-9 write guard does
not add its unfinished renderer. Saved-screen and host tests help isolate
logic changes; they do not establish electrical bus timing or compatibility
with every installed firmware/GAL revision.

## Recommended optional photo-fitted palette

The additional `optional-photo-palette.patch` supplies the photo-fitted native
normal palette as a recommended, opt-in profile. It is not in `series.txt`;
stock colors remain the default even after the optional source patch is applied.
Use `apply.py DESTINATION --photo-palette`, then build with
`-DPICOVIDEO_PHOTO_PALETTE=ON`. The user reports a good basic visual result with
ECM sources. [PHOTO-PALETTE.md](PHOTO-PALETTE.md) documents its creation, the
RGB-board/LEVELHIKE/display target, measured verification, startup Speccy-black
choice and uncalibrated bright bank. Measurements are supplied separately.
