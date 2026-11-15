
#ifndef TEXT80RTNS_H

#define TEXT80RTNS_H

#include <stdio.h>
#include <stdlib.h>

#define ULA_ADR_REG	0xbf3b
#define ULA_DAT_REG	0xff3b
#define ULA_ENA_REG	0x40
#define ULA_VID_MODE_REG 0x41
#define ULA_SPECCY_BLACK_REG 0x42
#define MEM_MIRR_REG 0x43
#define ULA_FONT_ADRL_REG 0x44
#define ULA_FONT_ADRH_REG 0x45
#define ULA_FONT_DAT_REG 0x46
#define ULA_BICOLOR_DAT_REG 0x47


// TS2068 attribute bit patterns
#define ATTR_BLACK 		0x00
#define ATTR_BLUE 		0x01
#define ATTR_RED 		0x02
#define ATTR_MAGENTA 	0x03
#define ATTR_GREEN 		0x04
#define ATTR_CYAN 		0x05
#define ATTR_YELLOW 	0x06
#define ATTR_WHITE 		0x07
#define ATTR_BRIGHT 	0x40
#define ATTR_FLASH 		0x80

#define VID_32DF1   0x00        // standard TS 32 column video using DF1 (this is the default on boot)
#define VID_32DF2   0x01        // TS 32 column video using DF2
#define VID_HIRES   0x02        // TS hires
#define VID_QCOLOR  0x03        // new quad color mode
#define VID_80C     0x05        // 80A column text mode -w- individual attributes in DF1
#define VID_64      0x06        // standard TS 64 column mode using DF1 & DF2 and a fixed attribute
#define VID_64A     0x07        // new TS 64 column mode that uses the attributes

#define ATTROFFSET 1920


extern unsigned char gAttr80;
extern unsigned int gTxtAdr80;
extern unsigned int gAttrAdr80;

void writeUlaReg(unsigned char regNr, unsigned char regVal);
void enableSpeccyBlack(void);
void disableSpeccyBlack(void);
void setBorder(unsigned char border);

void changeChar80(unsigned char ccode, unsigned char *bitMap);
unsigned int adrXYChar80(unsigned char x, unsigned char y);
unsigned int adrXYAttr80(unsigned char x, unsigned char y);
void clrScr80(unsigned char attr);
unsigned char makeInkPaper80(unsigned char ink, unsigned char paper);
unsigned char brightOn80(unsigned char attr);
unsigned char brightOff80(unsigned char attr);
unsigned char flashOn80(unsigned char attr);
unsigned char flashOff80(unsigned char attr);
void setModeText80(void);
void printStrZ80(char *txtStr);
void printAtZ80(unsigned char x, unsigned char y, char *txtStr);
void setAttr80(unsigned char attr);
unsigned char getAttr80(void);

void printStrN80(char *txtStr, unsigned int len);
void printStrAtN80(unsigned char x, unsigned char y, char *txtStr, unsigned int len);
void printChar80(unsigned char chr);
void printCharAttr80(unsigned char chr, unsigned char attr);
void printCharAt80(unsigned char x, unsigned char y, unsigned char chr);
void printCharAttrAt80(unsigned char x, unsigned char y, unsigned char chr, unsigned char attr);

#endif
