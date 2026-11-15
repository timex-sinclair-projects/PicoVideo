
zcc +ts2068 -create-app text80Rtns.c text80Test.c
zcc +ts2068 -a text80Rtns.c text80Test.c

echo off
del text80.tap
del a.bin
ren a.tap text80.tap

pause

