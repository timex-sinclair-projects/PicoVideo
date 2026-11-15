
	.include "text80Rtns.inc"


; These variables can be anywhere in RAM but MUST
;	be defined and mutable.

; default attribute
gAttr80			.db (ATTR_BLUE << 3) | ATTR_YELLOW
; current caret position for characters
gTxtAdr80		.dw 16384
; current caret position for attributes
gAttrAdr80	.dw	gTxtAdr80 + ATTROFFSET





;-----------------------------------------------
; Writes a value to a ULA register
;
; Entry:
;		d		register number
;		e		register value
;
; Exit:
;		Nothing returned.
;		BC destroyed
;
;//void writeUlaReg(unsigned char regNr, unsigned char regVal);
writeUlaReg:
	ld	bc, ULA_ADR_REG
	out	(c), d
	ld	b, $ff
	out	(c), e
	ret

;-----------------------------------------------
; Enables Speccy black
;
; Entry:
;		No entry values
;
; Exit:
;		Nothing returned.
;		All registers assumed to be destroyed
;
;//void enableSpeccyBlack(void);
enableSpeccyBlack:
	ld	d, ULA_SPECCY_BLACK_REG
	ld	e, 1
	jp	writeUlaReg		; exits via writeUlaReg


;-----------------------------------------------
; Disables Speccy black
;
; Entry:
;		No entry values
;
; Exit:
;		Nothing returned.
;		All registers assumed to be destroyed
;
;//void disableSpeccyBlack(void);
disableSpeccyBlack:
	ld	d, ULA_SPECCY_BLACK_REG
	ld	e, 0
	jp	writeUlaReg		; exits via writeUlaReg

;-----------------------------------------------
; Set the border color
;
; Entry:
;		A		border color
;
; Exit:
;		Nothing returned.
;		All registers assumed to be destroyed
;
;//void setBorder(unsigned char border);
setBorder:
	or	a, 7
	ld	c, a
	in	a, ($FE)
	and a, $f8
	or	a, c
	out	($FE), a
	ret


;-----------------------------------------------
; Write a new bit map to the character bit map
;		array in the Pico
;
; Entry:
;		HL	pointer to the 8 byte character bit map
;		A		the character code to change (0..255)
;
; Exit:
;		Nothing returned.
;		All registers assumed to be destroyed
;
;//void changeChar80(unsigned char ccode, unsigned char *bitMap);
changeChar80:
	push	hl				; save the buffer address
	ld	h, 0				; form the offset for the character
	ld	l, a				; into the Picos font table
	add	hl, hl
	add	hl, hl
	add	hl, hl			; HL now has the offset

; set the write address into the Pico font array
	ld	d, ULA_FONT_ADRL_REG
	ld	e, l
	call	writeUlaReg
	ld	d, ULA_FONT_ADRH_REG
	ld	e, l
	call	writeUlaReg

; tell the Pico we are writing to the
;		font data register
	ld	bc, ULA_ADR_REG
	ld	d, ULA_FONT_DAT_REG

	pop	hl					; restore the buffer pointer
	ld	b, $ff			; we are now writing to the Pico data register
	ld	d, 8				; byte count

; loop to send all 8 of the font bytes. the Pico
;		automatically increments the font address
changeChar80_1:
	ld	a, (hl)
	inc hl
	out (c), a
	dec d
	jr nz, changeChar80_1

	ret


;-----------------------------------------------
; This is a helper function that multiplies the
;		value in the A register by 80. The result
;		is deposited in the BC register
;
; Entry:
;		A		the value to be multiplied
;
; Exit:
;		DE	A * 80
;		A		destroyed
;		HL, BC preserved
;
mpyBy80:
	push hl

	ld	d, 0
	ld	e, a
	add	hl, de		; HL = A
	add	hl, hl		; HL = 2 * A
	add hl, hl		; HL = 4 * A
	add	hl, de		; HL = 5 * A
	add	hl, hl		; HL = 10 * A
	add	hl, hl		; HL = 20 * A
	add	hl, hl		; HL = 40 * A
	add	hl, hl		; HL = 80 * A

	ex de, hl			; DE = 80 * A

	pop	hl
	ret

;-----------------------------------------------
; Compute the video memory address of the character
;		position corresponding to the passed in line
;		and column.
; No range checking of the values are done, so
;		buyer beware.
;
; Entry:
;		D		line number (0..23)
;		E		column number (0..79)
;
; Exit:
;		DE	address
;
;		A destroyed
;		HL, BC preserved
;
;//unsigned int adrXYChar80(unsigned char x, unsigned char y);
adrXYChar80:
	push	hl				; save user's HL
	push	de				; save E
	ld	a, d
	call mpyBy80		; DE = 80 * A

	pop hl					; restore the column number into L
	ld	h, 0				;
	add	hl, de			; HL = (80 * D) + E

	ld	de, 16384		; HL = (80 * D) + E + 16384
	add hl, de			;

	ex	de, hl			; DE = (80 * D) + E + 16384

	pop	hl					; restore user's HL
	ret


;-----------------------------------------------
; Compute the video memory address of the attribute
;		position corresponding to the passed in line
;		and column.
; No range checking of the values are done, so
;		buyer beware.
;
; Entry:
;		D		line number (0..23)
;		E		column number (0..79)
;
; Exit:
;		DE	address
;
;		A destroyed
;		HL, BC preserved
;
;//unsigned int adrXYAttr80(unsigned char x, unsigned char y);
adrXYAttr80:
	call adrXYChar80
	push	hl
	ld	hl, ATTROFFSET
	add	hl, de
	ex	de, hl
	pop	hl
	ret


