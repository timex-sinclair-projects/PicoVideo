10 REM Speccy Black: turn bright black off and on
20 BORDER 0: PAPER 0: INK 7: CLS
30 PRINT AT 1,2;"BRIGHT 0";AT 1,18;"BRIGHT 1"
40 FOR r=3 TO 14
50 PRINT AT r,2; PAPER 0; BRIGHT 0;"            ";AT r,18; PAPER 0; BRIGHT 1;"            "
60 NEXT r
70 LET s=0
80 OUT 48955,66: OUT 65339,s
90 PRINT AT 17,2;"Speccy Black is ";("off" AND s=0);("on " AND s=1)
100 PRINT AT 19,2;"Press a key to switch it"
110 PAUSE 0: LET s=1-s: GO TO 80
