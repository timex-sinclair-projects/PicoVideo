/* mode2.c - TS2068 extended color (mode 2): an attribute for every
   pixel line of every character cell */
#include <stdio.h>
#include <string.h>
#include "picovideo.h"

/* offset of pixel line y, byte x, in the usual TS2068 layout */
unsigned int lineofs(unsigned char y, unsigned char x)
{
    return ((y & 0xC0) << 5) | ((y & 7) << 8) | ((y & 0x38) << 2) | x;
}

main()
{
    unsigned char *font = (unsigned char *)(*(unsigned int *)23606);
    unsigned char y, x;
    char *msg = "  EXTENDED COLOR:  MODE 2       ";  /* 32 chars */

    for (y = 0; y < 192; y++)
        for (x = 0; x < 32; x++) {
            unsigned int o = lineofs(y, x);
            unsigned char ch = (y & 8) ? ' ' : msg[x];  /* alternate rows */
            DF1[o] = font[ch * 8 + (y & 7)];           /* pixels */
            DF2[o] = 0x40 | (1 + ((y + x) >> 1) % 7);  /* BRIGHT INK */
        }

    pv_mode(2);
    while (getk()) ;  while (!getk()) ;
    pv_mode(0);
}