;-----------------------------------------------
; Clear the screen using the attribute in
;		A and a space character
;
; Entry:
;		A		attribute to use
;		C		character to use
;
; Exit:
;		Nothing returned
;		All registers destroyed
;
clrScr80:
	ld	c, ' '

;-----------------------------------------------
; Clear the screen using the attribute in
;		A and character in C
;
; Entry:
;		A		attribute to use
;		C		character to use
;
; Exit:
;		Nothing returned
;		All registers destroyed
;
;//void clrScr80(unsigned char attr);
initScr80:
	ld	hl, 16384
	ld	de, 16385
	ld	(hl), c
	ld	bc, 1919
	ldir

	ld	(hl), a
	ld	bc, 1919
	ldir
	ret


;-----------------------------------------------
; Make an ink and paper attribute from the
;		ink value in B and the paper value in C
;
; Entry:
;		B		ink
;		C		paper
;
; Exit:
;		A				ink/paper attribute
;		B, C		destroyed
;		HL, DE	prserved
;
;//unsigned char makeInkPaper80(unsigned char ink, unsigned char paper);
makeInkPaper80:
	ld	a, c			; get the paper value
	add	a, a			; shift left by three bits
	add a, a      ;
	add	a, a      ;
	and $38
	ld	c, a
	ld	a, b
	and 7
	or c
	ret

;-----------------------------------------------
; Turn the bright bit on.
;
; Entry:
;		A		attribute
;
; Exit:
;		A		attribute with bright on
;
;//unsigned char brightOn80(unsigned char attr);
	or	a, ATTR_BRIGHT
	ret

;-----------------------------------------------
; Turn the bright bit off.
;
; Entry:
;		A		attribute
;
; Exit:
;		A		attribute with bright of
;
	and	a, ~ATTR_BRIGHT
	ret


;-----------------------------------------------
; Turn the flash bit on.
;
; Entry:
;		A		attribute
;
; Exit:
;		A		attribute with bright on
;
;//unsigned char flashOn80(unsigned char attr);
	or	a, ATTR_FLASH
	ret

;-----------------------------------------------
; Turn the bright bit off.
;
; Entry:
;		A		attribute
;
; Exit:
;		A		attribute with bright of
;
;//unsigned char flashOff80(unsigned char attr);
flashOff80:
	and	a, ~ATTR_FLASH & $ff
	ret

;-----------------------------------------------
; Set 80 column mode
;
; Entry:
;		Nothing
;
; Exit:
;		Nothing
;		A destroyed
;
;//void setModeText80(void);
setModeText80:
	in	a, ($ff)
	and $f8
	or	5
	out	($ff), a
	ret


;-----------------------------------------------
; Print a character at the current caret using
;		the provided character code and the default
;		attribute
;
; Entry:
;		A			character
;
; Exit:
;		Nothing
;		All registers destroyed
;
printChar80:
	ld	bc, (gAttr80 - 1)

;-----------------------------------------------
; Print a character at the current caret using
;		the provided character code and attribute
;
; Entry:
;		A			character
;		B			attribute
;
; Exit:
;		Nothing
;		All registers destroyed
;
printCharAttr80:
	ld	hl, (gTxtAdr80)			; get the character caret
	ld	(hl), a							; store the character code

	ex	de, hl							; DE = character caret position
	ld	hl, ATTROFFSET			;
	add hl, de              ; HL = attribute caret position
	ld	(hl), b							; store the attribute

	inc	de									; advance the character caret
	ld	(gTxtAdr80), de			;		and save it
	ret

;-----------------------------------------------
; Set the character and attribute carets to
;		the memory address derived from the
;		parameters passed in.
;
; Entry:
;		D		line number (0..23)
;		E		column number (0..79)
;

gotoXY80:
	call adrXYChar80
	ld	(gTxtAdr80), de
	push	hl
	ld	hl, ATTROFFSET
	add	hl, de
	ld	(gAttrAdr80), hl
	pop	hl
	ret


;-----------------------------------------------
; Print a null terminated string at the current
;		caret position using the current attribute
;
; Entry:
;		HL		pointer to the string
;
; Exit:
;		Nothing
;		All registers destroyed
;
;//void printStrZ80(char *txtStr);
printStrZ80:
	ld	a, (gAttr80)			; get the attribute
	ld	b, a

;-----------------------------------------------
; entry point for printing with the attribute
;		being in B
printStrZAttrB80:
	ld	a, (hl)
	and	a
	ret	z
	call printCharAttr80
	inc	hl
	jr printStrZAttrB80


;-----------------------------------------------
; Print string of length N at the current
;		caret position using the default attribute
;
; Entry:
;		HL		pointer to the string
;		C			the length of the string
;
; Exit:
;		Nothing
;		All registers destroyed
;
printStrN80:
	ld	a, (gAttr80)
	ld	b, a

;-----------------------------------------------
; Print string of length N at the current
;		caret position using the provided attribute
;
; Entry:
;		HL		pointer to the string
;		B			attribute
;		C			the length of the string
;
; Exit:
;		Nothing
;		All registers destroyed
;
printStrAttrN80:
	ld	a, (hl)
	inc		hl
	push 	hl
	push	bc
	call	printCharAttr80
	pop		bc
	pop		hl
	dec		c
	jr		nz, printStrN80
	ret

;-----------------------------------------------
; Set the default attribute
;
; Entry:
;		A		attribute
;
; Exit:
;		Nothing
;
	ld (gAttr80), a
	ret


;-----------------------------------------------
; Get the default attribute
;
; Entry:
;		Nothing
;
; Exit:
;		A		attribute
;
	ld a, (gAttr80)
	ret

