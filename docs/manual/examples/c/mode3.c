/* mode3.c - quad-color mode: 256 x 192 pixels in 4 colors */
#include <stdio.h>
#include <string.h>
#include "picovideo.h"

/* Mode 3 maps memory linearly: 32 bytes a line, 2 bits a pixel.
   Pixels 0-3 of each 8-pixel group come from DF1, 4-7 from DF2. */
void plot4(unsigned char x, unsigned char y, unsigned char color)
{
    unsigned char *p = ((x & 4) ? DF2 : DF1) + y * 32 + (x >> 3);
    unsigned char shift = 6 - 2 * (x & 3);

    *p = (*p & ~(3 << shift)) | (color << shift);
}

main()
{
    unsigned char x, y;
    unsigned int dy2, d;
    int dx, dy;

    /* the four colors, in the G3R3B2 byte format */
    pv_write(PV_INDEX_LO, 0);           /* start at entry 0 */
    outp(PV_SELECT, PV_QUAD_COLOR);
    outp(PV_DATA, 0x02);                /* 0: dark blue */
    outp(PV_DATA, 0xC7);                /* 1: cyan      */
    outp(PV_DATA, 0x3B);                /* 2: magenta   */
    outp(PV_DATA, 0xFC);                /* 3: yellow    */

    memset(DF1, 0, 6144);
    memset(DF2, 0, 6144);
    pv_mode(3);

    /* rings around the center of the screen */
    for (y = 0; y < 192; y++) {
        dy = y - 96;  dy2 = dy * dy;
        x = 0;
        do {
            dx = x - 128;
            d = (dx * dx + dy2) >> 8;
            plot4(x, y, (d >> 2) & 3);
        } while (++x);
    }

    while (getk()) ;  while (!getk()) ;
    pv_mode(0);
}
