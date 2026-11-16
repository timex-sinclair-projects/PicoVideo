/* mode7.c - 64 columns with an attribute for every column (mode 7) */
#include <stdio.h>
#include <string.h>
#include "picovideo.h"

/* Even columns live in DF1, odd columns in DF2, each with its own
   bitmap and attribute area laid out like the normal screen. */
void put64(unsigned char col, unsigned char row, unsigned char ch, unsigned char attr)
{
    unsigned char *font = (unsigned char *)(*(unsigned int *)23606) + ch * 8;
    unsigned char *base = (col & 1) ? DF2 : DF1;
    unsigned char *pix  = base + ((row & 0x18) << 8) + ((row & 7) << 5) + (col >> 1);
    unsigned char line;

    for (line = 0; line < 8; line++)
        pix[line * 256] = font[line];
    base[0x1800 + row * 32 + (col >> 1)] = attr;
}

void print64(unsigned char col, unsigned char row, char *s, unsigned char attr)
{
    while (*s) put64(col++, row, *s++, attr);
}

main()
{
    unsigned char c;

    memset(DF1, 0, 6144);  memset(DF1 + 6144, 0x38, 768);  /* PAPER 7, INK 0 */
    memset(DF2, 0, 6144);  memset(DF2 + 6144, 0x38, 768);

    print64(0, 0, "  PicoVideo mode 7: 64 columns, an attribute for every column  ", 0x4F);
    print64(2, 2, "Mode 6 gives the whole 64-column screen one INK and PAPER.", 0x38);
    print64(2, 3, "Mode 7 uses the attribute areas of both display files, so", 0x38);
    print64(2, 4, "every column has its own. Even columns are DF1, odd are DF2.", 0x38);

    for (c = 0; c < 64; c++) {
        put64(c, 7, ' ', (c & 7) << 3);                  /* PAPER 0-7 */
        put64(c, 8, ' ', ((c & 7) << 3) | 0x40);         /* BRIGHT */
        put64(c, 10, 'A' + (c % 26), 0x40 | (1 + c % 7));      /* on PAPER 0 */
        put64(c, 11, '0' + (c % 10), ((7 - (c & 7)) << 3) | (c & 7));
    }

    print64(2, 14, "10 REM mode 7 listing", 0x38);
    print64(2, 15, "20 FOR i=0 TO 63: PRINT AT 0,i;CHR$ (65+i): NEXT i", 0x38);
    print64(2, 16, "30 STOP", 0x38);
    print64(2, 19, "Press a key for mode 6 (one color pair), then again to exit.", 0x30);

    pv_mode(7);
    while (getk()) ;  while (!getk()) ;
    pv_mode(6);
    while (getk()) ;  while (!getk()) ;
    pv_mode(0);
}
