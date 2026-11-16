; picovideo.asm - PicoVideo register routines in Z80 assembler
; Assemble with any Z80 assembler (tested with sjasmplus).

PV_SELECT   equ $BF3B       ; register number goes here
PV_DATA     equ $FF3B       ; then the value goes here

; Write E to PicoVideo register D. Destroys BC.
pvWrite:
        ld      bc,PV_SELECT
        out     (c),d
        ld      b,high PV_DATA  ; same low byte, so only B changes
        out     (c),e
        ret

; Read PicoVideo register D into A. Destroys BC.
pvRead:
        ld      bc,PV_SELECT
        out     (c),d
        ld      b,high PV_DATA
        in      a,(c)
        ret

; Set video mode A (0-7) in port $FF, keeping bits 3-7. Destroys B.
pvMode:
        ld      b,a
        in      a,($FF)
        and     $F8
        or      b
        out     ($FF),a
        ret

; Load a whole 2048-byte font from HL into the 80-column font.
; Don't use OTIR here: it counts in B, which is half of the port address.
pvLoadFont:
        ld      de,$4400        ; D = register $44, E = 0
        call    pvWrite         ; font index low byte = 0
        inc     d               ; register $45
        call    pvWrite         ; font index high byte = 0
        ld      d,$46
        ld      bc,PV_SELECT
        out     (c),d           ; select the font data register
        ld      b,high PV_DATA
        ld      de,2048
.next:  ld      a,(hl)
        out     (c),a           ; the index steps on by itself
        inc     hl
        dec     de
        ld      a,d
        or      e
        jr      nz,.next
        ret
