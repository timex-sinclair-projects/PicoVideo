; picovideo-demo: every feature of PicoVideo firmware 4.5, on a fixed timeline
; for recording. Build with  python3 build.py
;
; It takes over the TS2068: interrupts go to its own 60 Hz clock, and nothing
; of BASIC's is used, so both display files are free. It doesn't return.
;   SPACE start   N next scene   P pause   R back to the start screen

        device noslot64k

PV_SELECT   equ $BF3B
PV_DATA     equ $FF3B
R_ENABLE    equ $40
R_MODE      equ $41
R_SPECCY    equ $42
R_MIRROR    equ $43
R_INDEX_LO  equ $44
R_INDEX_HI  equ $45
R_FONT      equ $46
R_QUAD      equ $47
R_VIDEO     equ $48
R_VERSION   equ $7F

DF1         equ $4000
DF2         equ $6000
SPARE       equ DF1+$17FF       ; a blank pixel byte the checks may use
BUFFER      equ $E400           ; 1800h, for mirroring
STACK       equ $FDF0
VECTORS     equ $FE00
VECTOR_JP   equ $FDFD

; A time in tenths of a second, as ticks of the 60.1145 Hz interrupt
T       macro tenths
        ld      hl,(tenths)*601145/100000
        endm

; Wait until this far into the scene
AT      macro tenths
        T       tenths
        call    waitScene
        endm

SHOW    macro image, where
        ld      hl,image
        ld      de,where
        call    dzx0_standard
        endm

PV      macro register, value
        ld      de,(register)*256+(value)
        call    pvWrite
        endm

PRINTAT macro row, col, text, color
        ld      de,(row)*256+(col)
        ld      hl,text
        ld      c,color
        call    printText
        endm

        org     $8000

start:
        di
        ld      sp,STACK
        in      a,($F4)
        ld      (homeBanks),a
        ld      hl,VECTORS      ; whatever byte is on the bus, go to FDFDh
        ld      de,VECTORS+1
        ld      bc,256
        ld      (hl),high VECTOR_JP
        ldir
        ld      a,$C3
        ld      (VECTOR_JP),a
        ld      hl,tick
        ld      (VECTOR_JP+1),hl
        ld      a,high VECTORS
        ld      i,a
        im      2
        xor     a
        out     ($FF),a         ; mode 0, interrupts on, home bank
        ei

restart:
        ld      sp,STACK
        xor     a
        ld      (paused),a
        ld      (running),a
        call    resetAll
        SHOW    img_ready,DF1
.release:
        call    readKeys
        jr      nz,.release
.press:
        call    readKeys
        and     1               ; SPACE
        jr      z,.press
        ld      a,15
        ld      (lastKeys),a
        xor     a
        ld      (sceneNo),a
        di
        ld      hl,0
        ld      (ticks),hl
        inc     a
        ld      (running),a
        ei

runScene:
        ld      sp,STACK
        call    resetAll
        ld      a,(sceneNo)
        add     a,a
        add     a,a
        ld      e,a
        ld      d,0
        ld      hl,timeline
        add     hl,de
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        inc     hl
        ld      (sceneBase),de
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        inc     hl
        push    de              ; the scene
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        ld      (sceneNext),de
        pop     hl
        call    jumpHL
        ld      hl,(sceneNext)
        call    waitTicks
        ld      hl,sceneNo
        inc     (hl)
        jr      runScene

jumpHL: jp      (hl)

tick:
        push    af
        push    hl
        ld      a,(running)
        or      a
        jr      z,.done
        ld      a,(paused)
        or      a
        jr      nz,.done
        ld      hl,(ticks)
        inc     hl
        ld      (ticks),hl
.done:
        pop     hl
        pop     af
        ei
        reti

; ---------------------------------------------------------------- scenes

