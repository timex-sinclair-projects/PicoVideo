#ifndef PICOVIDEO_MACROS_H
#define PICOVIDEO_MACROS_H

#define SOFTWARE_VER 0x43

#define ATTR_BLACK 0x00
#define ATTR_BLUE 0x01
#define ATTR_RED 0x02
#define ATTR_MAGENTA 0x03
#define ATTR_GREEN 0x04
#define ATTR_CYAN 0x05
#define ATTR_YELLOW 0x06
#define ATTR_WHITE 0x7
#define ATTR_BRIGHT 0x40
#define ATTR_FLASH 0x80


// convenient mnemonics for the TS colors. these translate
//  the 3-bit values + intensity in the attributes to the
//  corresponding 8 bit value for VGA output.
// Board V.2 and above color map

// regular colors
#define BLACK   0x00
#define BLUE    0x02
#define RED     0x10
#define MAGEN   0x12
#define GREEN   0x80
#define CYAN    0x82
#define YELLOW  0x90
#define WHITE   0x92
// bright colors
#define BBLACK  0x25
#define BBLUE   0x03
#define BRED    0x1c
#define BMAGEN  0x1f
#define BGREEN  0xe0
#define BCYAN   0xe3
#define BYELLOW 0xfc
#define BRWHITE 0xff


// ULA Plus register I/O addresses
#define ULA_ADR_REG 0xbf3b
#define ULA_DAT_REG 0xff3b

// 80 column text port I/O address
#define TEXT_PORT 0x20
#define TEST_PORT 0x21

#define HIRES_DAT   0xE0
#define HIRES_ADRL  0xE1
#define HIRES_ADRH  0xE2


// Offset for the display files in our local 16k memory
#define DF1_OFFSET 0
#define DF2_OFFSET 8192
#define DF_RAM_SIZE (16384 * 3)

// #define TEXTSCRN_SIZE 2048
// #define TEXTCHAR_OFFSET ((48 * 1024) - (2 * TEXTSCRN_SIZE))
// #define TEXTATTR_OFFSET ((48 * 1024) - TEXTSCRN_SIZE)

//#define TEXTATTR_OFFSET 1536
#define TEXTATTR_OFFSET 1920



// video mode mnemonics
// Note that all of these modes are valid in ULA Plus mode. In that case, they use the ULA Plus 
//  palette instead of the standard TS palette.
//
#define VID_32DF1   0x00        // standard TS 32 column video using DF1 (this is the default on boot)
#define VID_32DF2   0x01        // TS 32 column video using DF2
#define VID_HIRES   0x02        // TS hires
#define VID_QCOLOR  0x03        // new quad color mode
#define VID_64C     0x05        // 64A column text mode -w- individual attributes in DF1
#define VID_64      0x06        // standard TS 64 column mode using DF1 & DF2 and a fixed attribute
#define VID_64A     0x07        // new TS 64 column mode that uses the attributes
#define VID_HIRES8  0x09

#endif
