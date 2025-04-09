This version of the Pico code corrects rogue pixels when using Atmel PLDs

# Upgrading the firmware in your PicoVideo

Updated: April 8, 2024

You will need a USB cable with a Micro B connector. This is not an endorsement, just an example of the kind of cable that will 
work for most folks. It has USB-A on the side to plug into the computer and a Micro B USB to plug into the Pico.

First, [download the updated UF2 file to your computer](https://github.com/timex-sinclair-projects/PicoVideo/blob/main/V4/picoTsVid.uf2).

This video shows how to connect and update the firmware on your PicoVideo:
https://www.youtube.com/watch?v=os4mv_8jWfU

For the sake completeness, here are the same instructions but tailored just for the PicoVideo:

1. Remove your PicoVideo from the expansion card.
2. Plug the cable into your computer.
3. Press and hold the white button on the Pico labeled "BOOTSEL".
4. Plug the Micro B end into the Pico chip.
5. If you don't have a file finder open on your computer, open one now.
6. The Pico will show up on the computer as RPI-PI2 or similar.
7. Drag the picoTsVid.uf2 from your computer to the RPI-PI2 drive.
8. After a few seconds, the Pico will disconnect. If you have a Mac, it'll likely complain about the disconnect.
9. Plug your PicoVideo board back into your 2068 expansion slot.
10. Power up your 2068: the video should show up just fine with no glitches.