sOpening:
        PV      R_INDEX_LO,0
        PV      R_INDEX_HI,0
        ld      hl,blackQuad
        call    loadQuad
        ld      a,3
        out     ($FF),a         ; switch first, so the drawing can't show
        SHOW    img_opening2,DF2
        SHOW    img_opening1,DF1
        xor     a
        ld      (phase),a
        ld      hl,0
        ld      (nextStep),hl
.cycle:
        ld      a,(phase)
        inc     a
        and     15
        ld      (phase),a
        ld      e,a
        ld      d,0
        ld      hl,ringColors
        add     hl,de
        ld      a,(hl)
        ld      (quadWork+1),a
        ld      a,e
        add     a,6
        and     15
        ld      e,a
        ld      hl,ringColors
        add     hl,de
        ld      a,(hl)
        ld      (quadWork+2),a
        xor     a
        ld      (quadWork),a
        dec     a
        ld      (quadWork+3),a
        ld      hl,quadWork
        call    loadQuad
        T       155
        ld      a,9
        call    step
        jr      c,.cycle
        ret

sIntro:
        SHOW    img_intro,DF1
        AT      90
        ld      bc,PV_SELECT    ; is the board there?
        ld      a,R_VERSION
        out     (c),a
        in      a,(c)
        cp      R_VERSION
        jr      nz,.absent
        ld      d,R_VERSION
        call    pvRead
        ld      e,a
        rrca
        rrca
        rrca
        rrca
        and     15
        add     a,'0'
        ld      (versionText),a
        ld      a,e
        and     15
        add     a,'0'
        ld      (versionText+2),a
        PRINTAT 20,23,versionText,$78
        ret
.absent:
        PRINTAT 20,22,noneText,$57
        ret

sColors:
        SHOW    img_colors,DF1
        ret

sSpeccy:
        PV      R_SPECCY,0
        SHOW    img_speccy,DF1
        PRINTAT 19,6,speccyOff,$46
        AT      60
        PV      R_SPECCY,1
        PRINTAT 19,6,speccyOn,$44
        AT      110
        PV      R_SPECCY,0
        PRINTAT 19,6,speccyOff,$46
        AT      145
        PV      R_SPECCY,1
        PRINTAT 19,6,speccyOn,$44
        ret

sBorder:
        SHOW    img_border,DF1
        xor     a
        ld      (phase),a
        ld      hl,0
        ld      (nextStep),hl
.colors:
        ld      a,(phase)
        and     7
        out     ($FE),a
        inc     a
        ld      (phase),a
        T       80
        ld      a,60
        call    step
        jr      c,.colors
        ld      hl,$0502        ; cyan and red: the tape leader
        ld      (stripePair),hl
        T       135
        ld      b,170
        call    stripes
        ld      hl,$0601        ; yellow and blue: the tape data
        ld      (stripePair),hl
        T       195
        ld      b,64
        call    stripes
        xor     a
        out     ($FE),a
        ret

sMode1:
        SHOW    img_mode1df2,DF2
        SHOW    img_mode1df1,DF1
        AT      45
        ld      a,1
        out     ($FF),a
        AT      85
        xor     a
        out     ($FF),a
        AT      105
        ld      a,1
        out     ($FF),a
        AT      125
        xor     a
        out     ($FF),a
        AT      145
        ld      a,1
        out     ($FF),a
        ret

sMode2:
        SHOW    img_mode2df2,DF2
        ld      a,2
        out     ($FF),a
        SHOW    img_mode2df1,DF1
        AT      100
        xor     a
        out     ($FF),a
        AT      140
        ld      a,2
        out     ($FF),a
        ret

sMode6:
        SHOW    img_mode6df2,DF2
        ld      a,6
        out     ($FF),a
        SHOW    img_mode6df1,DF1
        T       40
        ld      (nextStep),hl
        call    waitScene
        ld      a,14
        ld      (phase),a
.inks:
        ld      a,(phase)
        out     ($FF),a
        add     a,8
        and     $3F
        ld      (phase),a
        T       140
        ld      a,84
        call    step
        jr      c,.inks
        ret

