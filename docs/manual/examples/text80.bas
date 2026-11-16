10 REM 80-column text from BASIC (mode 5)
20 BORDER 1: CLS
30 OUT 255,5: REM PicoVideo shows 80 x 24 text
40 LET t=1*8+7: REM attribute: PAPER 1, INK 7
50 FOR i=18304 TO 20223: POKE i,t: NEXT i
60 LET t=1*8+6: LET r=1: LET c=25: LET a$="80 columns from TS2068 BASIC": GO SUB 200
70 LET t=5*8+0: LET r=3: LET c=2: LET a$=" Codes at 16384+80*row+col ": GO SUB 200
80 LET t=6*8+0: LET c=40: LET a$=" Attributes 1920 bytes later ": GO SUB 200
90 LET t=1*8+7: FOR n=0 TO 255
100 LET r=6+INT (n/32): LET c=8+2*(n-32*INT (n/32))
110 POKE 16384+80*r+c,n: POKE 18304+80*r+c,t
120 NEXT n
130 LET t=1*8+5: LET r=16: LET c=8: LET a$="The CP437 font gives box drawing, blocks and accents:": GO SUB 200
140 LET r=18: LET c=8: POKE 16384+80*r+c,201: FOR i=1 TO 20: POKE 16384+80*r+c+i,205: NEXT i: POKE 16384+80*r+c+21,187
150 LET c=8: LET r=19: POKE 16384+80*r+c,186: POKE 16384+80*r+c+21,186: LET c=10: LET a$="Caf"+CHR$ 130+" "+CHR$ 177+CHR$ 178+CHR$ 219+" "+CHR$ 3+CHR$ 14+CHR$ 15: GO SUB 200
160 LET r=20: LET c=8: POKE 16384+80*r+c,200: FOR i=1 TO 20: POKE 16384+80*r+c+i,205: NEXT i: POKE 16384+80*r+c+21,188
170 PAUSE 0
180 CLS: OUT 255,0: STOP
200 REM print a$ at row r, column c in attribute t
210 LET a=16384+80*r+c
220 FOR i=1 TO LEN a$
230 POKE a,CODE a$(i): POKE a+1920,t: LET a=a+1
240 NEXT i: RETURN
