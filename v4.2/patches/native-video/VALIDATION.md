# Validation record

Status: local source and build validation; physical-board results pending.

## Completed

- The published archive matches the reviewed SHA-256 in README.md.
- All seven patches apply to a fresh source extraction.
- Optimized host tests execute the actual bus and active renderer code.
- Native/ULA+ mode transitions, explicit overrides and `$80` fixed normal.
- All 256 native 64-column register values and attribute combinations;
  native/Speccy-black/FLASH and ULA+ palette-bank cases.
- All 192 ECM rows, independent pixel expectations and buffer guards.
- ULA+ PAPER borders; HOME/DOCK/EXROM gating and DF2 mirroring.
- Quad-color row addressing/output bounds and extended-write RAM bounds.
- Actual controller/ISR bodies exercise both free-buffer cases and repeated
  ownership cycles; native FLASH is checked over 128 simulated frames.
- The unchanged-source control reaches the expected ECM transition assertion.
- ARM Debug/Release builds from a freshly patched tree, using Pico SDK 1.5.1
  and GCC 12.3.0; timing-critical functions checked for SRAM placement.

Windows host compilation reports existing unused-variable warnings. No new
hardware timing or service-latency measurement is implied by these checks.

## Board validation to record

- [ ] Board revision and GAL marking; backup identity of installed firmware.
- [ ] Exact tested firmware hash and compiler/build configuration.
- [ ] BASIC and normal display; ordinary/ULA+ borders and native FLASH.
- [ ] Native-follow transitions between normal DF1, DF2, ECM and high resolution.
- [ ] Palette load followed by an unmodified ECM program; menu/game/boss/restart.
- [ ] All eight native high-resolution color pairs.
- [ ] Explicit ULA+ text and attributed high-resolution modes.
- [ ] Optional quad-color test, after ordinary modes; recovery if necessary.
- [ ] Extended ECM animation and bank-switch traffic, Debug then Release.

The local pvgallery.dck diagnostic cartridge can aid visual comparisons; it is
kept outside this source-only proposal. Use identical display/camera settings
for before/after captures. Report observed behavior and image hashes before
marking the proposal ready for adoption. The reported installed v1.4 binary
was not supplied; these changes are specifically against the V4.2 archive.

## Recommended optional photo profile

The profile was derived from measured ISO-50 DNG charts and verified with
three larger-chart DNG captures. The user also reports that the palette looks
good in visual tests with ECM sources: a basic qualitative check, not full
validation of this proposed firmware build. See PHOTO-PALETTE.md and the
measurement record. Core firmware board validation remains pending.

The freshly applied optional patch passes host assertions with the option both
off and on. ARM Debug builds pass for both choices; compiled UF2 palettes
match the stock and photo-fitted tables respectively. Timing-critical functions
remain in SRAM. These build checks do not expand the qualitative ECM report
into systematic board validation of the complete firmware candidate.
