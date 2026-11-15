# PicoVideo hardware

## Current board: PicoVideo SMD (EasyEDA Pro)

| Path | Contents |
|---|---|
| `easyeda/PicoVideo-SMD.epro2` | Full EasyEDA Pro project: three schematic sheets plus the PCB. |
| `easyeda/BOM.csv` | Parts list, generated from the PCB (41 parts). |

Exported from EasyEDA Pro 3.2.209 on 15 Nov 2026. Open the files in EasyEDA
Pro. Main parts:

- a Raspberry Pi Pico (U5)
- a GAL16V8D for bus decode (U7)
- three 74LVC245 bus buffers (U2–U4)
- a 74LV125 (U6)
- a VGA connector (DSUB1), driven through resistor networks

## Earlier board (Altium)

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
design and not the V3 board in `fab/`.

The bus-decode PLD is in [`../pld`](../pld/).