sMode7:
        SHOW    img_mode7df2,DF2
        ld      a,7
        out     ($FF),a
        SHOW    img_mode7df1,DF1
        AT      100
        ld      a,6
        out     ($FF),a
        AT      130
        ld      a,7
        out     ($FF),a
        ret

sMode5:
        ld      a,5
        out     ($FF),a
        SHOW    img_mode5,DF1
        ret

sFont:
        ld      a,5
        out     ($FF),a
        SHOW    img_font,DF1
        AT      45
        ld      hl,glyphBold
        call    loadFont
        AT      80
        ld      hl,glyphFlip
        call    loadFont
        AT      115
        ld      hl,glyphSlant
        call    loadFont
        AT      145
        ld      hl,glyphPlain
        jp      loadFont

sQuad:
        ld      hl,blackQuad
        call    loadQuad
        ld      a,3
        out     ($FF),a
        SHOW    img_quad2,DF2
        SHOW    img_quad1,DF1
        ld      hl,quadSets
        call    loadQuad
        AT      70
        ld      hl,quadSets+4
        call    loadQuad
        AT      110
        ld      hl,quadSets+8
        call    loadQuad
        AT      150
        ld      hl,quadSets+12
        call    loadQuad
        AT      185
        ld      hl,quadSets
        jp      loadQuad

sUla:
        SHOW    img_ulaplus,DF1
        AT      70
        xor     a
        ld      (phase),a
        call    loadWheel
        PV      R_ENABLE,1
        AT      130
        ld      a,4
        out     ($FE),a         ; the border follows entry 12
        T       130
        ld      (nextStep),hl
.cycle:
        ld      a,(phase)
        inc     a
        and     63
        ld      (phase),a
        call    loadWheel
        T       295
        ld      a,4
        call    step
        jr      c,.cycle
        xor     a
        out     ($FE),a
        PV      R_ENABLE,0
        ret

sMirror:
        SHOW    img_mirrordf2,BUFFER
        SHOW    img_mirrorempty,DF2
        ld      a,7
        out     ($FF),a
        SHOW    img_mirrordf1,DF1
        T       60
        ld      (nextStep),hl
        call    waitScene
        xor     a
        ld      (phase),a
.line:
        ld      a,(phase)
        call    lineAddress     ; HL: where this line is, from DF1's start
        push    hl
        ld      de,BUFFER
        add     hl,de
        ex      (sp),hl
        ld      de,DF1
        add     hl,de
        ex      de,hl           ; DE: the screen
        pop     hl              ; HL: the buffer
        di                      ; only these 32 writes are mirrored
        push    de
        PV      R_MIRROR,1
        pop     de
        ld      bc,32
        ldir
        PV      R_MIRROR,0
        ei
        T       170
        ld      a,2
        call    step
        ld      hl,phase
        inc     (hl)
        ld      a,(hl)
        cp      192
        jr      c,.line
        ret

sPinned:
        ld      hl,stockPalette
        call    loadPalette
        PV      R_MODE,5
        PV      R_ENABLE,1
        SHOW    img_pinned,DF1
        AT      50
        xor     a
        call    tryMode
        AT      62
        ld      a,2
        call    tryMode
        AT      74
        ld      a,6
        call    tryMode
        AT      86
        ld      a,7
        call    tryMode
        AT      98
        xor     a
        call    tryMode
        AT      115
        ld      hl,amberPalette
        jp      loadPalette

sChecks:
        SHOW    img_checks,DF1
        T       50
        ld      (nextStep),hl
        call    waitScene
        xor     a
        ld      (phase),a
        ld      (passes),a
.next:
        ld      a,(phase)
        add     a,a
        ld      e,a
        ld      d,0
        ld      hl,checkList
        add     hl,de
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        ex      de,hl
        di
        call    jumpHL          ; Z: passed
        push    af
        ld      a,(homeBanks)   ; whatever happened, put things back
        out     ($F4),a
        pop     af
        ei
        ld      hl,okText
        ld      c,$44
        jr      z,.passed
        ld      hl,failText
        ld      c,$57
        jr      .show
