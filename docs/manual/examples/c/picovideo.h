/* picovideo.h - PicoVideo register helpers for z88dk (zcc +ts2068) */
#ifndef PICOVIDEO_H
#define PICOVIDEO_H
#include <stdlib.h>

#define PV_SELECT  0xBF3B   /* write the register number here */
#define PV_DATA    0xFF3B   /* then read or write its value here */

#define PV_ULA_ENABLE   0x40
#define PV_MODE         0x41
#define PV_SPECCY_BLACK 0x42
#define PV_MIRROR       0x43
#define PV_INDEX_LO     0x44
#define PV_INDEX_HI     0x45
#define PV_FONT_DATA    0x46
#define PV_QUAD_COLOR   0x47
#define PV_VIDEO_READ   0x48
#define PV_VERSION      0x7F

#define DF1 ((unsigned char *)0x4000)
#define DF2 ((unsigned char *)0x6000)

void pv_write(unsigned char reg, unsigned char value)
{
    outp(PV_SELECT, reg);
    outp(PV_DATA, value);
}

unsigned char pv_read(unsigned char reg)
{
    outp(PV_SELECT, reg);
    return inp(PV_DATA);
}

/* Set the video mode in port $FF, keeping bits 3-7 as they are. */
void pv_mode(unsigned char mode)
{
    outp(0xFF, (inp(0xFF) & 0xF8) | mode);
}

#endif
