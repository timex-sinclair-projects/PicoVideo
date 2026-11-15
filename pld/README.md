# Bus-decode PLD

This is the WinCUPL design for the GAL16V8 (`g16v8a`) on the PicoVideo board.
It is by J. Burrell, revision 01, dated 12 Sep 2023, and has been unchanged
since V2.0.

| File | Contents |
|---|---|
| `PICOVID.PLD` | CUPL source. |
| `PICOVID.jed` | JEDEC fuse map for programming the chip. |
| `PICOVID.sim`, `PICOVID.abs` | Simulation input and the compiled object. |
| `PICOVID.pdf` | Printout of the source. |

Production boards use Atmel ATF16V8 parts, which are faster than GALs.
Firmware V4 adds a bus-read delay to account for that speed (see
`firmware/coreB/busMain.c`).

`PICOVID.jed` is stored byte-exact (see `.gitattributes`). Its checksum
covers the CR/LF line endings, so don't let an editor resave it.
