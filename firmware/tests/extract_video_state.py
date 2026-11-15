"""Test actual controller/ISR bodies without stubbing the entire Pico SDK.

Only platform initialization is omitted. The switch and interrupt bodies
are extracted verbatim, so the tests do not reimplement their behavior.
"""
from pathlib import Path

HERE = Path(__file__).resolve().parent
SOURCE = HERE.parent/'coreV/vidMain.c'
text = SOURCE.read_text()


def block(start):
    begin = text.index('{',start)
    depth = 1
    end = begin+1
    while depth:
        depth += (text[end]=='{')-(text[end]=='}')
        end += 1
    return text[begin:end]


line_sig = 'static void __not_in_flash_func(videoLine)(int flag)'
line = line_sig+block(text.index(line_sig+'{'))
irq_sig = 'void __not_in_flash_func(pixDmaHandler)(void)'
irq = irq_sig+block(text.index(irq_sig+'{'))
assert 'volatile int doLineFlag' in text
program = r'''
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "macros.h"
#define __not_in_flash_func(name) name
#define BUF_IDLE 0
#define BUF1_FREE 1
#define BUF0_FREE 2
extern unsigned char gLbuf0[642],gLbuf1[642],gBorder,gVidMode;
void makeScanLine(unsigned char *,unsigned int);
void topBottBorder(unsigned char *);
void waitForVsync(void) {}
volatile int doLineFlag;
static unsigned int vidState,lineCtr;
static uint32_t save_and_disable_interrupts(void) { return 0; }
static void restore_interrupts(uint32_t state) { (void)state; }
unsigned int dmaState;
unsigned int rgb_chan_1=1;
char *lineBuf;
struct { unsigned int ints0; } registers;
#define dma_hw (&registers)
LINE
void step(void) { videoLine(doLineFlag); }
IRQ
int main(void) {
    unsigned char before0[642],before1[642];
    gBorder=0x5a; gVidMode=0;
    for(unsigned int free=1;free<=2;++free)
    for(unsigned int test=0;test<4;++test) {
        memset(gLbuf0,0xa1,642); memset(gLbuf1,0xb2,642);
        gLbuf0[640]=gLbuf0[641]=gLbuf1[640]=gLbuf1[641]=0;
        memcpy(before0,gLbuf0,642); memcpy(before1,gLbuf1,642);
        doLineFlag=free;
        vidState=test==0?1:test==3?3:2;
        lineCtr=test==2?192:test==1?191:0;
        step();
        if(free==BUF1_FREE) assert(memcmp(before0,gLbuf0,642)==0);
        else assert(memcmp(before1,gLbuf1,642)==0);
        unsigned char *filled=free==BUF1_FREE?gLbuf1:gLbuf0;
        assert(filled[0]==0x5a && filled[639]==0x5a);
        assert(filled[640]==0 && filled[641]==0);
        assert(doLineFlag==BUF_IDLE);
    }
    unsigned int expected[]={BUF_IDLE,BUF0_FREE,BUF_IDLE,BUF1_FREE};
    dmaState=0;
    for(unsigned int i=0;i<12;++i) {
        pixDmaHandler();
        assert(doLineFlag==expected[i%4]);
        assert(lineBuf==(char *)((i%4)<2?gLbuf1:gLbuf0));
        assert(registers.ints0==2);
    }
    /* A newer free-buffer report posted while a line is drawn must survive. */
    vidState=2; lineCtr=0; doLineFlag=BUF0_FREE;
    videoLine(BUF1_FREE);
    assert(doLineFlag==BUF0_FREE);
    puts("PASS: actual video-state/IRQ bodies preserve busy buffers, blanking guards, double-line ownership and late ISR reports.");
    return 0;
}
'''.replace('LINE\n',line+'\n').replace('IRQ\n',irq+'\n')
out = HERE/'build'
out.mkdir(exist_ok=True)
(out/'video_state_regression.c').write_text(program)
