

ULA_ADR_REG				.equ 0xbf3b
ULA_DAT_REG				.equ 0xff3b
ULA_ENA_REG				.equ 0x40
ULA_VID_MODE_REG		.equ 0x41
ULA_SPECCY_BLACK_REG	.equ 0x42
MEM_MIRR_REG			.equ 0x43
ULA_FONT_ADRL_REG		.equ 0x44
ULA_FONT_ADRH_REG		.equ 0x45
ULA_FONT_DAT_REG		.equ 0x46
ULA_BICOLOR_DAT_REG		.equ 0x47


; TS2068 attribute bit patterns
ATTR_BLACK		.equ 0x00
ATTR_BLUE		.equ 0x01
ATTR_RED		.equ 0x02
ATTR_MAGENTA	.equ 0x03
ATTR_GREEN		.equ 0x04
ATTR_CYAN		.equ 0x05
ATTR_YELLOW		.equ 0x06
ATTR_WHITE		.equ 0x07
ATTR_BRIGHT		.equ 0x40
ATTR_FLASH		.equ 0x80

VID_32DF1		.equ 0x00        ; standard TS 32 column video using DF1 (this is the default on boot)
VID_32DF2		.equ 0x01        ; TS 32 column video using DF2
VID_HIRES		.equ 0x02        ; TS hires
VID_QCOLOR		.equ 0x03        ; new quad color mode
VID_80C			.equ 0x05        ; 80A column text mode -w- individual attributes in DF1
VID_64			.equ 0x06        ; standard TS 64 column mode using DF1 & DF2 and a fixed attribute
VID_64A			.equ 0x07        ; new TS 64 column mode that uses the attributes

ATTROFFSET 		.equ 1920


