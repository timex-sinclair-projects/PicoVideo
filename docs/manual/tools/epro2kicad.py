#!/usr/bin/env python3
"""Convert the EasyEDA Pro PicoVideo board (.epro2) into a .kicad_pcb that is
good enough to render in 3D with `kicad-cli pcb render`.

It carries over the board outline, copper (tracks, vias, pads, the hatched
pours), silkscreen, and puts KiCad library 3D models on the parts. The model
for each part is placed by fitting the matching KiCad footprint's pads onto
the EasyEDA pads, so it lines up whatever origin and rotation each library
uses. It is a rendering aid, not a design conversion: there are no nets.

usage: epro2kicad.py board.epro2 out.kicad_pcb
"""
import json, math, re, sys, zipfile
from pathlib import Path

KI = Path('/Applications/KiCad/KiCad.app/Contents/SharedSupport')
MIL = 0.0254

# EasyEDA footprint title -> (KiCad footprint library/name, 3D model, optional z lift)
MODELS = {
    'R1206': 'Resistor_SMD/R_1206_3216Metric',
    'R0805': 'Resistor_SMD/R_0805_2012Metric',
    'R0603': 'Resistor_SMD/R_0603_1608Metric',
    'C0805': 'Capacitor_SMD/C_0805_2012Metric',
    'C0603': 'Capacitor_SMD/C_0603_1608Metric',
    'CAP-SMD_BD6.3-L6.6-W6.6-FD': 'Capacitor_SMD/CP_Elec_6.3x7.7',
    'DO-15_BD3.1-L6.7-P10.70-D0.9-RD': 'Diode_THT/D_DO-15_P10.16mm_Horizontal',
    'TSSOP-20_L6.5-W4.4-P0.65-LS6.4-BL': 'Package_SO/TSSOP-20_4.4x6.5mm_P0.65mm',
    'TSSOP-14_L5.0-W4.4-P0.65-LS6.4-BL': 'Package_SO/TSSOP-14_4.4x5mm_P0.65mm',
    'DIP-20_L25.5-W10.0-P2.54-LS7.6-BL': 'Package_DIP/DIP-20_W7.62mm',
    'RES-ARRAY-SMD_0603-8P-L3.2-W1.6-BL': 'Resistor_SMD/R_Array_Convex_4x0603',
    'PICO-PKG-SMD': 'Module/RaspberryPi_Pico_SMD',
    'VGA-002': ['Connector_Dsub/DSUB-15-HD_Socket_Horizontal_P2.29x1.90mm_EdgePinOffset3.03mm_Housed_MountingHolesOffset4.94mm',
                'Connector_Dsub/DSUB-15-HD_Socket_Horizontal_P2.29x2.54mm_EdgePinOffset8.35mm_Housed_MountingHolesOffset10.89mm'],
    'AUDIO-SMD_PJ-320B_C2884998': None,   # KiCad has no PJ-320B: models/PJ-320B.wrl
}


def load(path):
    z = zipfile.ZipFile(path)
    name = [n for n in z.namelist() if n.endswith('.epru')][0]
    docs, cur = [], None
    for line in z.read(name).decode().split('\n'):
        parts = line.split('||')
        if len(parts) < 2:
            continue
        h = json.loads(parts[0]); b = json.loads(parts[1].rstrip('|'))
        if h['type'] == 'DOCHEAD':
            cur = {'type': b['docType'], 'uuid': b['uuid'], 'items': []}
            docs.append(cur)
        cur['items'].append((h, b))
    return docs


def path_points(path, seg=12):
    """EasyEDA path [x,y,'L',x,y,...,'ARC',angle,x,y,...] -> list of (x, y) in mil."""
    pts, i, mode = [], 0, 'L'
    while i < len(path):
        v = path[i]
        if isinstance(v, str):
            mode = v; i += 1; continue
        if mode == 'ARC':
            ang, ex, ey = path[i], path[i + 1], path[i + 2]; i += 3
            sx, sy = pts[-1]
            pts += arc_points(sx, sy, ex, ey, ang, seg)
        else:
            pts.append((path[i], path[i + 1])); i += 2
    return pts


def arc_points(sx, sy, ex, ey, ang, seg):
    a = math.radians(ang)
    if abs(a) < 1e-6:
        return [(ex, ey)]
    chord = math.hypot(ex - sx, ey - sy)
    r = chord / (2 * math.sin(abs(a) / 2))
    mx, my = (sx + ex) / 2, (sy + ey) / 2
    d = math.sqrt(max(r * r - (chord / 2) ** 2, 0))
    ux, uy = (ex - sx) / chord, (ey - sy) / chord
    s = 1 if a > 0 else -1
    cx, cy = mx - s * uy * d, my + s * ux * d
    a0 = math.atan2(sy - cy, sx - cx)
    return [(cx + r * math.cos(a0 + a * k / seg), cy + r * math.sin(a0 + a * k / seg)) for k in range(1, seg + 1)]


