"""The demo's screens, drawn in each mode's own memory layout.

Every screen is returned as {name: bytes}; build.py packs them with ZX0.
"""
import colorsys
import math
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
_src = (ROOT / 'firmware/globals.c').read_text().split('vgaFont[2048]')[1]
_src = re.sub(r'//[^\n]*|/\*.*?\*/', '', _src.split('{', 1)[1].split('};', 1)[0], flags=re.S)
FONT = bytes(int(v, 16) for v in re.findall(r'0x([0-9A-Fa-f]{1,2})\b', _src))
assert len(FONT) == 2048

BLACK, BLUE, RED, MAGENTA, GREEN, CYAN, YELLOW, WHITE = range(8)
BRIGHT, FLASH = 64, 128


def attr(ink, paper=0, extra=0):
    return ink | paper << 3 | extra


def line_offset(y):
    return (y & 0xC0) << 5 | (y & 7) << 8 | (y & 0x38) << 2


def color(red, green, blue):
    """A PicoVideo color byte from three 0..1 levels."""
    return round(green * 7) << 5 | round(red * 7) << 2 | round(blue * 3)


def codes(text):
    return text.replace('►', '\x10').encode('cp437')  # 16 isn't in Python's cp437


class Screen:
    """A display file: bitmap and attributes."""

    def __init__(self, fill=attr(WHITE)):
        self.mem = bytearray(0x1800) + bytearray([fill] * 768)

    def paint(self, row, col, height, width, value):
        for r in range(row, row + height):
            self.mem[0x1800 + r * 32 + col:0x1800 + r * 32 + col + width] = bytes([value] * width)

    def glyph(self, row, col, code, value=None):
        for k in range(8):
            self.mem[line_offset(row * 8 + k) + col] = FONT[code * 8 + k]
        if value is not None:
            self.mem[0x1800 + row * 32 + col] = value

    def text(self, row, col, text, value=None):
        assert col + len(text) <= 32, text
        for i, code in enumerate(codes(text)):
            self.glyph(row, col + i, code, value)

    def center(self, row, text, value=None):
        self.text(row, (32 - len(text)) // 2, text, value)

    def big(self, row, text, value=None):
        """Double-size text, centered: 16 characters at most."""
        assert len(text) <= 16, text
        left = (32 - 2 * len(text)) // 2
        for i, code in enumerate(codes(text)):
            for k in range(8):
                bits = 0
                for b in range(8):
                    if FONT[code * 8 + k] & 128 >> b:
                        bits |= 0xC000 >> 2 * b
                for y in (2 * k, 2 * k + 1):
                    at = line_offset(row * 8 + y) + left + 2 * i
                    self.mem[at], self.mem[at + 1] = bits >> 8, bits & 255
            if value is not None:
                self.paint(row, left + 2 * i, 2, 2, value)

    def solid(self, row, col, height, width):
        for y in range(row * 8, (row + height) * 8):
            self.mem[line_offset(y) + col:line_offset(y) + col + width] = b'\xff' * width

    def header(self, title, note, band=BLUE):
        ink = BLACK if band >= GREEN else WHITE
        self.paint(0, 0, 5, 32, attr(ink, band, BRIGHT))
        self.big(1, title)
        self.center(3, note)


class Wide:
    """The 64-column layout of modes 6 and 7: two display files."""

    def __init__(self, fill=attr(BLACK, WHITE)):
        self.files = Screen(fill), Screen(fill)

    def paint(self, row, col, height, width, value):
        for r in range(row, row + height):
            for c in range(col, col + width):
                self.files[c & 1].paint(r, c >> 1, 1, 1, value)

    def text(self, row, col, text, value=None):
        assert col + len(text) <= 64, text
        for c, code in enumerate(codes(text), col):
            self.files[c & 1].glyph(row, c >> 1, code, value)

    def center(self, row, text, value=None):
        self.text(row, (64 - len(text)) // 2, text, value)


class Text80:
    """Mode 5: character codes, then attributes."""

    def __init__(self, fill=attr(WHITE, BLUE)):
        self.mem = bytearray(1920) + bytearray([fill] * 1920)

    def paint(self, row, col, height, width, value):
        for r in range(row, row + height):
            self.mem[1920 + r * 80 + col:1920 + r * 80 + col + width] = bytes([value] * width)

    def put(self, row, col, text, value=None):
        data = text if isinstance(text, bytes) else codes(text)
        assert col + len(data) <= 80, text
        self.mem[row * 80 + col:row * 80 + col + len(data)] = data
        if value is not None:
            self.paint(row, col, 1, len(data), value)

    def center(self, row, text, value=None):
        self.put(row, (80 - len(text)) // 2, text, value)

    def box(self, row, col, height, width, value):
        self.paint(row, col, height, width, value)
        self.put(row, col, '╔' + '═' * (width - 2) + '╗')
        for r in range(row + 1, row + height - 1):
            self.put(r, col, '║' + ' ' * (width - 2) + '║')
        self.put(row + height - 1, col, '╚' + '═' * (width - 2) + '╝')


class Quad:
    """Mode 3: 2 bits a pixel, linear, split between the display files."""

    def __init__(self):
        self.pix = bytearray(256 * 192)

    def set(self, x, y, value):
        if 0 <= x < 256 and 0 <= y < 192:
            self.pix[y * 256 + x] = value

    def rect(self, x, y, w, h, value):
        for j in range(y, y + h):
            for i in range(x, x + w):
                self.set(i, j, value)

    def text(self, y, text, value, scale=1):
        x = (256 - 8 * scale * len(text)) // 2
        for i, code in enumerate(codes(text)):
            for k in range(8):
                for b in range(8):
                    if FONT[code * 8 + k] & 128 >> b:
                        self.rect(x + (8 * i + b) * scale, y + k * scale, scale, scale, value)

    def files(self):
        out = bytearray(0x1800), bytearray(0x1800)
        for y in range(192):
            for x in range(256):
                out[x >> 2 & 1][y * 32 + (x >> 3)] |= self.pix[y * 256 + x] << (6 - 2 * (x & 3))
        return out


BAYER = [[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]]


def ready():
    s = Screen()
    s.paint(2, 0, 4, 32, attr(WHITE, BLUE, BRIGHT))
    s.big(2, 'PicoVideo')
    s.center(4, 'feature demo for firmware 4.6')
    s.center(9, 'Runs by itself in six minutes')
    s.center(12, 'Start recording, then press', attr(CYAN, 0, BRIGHT))
    s.big(14, 'SPACE', attr(YELLOW, 0, BRIGHT))
    s.center(19, 'N next   P pause   R restart', attr(GREEN))
    return {'ready': s.mem}


def opening():
    q = Quad()
    for y in range(192):
        for x in range(256):
            q.set(x, y, (0, 1, 2)[int(math.hypot(x - 127.5, y - 95.5) / 8) % 3])
    q.rect(12, 52, 232, 88, 0)
    q.text(60, 'PicoVideo', 3, 3)
    q.text(94, 'TS2068 + VGA', 3, 2)
    q.text(120, 'firmware 4.6 feature demo', 3)
    a, b = q.files()
    return {'opening1': a, 'opening2': b}


def intro():
    s = Screen()
    s.header('PicoVideo', 'clean VGA from the TS2068 bus')
    bus = attr(BLACK, CYAN)
    for col, width, label, paper in ((1, 8, 'TS2068', WHITE), (12, 8, 'Pico', GREEN), (23, 8, 'VGA', YELLOW)):
        s.paint(6, col, 4, width, attr(BLACK, paper, BRIGHT))
        s.text(7, col + (width - len(label)) // 2, label)
    s.text(8, 9, '───', attr(CYAN, 0, BRIGHT))
    s.text(8, 20, '──►', attr(CYAN, 0, BRIGHT))
    s.text(12, 2, '■ Copies every screen write')
    s.text(14, 2, '■ Draws it at 60 pictures/s')
    s.text(16, 2, '■ No driver, no setup')
    s.paint(19, 0, 3, 32, bus)
    s.text(20, 2, 'Firmware read live:')
    return {'intro': s.mem}


def colors():
    s = Screen()
    s.header('PHOTO COLORS', 'normal is about half of BRIGHT')
    s.text(5, 0, 'Normal')
    s.text(12, 0, 'BRIGHT', attr(WHITE, 0, BRIGHT))
    for c in range(8):
        s.paint(6, c * 4, 5, 4, attr(c, c))
        s.paint(13, c * 4, 5, 4, attr(c, c, BRIGHT))
    for c, name in enumerate(('blk', 'blu', 'red', 'mag', 'grn', 'cyn', 'yel', 'wht')):
        s.text(19, c * 4, name, attr(c or WHITE, 0, BRIGHT))
    s.center(22, 'measured by jon0x0', attr(CYAN))
    return {'colors': s.mem}


def speccy():
    s = Screen()
    s.header('SPECCY BLACK', 'for ported Spectrum games', MAGENTA)
    # BRIGHT sprite cells on black: the case that shows gray on a TS2068
    ship = ('......XX......',
            '.....XXXX.....',
            '.....XooX.....',
            '....XXooXX....',
            '..XXXXXXXXXX..',
            'XXXXX.XX.XXXXX',
            'XX....XX....XX',
            '.....ffff.....',
            '......ff......')
    for r, line in enumerate(ship):
        s.paint(6 + r, 8, 1, 16, attr(WHITE, 0, BRIGHT))
        for c, ch in enumerate(line):
            if ch != '.':
                s.solid(6 + r, 9 + c, 1, 1)
                s.paint(6 + r, 9 + c, 1, 1, attr({'X': CYAN, 'o': YELLOW, 'f': RED}[ch], 0, BRIGHT))
    s.center(16, 'The box is BRIGHT black', attr(WHITE))
    return {'speccy': s.mem}


def border():
    s = Screen()
    s.header('BORDER', 'all 8 colors, stripes, FLASH', RED)
    for c in range(8):
        s.paint(6, c * 4, 3, 4, attr(c, c))
    s.paint(12, 4, 5, 24, attr(YELLOW, RED, BRIGHT | FLASH))
    s.big(13, 'FLASH')
    s.center(19, 'Stripes follow the Pico,')
    s.center(20, 'not the television beam.')
    return {'border': s.mem}


def mode1():
    out = {}
    for n, ink, paper, shade in ((1, WHITE, BLUE, '░'), (2, BLACK, YELLOW, '▒')):
        s = Screen(attr(CYAN if n == 1 else RED, paper, BRIGHT))
        for row in (0, 1, 2, 20, 21, 22, 23):
            s.text(row, 0, shade * 32)
        s.paint(5, 1, 13, 30, attr(ink, paper, BRIGHT))
        s.big(6, 'DISPLAY FILE %d' % n)
        s.center(10, 'Mode %d' % (n - 1))
        s.center(12, 'from %04Xh' % (0x4000 if n == 1 else 0x6000))
        s.center(15, 'switched with one OUT to 255')
        out['mode1df%d' % n] = s.mem
    return out


def mode2():
    s = Screen()
    attrs = bytearray(0x1800)
    ramp = (BLUE, MAGENTA, RED, YELLOW, GREEN, CYAN)
    for y in range(192):
        for col in range(32):
            x = col / 31
            level = (y / 24 + 1.3 * math.sin(x * 7) + 0.8 * math.sin(x * 15 + y / 37)) % 6
            first = int(level)
            attrs[line_offset(y) + col] = attr(ramp[(first + 1) % 6], ramp[first], BRIGHT)
            bits = 0
            for b in range(8):
                if int((level - first) * 4) * 4 > BAYER[y & 3][b & 3] + .5:
                    bits |= 128 >> b
            s.mem[line_offset(y) + col] = bits
    for row in range(24):  # what a cell-sized attribute can manage
        for col in range(32):
            s.paint(row, col, 1, 1, attrs[line_offset(row * 8 + 3) + col])
    label = Screen()
    label.big(1, 'EXTENDED COLOR')
    label.center(20, 'mode 2: colors on every line')
    for top, height in ((1, 2), (20, 1)):
        for y in range(top * 8, (top + height) * 8):
            at = line_offset(y)
            s.mem[at:at + 32] = label.mem[at:at + 32]
            attrs[at:at + 32] = bytes([attr(WHITE, 0, BRIGHT)] * 32)
        s.paint(top, 0, height, 32, attr(WHITE, 0, BRIGHT))
    return {'mode2df1': s.mem, 'mode2df2': attrs}


def mode6():
    w = Wide()
    w.center(1, '═' * 60)
    w.center(3, 'MODE 6: THE TS2068 64-COLUMN SCREEN')
    w.center(5, '512 x 192 pixels, one INK and PAPER for the whole screen')
    w.center(7, '═' * 60)
    for i in range(8):
        w.text(9 + i, 8, 'OUT 255,%-2d  INK %d on PAPER %d  %s' % (6 + 8 * i, i, 7 - i, '▓▒░' * 5))
    w.center(19, '0123456789012345678901234567890123456789012345678901234567890123')
    w.center(21, 'Port 255 bits 3 to 5 pick the color pair.')
    return {'mode6df1': w.files[0].mem, 'mode6df2': w.files[1].mem}


def mode7():
    w = Wide(attr(WHITE))
    w.paint(0, 0, 3, 64, attr(WHITE, BLUE, BRIGHT))
    w.center(1, 'MODE 7: 64 COLUMNS, EACH WITH ITS OWN COLORS')
    for c in range(64):
        w.paint(4, c, 1, 1, attr(0, c & 7))
        w.paint(5, c, 1, 1, attr(0, c & 7, BRIGHT))
        w.text(7, c, chr(65 + c % 26), attr(1 + c % 7, 0, BRIGHT))
    code = (('10', 'REM', ' the same pixel layout as mode 6'),
            ('20', 'FOR', ' c=0 TO 63'),
            ('30', 'PRINT', ' "every column has an attribute"'),
            ('40', 'NEXT', ' c'))
    for r, (number, word, rest) in enumerate(code):
        w.text(10 + r, 4, number, attr(YELLOW, 0, BRIGHT))
        w.text(10 + r, 7, word, attr(MAGENTA, 0, BRIGHT))
        w.text(10 + r, 7 + len(word), rest, attr(GREEN if r == 0 else CYAN, 0, BRIGHT))
    for i, label in enumerate((' EDIT ', ' FIND ', ' SAVE ', ' RUN ', ' HELP ')):
        w.text(16, 3 + 12 * i, label, attr(BLACK, 2 + i, BRIGHT))
    w.paint(20, 0, 4, 64, attr(BLACK, YELLOW, BRIGHT))
    w.center(21, 'Even columns: display file 1.  Odd columns: display file 2.')
    return {'mode7df1': w.files[0].mem, 'mode7df2': w.files[1].mem}


def mode5():
    t = Text80()
    t.paint(0, 0, 3, 80, attr(BLACK, CYAN, BRIGHT))
    t.center(1, 'MODE 5: 80 x 24 TEXT, WITH AN ATTRIBUTE FOR EVERY CHARACTER')
    t.box(4, 2, 12, 36, attr(WHITE, BLUE, BRIGHT))
    t.put(4, 5, ' Code page 437 ')
    for code in range(256):
        t.put(5 + code // 32, 4 + code % 32, bytes([code]), attr(YELLOW, BLUE, BRIGHT))
    t.put(14, 4, 'All 256 characters', attr(CYAN, BLUE, BRIGHT))
    t.box(4, 42, 12, 36, attr(WHITE, BLACK, BRIGHT))
    t.put(4, 45, ' Every cell its own colors ')
    for r in range(8):
        for i in range(32):
            t.put(6 + r, 44 + i, '░▒▓█'[i % 8 // 2], attr(1 + r % 7, (i // 8 * 2 + r) % 8, BRIGHT))
    t.center(17, 'A character takes one byte for its code and one for its color.')
    t.center(19, 'No bitmap to draw: PicoVideo draws the letters.', attr(YELLOW, BLUE, BRIGHT))
    t.paint(23, 0, 1, 80, attr(BLACK, WHITE))
    t.put(23, 1, 'F1 Help   F2 Save   F3 Load   F10 Quit')
    return {'mode5': t.mem}


def font():
    t = Text80(attr(WHITE, BLACK, BRIGHT))
    t.paint(0, 0, 3, 80, attr(WHITE, RED, BRIGHT))
    t.center(1, 'A FONT YOU CAN CHANGE')
    lines = ('The quick brown fox jumps over the lazy dog.',
             'THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG.',
             '0123456789  ! @ # $ % & * ( ) [ ] { } < > ? /',
             '┌──────────────┬──────────────┐',
             '│ TS2068       │ PicoVideo    │',
             '└──────────────┴──────────────┘')
    for i, line in enumerate(lines):
        t.center(5 + 2 * i if i < 3 else 8 + i, line,
                 attr((YELLOW, CYAN, GREEN, WHITE, WHITE, WHITE)[i], 0, BRIGHT))
    t.center(16, 'The text in memory stays the same.')
    t.center(18, 'Only the 2,048 bytes of font in the Pico change.')
    return {'font': t.mem}


def quad():
    q = Quad()
    for y in range(120):
        for x in range(256):
            q.set(x, y, int(y / 130 * 16 > BAYER[y & 3][x & 3] + .5))
    for y in range(40, 120):
        for x in range(76, 180):
            if math.hypot(x - 128, y - 94) < 45 and not (y > 84 and (y - 84) % 9 < (y - 78) // 10):
                q.set(x, y, 3)
    for x in range(256):
        ridge = int(104 + 9 * math.sin(x / 17) + 5 * math.sin(x / 6.3))
        for y in range(min(ridge, 119), 120):
            q.set(x, y, 0)
    for y in range(120, 192):
        depth = (y - 117) / 75
        for x in range(256):
            u = ((x - 127.5) / depth / 30 + .5) % 1
            v = (1.4 / depth) % 1
            q.set(x, y, 2 if u < .07 / math.sqrt(depth) or v < .15 else 0)
    q.text(8, 'QUAD COLOR', 3, 2)
    q.text(27, 'mode 3: any pixel, any of four', 3)
    a, b = q.files()
    return {'quad1': a, 'quad2': b}


def ulaplus():
    # INK 7 and PAPER 0 of table 0 (entries 7 and 8) are kept for the text.
    s = Screen(attr(WHITE, BLACK))
    s.big(0, 'ULAplus')
    s.center(2, '64 colors from a table you load')
    for table in range(4):
        extra = table * 64
        row = 4 + 5 * table
        for i in range(16):
            # solid pixels show the INK entry, empty ones the PAPER entry
            s.paint(row, 2 * i, 4, 2, attr(i & 7, i & 7, extra))
            if i >= 8:
                s.solid(row, 2 * i, 4, 2)
    return {'ulaplus': s.mem}


def mirror():
    w = Wide(attr(WHITE, BLUE, BRIGHT))
    w.paint(0, 0, 3, 64, attr(BLACK, GREEN, BRIGHT))
    w.center(1, 'MIRRORING: REACH DISPLAY FILE 2 THROUGH DISPLAY FILE 1')
    w.center(5, '64-column text needs both display files.')
    w.center(7, 'Until the second one is filled, every other column is empty.')
    w.center(10, 'Register 67 = 1')
    w.center(12, 'Now each write to 4000h lands in the Pico copy of 6000h,')
    w.center(14, 'so a routine that only knows one display file can draw both.')
    for i in range(1, 8):
        w.text(17, 4 + 8 * (i - 1), ' INK %d ' % i, attr(i, 0, BRIGHT))
    w.center(20, 'The drawing writes only to 4000h-57FFh.')
    empty = bytes(0x1800) + w.files[1].mem[0x1800:]
    return {'mirrordf1': w.files[0].mem, 'mirrordf2': w.files[1].mem[:0x1800], 'mirrorempty': empty}


def pinned():
    t = Text80(attr(WHITE, BLACK))
    t.paint(0, 0, 3, 80, attr(BLACK, WHITE, BRIGHT))
    t.center(1, 'MODE PINNING')
    t.center(5, 'Register 65 = 5 holds PicoVideo in 80 columns.')
    t.center(7, 'It works while the ULAplus palette is on.')
    t.box(10, 18, 7, 44, attr(WHITE, BLACK, BRIGHT))
    t.put(12, 22, 'The program now does  OUT 255, -')
    t.put(14, 22, 'The picture stays in mode 5.')
    t.center(19, 'The palette works here too: watch it turn amber.')
    return {'pinned': t.mem}


CHECKS = ('Version 4.6', 'Register select', 'ULAplus color', 'ULAplus switch', 'Mode pin',
          'Mirror switch', 'Quad color', 'Font memory', 'Video memory', 'Mirrored write',
          'Port F4 paging', 'Megabank owner')


def checks():
    s = Screen()
    s.header('READ IT BACK', 'twelve live checks', GREEN)
    for i, label in enumerate(CHECKS):
        s.text(6 + i, 2, label.ljust(22, '·'), attr(CYAN if i % 2 else WHITE, 0, BRIGHT))
    s.center(19, 'Each result is read from', attr(WHITE))
    s.center(20, 'PicoVideo as you watch.', attr(WHITE))
    return {'checks': s.mem}


def sound():
    s = Screen()
    s.header('SOUND', 'TS2068 audio on the jack', YELLOW)
    for c in range(8):
        s.solid(18 - 2 * (c % 4 + 1) - c // 4, 4 * c, 2 * (c % 4 + 1) + c // 4, 3)
    for c in range(8):
        s.paint(6, 4 * c, 12, 3, attr(BLUE))
    s.center(20, 'PicoVideo passes the sound on.')
    return {'sound': s.mem}


def ending():
    s = Screen()
    s.header('PicoVideo', 'firmware 4.6 beta')
    s.center(6, 'Designed by Jeff Burrell', attr(CYAN, 0, BRIGHT))
    s.center(8, 'Photo palette and fixes by', attr(WHITE))
    s.center(9, 'jon0x0', attr(CYAN, 0, BRIGHT))
    s.center(11, 'Open source, MIT License', attr(WHITE))
    s.center(14, 'github.com/', attr(YELLOW, 0, BRIGHT))
    s.center(15, 'timex-sinclair-projects/', attr(YELLOW, 0, BRIGHT))
    s.center(16, 'PicoVideo', attr(YELLOW, 0, BRIGHT))
    s.center(19, 'timexsinclair.com', attr(GREEN, 0, BRIGHT))
    for c in range(1, 8):
        s.paint(22, 4 * c - 2, 2, 4, attr(c, c, BRIGHT))
    return {'ending': s.mem}


def all_screens():
    out = {}
    for make in (ready, opening, intro, colors, speccy, border, mode1, mode2, mode6, mode7,
                 mode5, font, quad, ulaplus, mirror, pinned, checks, sound, ending):
        out.update(make())
    return out


def tables():
    """Color tables the program loads into the Pico."""
    def hsv(h, s, v):
        return color(*colorsys.hsv_to_rgb(h % 1, s, v))

    wheel = []
    for saturation, value in ((1, 1), (.55, 1), (1, .65), (.8, .85)):
        wheel += [hsv(i / 64, saturation, value) for i in range(64)]

    normal = [0x00, 0x02, 0x14, 0x16, 0xA0, 0xA2, 0xB4, 0xB6]
    bright = [0x00, 0x03, 0x1C, 0x1F, 0xE0, 0xE3, 0xFC, 0xFF]
    stock = (normal * 2 + bright * 2) * 2
    amber = []
    for level in ([0, .3, .4, .5, .55, .65, .75, .85] * 2 + [0, .4, .5, .6, .7, .8, .9, 1] * 2) * 2:
        amber.append(color(level, level * .62, 0))

    quads = [color(.14, 0, .34), color(.85, 0, .67), color(0, 1, 1), color(1, .86, 0),     # dusk
             color(0, 0, .34), color(.14, .3, 1), color(.45, .7, 1), color(1, 1, 1),        # night
             color(0, .14, 0), color(0, .43, 0), color(0, .72, 0), color(.57, 1, .34),      # phosphor
             color(.28, 0, 0), color(1, .28, 0), color(1, .57, 0), color(1, 1, .67)]        # heat
    return {'wheel': bytes(wheel), 'stockPalette': bytes(stock), 'amberPalette': bytes(amber),
            'quadSets': bytes(quads), 'ringColors': bytes(hsv(i / 16, 1, 1) for i in range(16))}
