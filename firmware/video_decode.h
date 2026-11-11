#ifndef PICOVIDEO_VIDEO_DECODE_H
#define PICOVIDEO_VIDEO_DECODE_H

/* Pure register decoding shared by the real bus handler and host tests.
 * Always inline on GCC: the timing-sensitive bus loop must remain in RAM.
 */
#if defined(__GNUC__)
#define PV_INLINE static inline __attribute__((always_inline))
#else
#define PV_INLINE static inline
#endif

/* $41 = 0 follows native $FF modes, independently of palette enable.
 * Nonzero values retain explicit mode selection, including custom modes.
 * $80 explicitly forces mode 0 (the low nibble remains the mode number).
 */
PV_INLINE unsigned char pv_video_mode(unsigned char palette_enabled,
                                     unsigned char extended_mode,
                                     unsigned char native_register) {
    if (palette_enabled && extended_mode != 0)
        return extended_mode & 0x0f;
    return native_register & 0x07;
}

/* Native 64-column color bits occupy $FF bits 3..5. PAPER is their
 * complement. BRIGHT/FLASH are zero; mode/control bits are not colors.
 */
PV_INLINE unsigned char pv_64_attribute(unsigned char native_register) {
    return ((~native_register) & 0x38) | ((native_register >> 3) & 7);
}

/* ULAplus BORDER uses PAPER in CLUT 0, not its INK entries. */
PV_INLINE unsigned char pv_ula_border(unsigned char border_register) {
    return 8 | (border_register & 7);
}

/* TS2068 FLASH changes phase every 32 frames (about 1.879 swaps/s on
 * the native 60.1145 Hz display). VGA is independently clocked.
 */
PV_INLINE void pv_flash_frame(unsigned short *frames, unsigned char *mask) {
    if (++*frames == 32) {
        *frames = 0;
        *mask ^= 0xff;
    }
}

#undef PV_INLINE
#endif
