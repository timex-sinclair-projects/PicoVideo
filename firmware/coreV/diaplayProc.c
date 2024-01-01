#include "pico/stdlib.h"
#include "string.h"
#include "../macros.h"

extern unsigned char testBits[33];
extern unsigned char testAttr[33];


// program global declarations
extern unsigned char gVidMode;
extern unsigned char gBorder;
extern unsigned char gUlaEna;
extern unsigned char gTsCol64Ink;
extern unsigned char gRegFE;
extern unsigned char gDfRam[16384];
extern unsigned char gLbuf0[640];     // VGA line buffer 0
extern unsigned char gLbuf1[640];     // VGA line buffer 1
extern unsigned char biColor[];
extern unsigned char gUlaColors[];
extern const unsigned char tsColors[];
extern const unsigned char ts64Colors[];
extern unsigned short bmapOffset[192];
extern unsigned short char80Bmap[2048];

extern int fillCmd;

extern unsigned char gFlashMask;

unsigned char gPaper, gInk, gAttr, gBmap;
unsigned char *gpPalette;
unsigned char gPalInk, gPalPaper;


void __not_in_flash_func(do32Col)(unsigned char *outBuf, unsigned char bmap);
void __not_in_flash_func(do64Col)(unsigned char *outBuf, unsigned char bmap);



void __not_in_flash_func(topBottBorder)(unsigned char *bordBuf){
unsigned int i;
    memset(bordBuf, gBorder, 640);
}

void __not_in_flash_func(getInkPaper)(void){

    if (gUlaEna){
        gPalInk     = gUlaColors[(((gAttr & 0xC0) >> 2) | (gAttr & 7))];
        gPalPaper   = gUlaColors[(((gAttr & 0xC0) >> 2) | (((gAttr >> 3) & 0x07) | 8))];
        return;
    }
// // get the paper color and keep the bright bit
//     gPaper  = (gAttr >> 3) & 0x0F;
// // get the ink color and keep the bright bit
//     gInk    = ((gAttr & 0x40) >> 3) | (gAttr & 7);

    gPalInk     = tsColors[(((gAttr & 0x40) >> 3) | (gAttr & 7))];
    gPalPaper   = tsColors[((gAttr >> 3) & 0x0F)];
 
// apply flash if it is enabled
    if (gAttr & 0x80) gBmap = gBmap ^ gFlashMask;
}

