/**
 * Hunter Adams (vha3@cornell.edu)
 * Jeff Burrell
 * 
 * VGA driver using PIO assembler
 *
 * HARDWARE CONNECTIONS
 *  - GPIO 8        ---> VGA Hsync
 *  - GPIO 9        ---> VGA Vsync
 *  - GPIO 7..10    ---> VGA Red
 *  - GPIO 4..6     ---> VGA Green
 *  - GPIO 0..1     ---> VGA Blue
 *  - RP2040 GND    ---> VGA GND
 *
 * RESOURCES USED
 *  - PIO state machines 0, 1, and 2 on PIO instance 0
 *  - DMA channels 0 and 1
 *  - Memory
 *      DF0         -- 6912 bytes
 *      DF1         -- 6912 bytes
 *      line buf0   -- 640 bytes
 *      line buf1   -- 640 bytes
 * 
 * HOW TO USE THIS CODE
 * This code outputs TS2068 compatible video to the VGA port
 *  TS2068 video is a superset of Sinclair Spectrum video
 *  and this module provides full support as well as enhancements.
 * 
 * One of the PIO state machines produces the HSync signal. This PIO
 * is free running and starts immediately when the state machine
 * is enabled. The second state machine produces the VSync signal
 * and is triggered by the HSync state machine. The third state
 * machine reads raw color data from one of two line buffers and
 * outputs 8 bit color as RRRGGGBB to the VGA color output pins.
 * 
 *
 */
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/sync.h"
#include "string.h"
// Our assembled PIO programs:
#include "vgaPioSm.pio.h"
#include "../macros.h"
#include "../video_decode.h"
#include "../globals.h"

void doTextMode(void);

#define BUF_IDLE 0
#define BUF1_FREE 1
#define BUF0_FREE 2

void testFillDf0(void);
void testLines(void);

void __not_in_flash_func(pixDmaHandler)(void);
void topBottBorder(unsigned char *bordBuf);
void __not_in_flash_func(waitForVsync)(void);
void __not_in_flash_func(makeScanLine)(unsigned char *outBuf, unsigned int lineNr);


// DMA channels - 0 sends color data, 1 reconfigures and restarts 0
int rgb_chan_0  = 0;
int rgb_chan_1  = 1;

int fillCmd     = 0;
volatile int doLineFlag = BUF1_FREE;



// VGA timing constants for the PIO FSMs
#define H_ACTIVE   655    // (active + frontporch - 1) - one cycle delay for mov
#define V_ACTIVE   479    // (active - 1)
#define RGB_ACTIVE 320    // (horizontal active)/2 - 1
#define TXCOUNT 321

// Pixel color array that is DMA's to the PIO machines and
// a pointer to the ADDRESS of this color array.
// Note that this array is automatically initialized to all 0's (black)
// This variable is initialized to point to the first line buffer
char *lineBuf = (char *)&gLbuf0[0];

// video state machine position, advanced once per free line buffer
static unsigned int vidState = 0;
static unsigned int lineCtr  = 0;

static void __not_in_flash_func(videoLine)(int flag);



