#ifndef PICOVIDEO_PALETTE_PROFILE_H
#define PICOVIDEO_PALETTE_PROFILE_H

/* Camera-matched normal palette, measured on one TS2068 edge-RGB board
 * through a LEVELHIKE converter and display. Bright colors remain stock.
 * This is the default; build with -DPICOVIDEO_PHOTO_PALETTE=OFF for the
 * original stock colors. See docs/photo-palette.md for the method.
 */
#ifdef PICOVIDEO_PHOTO_PALETTE
#define PV_NORMAL_COLORS 0, 3, 28, 31, 233, 231, 221, 219
#define PV_DEFAULT_SPECCY_BLACK 1
#define PV_DEFAULT_BORDER 219
#else
#define PV_NORMAL_COLORS BLACK, BLUE, RED, MAGEN, GREEN, CYAN, YELLOW, WHITE
#define PV_DEFAULT_SPECCY_BLACK 0
#define PV_DEFAULT_BORDER WHITE
#endif

#endif
