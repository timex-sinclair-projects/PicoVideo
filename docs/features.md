## Features

### Speccy Black

The Speccy Black mode is used to turn off the bright black color in the Pico Video board. This feature is most useful when running ported Sinclair Spectrum software. The Sinclair Spectrum black color is unaffected by the bright bit in the attribute byte. Spectrum programmers use the bright black for collision detection and other functions and, since the Spectrum has only one shade of black, this is not evident when the game is run on the Spectrum. On the TS2068, however, the use of “bright black” shows as a dark gray outline in the display.
```
Enabling Speccy black:				Disable Speccy black
OUT 48955, 66					OUT 48955, 66		
OUT 65339, 1 (any non-zero value will do)		OUT 65339, 0 (must be zero)
```

### 64 Column Mode Using Mirrored Memory
This feature allows the user to write to DF2 by banking it into DF1 memory space. This eliminates the requirement to run the Video Mode Change Service and move the BASIC data structures around. It is important to remember that TS2068 sees all video writes as being done to DF1. Since there is no readback from the Pico Video memory, this mode should be considered to be a write only mode. This makes it of limited utility, but it may still be useful.
```
Bank DF2 into DF1							Bank DF1 back into DF1
OUT 48955, 67	; access the control register			OUT 48955, 67
OUT 65339, 1		; any non-zero value will do			OUT 65339, 0
```

Note: this video mode is used only used to select the bank to be written. The actual display mode should be set to video modes 3, 6 and 7.

### Enhanced Video Modes

In addition to the standard video modes, the Pico Video board adds several new video modes. To activate the video mode, write the mode number to I/O port $FF

#### Quad Color Mode (Mode number 3)
This mode uses both display files to output a four color display. If using BASIC, this requires executing the Video Mode Change Service to move various data structures in memory.
In this 32 column mode, a byte is read from display file 1 and display file 2, the values are concatenated and then pairwise combined to form an index into a special four value color table. The next figure shows how this is done:
```
DF1 Byte	DF2 Byte
Color(7)	Color(6)	Color(5)	Color(4)	Color(3)	Color(2)	Color(1)	Color(0)
df1(7)	df1(6)	df1(5)	df1(4)	df1(3)	df1(2)	df1(1)	df1(0)	df2(7)	df2(6)	df2(5)	df2(4)	df2(3)	df2(2)	df2(1)	df2(0)
```

The memory map for this mode is linear meaning that values from display file 1 and display file 2 are read sequentially and do not use the TS2068 display memory memory mapping. This was done to speed up memory the firmware as well a provide a more intuitive memory map
The color table contains four values that can be written as follows:
```
OUT 48955, 68	; access the color table address register
OUT 65339, 0		; set the color table address to zero
OUT 48955, 71	; the color table data address
OUT 65339, <color0>	; write the color values
OUT 65339, <color1>
OUT 65339, <color2>
OUT 65339, <color3>
```

The firmware automatically increments the address when the color table is written.

#### 64 Column Text Mode With Individual Attributes (Mode number 7)

This mode is is almost the same as the standard 64 column mode (video mode 6) except the attribute bytes are used instead of the single 64 column attribute byte.

#### 80 Column Mode With Individual Attributes (Mode number 5)

This mode displays an 80 column, 24 line text mode display. The Pico uses a font stored in its memory for the character bit maps. The default font is the VGA 8X8 code page 437 font.  This mode uses linear memory mapping with the character codes starting at 16384 and the corresponding attribute bytes starting at 18304. This mode uses 3840 bytes in DF1 allowing the remainder to be used for other purposes.
The font can be changed as follows:
```
OUT 48955, 68	; access the low byte of the font table address register
OUT 65339, <low 8 bits of the font character bit map address>
OUT 48955, 69	; access the high byte of the font table address register
OUT 65339, <high 8 bits of the font character bit map address>
OUT 48955, 70	; access the font data register
OUT 65339		; write the 8 character bit map bytes
OUT 65339
OUT 65339
OUT 65339
OUT 65339
OUT 65339
OUT 65339
OUT 65339
```

Note: the font address auto increments for each byte written.

Note: the size of the font table is 2048 bytes (256 characters * 8 bytes per character)