void __not_in_flash_func(corevMain)(void) {
unsigned int i;

// These locations are initialized to set the color value
//  during the inactive video time. This MUST be done due to
//  how the PIO state machine works.
   gLbuf0[640] = 0;
   gLbuf0[641] = 0;
   gLbuf1[640] = 0;
   gLbuf1[641] = 0;


    // Choose which PIO instance to use (there are two instances, each with 4 state machines)
    PIO pio = pio0;

    // Our assembled program needs to be loaded into this PIO's instruction
    // memory. This SDK function will find a location (offset) in the
    // instruction memory where there is enough space for our program. We need
    // to remember these locations!
    //
    // We only have 32 instructions to spend! If the PIO programs contain more than
    // 32 instructions, then an error message will get thrown at these lines of code.
    //
    // The program name comes from the .program part of the pio file
    // and is of the form <program name_program>
    uint hsync_offset = pio_add_program(pio, &hsync_program);
    uint vsync_offset = pio_add_program(pio, &vsync_program);
    uint rgb_offset = pio_add_program(pio, &rgb_program);

    // Manually select a few state machines from pio instance pio0.
    uint hsync_sm = 0;
    uint vsync_sm = 1;
    uint rgb_sm = 2;

    // Call the initialization functions that are defined within each PIO file.
    // Why not create these programs here? By putting the initialization function in
    // the pio file, then all information about how to use/setup that state machine
    // is consolidated in one place. Here in the C, we then just import and use it.
    hsync_program_init(pio, hsync_sm, hsync_offset);
    vsync_program_init(pio, vsync_sm, vsync_offset);
    rgb_program_init(pio, rgb_sm, rgb_offset);


    ///////////////////////////////////////////////////////////////////////////////////////////////////
//  ===========================-== DMA Data Channels =================================================
    ///////////////////////////////////////////////////////////////////////////////////////////////////


    // Channel Zero (sends color data to PIO VGA machine)
    dma_channel_config c0 = dma_channel_get_default_config(rgb_chan_0);  // default configs
    channel_config_set_transfer_data_size(&c0, DMA_SIZE_16);             // 16-bit txfers
    channel_config_set_read_increment(&c0, true);                        // yes read incrementing
    channel_config_set_write_increment(&c0, false);                      // no write incrementing
    channel_config_set_dreq(&c0, DREQ_PIO0_TX2) ;                        // DREQ_PIO0_TX2 pacing (FIFO)
    channel_config_set_chain_to(&c0, rgb_chan_1);                        // chain to other channel

    dma_channel_configure(
        rgb_chan_0,                 // Channel to be configured
        &c0,                        // The configuration we just created
        &pio->txf[rgb_sm],          // write address (RGB PIO TX FIFO)
        &gLbuf0[0],                 // The initial read address (pixel color array)
        TXCOUNT,                    // Number of transfers; in this case each is 2 bytes.
        false                       // Don't start immediately.
    );

    // Channel One (reconfigures the first channel)
    dma_channel_config c1 = dma_channel_get_default_config(rgb_chan_1);   // default configs
    channel_config_set_transfer_data_size(&c1, DMA_SIZE_32);              // 32-bit txfers
    channel_config_set_read_increment(&c1, false);                        // no read incrementing
    channel_config_set_write_increment(&c1, false);                       // no write incrementing
    channel_config_set_chain_to(&c1, rgb_chan_0);                         // chain to other channel

    dma_channel_configure(
        rgb_chan_1,                         // Channel to be configured
        &c1,                                // The configuration we just created
        &dma_hw->ch[rgb_chan_0].read_addr,  // Write address (channel 0 read address)
        &lineBuf,                           // Read address (POINTER TO AN ADDRESS)
        1,                                  // Number of transfers, in this case each is 4 byte
        false                               // Don't start immediately.
    );

    /////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////

    // Initialize PIO state machine counters. This passes the information to the state machines
    // that they retrieve in the first 'pull' instructions, before the .wrap_target directive
    // in the assembly. Each uses these values to initialize some counting registers.
    pio_sm_put_blocking(pio, hsync_sm, H_ACTIVE);
    pio_sm_put_blocking(pio, vsync_sm, V_ACTIVE);
    pio_sm_put_blocking(pio, rgb_sm, RGB_ACTIVE);


    // Start the two pio machine IN SYNC
    // Note that the RGB state machine is running at full speed,
    // so synchronization doesn't matter for that one. But, we'll
    // start them all simultaneously anyway.
    pio_enable_sm_mask_in_sync(pio, ((1u << hsync_sm) | (1u << vsync_sm) | (1u << rgb_sm)));

    // Start DMA channel 0. Once started, the contents of the pixel color array
    // will be continously DMA'ed to the PIO machines that are driving the screen.
    // To change the contents of the screen, we need only change the contents
    // of that array.
    dma_start_channel_mask((1u << rgb_chan_0)) ;

// Tell the DMA to raise IRQ line 0 when the channel finishes a block
    dma_channel_set_irq0_enabled(rgb_chan_1, true);
// Configure the processor to run dma_handler() when DMA IRQ 0 is asserted
    irq_set_exclusive_handler(DMA_IRQ_0, pixDmaHandler);
    irq_set_enabled(DMA_IRQ_0, true);



// Wait until we know we won't contend with the doLineFlag
    waitForVsync();

// Force the FSM to run
    doLineFlag              = BUF1_FREE;

    while(true){
    // read the flag once: the DMA ISR can change it at any moment
        int flag = doLineFlag;
        if (flag != BUF_IDLE) videoLine(flag);
    }
}