.passed:
        ld      a,(passes)
        inc     a
        ld      (passes),a
.show:
        ld      a,(phase)
        add     a,6
        ld      d,a
        ld      e,25
        call    printText
        T       180
        ld      a,48
        call    step
        ld      hl,phase
        inc     (hl)
        ld      a,(hl)
        cp      12
        jr      c,.next
        AT      160
        ld      a,(passes)
        cp      12
        jr      nz,.some
        PRINTAT 22,7,allText,$60
        ret
.some:
        PRINTAT 22,7,someText,$57
        ret

sSound:
        SHOW    img_sound,DF1
        ld      de,7*256+$3E    ; tone A only
        call    ayWrite
        T       30
        ld      (nextStep),hl
        call    waitScene
        ld      de,8*256+12
        call    ayWrite
        xor     a
        ld      (phase),a
.note:
        ld      a,(phase)
        and     15
        ld      e,a
        ld      d,0
        ld      hl,tune
        add     hl,de
        ld      a,(hl)
        push    af
        add     a,a
        ld      e,a
        ld      hl,periods
        add     hl,de
        ld      e,(hl)
        inc     hl
        ld      b,(hl)
        ld      d,0
        call    ayWrite
        ld      d,1
        ld      e,b
        call    ayWrite
        pop     af
        call    lightBar
        ld      hl,phase
        inc     (hl)
        T       125
        ld      a,10
        call    step
        jr      c,.note
        ld      de,8*256+0
        jp      ayWrite

sEnding:
        SHOW    img_ending,DF1
.hold:
        call    poll            ; R starts again
        jr      .hold

; ---------------------------------------------------------------- checks
; Each returns Z for a pass. Interrupts are off and the stack is high.

checkList:
        dw      chkVersion,chkSelect,chkColor,chkEnable,chkPin,chkMirror
        dw      chkQuad,chkFont,chkVideo,chkMirrored,chkPaging,chkOwner

chkVersion:
        ld      d,R_VERSION
        call    pvRead
        cp      $45
        ret

chkSelect:
        ld      bc,PV_SELECT
        ld      a,$2A
        out     (c),a
        in      a,(c)
        cp      $2A
        ret

chkColor:
        PV      20,$5A
        ld      d,20
        call    pvRead
        cp      $5A
        ret

chkEnable:
        PV      R_ENABLE,1
        ld      d,R_ENABLE
        call    pvRead
        push    af
        PV      R_ENABLE,0
        pop     af
        cp      1
        ret

chkPin:                         ; no effect: the palette is off
        PV      R_MODE,5
        ld      d,R_MODE
        call    pvRead
        push    af
        PV      R_MODE,0
        pop     af
        cp      5
        ret

chkMirror:
        ld      de,R_MIRROR*256+1
        ld      bc,PV_SELECT    ; no calls: a mirrored write would be seen
        out     (c),d
        ld      b,high PV_DATA
        out     (c),e
        in      a,(c)
        ld      e,0
        out     (c),e
        cp      1
        ret

chkQuad:
        ld      hl,2
        call    setIndex
        PV      R_QUAD,$66
        ld      hl,2
        call    setIndex
        ld      d,R_QUAD
        call    pvRead
        cp      $66
        ret

chkFont:
        ld      hl,255*8
        call    setIndex
        PV      R_FONT,$3C
        ld      hl,255*8
        call    setIndex
        ld      d,R_FONT
        call    pvRead          ; resetAll puts the font back
        cp      $3C
        ret

chkVideo:
        ld      a,$A5
        ld      (SPARE),a
        call    picoSpare
        push    af
        xor     a
        ld      (SPARE),a
        pop     af
        cp      $A5
        ret

chkMirrored:
        xor     a
        ld      (SPARE),a
        PV      R_MIRROR,1
        ld      a,$96
        ld      (SPARE),a       ; goes to the Pico's DF2
        PV      R_MIRROR,0
        call    picoSpare       ; so its DF1 is unchanged
        or      a
        ld      a,0
        ld      (SPARE),a
        ret     nz
        ld      hl,$37FF
        call    setIndex
        ld      d,R_VIDEO
        call    pvRead
        cp      $96
        ret

