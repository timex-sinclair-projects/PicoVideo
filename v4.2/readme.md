This zip file contains the latest version of the Pico Video,
version 4.2. Here is a brief description of the folders
and files:

<dox>
This folder contains file that describes the added
functions in this version of the Pico Video.

<picoCode>
This folder contains the complete build environment
that defines and produces the executable. The
environment works under Visual Studio Code and
uses the Raspberry Pi Pico SDK.

<testCode\text80\asm>
Assembler routines that provide low level access
to the 80 column display mode. These routines
have assembled but, as of 25Sep2025, they have
not been tested.

<testCode\text80\z88dk>
C routines that can be compiled using z88dk that
demonstrate use of the 80 column routines. I may
better integrate them as z88dk libraries. The
text80.tap file is a demonstration program that
can be loaded and run to show some of the
capabilities of the 80 column mode.

picoTsVid.uf2
This is a copy of the uf2 in the <picoCode\build>
directory. It is placed here so you do not need
to root around the file to find it.

readme.txt
This file.



