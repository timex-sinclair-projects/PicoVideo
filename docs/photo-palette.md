# Photo palette

The photo palette is the firmware default. Build with
`-DPICOVIDEO_PHOTO_PALETTE=OFF` for the original stock colors.

## Current palette (revision 3)

Each normal color uses red 5, green 5 and blue 2 in the channels it lights,
and each BRIGHT color keeps the stock maximum: red 7, green 7, blue 3. Every
normal color is therefore dimmer than its BRIGHT version. The previous
revision (below) gave normal and BRIGHT blue, red and magenta the same
output codes. This revision was proposed and measured by jon0x0 in
[issue #10](https://github.com/timex-sinclair-projects/PicoVideo/issues/10).

Order: black, blue, red, magenta, green, cyan, yellow, white. Codes are
G3R3B2 bytes: `32G + 4R + B`.

| Bank | Hex | Decimal |
|---|---|---|
| Normal | 00 02 14 16 A0 A2 B4 B6 | 0, 2, 20, 22, 160, 162, 180, 182 |
| BRIGHT, stock | 25 03 1C 1F E0 E3 FC FF | 37, 3, 28, 31, 224, 227, 252, 255 |
| Normal, stock (`OFF`) | 00 02 10 12 80 82 90 92 | 0, 2, 16, 18, 128, 130, 144, 146 |

The startup border is normal white, `B6` (182). The profile starts with
Speccy Black on, so BRIGHT black (37) is drawn as black; register `$42 = 0`
turns it off again.

### How it was measured

The levels come from jon0x0's CAL50 test on an ASUS VS238 monitor in sRGB
mode. CAL50 shows each channel code next to full-scale and black patches and
rotates all three through the same three screen positions over the pages
1, 2, 3, 1. Each page was photographed as a DNG (ISO 50, 1/60 s, f/1.8), and
the light at each position was read from the raw Bayer data as
`(test - black) / (full - black)`. The table is the median across positions,
as a percentage of that channel's full light:

| Code | Red | Green | Blue |
|---|---|---|---|
| 0 | 0.0 | 0.1 | 0.0 |
| 1 | 2.4 | 4.3 | 17.4 |
| 2 | 7.8 | 11.6 | 51.1 |
| 3 | 17.1 | 22.5 | 99.5 |
| 4 | 27.6 | 35.4 | |
| 5 | 44.9 | 52.1 | |
| 6 | 69.6 | 73.6 | |
| 7 | 99.7 | 99.6 | |

Relative to the maximum code, normal is 45.1% for red (R5), 52.3% for green
(G5) and 51.3% for blue (B2). The stock normal levels are 27.7%, 35.5% and
51.3%. For comparison, Fuse's normal level of 192/255 is about 52.7% linear
light on an ideal sRGB display. R6 is already about 70%, so R5 is the nearest
code to half light.

These are measurements of light from the whole PicoVideo and monitor path on
one board and one display, not of DAC voltages. Position effects reached about
4 percentage points. The issue suggests repeating the test on another board
and display, with a longer exposure, and measuring the DAC voltages.

### Test

`firmware/tests/run.py` checks that every nonblack normal color differs from
its BRIGHT version and is no brighter in any channel. It runs for both the
stock palette and the photo palette (`--photo`).

## Superseded: revision 2 (firmware 4.3 beta)

> Firmware 4.3 shipped this earlier revision: normal codes 0, 3, 28, 31, 233,
> 231, 221, 219 and a startup border of 219. It is kept here as a record.
> Its normal blue, red and magenta equal their BRIGHT versions, and its
> measurements were taken with an unrecorded monitor preset that flattened
> the top output levels. Use revision 3 above. The text below is jon0x0's
> original write-up from PR #1, and
> [`photo-palette-measurements.json`](photo-palette-measurements.json)
> holds its data.

The photo-fitted palette is recommended for users who prefer the appearance
verified in this comparison. It is supplied as a separate optional patch,
not merely as a possible future experiment. The stock palette remains the
default so users can choose either rendering and maintainers can review the
functional changes independently of a color preference.

### What it matches, and why

The target is one TS2068 edge-connector RGB board, through a LEVELHIKE
GenesisCable RGB-to-HDMI converter, displayed on the same monitor used for
PicoVideo. This supplies a practical reference image from the original
computer's RGB outputs. Comparing both paths through the same display and
camera reduces differences caused by unrelated viewing conditions.

The fit includes the complete reference chain: RGB-board levels, converter,
display processing and camera response. It is not a universal analog-voltage
calibration or a reconstruction of the original NTSC composite decoder.
The reference RGB board does not carry BRIGHT, so it cannot establish an
original-computer bright palette. TS2068 chroma phase angles alone would not
define RGB colors without luminance/chroma amplitudes and decoder behavior.

### Palette and behavior

Order: black, blue, red, magenta, green, cyan, yellow, white.

| Bank | Decimal G3R3B2 codes |
|---|---|
| Photo-fitted normal | 0, 3, 28, 31, 233, 231, 221, 219 |
| Bright, stock values | 37, 3, 28, 31, 224, 227, 252, 255 |

The normal native palette and native border use the fitted values. The
profile also starts with Speccy-black enabled, suppressing bright-black gray
for Spectrum-derived software; register `$42=0` restores that gray when wanted.
Thus the BRIGHT palette *entries* remain stock, while startup bright-black
appearance follows the profile's Speccy-black preference. With the option off,
stock palette values, startup border and Speccy-black default are retained.

This is a native firmware profile. It does not upload or replace an arbitrary
application's ULA+ CLUT. With ULA+ disabled, BASIC, native FLASH and native mode
changes work normally. Do not run a palette uploader merely to activate this
profile; programs that deliberately load a ULA+ palette still control their
own CLUT. Source-mode/register fixes are supplied by the core patch series.

### Measurement procedure

1. Display a solid-color reference chart through the edge-RGB chain. Display
   five DAC-code pages through PicoVideo, covering all 256 G3R3B2 output codes,
   with repeated white anchors. Use the same display settings and input
   configuration as far as the two physical video paths permit.
2. Capture DNG plus JPEG pairs in camera Pro mode. The accepted set used
   ISO 50, 1/60 second and fixed 4700 K white balance. Keep exposure, focus,
   framing and display processing fixed. Check clipping before completing
   the series; the earlier higher-ISO captures were unsuitable for fitting.
3. Register each chart to logical screen coordinates. Inspect JPEG overlays
   to verify patch locations, then transform those regions into the DNG's
   raw-sensor coordinates, accounting for the JPEG's crop.
4. Read visible Bayer samples directly with rawpy/LibRaw. For each sensor
   channel, subtract the DNG black level and divide by white minus black.
   Average R sites, both G-site types together, and B sites within broad patch
   interiors, excluding labels, edges and numerals. Check fractions at or
   above 98% of white; accepted ISO-50 measurement regions had no clipping.
   JPEGs are used for visual QA, not numerical color fitting.
5. Preserve linear sensor-space RGB. Do not independently white-balance,
   auto-expose, or normalize each photograph's white to the reference white:
   that would conceal some of the brightness differences being measured.
   Apply one common fixed camera-WB channel weighting to comparison distances.
6. Fit a robust additive channel-level model to all measured DAC codes. For
   each sensor channel, model output as an intercept plus contributions for
   the selected red level (7 nonzero levels), green level (7), and blue level
   (3): 18 coefficients per sensor channel. Iteratively downweight large
   residuals. This smooths spatial/moire variation rather than selecting a
   code solely because one small patch happens to match. Rank candidate codes
   against the eight normal reference colors with the common channel weights.
7. Upload the candidate and photograph large verification swatches. Refine
   using those larger areas rather than trusting the small-patch fit alone.
   Revision 2 was checked against three DNG captures with the same settings.
   Average broad regions and record repeatability; nearby pixels are not
   independent samples when display/camera patterns are present.

The accompanying [`photo-palette-measurements.json`](photo-palette-measurements.json) records the 256-code
measurements, fitted model, reference and revision-2 wide-area summaries,
source capture names and camera weighting. Original photographs are not
included in this source proposal. These data support the documented result;
repeating the experiment requires new captures or the original DNGs.

### Results and basic visual check

Revision-2 red and magenta closely match the RGB reference in camera space;
green and cyan also remain close. Blue's dominant sensor signal is about 4%
low despite maximum blue DAC output. Yellow's green signal is about 13% low;
white is approximately 4%, 10%, 6% low in sensor R/G/B. These are linear sensor
signals, not perceived brightness or Delta E. The three-frame weighted sensor
RMSE was approximately 0.0086, compared with 0.0248 for the earlier revision-1
verification; that comparison concerns palette revisions, not a new stock
firmware before/after measurement.

The user additionally reports that the photo-fitted palette looks good in
visual tests with ECM sources. This is a useful basic visual check and supports
recommending the profile as an optional choice. It does not establish the exact
firmware image used or complete the separate core-firmware hardware test list.
No quantitative ECM-photo comparison is inferred from that visual report.

Dot crawl, moire and positional response remain sources of uncertainty.
Averaging can reduce some variation but cannot remove systematic converter
shifts. The hardware's two-bit blue channel also limits attainable matches.
Bright colors remain uncalibrated because the RGB reference lacks BRIGHT.

## Building with or without it

The palette is on by default:

```bash
cmake -S firmware -B build -G Ninja -DPICO_SDK_PATH=/path/to/pico-sdk
```

To build with the original stock colors and Speccy Black off:

```bash
cmake -S firmware -B build-stock -G Ninja -DPICO_SDK_PATH=/path/to/pico-sdk -DPICOVIDEO_PHOTO_PALETTE=OFF
```

To test it, run `python3 firmware/tests/run.py --photo`. Run it without `--photo` to test the stock colors.