; The two banking checks write one byte to whatever is paged in, then put
; that byte back before they let go of it.
chkPaging:
        xor     a
        ld      (SPARE),a
        ld      a,(homeBanks)
        or      4               ; chunk 2 to the dock
        out     ($F4),a
        ld      a,(SPARE)
        ld      (saved),a
        ld      a,$55
        ld      (SPARE),a
        call    picoSpare
        ld      e,a
        ld      a,(saved)
        ld      (SPARE),a
        ld      a,(homeBanks)
        out     ($F4),a
        ld      a,e
        or      a
        ret

chkOwner:
        xor     a
        ld      (SPARE),a
        ld      a,2             ; OWN2
        out     ($FD),a
        ld      a,253           ; a bank that isn't fitted
        out     ($FC),a
        ld      a,(SPARE)
        ld      (saved),a
        ld      a,$55
        ld      (SPARE),a
        call    picoSpare
        ld      e,a
        ld      a,(saved)
        ld      (SPARE),a
        xor     a               ; no owner
        out     ($FC),a
        ld      a,e
        or      a
        ret

picoSpare:
        ld      hl,$17FF
        call    setIndex
        ld      d,R_VIDEO
        jp      pvRead

; ---------------------------------------------------------------- routines

; D = register, E = value. Destroys BC.
pvWrite:
        ld      bc,PV_SELECT
        out     (c),d
        ld      b,high PV_DATA
        out     (c),e
        ret

; D = register. Returns A. Destroys BC.
pvRead:
        ld      bc,PV_SELECT
        out     (c),d
        ld      b,high PV_DATA
        in      a,(c)
        ret

setIndex:
        ld      d,R_INDEX_LO
        ld      e,l
        call    pvWrite
        ld      d,R_INDEX_HI
        ld      e,h
        jp      pvWrite

; HL = four colors
loadQuad:
        push    hl
        ld      hl,0
        call    setIndex
        pop     hl
        ld      d,R_QUAD
        ld      bc,PV_SELECT
        out     (c),d
        ld      b,high PV_DATA
        ld      d,4
.next:  ld      a,(hl)
        out     (c),a
        inc     hl
        dec     d
        jr      nz,.next
        ret

; HL = 64 colors
loadPalette:
        ld      d,0
.next:  ld      e,(hl)
        call    pvWrite
        inc     hl
        inc     d
        ld      a,d
        cp      64
        jr      c,.next
        ret

; A = turn of the wheel. Entries 7 and 8 stay white and black for the text.
; Each table spans the whole wheel, in the order it is drawn: PAPER, then INK.
loadWheel:
        ld      l,a
        ld      d,0
.next:  ld      a,d
        add     a,8
        and     15
        add     a,a
        add     a,a
        add     a,l
        and     63
        ld      h,a
        ld      a,d
        and     $30             ; the table, times 16
        add     a,a
        add     a,a
        add     a,h
        push    hl
        ld      hl,wheel
        add     a,l
        ld      l,a
        jr      nc,.same
        inc     h
.same:  ld      e,(hl)
        pop     hl
        ld      a,d
        cp      7
        jr      nz,.not7
        ld      e,$FF
.not7:  cp      8
        jr      nz,.not8
        ld      e,0
.not8:  call    pvWrite
        inc     d
        ld      a,d
        cp      64
        jr      c,.next
        ret

; HL = a routine that reshapes a row of pixels in A
loadFont:
        ld      (.shape+1),hl
        ld      hl,0
        call    setIndex
        ld      bc,PV_SELECT
        ld      a,R_FONT
        out     (c),a
        ld      b,high PV_DATA
        ld      hl,fontData
        ld      de,2048
.next:  ld      a,(hl)
.shape: call    glyphPlain
        out     (c),a
        inc     hl
        dec     de
        ld      a,d
        or      e
        jr      nz,.next
        ret

