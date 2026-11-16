10 REM Detect PicoVideo and read its firmware version
20 OUT 48955,127
30 IF IN 48955<>127 THEN PRINT "No PicoVideo found": STOP
40 LET v=IN 65339
50 PRINT "PicoVideo firmware ";INT (v/16);".";v-16*INT (v/16)
60 IF v<66 THEN PRINT "Registers 66-72 need 4.2 or later"