// candidate function for outputting a scan line of
//  TS video. We assume that 
void __not_in_flash_func(makeScanLine)(unsigned char *outBuf, unsigned int lineNr){
unsigned int i;
unsigned char *pLine;
unsigned char *pAttr;
unsigned char *pLine1;
unsigned char *pAttr1;
unsigned char *pBuf;
unsigned short bmapOfs, attrOfs;

unsigned short charIndex;
unsigned char charCode, charBmap, charAttr;

// populate left border...
    memset(outBuf, gBorder, 64);
// ... and the right boarder
    memset(&outBuf[576], gBorder, 64);
    pBuf    = &outBuf[64];

// form the convoluted TS video address by reading the value
//  from a table using the scan line number as the index.
//    bmapOfs = bmapOffset[lineNr];

// form the convoluted TS video address by bit shifting the scan line number
    bmapOfs = ((lineNr & 0xC0) << 5) | ((lineNr & 0x38) << 2) | ((lineNr & 0x07) << 8);

// the address of the attribute is much less convoluted so
//  simple instructions can form it.
    attrOfs = ((lineNr & 0x00f8) << 2) + 0x1800;

// process the scan line based on the video mode
    switch (gVidMode){
    case VID_QCOLOR:
    // quad color mode
//        bmapOfs = bmapOffset[lineNr];
        pLine   = &gDfRam[bmapOfs + DF1_OFFSET];
        pLine1  = &gDfRam[bmapOfs + DF2_OFFSET];

        for (i = 0; i < 32; i++){
            gBmap = *pLine++;
            gAttr = *pLine1++;

            *pBuf++ = biColor[(gBmap >> 6) & 0x03];
            *pBuf++ = biColor[(gBmap >> 6) & 0x03];

            *pBuf++ = biColor[(gBmap >> 4) & 0x03];
            *pBuf++ = biColor[(gBmap >> 4) & 0x03];

            *pBuf++ = biColor[(gBmap >> 2) & 0x03];
            *pBuf++ = biColor[(gBmap >> 2) & 0x03];

            *pBuf++ = biColor[gBmap & 0x03];
            *pBuf++ = biColor[gBmap & 0x03];

            *pBuf++ = biColor[(gAttr >> 6) & 0x03];
            *pBuf++ = biColor[(gAttr >> 6) & 0x03];

            *pBuf++ = biColor[(gAttr >> 4) & 0x03];
            *pBuf++ = biColor[(gAttr >> 4) & 0x03];

            *pBuf++ = biColor[(gAttr >> 2) & 0x03];
            *pBuf++ = biColor[(gAttr >> 2) & 0x03];

            *pBuf++ = biColor[gAttr & 0x03];
            *pBuf++ = biColor[gAttr & 0x03];

            pBuf += 16;
        }
        break;

//     case VID_80COL:
// // toy code for 80 column text mode
//         pBuf -= 64;                         // point to the start of the line buffer
//         charIndex = (lineNr >> 3)  * 80;    // the location of the first character on this line
//         for (i = 0; i < 80; i++){
//             charCode    =  gDfRam[charIndex];
//             gAttr       =  gDfRam[charIndex + 2048];
//             gBmap       =  char80Bmap[(charCode << 3) | (lineNr & 7)];
//             getInkPaper();
//             do64Col(pBuf, gBmap);
//             charIndex++;
//             pBuf += 8;
//         }
//         break;

//     case VID_HIRES8:
//         charIndex = lineNr << 8;            // 
//         for (i = 0; i < 256; i++){
//             gAttr = gDfRam[charIndex++];
//             *pBuf++ = gAttr;
//             *pBuf++ = gAttr;
//         }
//         break;

    case VID_64:
    case VID_64A:

    // standard 64 column mode
        pAttr   = &gDfRam[attrOfs + DF1_OFFSET];
        pAttr1  = &gDfRam[attrOfs + DF2_OFFSET];
        pLine   = &gDfRam[bmapOfs + DF1_OFFSET];
        pLine1  = &gDfRam[bmapOfs + DF2_OFFSET];

        for (i = 0; i < 32; i++){
            gAttr   = gTsCol64Ink;
            if (gVidMode == VID_64A) gAttr   = *pAttr++;
            gBmap   = *pLine++;
            getInkPaper();
            do64Col(pBuf, gBmap);
             pBuf += 8;

            gAttr   = gTsCol64Ink;
            if (gVidMode == VID_64A) gAttr   = *pAttr1++;
//            if (gVidMode == VID_64A) gAttr   = *pAttr++;
            gBmap   = *pLine1++;
            getInkPaper();
            do64Col(pBuf, gBmap);
            pBuf += 8;
        }
        break;

    case VID_32DF1:
        pLine   = &gDfRam[bmapOfs + DF1_OFFSET];
        pAttr   = &gDfRam[attrOfs + DF1_OFFSET];
        goto col32Out;
    case VID_32DF2:
        pLine   = &gDfRam[bmapOfs + DF2_OFFSET];
        pAttr   = &gDfRam[attrOfs + DF2_OFFSET];
        goto col32Out;
    case VID_HIRES:
        pLine   = &gDfRam[bmapOfs + DF1_OFFSET];
        pAttr   = &gDfRam[bmapOfs + DF2_OFFSET];
        goto col32Out;
    default:
        pLine   = &gDfRam[bmapOfs + DF1_OFFSET];
        pAttr   = &gDfRam[attrOfs + DF1_OFFSET];

col32Out:
        for (i = 0; i < 32; i++){
            gBmap   = *pLine++;
            gAttr   = *pAttr++;
            getInkPaper();
            do32Col(pBuf, gBmap);
        // point to the next slot in the video output buffer
            pBuf += 16;
        }
        break;

    }
}

void __not_in_flash_func(do64Col)(unsigned char *outBuf, unsigned char bmap){
unsigned char color;

    *outBuf++ = (bmap & 0x80) ? gPalInk : gPalPaper;
    *outBuf++ = (bmap & 0x40) ? gPalInk : gPalPaper;
    *outBuf++ = (bmap & 0x20) ? gPalInk : gPalPaper;
    *outBuf++ = (bmap & 0x10) ? gPalInk : gPalPaper;
    *outBuf++ = (bmap & 0x08) ? gPalInk : gPalPaper;
    *outBuf++ = (bmap & 0x04) ? gPalInk : gPalPaper;
    *outBuf++ = (bmap & 0x02) ? gPalInk : gPalPaper;
    *outBuf++ = (bmap & 0x01) ? gPalInk : gPalPaper;

}



void __not_in_flash_func(do32Col)(unsigned char *outBuf, unsigned char bmap){
unsigned char color;

    color = (bmap & 0x80) ? gPalInk : gPalPaper;
    *outBuf++ = color;
    *outBuf++ = color;
    color = (bmap & 0x40) ? gPalInk : gPalPaper;
    *outBuf++ = color;
    *outBuf++ = color;
    color = (bmap & 0x20) ? gPalInk : gPalPaper;
    *outBuf++ = color;
    *outBuf++ = color;
    color = (bmap & 0x10) ? gPalInk : gPalPaper;
    *outBuf++ = color;
    *outBuf++ = color;
    color = (bmap & 0x08) ? gPalInk : gPalPaper;
    *outBuf++ = color;
    *outBuf++ = color;
    color = (bmap & 0x04) ? gPalInk : gPalPaper;
    *outBuf++ = color;
    *outBuf++ = color;
    color = (bmap & 0x02) ? gPalInk : gPalPaper;
    *outBuf++ = color;
    *outBuf++ = color;
    color = (bmap & 0x01) ? gPalInk : gPalPaper;
    *outBuf++ = color;
    *outBuf++ = color;
}
