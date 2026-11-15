
#include <stdio.h>
#include <stdlib.h>

#include "text80Rtns.h"
			  //        11111111112222222222333333333344444444445555555555666666666677777777778
             //12345678901234567890123456789012345678901234567890123456789012345678901234567890
char str2[] = " Index   File name                                            File Size";
char str1[] = " TIMEX             SD Card Commander";

unsigned char spcBitMap[8] = {0xAA, 0xAA, 0x55, 0x55, 0xAA, 0xAA, 0x55, 0x55};


void printForm(void){
unsigned char i;

// top corners and horizontal
	printCharAt80(0, 0, 0xc9);
	for(i = 0; i < 78; i++){
		printChar80(0xCD);
	}
	printChar80(0xBB);

// sides for title
	printCharAt80(0, 1, 0xba);
	printCharAt80(0, 2, 0xba);
	printCharAt80(0, 3, 0xc7);

// horizontal divider
	for(i = 0; i < 78; i++) printChar80(0xc4);
	printCharAt80(8, 3, 0xc2);
	printCharAt80(51, 3, 0xc2);

// vertical divider
	for(i = 4; i < 23; i++){
		printCharAt80(8, i, 0xb3);
		printCharAt80(51, i, 0xb3);
	}


	printCharAt80(79, 1, 0xba);
	printCharAt80(79, 2, 0xba);
	printCharAt80(79, 3, 0xb6);

	for(i = 4; i < 23; i++){
		printCharAt80(0, i, 0xba);
		printCharAt80(79, i, 0xba);
	}

	printCharAt80(0, 23, 0xc8);
	for(i = 0; i < 78; i++){
		printChar80(0xCD);
	}
	printChar80(0xBC);


	printCharAt80(8, 23, 0xcf);
	printCharAt80(51, 23, 0xcf);

	makeInkPaper80(ATTR_RED, ATTR_GREEN);
	printCharAttrAt80(76, 1, 221, makeInkPaper80(ATTR_BLUE, ATTR_BLACK));
	makeInkPaper80(ATTR_RED, ATTR_GREEN);
	printCharAttrAt80(77, 1, 221, makeInkPaper80(ATTR_RED, ATTR_GREEN ));
	makeInkPaper80(ATTR_BLUE, ATTR_CYAN);
	printCharAttrAt80(78, 1, 221, makeInkPaper80(ATTR_BLUE, ATTR_WHITE));

}


void printBanner(void){
	setAttr80(makeInkPaper80(ATTR_CYAN, ATTR_BLACK));
	printStrAtN80(1, 1, &str1[0], 7);
	setAttr80(makeInkPaper80(ATTR_YELLOW, ATTR_BLUE));
	printStrZ80(&str1[7]);

	printAtZ80(1, 2, &str2[0]);
}

main(){
unsigned int i, j;

	gAttr80 = makeInkPaper80(ATTR_YELLOW, ATTR_BLUE);

	setModeText80();
	enableSpeccyBlack();
	setBorder(ATTR_CYAN);

	clrScr80(gAttr80);

	adrXYChar80(0, 0);

	printForm();
	printBanner();

	getkey();

	changeChar80(' ', spcBitMap);

	while(1);

}