def mm(x, y):
    return x * MIL, -y * MIL


def kicad_pads(fp):
    lib, name = fp.split('/')
    txt = (KI / 'footprints' / f'{lib}.pretty' / f'{name}.kicad_mod').read_text()
    pads = {}
    for m in re.finditer(r'\(pad\s+"([^"]*)"\s+\S+\s+\S+\s*\(at\s+([-\d.]+)\s+([-\d.]+)', txt):
        pads.setdefault(m.group(1), (float(m.group(2)), float(m.group(3))))
    num = r'\s+([-\d.]+)'
    mod = re.search(r'\(model\s+"?([^"\s)]+)"?\s*\(offset\s*\(xyz' + num * 3 + r'\s*\)\s*\)\s*\(scale\s*\(xyz[^)]*\)\s*\)\s*\(rotate\s*\(xyz' + num * 3, txt)
    model = None
    if mod:
        p = mod.group(1).replace('${KICAD10_3DMODEL_DIR}', str(KI / '3dmodels')).replace('${KICAD9_3DMODEL_DIR}', str(KI / '3dmodels')).replace('${KICAD8_3DMODEL_DIR}', str(KI / '3dmodels'))
        p = re.sub(r'\$\{KICAD\d+_3DMODEL_DIR\}', str(KI / '3dmodels'), p)
        if p.endswith('.wrl'):
            p = p[:-4] + '.step'
        model = (p, [float(mod.group(k)) for k in (2, 3, 4)], [float(mod.group(k)) for k in (5, 6, 7)])
    return pads, model


def rot(x, y, deg):
    """Rotate a y-down point by `deg` counter-clockwise as seen from above."""
    a = math.radians(deg)
    return x * math.cos(a) + y * math.sin(a), -x * math.sin(a) + y * math.cos(a)


def fit(kpads, epads):
    common = [n for n in kpads if n in epads]
    if len(common) < 2:
        return None
    best = None
    for th in (0, 90, 180, 270):
        rk = [rot(*kpads[n], th) for n in common]
        tx = sum(epads[n][0] - r[0] for n, r in zip(common, rk)) / len(common)
        ty = sum(epads[n][1] - r[1] for n, r in zip(common, rk)) / len(common)
        res = sum(math.hypot(r[0] + tx - epads[n][0], r[1] + ty - epads[n][1]) for n, r in zip(common, rk)) / len(common)
        if best is None or res < best[0]:
            best = (res, th, tx, ty)
    return best


