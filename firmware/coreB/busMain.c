/*
    This file handles communication with the Z80 bus. This is a very hardware dependant file and it
    a very specific configuration for the GAL is assumed. One of the issues with the GAL and Pico is
    that the GAL will throw invalid select pulses to the Pico because of the speed of the GAL. The Pico
    is operating at 250MHz and so is fast enough to respond to these pulses. A second issue is that the
    buffers used to act as multiplexers are not fast enough to switch valid data onto the Pico data I/O
    pins as quickly as the Pico can operate. Special functions have been written to deal with these
    issues.

    The Pico interrupt system is not fast enough to meet the real time requirements of the Z80 bus so
    we poll the select signal. Polling can provide as little a 70ns resolution so is generally fast enough
    to allow real time reading and writing from/to the Z80 bus.

    We use the SDK routines to read all pins and write to others based on a mask. This speeds up reading
    and writing to the pins by a large margin over using C bit banging.

    Globals are used extensively for speed since their addresses will be known at compile time. Automatic
    variables are used primarily for counters and other non time critical uses.
    
*/


#include <stdio.h>
#include "pico/stdlib.h"
#include "string.h"


#include "hardware/irq.h"
#include "hardware/clocks.h"

#include "../macros.h"

// Mask bits for the control word (gZ80Ctl)
#define IOMASK 1
#define READMASK 2
#define MRQMASK 4

#define ISIORD 4
#define ISIOWR 6
#define ISMEMWR 3


// Pin numbers for special purpose pins
#define PSEL 10
#define IOPORT 12
#define RDPORT 13
#define PDDIR 26

// Mask that defines the data pins in the Pico pin set
//  This mask MUST change if these pins are reassigned
#define DATAPINS_MASK   0b00000000011111111000000000000000

// Mask for the buffer select pins.
//  This mask MUST change if these pins are reassigned
#define PG_MASK         0b00011000000000000000000000000000
// Mask used to address one of the four buffers
#define PG0             0b00000000000000000000000000000000
#define PG1             0b00001000000000000000000000000000
#define PG2             0b00010000000000000000000000000000
#define PG3             0b00011000000000000000000000000000


#define ADRL PG0
#define ADRH PG1
#define DATA PG2
#define CTLSIG PG3

// Convenience macro to fetch the data bits
#define GETDATA ((unsigned short)((gpio_get_all() >> 15) & 0xff));


// globals used throughout the progam
extern unsigned char gTsVidMode;
extern unsigned char gTsRegFE;
extern unsigned char gTsRegFF;
extern unsigned char gTsVidMode;
extern unsigned char gTsCol64Ink;

extern unsigned char gVidMode;
extern unsigned char gBorder;

extern const unsigned char tsColors[16];
extern unsigned char borderColors[8];

extern unsigned char gUlaColors[64];
extern unsigned char gUlaEna;
extern unsigned char gUlaVmode;
extern unsigned char gUlaAdrReg;

extern unsigned char gDfRam[16384];

extern unsigned char gTextPort;
extern unsigned char gTextStat;

extern unsigned short gHiresAdr;


unsigned char gZ80Ctl;          // selected bits from the Z80 control bus
unsigned char gZ80Data;
unsigned short gZ80Adr;
unsigned char gZ80AdrL;
unsigned char gZ80AdrH;

unsigned char gOutVal;

unsigned short dfAdr;

unsigned char testPort;
unsigned char xPort = 0;

// test registers
extern unsigned short gTestMemAdr;
extern unsigned short gTestCrlReg;

void mySerOut(unsigned short myVal){
unsigned char i;
unsigned short mask = 0x0001;

// ensure the line is high
    gpio_put(11, 1);
    sleep_us(5);

// start bit
    gpio_put(11, 0);
    sleep_us(1);

    for(i = 0; i < 16; i++){
        if (myVal & mask){
            gpio_put(11, 1);
        } else {
            gpio_put(11, 0);
        }
        mask = mask << 1;
        sleep_us(1);
    }

    gpio_put(11, 1);
    sleep_us(5);

}

/**************************************************************
 * This function sets up the Pico pins that interface with the
 *  Z80 bus. It is only called when the program starts so
 *  it can reside in the serial flash and use XIP
**************************************************************/
void busSetup(void){

    irq_set_mask_enabled(0xffcfbf, false);

    gpio_init(IOPORT);
    gpio_set_dir(IOPORT, GPIO_IN);
    gpio_init(RDPORT);
    gpio_set_dir(RDPORT, GPIO_IN);

// Define the buffer select pins and set them 
    gpio_init(PSEL);
    gpio_set_dir(PSEL, GPIO_IN);
    gpio_put_masked(PG_MASK, ADRL);


// Define the data direction pin and set it to
//  high
    gpio_init(PDDIR);
    gpio_set_dir(PDDIR, GPIO_OUT);
    gpio_put(PDDIR, 1);

    gpio_init_mask(PG_MASK | DATAPINS_MASK);
    gpio_set_dir_out_masked(PG_MASK);
    gpio_set_dir_in_masked(DATAPINS_MASK);

}

/**************************************************************
 * This function handles the Z80 bus interface.
 *  
 *  This is a time critical routine so it resides in RAM
**************************************************************/

