#include <stdio.h>
#include <stdlib.h>
#include "text80Rtns.h"


unsigned char gAttr80 = (ATTR_BLUE << 3) | ATTR_YELLOW;
unsigned int gTxtAdr80;
unsigned int gAttrAdr80;

/*
	Write to a ULA Plus register

*/
void writeUlaReg(unsigned char regNr, unsigned char regVal){
	outp(ULA_ADR_REG, regNr);
	outp(ULA_DAT_REG, regVal);
}

/*
	Disable bright black

*/
void enableSpeccyBlack(void){
	writeUlaReg(ULA_SPECCY_BLACK_REG, 0xff);
}

/*
	Enable bright black

*/
void disableSpeccyBlack(void){
	writeUlaReg(ULA_SPECCY_BLACK_REG, 0);
}


void setBorder(unsigned char border){
unsigned char x;

	x = (inp(0xFE) & 0xf8) | (border & 7);
	outp(0xFE, x);
}

/*
	Write a new bit map into font memory

	Inputs:
		ccode		The character code to change.
					The range is 0..255
		bitMap		Pointer to an 8 byte buffer containing
					the new bit map values
*/

void changeChar80(unsigned char ccode, unsigned char *bitMap){
unsigned int charOffset;
unsigned char i;

	charOffset = (ccode << 3);
	writeUlaReg(ULA_FONT_ADRL_REG, (unsigned char)(charOffset & 0xFF));
	writeUlaReg(ULA_FONT_ADRH_REG, (unsigned char)(charOffset >> 8));

	for(i = 0; i < 8; i++){
		writeUlaReg(ULA_FONT_DAT_REG, bitMap[i]);
	}
}


/*
	Return the video memory address of a character
	for a column and line. The function clips
	x to 0..79
	y to 0..23

	Inputs:
		x		the column number. range 0..79
		y		the line number. range 0..23

	Returns:
		The memory address
*/
unsigned int adrXYChar80(unsigned char x, unsigned char y){
unsigned int tempAdr;

	if (y > 23) y = 23;
	if (x > 79) x = 79;
	gTxtAdr80 = (y * 80) + (unsigned int)(x) + 16384;
}


/*
	Return the video memory address of an attribute
	for a column and line. The function clips
	x to 0..79
	y to 0..23

	Inputs:
		x		the column number. range 0..79
		y		the line number. range 0..23

	Returns:
		The memory address
*/
unsigned int adrXYAttr80(unsigned char x, unsigned char y){
	gAttrAdr80 = adrXYChar80(x, y) + ATTROFFSET;
}


/*
	Clear the 80 column screen.

	Inputs:
		attr	the attribute to use to
				clear the screen
*/
void clrScr80(unsigned char attr){
unsigned int i, x;

	x = 16384;
	for (i = 0; i < ATTROFFSET; i++){
		bpoke(x++, ' ');
	}
	x = 16384 + ATTROFFSET;
	for (i = 0; i < ATTROFFSET; i++){
		bpoke(x++, attr);
	}

}


/*
	Make an attribute byte from the supplied ink
	and paper values. See the defines for valid
	values.

	Inputs:
		ink		color for ink
		paper	color for paper

	Returns:
		the attribute byte
*/
unsigned char makeInkPaper80(unsigned char ink, unsigned char paper){
	return (paper << 3) | ink;
}


/*
	Enable the bright attribute in the supplied
	attribute value.

	Inputs:
		attr	attribute byte

	Returns:
		the attribute byte with bright on
*/
unsigned char brightOn80(unsigned char attr){
	return attr | ATTR_BRIGHT;
}

/*
	Disable the bright attribute in the supplied
	attribute value.

	Inputs:
		attr	attribute byte

	Returns:
		the attribute byte with bright off
*/
unsigned char brightOff80(unsigned char attr){
	return attr & ~ATTR_BRIGHT;
}

/*
	Enable the flash attribute in the supplied
	attribute value.

	Inputs:
		attr	attribute byte

	Returns:
		the attribute byte with flash on
*/
unsigned char flashOn80(unsigned char attr){
	return attr | ATTR_FLASH;
}


/*
	Disable the flash attribute in the supplied
	attribute value.

	Inputs:
		attr	attribute byte

	Returns:
		the attribute byte with flash off
*/
unsigned char flashOff80(unsigned char attr){
	return attr & ~ATTR_FLASH;
}


/*
	Set the video mode to 80 column text mode
*/
void setModeText80(void){
unsigned char vMode;

	vMode = (inp(0xFF) & 0x07) | VID_80C;
	outp(0xFF, vMode);
}



/*
	Print a nul terminated text string at the
	caret current position.

	Input:
		txtStr		pointer to the nul terminated
					text string
*/
void printStrZ80(char *txtStr){
unsigned int i;

	while(1){
		if(*txtStr == 0) return;
		bpoke(gTxtAdr80++, *txtStr++);
		bpoke(gAttrAdr80++, gAttr80);
	}
}


/*
	Print a nul terminated text string at the
	user specified position

	Inputs:
		x			column to start printing
		y			line to start printing
		txtStr		pointer to the nul terminated
					text string
*/
void printAtZ80(unsigned char x, unsigned char y, char *txtStr){

	gTxtAdr80		= adrXYChar80(x, y);
	gAttrAdr80	= adrXYAttr80(x, y);
	printStrZ80(txtStr);

}



void printStrN80(char *txtStr, unsigned int len){
unsigned int i;

	for(i = 0; i < len; i++){
		bpoke(gTxtAdr80++, *txtStr++);
		bpoke(gAttrAdr80++, gAttr80);
	}
}



void printStrAtN80(unsigned char x, unsigned char y, char *txtStr, unsigned int len){

	gTxtAdr80	= adrXYChar80(x, y);
	gAttrAdr80	= adrXYAttr80(x, y);
	printStrN80(txtStr, len);

}


void printChar80(unsigned char chr){

	bpoke(gTxtAdr80, chr);
	bpoke(gAttrAdr80, gAttr80);
	gTxtAdr80		+= 1;
	gAttrAdr80	+= 1;
}

void printCharAt80(unsigned char x, unsigned char y, unsigned char chr){

	adrXYChar80(x, y);
	adrXYAttr80(x, y);

	bpoke(gTxtAdr80, chr);
	bpoke(gAttrAdr80, gAttr80);
	gTxtAdr80		+= 1;
	gAttrAdr80	+= 1;
}



void printCharAttr80(unsigned char chr, unsigned char attr){

	bpoke(gTxtAdr80, chr);
	bpoke(gAttrAdr80, attr);
	gTxtAdr80		+= 1;
	gAttrAdr80	+= 1;
}

void printCharAttrAt80(unsigned char x, unsigned char y, unsigned char chr, unsigned char attr){

	adrXYChar80(x, y);
	adrXYAttr80(x, y);

	bpoke(gTxtAdr80, chr);
	bpoke(gAttrAdr80, attr);
	gTxtAdr80		+= 1;
	gAttrAdr80	+= 1;
}


/*
	Set the global attribute to the user supplied
	attribute

	Inputs:
		attr		new attribute
*/
void setAttr80(unsigned char attr){
	gAttr80 = attr;
}


/*
	Return the current global attribute

*/
unsigned char getAttr80(void){

	return gAttr80;
}


