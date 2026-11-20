#ifndef PICOVIDEO_PALETTE_PROFILE_H
#define PICOVIDEO_PALETTE_PROFILE_H

/* Photo palette: normal colors at about half the light of BRIGHT.
 * Each active channel uses R5/G5/B2 for normal and keeps the stock
 * R7/G7/B3 for BRIGHT, so every normal color is distinct from its BRIGHT
 * version. Measured on an ASUS VS238 in sRGB mode (CAL50, issue #10):
 * R5 = 45.1%, G5 = 52.3% and B2 = 51.3% of full light.
 * This is the default; build with -DPICOVIDEO_PHOTO_PALETTE=OFF for the
 * original stock colors. See docs/photo-palette.md for the method.
 */
#ifdef PICOVIDEO_PHOTO_PALETTE
#define PV_NORMAL_COLORS 0x00, 0x02, 0x14, 0x16, 0xA0, 0xA2, 0xB4, 0xB6
#define PV_DEFAULT_SPECCY_BLACK 1
#define PV_DEFAULT_BORDER 0xB6
#else
#define PV_NORMAL_COLORS BLACK, BLUE, RED, MAGEN, GREEN, CYAN, YELLOW, WHITE
#define PV_DEFAULT_SPECCY_BLACK 0
#define PV_DEFAULT_BORDER WHITE
#endif

#endif
