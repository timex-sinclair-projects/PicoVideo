#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "string.h"
// Our assembled PIO programs:
#include "vgaPioSm.pio.h"
#include "../macros.h"

extern unsigned char gDfRam[16384 * 3];
extern unsigned char gTextPort;
extern unsigned char gTextStat;
extern unsigned char gTextAttr;


static unsigned char textModeState     = 0;
static unsigned char textModeRtnState  = 0;
static unsigned char escFlag       = 0;
static unsigned char textLine      = 0;
static unsigned char textCol       = 0;
static unsigned char params[4]     = {0,0,0,0};
static unsigned char paramIndex    = 0;
static unsigned short tmemIndex;
static unsigned char tempLine, tempCol;
static unsigned char curAttr;
static unsigned char numStat;

void doTextMode(void){

// disable for now
    return;

    if (gTextStat){
        switch (textModeState){
        case 0:
            for (paramIndex = 0; paramIndex < 4; paramIndex++) params[paramIndex] = 0;
            paramIndex = 0;
            textModeState = 1;
        case 1:
            if (gTextPort == 0x1B){
                textModeState   = 2;       // go handle escape sequences
                gTextStat       = 0;
            } else {
            // write the character
                col80WriteChar();
                gTextStat       = 0;
            }
            break;

    // start ESC handling
        case 2:
            if (gTextPort == '['){
                textModeState   = 3;
                gTextStat       = 0;
            } else {
            // we did not receive the required chatacter
                col80WriteChar();
                textModeState   = 1;
                gTextStat       = 0;
            }
            break;
        case 3:
            if ((gTextPort >= '0') && (gTextPort <= '9')){
        // there is a parameter being sent
                params[paramIndex]  = 0;
                textModeState       = 4;
            } else {

            }

        case 4:
            numStat = col80GetNum();
            switch (numStat){
            case 0:
        // a digit was found, so keep accumulating the value
                gTextStat       = 0;
                break;
            case 1:
        // end of one parameter and start of another
        
                gTextStat       = 0;
                break;
            case 2:
        // a command letter was received
                break;
            }

            break;
        case 'H':
        // home the cursor
            textLine = 0;
            textCol = 0;
            break;
        case 'K':
        // clear to the end of the line
            break;
        case 's':
        // save the cursor position
            break;
        case 'u':
        // restore the cursor position
            break;
        case 'J':
        // clear screen
            break;
        case 'A':
        // cursor up x spaces
            break;
        case 'B':
        // cursor down x spaces
            break;
        case 'C':
        // cursor forwared x spaces
            break;
        case 'D':
        // cursor backward x spaces
            break;
        case 'L':
        // delete line
            break;
        case 'M':
        // insert line
            break;
        case 'm':
        // set graphics rendition
            break;




        case 255:
    // convenience state to clean up escape processing
            textModeState   = 0;
            gTextStat       = 0;
            break;
        }
    }
}


void col80WriteChar(void){
unsigned short tmemIndex;

    tmemIndex = textLine * 80 + textCol;
    gDfRam[tmemIndex + TEXTCHAR_OFFSET]   = gTextPort;
    gDfRam[tmemIndex + TEXTATTR_OFFSET]   = gTextAttr;
    textCol++;
    if (textCol > 79){
    // we were at the end of the line, so do an auto CRLF
        textCol = 0;
        textLine++;
        if (textLine > 23){
    // we were on the botton screen line, so scroll the screen up
        // clip the current line number
            textLine = 23;
        // delete the top line and move the remainder up
            col80DeleteLine(0);
            col80FillLines(23, 0, 80);
        }
    }
}

unsigned char col80GetNum(void){
    if ((gTextPort >= '0') && (gTextPort <= '9')){
        params[paramIndex] = params[paramIndex] + (gTextPort = '0');
        return 0;
    } else {
        if (paramIndex < 4) paramIndex++;
        if (gTextPort = ';'){
        // another parameter is being sent
            return 1;
        } else {
        // this is a command (possibly)
            return 2;            
        }
    }
}

// Place n characters in the text and attribute memories. This routine is used
//  by many commands to fill parts of the display. Filling stars at the line and
//  column provided. This routine may take up to 20us to run.
void col80FillLines(unsigned char fillLine, unsigned char fillCol, unsigned char nrChars){
unsigned char fillAdr;

    if (nrChars){
        fillAdr = fillLine * 80 + fillCol + TEXTCHAR_OFFSET;
        memset(&gDfRam[fillAdr], ' ', nrChars);
        fillAdr += TEXTSCRN_SIZE;
        memset(&gDfRam[fillAdr], curAttr, nrChars);
    }
}


void col80InsertLine(unsigned char insertLine){
unsigned short destAdr, srcAdr, nrBytes;

// there is nothing to move if we are inserting at line 23
//  so just return
    if (insertLine >= 23) return;

// form the source address for the move
    srcAdr = insertLine * 80 + TEXTCHAR_OFFSET;
// form the destination address for the move
    destAdr = insertLine + 1;
    destAdr = destAdr * 80 + TEXTCHAR_OFFSET;

    nrBytes = (23 - insertLine) * 80;

// copy the character bytes
    memmove(destAdr, srcAdr, nrBytes);
// copy the attribute bytes
    destAdr += TEXTSCRN_SIZE;
    srcAdr += TEXTSCRN_SIZE;
    memmove(destAdr, srcAdr, nrBytes);


}

// delete 1 line at delLine by copying the screen lines up
//  this routine will take approximately 20us
void col80DeleteLine(unsigned char delLine){
unsigned short destAdr, srcAdr, delBytes;

// there is nothing to copy if we are deleting line 23
//  so just return
    if (delLine >= 23) return;

// array index we are moving to
    destAdr = delLine * 80 + TEXTCHAR_OFFSET;
// form the source address (we are using srcAdr as a temporary variable)
    srcAdr = delLine + 1;
    srcAdr = srcAdr * 80 + TEXTCHAR_OFFSET;
// determine the count of bytes to move
    delBytes = (23 - delLine) * 80;
// copy the lines up
    memcpy(&gDfRam[destAdr], &gDfRam[srcAdr], delBytes);
// copy the attributes up
    destAdr += TEXTSCRN_SIZE;
    srcAdr += TEXTSCRN_SIZE;
    memcpy(&gDfRam[destAdr], &gDfRam[srcAdr], delBytes);
}