// Fill the line buffer the ISR reported free (flag) with the next line of
//  the frame, then mark that report handled.
static void __not_in_flash_func(videoLine)(int flag){
unsigned char *buf = (flag == BUF1_FREE) ? &gLbuf1[0] : &gLbuf0[0];
uint32_t irqState;

    switch (vidState){
    case 0:
    // spin until we know we are not in the active video time
        waitForVsync();
        lineCtr     = 0;
    // Due to the way the PIO FSMs work, we know that there will be
    //  30-ish line periods available for this initialization.
    // Discard any report that arrived while waiting and start the
    //  frame on the next interrupt.
        doLineFlag  = BUF_IDLE;
        vidState    = 1;
        return;
    case 1:
    // This state handles the top border period
        if (lineCtr < 22){
            lineCtr++;
            topBottBorder(buf);
        } else {
            lineCtr     = 0;
            vidState    = 2;
        }
        break;

    case 2:
    // output the bit map data
        if (lineCtr < 192){
            makeScanLine(buf, lineCtr);
            lineCtr++;
        } else {
            topBottBorder(buf);
            lineCtr     = 0;
            vidState    = 3;
        }
        break;

    case 3:
    // This state handles the bottom border period
        if (lineCtr < 8){
            topBottBorder(buf);
            lineCtr++;
        } else {
            lineCtr = 0;
            vidState = 0;
        }
        break;
    }

// Clear the report only if the ISR has not posted a newer one meanwhile;
//  otherwise that line would be skipped and the frame would shift.
    irqState = save_and_disable_interrupts();
    if (doLineFlag == flag) doLineFlag = BUF_IDLE;
    restore_interrupts(irqState);
}



// Spin here until we detect a falling edge on the VSync line
void __not_in_flash_func(waitForVsync)(void){
// spin until VSync is high
    while(!gpio_get(9)){
    };
// spin while VSync is high
    while(gpio_get(9)){
    };

    pv_flash_frame(&gFlashCtr, &gFlashMask);
}


// This function handles the interrupt from DMA channel #1
//  The function is called after DMA #1 has updated the
//  buffer address in DMA #0 controller. This function assumes
//  that lineBuf has been initlaized to &buf0[0].
//
unsigned char dmaState = 0;
unsigned int dmaLineCtr = 0;
void __not_in_flash_func(pixDmaHandler)(void){
// Clear the interrupt request.
    dma_hw->ints0 = (1u << rgb_chan_1);

    switch (dmaState){
    case 0:
    // the first line has been sent and the
    //  DMA controller has been reloaded so
    //  update the buffer pointer.
        lineBuf   = &gLbuf1[0];
        doLineFlag = BUF_IDLE;
        dmaState++;
        break;
    case 1:
        dmaState++;
    // set a global flag telling the system buffer 0 is free.
        doLineFlag = BUF0_FREE;
        break;
    case 2:
        lineBuf   = &gLbuf0[0];
        doLineFlag = BUF_IDLE;
        dmaState++;
        break;
    case 3:
    // set a global flag telling the system buffer 1 is free.
        doLineFlag = BUF1_FREE;
        dmaState = 0;
        break;
    }
}