glyphPlain:
        ret
glyphBold:
        push    de
        ld      d,a
        srl     a
        or      d
        pop     de
        ret
glyphSlant:                     ; the top four rows one pixel right
        bit     2,l             ; L: the row within the glyph
        ret     nz
        srl     a
        ret
glyphFlip:                      ; left to right
        push    de
        ld      d,8
.bit:   rrca
        rl      e
        dec     d
        jr      nz,.bit
        ld      a,e
        pop     de
        ret

; A = line. Returns HL = its offset in a display file.
lineAddress:
        ld      e,a
        and     7
        ld      h,a
        ld      a,e
        and     $C0
        rrca
        rrca
        rrca
        or      h
        ld      h,a
        ld      a,e
        and     $38
        add     a,a
        add     a,a
        ld      l,a
        ret

; A = the mode to ask port 255 for, while the mode is pinned
tryMode:
        out     ($FF),a
        add     a,'0'
        ld      (DF1+12*80+53),a
        ld      a,$46
        ld      (DF1+1920+12*80+53),a
        ret

; D, E = row and column, HL = text ending in 0, C = attribute
printText:
.next:  ld      a,(hl)
        or      a
        ret     z
        push    hl
        push    de
        ld      l,a
        ld      h,0
        add     hl,hl
        add     hl,hl
        add     hl,hl
        ld      a,high fontData
        add     a,h
        ld      h,a             ; HL: the glyph (fontData is page aligned)
        push    de
        ld      a,d
        rrca
        rrca
        rrca
        and     $E0
        or      e
        ld      e,a
        ld      a,d
        and     $18
        or      high DF1
        ld      d,a
        ld      b,8
.row:   ld      a,(hl)
        ld      (de),a
        inc     hl
        inc     d
        djnz    .row
        pop     de
        ld      l,d
        ld      h,0
        add     hl,hl
        add     hl,hl
        add     hl,hl
        add     hl,hl
        add     hl,hl
        ld      d,high (DF1+$1800)
        add     hl,de
        ld      (hl),c
        pop     de
        pop     hl
        inc     hl
        inc     e
        jr      .next

; A = note 0-7: light that bar, dim the rest
lightBar:
        ld      c,a
        ld      hl,DF1+$1800+6*32
        ld      b,12
.row:   ld      d,0
.col:   ld      a,d
        cp      c
        ld      e,$01
        jr      nz,.dim
        add     a,$42
        cp      $48
        jr      c,.lit
        ld      a,$47
.lit:   ld      e,a
.dim:   ld      (hl),e
        inc     hl
        ld      (hl),e
        inc     hl
        ld      (hl),e
        inc     hl
        inc     hl
        inc     d
        ld      a,d
        cp      8
        jr      c,.col
        djnz    .row
        ret

; D = AY register, E = value
ayWrite:
        ld      a,d
        out     ($F5),a
        ld      a,e
        out     ($F6),a
        ret

; HL = until when in the scene, B = the width of a stripe
stripes:
        ld      de,(sceneBase)
        add     hl,de
        ld      (stripeEnd),hl
        ld      a,b
        ld      (stripeWidth),a
.next:  ld      hl,stripePair
        ld      a,(hl)
        out     ($FE),a
        inc     hl
        ld      e,(hl)
        ld      (hl),a
        dec     hl
        ld      (hl),e
        ld      a,(stripeWidth)
        ld      b,a
.wait:  djnz    .wait
        call    poll
        ld      hl,(ticks)
        ld      de,(stripeEnd)
        or      a
        sbc     hl,de
        jr      c,.next
        ret

; A = ticks to the next step, HL = the last step's time in the scene.
; Waits, then returns carry while there are steps to come.
step:
        ld      (stepLimit),hl
        ld      hl,(nextStep)
        ld      e,a
        ld      d,0
        add     hl,de
        ld      (nextStep),hl
        push    hl
        call    waitScene
        pop     hl
        ld      de,(stepLimit)
        or      a
        sbc     hl,de
        ret

