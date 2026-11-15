# 80-column text routines (firmware 4.2+)

These are helper routines for PicoVideo's 80×24 text mode (video mode 5).
See [../../docs/features.md](../../docs/features.md) for the register map.

| Path | Contents |
|---|---|
| `z88dk/text80Rtns.c`, `.h` | C library: clear the screen, position the cursor, print characters and strings, set attributes and load the font. |
| `z88dk/text80Test.c` | A demo that draws an "SD Card Commander"-style screen. |
| `z88dk/text80.tap` | The demo, built and ready to `LOAD ""` on a TS2068 or in an emulator. |
| `asm/text80Rtns.asm`, `.inc` | The same routines in Z80 assembler, for zmac. **Not yet tested.** |
| `asm/text80Rtns.tap` | The assembled routines. |

To build the demo:

```bash
zcc +ts2068 -create-app text80Rtns.c text80Test.c
```

The assembler `_ASSEMBLE.BAT` points at zmac on the author's machine. Edit
the path before you use it.
