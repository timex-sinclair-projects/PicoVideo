"""Run a .tap in headless ZEsarUX (TS2068), dump 0x4000-0x7FFF, render through PicoVideo's renderer."""
import subprocess, sys, time, os, signal, json, argparse
from pathlib import Path
sys.path.insert(0, os.path.expanduser('~/.claude/skills/ts-program-demo-video/scripts'))
from zrcp import connect
from PIL import Image
HERE = Path(__file__).parent
ZX = '/Applications/zesarux.app/Contents/MacOS/zesarux'

def run_tap(tap, wait, keys=(), port=10123):
    p = subprocess.Popen([ZX, '--noconfigfile', '--machine', 'TS2068', '--vo', 'null', '--ao', 'null',
                          '--enable-remoteprotocol', '--remoteprotocol-port', str(port), '--fastautoload', str(Path(tap).resolve())],
                         stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, start_new_session=True)
    try:
        z = connect(port)
        time.sleep(wait)
        for k, pause in keys:
            z.press(k); time.sleep(pause)
        hexs = z.cmd('read-memory 16384 16384').replace(' ', '').strip()
        ocr = z.ocr()
        z.close()
    finally:
        os.killpg(os.getpgid(p.pid), signal.SIGKILL)
    return bytes.fromhex(hexs), ocr

def vga_rgb(b):
    g, r, bl = (b >> 5) & 7, (b >> 2) & 7, b & 3
    return (r * 255 // 7, g * 255 // 7, bl * 255 // 3)

def render(dump, out_png, mode=0, border=7, ula=0, sb=1, ff=0, pal=None, bicolor=None, full=False, frame=False):
    d = HERE / 'tmp'; d.mkdir(exist_ok=True)
    (d / 'dump.bin').write_bytes(dump)
    palf = '-'
    if pal is not None:
        (d / 'pal.bin').write_bytes(bytes(pal)); palf = str(d / 'pal.bin')
    args = [str(HERE / 'render'), str(d / 'dump.bin'), str(d / 'out.raw'), str(mode), str(border), str(ula), str(sb), str(ff), palf]
    if bicolor: args.append(bicolor)
    subprocess.run(args, check=True)
    raw = (d / 'out.raw').read_bytes()
    lut = [vga_rgb(i) for i in range(256)]
    img = Image.new('RGB', (640, 192))
    img.putdata([lut[b] for b in raw])
    if not full and mode != 5:
        img = img.crop((64, 0, 576, 192))
    img = img.resize((img.width, 384), Image.NEAREST)
    if frame:  # add border band like the monitor shows
        bc = lut[raw[0]] if mode != 5 else (0, 0, 0)
        big = Image.new('RGB', (img.width + 64, img.height + 48), bc); big.paste(img, (32, 24)); img = big
    img.save(out_png)

if __name__ == '__main__':
    ap = argparse.ArgumentParser(); ap.add_argument('tap'); ap.add_argument('--wait', type=float, default=15)
    ap.add_argument('--dump'); a = ap.parse_args()
    m, ocr = run_tap(a.tap, a.wait)
    print('\n'.join(ocr))
    if a.dump: Path(a.dump).write_bytes(m)
