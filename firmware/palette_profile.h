#ifndef PICOVIDEO_PALETTE_PROFILE_H
#define PICOVIDEO_PALETTE_PROFILE_H

/* Optional camera-matched normal palette for one TS2068 edge-RGB board,
 * LEVELHIKE converter and display chain. Bright colors remain stock.
 * See PHOTO-PALETTE.md in the patch proposal for method and limitations.
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