def main(src, dst):
    docs = load(src)
    fps = {d['uuid']: d for d in docs if d['type'] == 'FOOTPRINT'}
    for d in fps.values():
        d['title'] = [b['title'] for h, b in d['items'] if h['type'] == 'META'][0]
    pcb = [d for d in docs if d['type'] == 'PCB'][0]
    out = []
    w = out.append
    w('(kicad_pcb (version 20240108) (generator "epro2kicad")')
    w(' (general (thickness 1.6))')
    w(' (paper "A4")')
    w(''' (layers (0 "F.Cu" signal) (31 "B.Cu" signal) (36 "B.SilkS" user) (37 "F.SilkS" user)
  (38 "B.Mask" user) (39 "F.Mask" user) (34 "B.Paste" user) (35 "F.Paste" user) (44 "Edge.Cuts" user))''')
    w(''' (setup (stackup
   (layer "F.SilkS" (type "Top Silk Screen") (color "White"))
   (layer "F.Paste" (type "Top Solder Paste"))
   (layer "F.Mask" (type "Top Solder Mask") (color "Blue") (thickness 0.01))
   (layer "F.Cu" (type "copper") (thickness 0.035))
   (layer "dielectric 1" (type "core") (thickness 1.51) (material "FR4") (epsilon_r 4.5) (loss_tangent 0.02))
   (layer "B.Cu" (type "copper") (thickness 0.035))
   (layer "B.Mask" (type "Bottom Solder Mask") (color "Blue") (thickness 0.01))
   (layer "B.Paste" (type "Bottom Solder Paste"))
   (layer "B.SilkS" (type "Bottom Silk Screen") (color "White"))
   (copper_finish "ENIG") (dielectric_constraints no))
  (pad_to_mask_clearance 0))''')
    w(' (net 0 "")')
    CU = {1: 'F.Cu', 2: 'B.Cu'}

    comps = {}
    for h, b in pcb['items']:
        t = h['type']
        if t == 'COMPONENT':
            comps[h['id']] = {'c': b}
        elif t == 'ATTR' and b.get('parentId') in comps:
            comps[b['parentId']][b['key']] = b
        elif t == 'LINE' and b['layerId'] in CU:
            (x1, y1), (x2, y2) = mm(b['startX'], b['startY']), mm(b['endX'], b['endY'])
            w(f' (segment (start {x1:.4f} {y1:.4f}) (end {x2:.4f} {y2:.4f}) (width {b["width"] * MIL:.4f}) (layer "{CU[b["layerId"]]}") (net 0))')
        elif t == 'VIA':
            x, y = mm(b['centerX'], b['centerY'])
            w(f' (via (at {x:.4f} {y:.4f}) (size {b["viaDiameter"] * MIL:.4f}) (drill {b["holeDiameter"] * MIL:.4f}) (layers "F.Cu" "B.Cu") (net 0))')
        elif t == 'POLY' and b['layerId'] == 11:
            p = path_points(b['path'])
            for (x1, y1), (x2, y2) in zip(p, p[1:]):
                (a, c), (d, e) = mm(x1, y1), mm(x2, y2)
                w(f' (gr_line (start {a:.4f} {c:.4f}) (end {d:.4f} {e:.4f}) (layer "Edge.Cuts") (width 0.1))')
        elif t == 'POURED':
            layer = None
        elif t == 'STRING' and b['layerId'] == 3:
            x, y = mm(b['x'], b['y'])
            sz = b['fontSize'] * MIL * 0.8      # the board uses Chakra Petch Bold, narrower than KiCad's stroke font
            w(f' (gr_text "{b["text"]}" (at {x:.4f} {y:.4f} {b["angle"]}) (layer "F.SilkS") (effects (font (size {sz:.3f} {sz * 0.62:.3f}) (thickness {b["strokeWidth"] * MIL * 1.6:.3f}) bold) (justify left bottom)))')

    # pours: POURED follows its POUR; the fill strokes are in 10-mil units
    pour_layer = {}
    for h, b in pcb['items']:
        if h['type'] == 'POUR':
            pour_layer[h['id']] = b['layerId']
        if h['type'] == 'POURED':
            pid = json.loads(h['id'])[1]
            lay = CU[pour_layer[pid]]
            for f in b['pourFill']:
                sw = f.get('strokeWidth', 1) * 10 * MIL
                for seg in f['path']:
                    p = path_points(seg)
                    if f.get('fill'):
                        pts = ' '.join(f'(xy {mm(x * 10, y * 10)[0]:.4f} {mm(x * 10, y * 10)[1]:.4f})' for x, y in p)
                        w(f' (gr_poly (pts {pts}) (layer "{lay}") (width 0) (fill solid))')
                    else:
                        for (x1, y1), (x2, y2) in zip(p, p[1:]):
                            (a, c), (d, e) = mm(x1 * 10, y1 * 10), mm(x2 * 10, y2 * 10)
                            w(f' (segment (start {a:.4f} {c:.4f}) (end {d:.4f} {e:.4f}) (width {sw:.4f}) (layer "{lay}") (net 0))')

    report = []
    for cid, c in comps.items():
        b = c['c']
        fp = fps[c['FootprintName']['value']]
        ref = c['Designator']['value']
        X, Y = mm(b['x'], b['y'])
        ang = b['angle']
        w(f' (footprint "{fp["title"]}" (layer "F.Cu") (at {X:.4f} {Y:.4f} {ang})')
        w(f'  (property "Reference" "{ref}" (at 0 0 0) (layer "F.SilkS") hide (effects (font (size 1 1))))')
        epads = {}
        for h, p in fp['items']:
            t = h['type']
            if t == 'PAD':
                lx, ly = mm(p['centerX'], p['centerY'])
                epads.setdefault(p['num'].replace('P$', ''), (lx, ly))
                dp, hole = p['defaultPad'], p['hole'] or {}
                pa = (p.get('padAngle') or 0) + ang
                tht = p['layerId'] == 12
                layers = '"*.Cu" "*.Mask"' if tht else ('"F.Cu" "F.Mask" "F.Paste"' if p['layerId'] == 1 else '"B.Cu" "B.Mask"')
                kind = 'thru_hole' if tht else 'smd'
                drill = ''
                if tht:
                    if hole.get('holeType') == 'SLOT':
                        drill = f'(drill oval {hole["width"] * MIL:.4f} {hole["height"] * MIL:.4f})'
                    else:
                        drill = f'(drill {hole["width"] * MIL:.4f})'
                if dp['padType'] == 'POLYGON':
                    layers = layers.replace(' "F.Paste"', '')   # card-edge fingers: gold, no solder
                    pts = ' '.join(f'(xy {(x - p["centerX"]) * MIL:.4f} {-(y - p["centerY"]) * MIL:.4f})' for x, y in path_points(dp['path']))
                    w(f'  (pad "{p["num"]}" {kind} custom (at {lx:.4f} {ly:.4f} {ang}) (size 0.2 0.2) (layers {layers}) (options (clearance outline) (anchor circle)) (primitives (gr_poly (pts {pts}) (width 0) (fill yes))))')
                else:
                    shape = {'RECT': 'rect', 'OVAL': 'oval', 'ELLIPSE': 'oval'}[dp['padType']]
                    if dp['padType'] == 'ELLIPSE' and abs(dp['width'] - dp['height']) < 0.01:
                        shape = 'circle'
                    w(f'  (pad "{p["num"]}" {kind} {shape} (at {lx:.4f} {ly:.4f} {pa}) (size {dp["width"] * MIL:.4f} {dp["height"] * MIL:.4f}) {drill} (layers {layers}))')
            elif t == 'POLY' and p['layerId'] == 3:
                path = p['path']
                if path and path[0] == 'CIRCLE':
                    cx, cy = mm(path[1], path[2])
                    w(f'  (fp_circle (center {cx:.4f} {cy:.4f}) (end {cx + path[3] * MIL:.4f} {cy:.4f}) (layer "F.SilkS") (width {p["width"] * MIL:.4f}))')
                    continue
                pts = path_points(path)
                for (x1, y1), (x2, y2) in zip(pts, pts[1:]):
                    (a, cc), (d, e) = mm(x1, y1), mm(x2, y2)
                    w(f'  (fp_line (start {a:.4f} {cc:.4f}) (end {d:.4f} {e:.4f}) (layer "F.SilkS") (width {p["width"] * MIL:.4f}))')
            elif t == 'FILL' and p['layerId'] == 3:
                for sub in p['path']:
                    if sub and sub[0] == 'CIRCLE':
                        cx, cy = mm(sub[1], sub[2])
                        w(f'  (fp_circle (center {cx:.4f} {cy:.4f}) (end {cx + sub[3] * MIL:.4f} {cy:.4f}) (layer "F.SilkS") (width 0) (fill solid))')
                        continue
                    pts = ' '.join(f'(xy {mm(x, y)[0]:.4f} {mm(x, y)[1]:.4f})' for x, y in path_points(sub))
                    w(f'  (fp_poly (pts {pts}) (layer "F.SilkS") (width 0) (fill solid))')
            elif t == 'POLY' and p['layerId'] == 11:
                # the card-edge footprint carries part of the board outline
                pts = path_points(p['path'])
                for (x1, y1), (x2, y2) in zip(pts, pts[1:]):
                    (a, cc), (d, e) = mm(x1, y1), mm(x2, y2)
                    w(f'  (fp_line (start {a:.4f} {cc:.4f}) (end {d:.4f} {e:.4f}) (layer "Edge.Cuts") (width 0.1))')
        # 3D model
        choice = MODELS.get(fp['title'])
        if choice:
            best = None
            for fpname in (choice if isinstance(choice, list) else [choice]):
                kp, model = kicad_pads(fpname)
                f = fit(kp, epads)
                if f and model and (best is None or f[0] < best[0][0]):
                    best = (f, model, fpname)
            if best:
                (res, th, tx, ty), (mpath, off, mrot), fpname = best
                ox, oy = rot(off[0], -off[1], th)          # model offset is y-up
                w(f'  (model "{mpath}" (offset (xyz {tx + ox:.4f} {-(ty + oy):.4f} {off[2]})) (scale (xyz 1 1 1)) (rotate (xyz {mrot[0]} {mrot[1]} {mrot[2] - th})))')
                report.append(f'{ref:6} {fp["title"][:34]:34} -> {fpname.split("/")[1][:40]:40} rot {th:3} fit {res:.3f} mm')
            else:
                report.append(f'{ref:6} {fp["title"]}: no fit')
        elif fp['title'].startswith('AUDIO'):
            mpath = Path(__file__).resolve().parent / 'models' / 'PJ-320B.wrl'
            w(f'  (model "{mpath}" (offset (xyz 0 0 0)) (scale (xyz 1 1 1)) (rotate (xyz 0 0 0)))')
            report.append(f'{ref:6} hand-made PJ-320B model')
        w(' )')
        # visible designator on the silkscreen
        d = c['Designator']
        if d.get('valueVisible') and d.get('x') is not None:
            x, y = mm(d['x'], d['y'])
            sz = d['fontSize'] * MIL * 0.75
            w(f' (gr_text "{ref}" (at {x:.4f} {y:.4f} {d.get("angle") or 0}) (layer "F.SilkS") (effects (font (size {sz:.3f} {sz:.3f}) (thickness {d["strokeWidth"] * MIL:.3f})) (justify left bottom)))')
    w(')')
    Path(dst).write_text('\n'.join(out) + '\n')
    print('\n'.join(sorted(report)))



if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2])