; HL = ticks into the scene
waitScene:
        ld      de,(sceneBase)
        add     hl,de
waitTicks:
        push    hl
        call    poll
        pop     hl
        ld      de,(ticks)
        ex      de,hl
        or      a
        sbc     hl,de
        ex      de,hl
        jr      c,waitTicks
        ret

poll:
        call    readKeys
        ld      c,a
        ld      a,(lastKeys)
        cpl
        and     c               ; newly pressed
        ld      b,a
        ld      a,c
        ld      (lastKeys),a
        bit     3,b
        jp      nz,restart
        bit     2,b
        jr      z,.notPause
        ld      a,(paused)
        xor     1
        ld      (paused),a
.notPause:
        bit     1,b
        ret     z
        ld      a,(sceneNo)     ; N: jump the clock to the next scene
        cp      SCENES-1
        ret     z
        di
        inc     a
        ld      (sceneNo),a
        ld      hl,(sceneNext)
        ld      (ticks),hl
        xor     a
        ld      (paused),a
        ei
        jp      runScene

; A = keys down: bit 0 SPACE, 1 N, 2 P, 3 R. Z if none.
readKeys:
        ld      e,0
        ld      a,$7F
        in      a,($FE)
        bit     0,a
        jr      nz,.n
        set     0,e
.n:     bit     3,a
        jr      nz,.p
        set     1,e
.p:     ld      a,$DF
        in      a,($FE)
        bit     0,a
        jr      nz,.r
        set     2,e
.r:     ld      a,$FB
        in      a,($FE)
        bit     3,a
        jr      nz,.out
        set     3,e
.out:   ld      a,e
        or      a
        ret

; Back to the power-on state, behind a black screen
resetAll:
        ei
        PV      R_MIRROR,0
        ld      hl,DF1+$1800    ; black out mode 0 before switching to it
        ld      de,DF1+$1801
        ld      bc,767
        ld      (hl),0
        ldir
        xor     a
        out     ($FF),a
        out     ($FE),a
        PV      R_MODE,0
        PV      R_ENABLE,0
        PV      R_SPECCY,1
        ld      de,7*256+$3F
        call    ayWrite
        ld      de,8*256+0
        call    ayWrite
        ld      hl,DF1
        ld      de,DF1+1
        ld      bc,$17FF
        ld      (hl),0
        ldir
        ld      hl,DF2
        ld      de,DF2+1
        ld      bc,$1AFF
        ld      (hl),0
        ldir
        ld      hl,powerOnQuad
        call    loadQuad
        ld      hl,glyphPlain
        jp      loadFont

; ---------------------------------------------------------------- data

versionText db  "0.0",0
noneText    db  "NONE",0
speccyOff   db  "Speccy Black: off ",0
speccyOn    db  "Speccy Black: on  ",0
okText      db  "OK  ",0
failText    db  "FAIL",0
allText     db  " All twelve pass. ",0
someText    db  " Some checks fail ",0

blackQuad   db  0,0,0,0
powerOnQuad db  0,$FF,$E0,$FC

; 1764750 / 16 / Hz, for C major from middle C
periods     dw  422,376,335,316,281,251,223,211
tune        db  0,2,4,7,4,2,1,3,5,7,5,3,2,4,6,7

ticks       dw  0
running     db  0
paused      db  0
lastKeys    db  0
homeBanks   db  0
sceneNo     db  0
sceneBase   dw  0
sceneNext   dw  0
nextStep    dw  0
stepLimit   dw  0
phase       db  0
passes      db  0
saved       db  0
quadWork    ds  4
stripePair  dw  0
stripeWidth db  0
stripeEnd   dw  0

        include "dzx0.asm"
        include "build/timeline.inc"
        align   256
        include "build/data.inc"

demoEnd:
        assert  demoEnd <= BUFFER
        savebin "build/demo.bin",$8000,demoEnd-$8000
