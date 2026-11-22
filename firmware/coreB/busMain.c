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
#include "../video_decode.h"

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
#define GETDATA ((unsigned short)((gpio_get_all() >> 15) & 0xff))


// shared state lives in globals.c
#include "../globals.h"


unsigned char gZ80Ctl;          // selected bits from the Z80 control bus
unsigned char gZ80Data;
unsigned short gZ80Adr;
unsigned char gZ80AdrL;
unsigned char gZ80AdrH;

unsigned char gOutVal;

unsigned short dfAdr;

unsigned char testPort;
unsigned char xPort = 0;

unsigned char memMirror = 0;

// The TS-2068 Megabank (github.com/timex-sinclair-projects/TS-2068-Megabank):
//  OUT FDh selects one of its registers, OUT FCh writes it. Registers 02h and
//  03h (OWN2, OWN3) say which expansion bank owns chunk 2 (4000h-5FFFh) or
//  chunk 3 (6000h-7FFFh): 1-253 a bank, 0, FEh or FFh none. While a bank owns
//  the chunk, a write there goes to the bank, not the 2068's screen, so it is
//  not copied. The board decodes A7-A0 only, and so does this. Which 8K of the
//  bank shows (SUB2, SUB3) does not matter here. The Megabank ROM writes every
//  OWNn = 0 at boot, which resets these after a reset PicoVideo cannot see.
unsigned char mbReg = 0;        // the register last selected with OUT FDh
unsigned char mbOwn2 = 0;       // OWN2: chunk 2's owner
unsigned char mbOwn3 = 0;       // OWN3: chunk 3's owner

#define MB_OWNED(own) ((unsigned char)((own) - 1) < 253)    // a bank, 1-253

// What a read of the data port (ULA_DAT_REG) will return, worked out in
//  advance. The Z80 samples an IN about 600 ns after it starts, and working the
//  answer out during the read took up to 604 ns for registers $46-$48: on a
//  board those reads returned stale or mixed values. datAnswer() runs after
//  every bus cycle instead, so a read only has to put this byte on the bus.
unsigned char gDatAnswer = 0;

