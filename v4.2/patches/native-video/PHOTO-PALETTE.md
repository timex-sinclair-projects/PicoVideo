# Recommended optional photo-fitted palette

The photo-fitted palette is recommended for users who prefer the appearance
verified in this comparison. It is supplied as a separate optional patch,
not merely as a possible future experiment. The stock palette remains the
default so users can choose either rendering and maintainers can review the
functional changes independently of a color preference.

## What it matches, and why

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

## Palette and behavior

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

## Measurement procedure

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

The accompanying `photo-palette-measurements.json` records the 256-code
measurements, fitted model, reference and revision-2 wide-area summaries,
source capture names and camera weighting. Original photographs are not
included in this source proposal. These data support the documented result;
repeating the experiment requires new captures or the original DNGs.

## Results and basic visual check

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

## Applying and selecting the recommended profile

From the repository root, apply the seven core patches plus this separate one:

```sh
python v4.2/patches/native-video/apply.py /path/to/new/work-directory --photo-palette
```

Alternatively apply `optional-photo-palette.patch` after the seven files in
`series.txt`. The optional patch is deliberately absent from that default list.
After applying it, the CMake option still defaults to OFF. Enable it explicitly:

```sh
cmake -S picoCode -B build-photo -DPICO_SDK_PATH=/path/to/pico-sdk -DPICO_BOARD=pico -DCMAKE_BUILD_TYPE=Debug -DPICOVIDEO_PHOTO_PALETTE=ON
cmake --build build-photo
```

Use `tests/run.py --photo` or `tests\run_windows.cmd --photo` to exercise the
selected palette. Run without `--photo` to verify stock defaults in the same
patched source. Keep a separate build directory for each selected profile.
