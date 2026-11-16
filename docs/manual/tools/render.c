/* Host renderer: runs the firmware's own makeScanLine() over a memory dump.
 * usage: render dump.bin out.raw mode border ulaena speccyblack ff [pal64.bin] [bicolor4hex]
 * dump.bin is 16384 bytes of TS2068 memory from 0x4000. out.raw is 640x192 bytes. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "macros.h"
#include "video_decode.h"
#include "globals.h"
void makeScanLine(unsigned char *, unsigned int);
bool gpio_get(unsigned int p){(void)p;return 0;}
int main(int argc,char**argv){
  if(argc<8){fprintf(stderr,"usage\n");return 2;}
  FILE*f=fopen(argv[1],"rb"); if(!f) return 1; fread(gDfRam,1,16384,f); fclose(f);
  gVidMode=strtol(argv[3],0,0); unsigned bord=strtol(argv[4],0,0);
  gUlaEna=strtol(argv[5],0,0); speccyBlack=strtol(argv[6],0,0);
  gTsRegFF=strtol(argv[7],0,0); gTsCol64Ink=pv_64_attribute(gTsRegFF);
  if(argc>8 && strcmp(argv[8],"-")){f=fopen(argv[8],"rb"); fread(gUlaColors,1,64,f); fclose(f);}
  if(argc>9){unsigned v=strtoul(argv[9],0,16); for(int i=0;i<4;i++) biColor[i]=(v>>(24-8*i))&255;}
  gBorder = gUlaEna ? gUlaColors[pv_ula_border(bord)] : borderColors[bord&7];
  FILE*o=fopen(argv[2],"wb"); unsigned char buf[LINE_BUF_SIZE];
  for(unsigned l=0;l<192;l++){makeScanLine(buf,l); fwrite(buf,1,640,o);}
  fclose(o); return 0;}
