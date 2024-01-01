/**
 * Hunter Adams (vha3@cornell.edu)
 * 
 * VGA driver using PIO assembler
 *
 * HARDWARE CONNECTIONS
 *  - GPIO 16 ---> VGA Hsync
 *  - GPIO 17 ---> VGA Vsync
 *  - GPIO 18 ---> 330 ohm resistor ---> VGA Red
 *  - GPIO 19 ---> 330 ohm resistor ---> VGA Green
 *  - GPIO 20 ---> 330 ohm resistor ---> VGA Blue
 *  - RP2040 GND ---> VGA GND
 *
 * RESOURCES USED
 *  - PIO state machines 0, 1, and 2 on PIO instance 0
 *  - DMA channels 0 and 1
 *  - 153.6 kBytes of RAM (for pixel color data)
 *
 * HOW TO USE THIS CODE
 *  This code uses one DMA channel to send pixel data to a PIO state machine
 *  that is driving the VGA display, and a second DMA channel to reconfigure
 *  and restart the first. As such, changing any value in the pixel color
 *  array will be automatically reflected on the VGA display screen.
 *  
 *  To help with this, I have included a function called drawPixel which takes,
 *  as arguments, a VGA x-coordinate (int), a VGA y-coordinate (int), and a
 *  pixel color (char). Only 3 bits are used for RGB, so there are only 8 possible
 *  colors. If you keep all of the code above line 200, this interface will work.
 *
 */
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
// Our assembled programs:
// Each gets the name <pio_filename.pio.h>
#include "vgaPioSm.pio.h"
#include "macros.h"

// video memory
extern unsigned char gDfRam[16384];
extern unsigned char gBorder;
extern unsigned char gVidMode;
extern unsigned short bmapOffset[192];


void corevMain(void);
void busMain();


int main() {

// set the system clock to 250MHz
    set_sys_clock_khz(250000, true);

// Initialize stdio
    stdio_init_all();

    gpio_init(25);
    gpio_set_dir(25, GPIO_OUT);
    gpio_put(25, 0);



    gpio_init(11);
    gpio_set_dir(11, GPIO_OUT);
    gpio_put(11, 0);

// start the video code on core #1
    multicore_launch_core1(corevMain);
// run the bus interface on core #0
    busMain();
}