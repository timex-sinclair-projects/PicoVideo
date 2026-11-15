#ifndef PICOVIDEO_GLOBALS_H
#define PICOVIDEO_GLOBALS_H

/* Shared state defined in globals.c. Include this header and do not write
 * extern declarations by hand, so the compiler catches type or size
 * mismatches between the definition and its users.
 */
#include "macros.h"

#define LINE_BUF_SIZE 642       // 640 visible pixels + 2 blanking bytes

// palettes
extern const unsigned char tsColors[16];
extern const unsigned char speccyColors[16];
extern unsigned char speccyBlack;
extern unsigned char biColor[4];
extern unsigned char borderColors[8];

// ULA Plus
extern unsigned char gUlaColors[64];
extern unsigned char gUlaEna;
extern unsigned char gUlaVmode;
extern unsigned char gUlaAdrReg;

// TS2068 registers as seen on the bus
extern unsigned char gTsVidMode;
extern unsigned char gTsCol64Ink;
extern unsigned char gTsRegFE;
extern unsigned char gTsRegFF;
extern unsigned char gTsRegF4;

// display state
extern unsigned short gFlashCtr;
extern unsigned char gFlashMask;
extern unsigned char gVidMode;
extern unsigned char gBorder;

// 80-column text port
extern unsigned char gTextPort;
extern unsigned char gTextStat;
extern unsigned char gTextAttr;

// video memory and VGA line buffers
extern unsigned char gDfRam[DF_RAM_SIZE];
extern unsigned char gLbuf0[LINE_BUF_SIZE];
extern unsigned char gLbuf1[LINE_BUF_SIZE];
extern unsigned short gHiresAdr;

// test registers
extern unsigned short gTestMemAdr;
extern unsigned short gTestCrlReg;

// font
extern unsigned int gfontIndex;
extern unsigned char vgaFont[2048];

#endif
