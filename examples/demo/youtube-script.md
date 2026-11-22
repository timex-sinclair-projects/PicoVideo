# PicoVideo 4.5 demo: YouTube script

The video runs 5:58 from the moment you press SPACE. The program keeps time by
itself, so every timecode below is measured from that press. They assume the
60 Hz interrupt of a US TS2068. If the clock and the screen ever disagree,
follow the screen.

## Before you record

1. Switch the TS2068 off and on, so PicoVideo starts fresh.
2. Copy `picovideo-demo.tap` to your TS-Pico's `/TAP` folder, then type:

   ```text
   SAVE "tpi:cd /"
   LOAD "tpi:picovideo-demo.tap"
   LOAD ""
   ```

3. Record PicoVideo's VGA output, through a VGA-to-HDMI converter and capture
   device if you have them. Take the sound from PicoVideo's audio jack. The
   TS2068's own video output can't show modes 3, 5 and 7.
4. Set the capture to 60 frames a second, so the border stripes and color
   cycling record properly.
5. Start recording at the "press SPACE" screen, press SPACE, and trim the video
   to that moment. Then the chapters below are correct as printed.
6. Read the narration afterward, over the video.

Keys: **N** next scene, **P** pause and resume, **R** back to the start. Don't
use them during the real take. The program doesn't return to BASIC: switch off
when you're done.

Watch one take before you narrate. If any line on the "Read it back" screen
says FAIL, use the alternate line in that scene.

The video has flashing and fast-moving color, mainly at 1:16 and 3:48. The
warning in the description is there for that reason.

---

## Narration

### 0:00: Rings, with the title

This is a Timex Sinclair 2068, and this picture is coming out of it as VGA.
The board doing that is PicoVideo. Here's everything it can do.

### 0:16: How it works

PicoVideo plugs into the expansion bus and watches. Every write to the screen,
it copies, and a Raspberry Pi Pico draws the picture. There's nothing to
install.

`[0:25: the version number appears]` And it just told us its own firmware
version.

### 0:38: Photo colors

Sixteen colors: normal above, bright below. In four point five, each normal
color is about half as bright as its partner. Jon zero x zero measured those
levels with a camera.

### 0:58: Speccy Black

On a 2068, bright black is dark gray. Spectrum games don't expect that, so you
get boxes around things.

`[1:04: the box disappears]` Speccy Black fixes it. It's on when you switch on.

### 1:16: Border, stripes, FLASH

The border has all eight colors, and flash works.

`[1:24: the stripes start]` And yes, loading stripes. They move differently
from a television's, because the Pico draws on its own clock.

### 1:36: Second display file

The 2068 has a second screen in memory.

`[1:40: it flips to display file two]` One OUT to port two fifty-five switches
to it, and PicoVideo follows.

### 1:52: Extended color

Extended color is the 2068's own mode two. Every line of every character gets
its own pair of colors.

`[2:02: ordinary cell colors, for comparison]` Here's that picture with normal
attributes. And back.

### 2:12: 64 columns

Mode six is the Timex sixty-four column screen. It's sharp, but you get two
colors for the whole screen. You choose from eight pairs.

### 2:28: 64 columns in color

So PicoVideo adds mode seven. Same pixels, but every column has its own ink
and paper.

`[2:38: it drops to mode six]` That's what the Timex gives you.

`[2:41: color returns]` And that's mode seven.

### 2:46: 80 columns

Mode five is new as well: eighty columns by twenty-four rows. You don't draw
the letters. You store a character and a color, and the Pico draws it. It
starts with the IBM PC character set.

### 3:08: Changing the font

And that font is yours to change.

`[3:12: bold]` Bold.

`[3:16: backward]` Backward.

`[3:19: slanted]` Slanted. The text in memory never changes, only the font
inside the Pico.

### 3:26: Quad color

Mode three is quad color. Any pixel can be any of four colors, with no color
clash at all.

`[3:33: the colors change]` You pick the four, and changing them doesn't
redraw a thing.

### 3:48: ULAplus

This is everything a standard attribute can show. PicoVideo also follows the
ULAplus standard from the Spectrum world.

`[3:55: sixty-four colors appear]` Sixty-four colors.

`[4:01 to 4:17: STOP TALKING. Let the colors run.]`

### 4:20: Mirroring

This text needs both display files, and only one is filled.

`[4:26: the gaps fill in from the top]` With mirroring on, writes to the first
file land in the second, inside the Pico.

### 4:38: Mode pinning

With the palette on, a program can pin the mode.

`[4:43: the number after OUT 255 changes]` It's asking for other modes now,
and the screen holds at eighty columns.

`[4:49: amber]` In any colors you like.

### 4:56: Read it back

Almost everything can be read back, so the program is testing the board live:
the registers, the font, video memory, and the paging that keeps bank writes
off the screen.

`[5:12: the result appears]` Twelve out of twelve.

> Alternate, if any line fails: "Not quite twelve. That's why it's a beta."

### 5:20: Sound

The audio jack carries the 2068's own sound.

`[5:23 to 5:32: let the tune play]`

### 5:34: End card

PicoVideo was designed by Jeff Burrell, and it's open source. Firmware four
point five is a beta, so if you try it, tell us what you find. The link is in
the description.

`[hold the end card until 5:58]`

---

## Notes for reading

- Read it slower than feels natural. The gaps are for the pictures.
- If you run long, cut "It starts with the IBM PC character set" first.
- Protect the silence in ULAplus, from 4:01 to 4:17. It's the best shot in the
  video.
- The tone is showing a friend your board, not selling one.

---

## For YouTube

**Title**

```text
PicoVideo 4.5: VGA, 80 columns and 64 colors on the Timex Sinclair 2068
```

**Description**

```text
PicoVideo is an open-source expansion board that gives the Timex Sinclair 2068 a VGA picture, using a Raspberry Pi Pico. It shows the TS2068's own video modes and adds quad color, 64 columns with color, 80-column text with a loadable font, and the 64-color ULAplus palette. This video is one demo program running on the TS2068, showing each feature of firmware 4.5.

Warning: this video has flashing and fast-changing colors.

Firmware 4.5 (beta): https://github.com/timex-sinclair-projects/PicoVideo/releases/tag/v4.5
Project, manual and source: https://github.com/timex-sinclair-projects/PicoVideo
More Timex/Sinclair: https://timexsinclair.com

4.5 is a beta. V4 is still the stable firmware. Don't use 4.3 or 4.4: those builds are broken.

PicoVideo is designed by Jeff Burrell. Photo palette and fixes by jon0x0.
The demo uses ZX0 compression by Einar Saukas.

Chapters
0:00 PicoVideo
0:16 How it works
0:38 Photo colors
0:58 Speccy Black
1:16 Border, stripes and FLASH
1:36 Second display file
1:52 Extended color
2:12 64 columns
2:28 64 columns in color
2:46 80-column text
3:08 Changing the font
3:26 Quad color
3:48 ULAplus: 64 colors
4:20 Mirroring
4:38 Mode pinning
4:56 Live self-test
5:20 Sound
5:34 Credits

#TimexSinclair #TS2068 #RetroComputing
```

**Tags**

```text
PicoVideo, Timex Sinclair 2068, TS2068, VGA, ULAplus, Raspberry Pi Pico, RP2040, ZX Spectrum, retrocomputing, 80 columns
```

**Thumbnail:** a frame from the sunset at 3:28, or from ULAplus at 4:08.