void __not_in_flash_func(busMain(void)){
unsigned char flag;
unsigned char i;

    gpio_put(25, 1);
    busSetup();
    gpio_put_masked(PG_MASK, ADRL);     // set up for the next round

    while(true){
    // wait for the PSEL line to go active
        while(true){
            if (gpio_get(PSEL)){
                if (gpio_get(PSEL)) break;
            }
        }

// collect the bus data from the Z80
// the double reads allow the data more time to settle after
//  the buffer is activated.
        gZ80AdrL     = GETDATA;
        gZ80AdrL     = GETDATA;
        gpio_put_masked(PG_MASK, ADRH);                 // get the high byte of the address
        gZ80AdrH     = GETDATA;
        gZ80AdrH     = GETDATA;
        gZ80AdrH     = GETDATA;
        gpio_put_masked(PG_MASK, DATA);                 // set up to get the data
        gZ80Adr     = (gZ80AdrH << 8) | gZ80AdrL;       // make the 16 bit address for ULA Plus and video memory

        flag    = 0;
        gOutVal = 0;
    // the GAL ensures that, when PSEL is assterted, the only reads are I/O reads.
    //  this means we do not need to worry about memory accesses.
        if (!gpio_get(RDPORT)){
            // reading from our ports
                if (gZ80Adr == ULA_ADR_REG){
                    flag = 1;
                    gOutVal = gUlaAdrReg;
                } else {
                    if (gZ80Adr == ULA_DAT_REG){
                        flag = 1;
                        gOutVal = 0xFF;
                        if (gUlaAdrReg == 0x40) gOutVal = gUlaEna;
                        if (gUlaAdrReg == 0x41) gOutVal = gUlaVmode;
                        if (gUlaAdrReg == 0x7F) gOutVal = SOFTWARE_VER;
                        if (gUlaAdrReg < 0x40) gOutVal = gUlaColors[(gUlaAdrReg & 0x3F)];
                    } else {
                        if (gZ80AdrL == TEXT_PORT){
                            flag = 1;
                            gOutVal     = (gpio_get(9) << 7) | (gpio_get(8) << 6) | (gTextStat & 1);
                        }
                        if (gZ80AdrL == TEST_PORT){
                            flag = 1;
                            gOutVal     = testPort;
                        }
                    }     
                } 
                if (flag){

                // the data buffer is enabled when we arrive here
                    // set the buffer data direction to send to the Z80
                        gpio_put(PDDIR, 0);
                        gpio_put(PDDIR, 0);
                        gpio_put(PDDIR, 0);
                        gpio_put(PDDIR, 0);
                    // make our data bus I/O pins outputs
                        gpio_set_dir_out_masked(DATAPINS_MASK);
                    // output the data
                        gpio_put_masked(DATAPINS_MASK, gOutVal << 15);
                    // spin until the Z80 is done
                        while (gpio_get(PSEL));
                    // make the Pico pins input
                        gpio_set_dir_in_masked(DATAPINS_MASK);
                        gpio_set_dir_in_masked(DATAPINS_MASK);
                        gpio_set_dir_in_masked(DATAPINS_MASK);
                        gpio_set_dir_in_masked(DATAPINS_MASK);
                    // set the dat direction to input from the Z80
                        gpio_put(PDDIR, 1); 
                    // unslect the data buffer by enabling the address buffer
                        gpio_put_masked(PG_MASK, ADRL);
                }
                goto endoFunc;
        } else {
        // some sort of write operation
        //  get the data from the Z80 bus since we may need it
            gZ80Data     = GETDATA;             // get the value from the data bus
            gZ80Data     = GETDATA;             // get the value from the data bus
//            gZ80Data     = GETDATA;             // get the value from the data bus

            if (!gpio_get(IOPORT)){
        // writing to our I/O ports
                if (gZ80Adr == ULA_ADR_REG){
                    gUlaAdrReg = gZ80Data;
                } else if (gZ80Adr == ULA_DAT_REG){
                    if (gUlaAdrReg == 0x40) gUlaEna = gZ80Data;
                    if (gUlaAdrReg == 0x41) gUlaVmode = gZ80Data;
                    if (gUlaAdrReg < 0x40) gUlaColors[(gUlaAdrReg & 0x3F)] = gZ80Data;
                } else {
                    switch (gZ80AdrL){
                    case 0xFF:
                    // our local copy of the TS register
                        gTsRegFF    = gZ80Data;
                    // this converts the 3-bit 64 column paper/ink value to the
                    //  equivalent bright attribute byte.
                        gTsCol64Ink = ((~gTsRegFF) & 0x3f) | ((gTsRegFF >> 3) & 7);
                        break;
                    case HIRES_ADRL:
                        gHiresAdr = (gHiresAdr & 0xff00) | gZ80Data;
                        break;
                    case HIRES_ADRH:
                        gHiresAdr = (gHiresAdr & 0x00ff) | (gZ80Data << 8);
                        break;
                    case HIRES_DAT:
                        if (gVidMode == VID_HIRES8){
                            gDfRam[gHiresAdr] = gZ80Data;
                            ++gHiresAdr;
                        }
                        break;
                    case 0xFE:
                        gTsRegFE = gZ80Data;
                        break;
                    case TEXT_PORT:
                        gTextPort   = gZ80Data;
                        gTextStat   = 1;
                        break;
                    case TEST_PORT:
                        testPort    = gZ80Data;
                        xPort       = gZ80Data;
                        break;
                    }
                }
        // ensure that global variables are updated
                if (gUlaEna){
                // ULA Plus active
                    gBorder = gUlaColors[gTsRegFE & 0x07];
                    gVidMode  = gUlaVmode & 0x0F;
                } else {
                    gBorder = borderColors[gTsRegFE & 0x07];
                // save the video mode
                    gVidMode  = gTsRegFF & 0x07;
                }
                goto endoFunc;
            }
        // the GAL ensures that only video memory addresses assert PSEL
            gDfRam[gZ80Adr & 0x3fff] = gZ80Data; 
        }


endoFunc:

    // spin while the PSEL line is high. We MUST complete all bus operations
    //  prior to PSEL going inactive so there is no chance a race condition
    //  will cause problems.
        gpio_put_masked(PG_MASK, ADRL);     // set up for the next round
        while (gpio_get(PSEL));
    }
}