static inline __attribute__((always_inline)) unsigned char datAnswer(void){
    unsigned char reg = gUlaAdrReg;

    if (reg < 0x40) return gUlaColors[reg];
    switch (reg){
    case 0x40: return gUlaEna;
    case 0x41: return gUlaVmode;
    case 0x43: return memMirror;
    case 0x46: return vgaFont[gfontIndex & 2047];
    case 0x47: return biColor[gfontIndex & 3];
    case 0x48: return gDfRam[(gfontIndex < DF_RAM_SIZE) ? gfontIndex : 0];
    case 0x7F: return SOFTWARE_VER;
    }
    return 0xFF;
}

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
uint16_t fontAdr;

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
//  the buffer is activated. How long that is depends on the compiler, so
//  CMakeLists.txt builds this file -Og and tests/check_arm.py times it.
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
                        flag = 2;
                        gOutVal = gDatAnswer;
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
                    // load the answer while the pins are still inputs, so
                    //  they drive it as soon as they turn round
                        gpio_put_masked(DATAPINS_MASK, gOutVal << 15);
                    // set the buffer data direction to send to the Z80. The
                    //  repeats give it time to turn before the pins drive it:
                    //  check_arm.py holds this gap to at least V4's.
                        gpio_put(PDDIR, 0);
                        gpio_put(PDDIR, 0);
                        gpio_put(PDDIR, 0);
                        gpio_put(PDDIR, 0);
                        gpio_put(PDDIR, 0);
                        gpio_put(PDDIR, 0);
                    // make our data bus I/O pins outputs
                        gpio_set_dir_out_masked(DATAPINS_MASK);
                        if (flag == 2){
                        // while the Z80 reads it, step the index past this
                        //  byte and work out the next answer: once PSEL drops,
                        //  the next bus cycle can come within a few hundred ns
                            if (gUlaAdrReg == 0x46) gfontIndex = ((gfontIndex & 2047) + 1) & 2047;
                            if (gUlaAdrReg == 0x47) gfontIndex = ((gfontIndex & 3) + 1) & 3;
                            if (gUlaAdrReg == 0x48){
                                if (gfontIndex >= DF_RAM_SIZE) gfontIndex = 0;
                                if (++gfontIndex >= DF_RAM_SIZE) gfontIndex = 0;
                            }
                            gDatAnswer = datAnswer();
                        }
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
                goto endoRead;
        } else {
        // some sort of write operation
        //  get the data from the Z80 bus since we may need it
        //  the double read allows more time for the data to settle
            gZ80Data     = GETDATA;             // get the value from the data bus
            gZ80Data     = GETDATA;             // get the value from the data bus

            if (!gpio_get(IOPORT)){
        // writing to our I/O ports
                if (gZ80Adr == ULA_ADR_REG){
                    gUlaAdrReg = gZ80Data;
                } else if (gZ80Adr == ULA_DAT_REG){
                    if (gUlaAdrReg == 0x40) gUlaEna = gZ80Data;
                    if (gUlaAdrReg == 0x41) gUlaVmode = gZ80Data;
                    if (gUlaAdrReg == 0x42) speccyBlack = gZ80Data;
                    if (gUlaAdrReg == 0x43) memMirror = gZ80Data;
                    if (gUlaAdrReg == 0x44) gfontIndex = (gfontIndex & 0xFF00) | gZ80Data;
                    if (gUlaAdrReg == 0x45) gfontIndex = (gfontIndex & 0x00FF) | (gZ80Data << 8);
                    if (gUlaAdrReg == 0x46){
                        gfontIndex &= 2047;
                        vgaFont[gfontIndex++] = gZ80Data;
                        gfontIndex &= 2047;
                    }
                    if (gUlaAdrReg == 0x47){
                        gfontIndex &= 3;
                        biColor[gfontIndex++] = gZ80Data;
                        gfontIndex &= 3;
                    }
                    
                    if (gUlaAdrReg < 0x40) gUlaColors[(gUlaAdrReg & 0x3F)] = gZ80Data;
                } else {
                    switch (gZ80AdrL){
                    case 0xFF:
                    // our local copy of the TS register
                        gTsRegFF    = gZ80Data;
                    // Decode only color bits; native 64-column BRIGHT is zero.
                        gTsCol64Ink = pv_64_attribute(gTsRegFF);
                        break;
                    case 0xF4:
                        gTsRegF4    = gZ80Data;
                        break;
                    case HIRES_ADRL:
                        gHiresAdr = (gHiresAdr & 0xff00) | gZ80Data;
                        break;
                    case HIRES_ADRH:
                        gHiresAdr = (gHiresAdr & 0x00ff) | (gZ80Data << 8);
                        break;
                    case HIRES_DAT:
                        if (gVidMode == VID_HIRES8){
                            if (gHiresAdr < DF_RAM_SIZE)
                                gDfRam[gHiresAdr] = gZ80Data;
                            ++gHiresAdr;
                        }
                        break;
                    case 0xFE:
                        gTsRegFE = gZ80Data;
                        break;
                    case 0xFD:
                    // Megabank: select a register
                        mbReg = gZ80Data;
                        break;
                    case 0xFC:
                    // Megabank: write the selected register; keep OWN2 and OWN3
                        if (mbReg == 0x02) mbOwn2 = gZ80Data;
                        if (mbReg == 0x03) mbOwn3 = gZ80Data;
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
                    gBorder = gUlaColors[pv_ula_border(gTsRegFE)];
                } else {
                    gBorder = borderColors[gTsRegFE & 0x07];
                }
                // Palette selection must not pin native software to mode 0.
                gVidMode = pv_video_mode(gUlaEna, gUlaVmode, gTsRegFF);
                goto endoFunc;
            } else {
        // the GAL ensures that only video memory addresses assert PSEL
            // this code ensures that the video memory is updated only when the
            //  TS2068 HSR register is set to allow access to the home bank to
            //  the TS2068. this allows writing to the dock and EXROM banks
            //  without munging the video memory. Nor when a Megabank bank owns
            //  the chunk: the write goes to the bank.

                if (((gTsRegF4 & 0x04) == 0) && !MB_OWNED(mbOwn2) && ((gZ80Adr & 0x2000) == 0)){
                // writing to the low memory addresses
                    if (memMirror){
                    // we are mirroring the high memory here
                        gDfRam[(gZ80Adr & 0x3fff) + 8192] = gZ80Data;
                    } else {
                    // normal memory write
                        gDfRam[gZ80Adr & 0x3fff] = gZ80Data;
                    }
                }
                if (((gTsRegF4 & 0x08) == 0) && !MB_OWNED(mbOwn3) && ((gZ80Adr & 0x2000) != 0)) gDfRam[gZ80Adr & 0x3fff] = gZ80Data;
            // only a video memory read can change with a screen write. Keep
            //  this short: a PUSH writes twice, about 850 ns apart.
                if (gUlaAdrReg == 0x48) gDatAnswer = gDfRam[(gfontIndex < DF_RAM_SIZE) ? gfontIndex : 0];
                goto endoRead;
            }
        }


endoFunc:
    // an I/O cycle may have changed what the next read of the data port
    //  returns. The Z80 holds IORQ for about 700 ns, so this is done before it
    //  ends.
        gDatAnswer = datAnswer();

endoRead:

    // busMain never returns, so without this the compiler may keep globals in
    //  registers and drop the stores that only core 1 reads (gBorder, gVidMode,
    //  gUlaEna). tests/check_arm.py checks the linked code for this.
        __compiler_memory_barrier();

    // spin while the PSEL line is high. We MUST complete all bus operations
    //  prior to PSEL going inactive so there is no chance a race condition
    //  will cause problems.
        gpio_put_masked(PG_MASK, ADRL);     // set up for the next round
        while (gpio_get(PSEL));
    }
}


