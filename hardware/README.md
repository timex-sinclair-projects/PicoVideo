# PicoVideo hardware

| Path | Contents |
|---|---|
| `altium/` | Altium Designer project (`tsPicoVid.PrjPcb`): top sheet, TS2068 interface, logic, Pico and peripheral sheets, and the PCB (`tsPicoVid.CSPCBDoc`). |
| `fab/Gerber/`, `fab/NC Drill/` | Fabrication outputs for the V3 board, generated 15 Jan 2024. |
| `fab/BOM/` | Bill of materials. |
| `fab/tsPicoVid.PDF` | Schematic as a PDF. |

The V2 board's fab outputs are in the history. Check out the commit titled
"Import V2.0 firmware, PLD and board files from zips".

**Note:** the V2.0 and V3.0 RC1 releases contained byte-identical Altium
zips, but their Gerbers differ. So the Altium source here may be the V2
design and not the V3 board in `fab/`. Confirm this before you change and
re-fab the board.

The bus-decode PLD is in [`../pld`](../pld/).
