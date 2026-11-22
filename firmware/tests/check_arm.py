#!/usr/bin/env python3
"""Run the linked busMain(): check what it leaves in RAM, and when it reads the bus.

    python3 firmware/tests/check_arm.py build/picoTsVid.elf

run.py can't do this job. Its stubbed gpio_get() is an external call, so the
compiler has to store every global before each poll. In the firmware the poll
is inline and busMain() never returns, so without a barrier GCC -O3 drops
stores that only the other core reads (that is how 4.3 and 4.4 lost the
border, video mode and ULAplus).

The repeated pin reads in busMain() are also delays, while the buffers switch
the bus onto the data pins. An optimizer can pack them together: a Release
build that stored everything correctly still failed on a board, because it
read the address high byte 12 ns after the switch. So this also counts CPU
cycles between those accesses and fails if any gap is shorter than in V4_TIMING.

This interprets the Thumb code in the ELF and feeds it bus cycles through a
model of the GPIO pins, which follow a switch instantly. It needs only Python,
no toolchain. It does not model the PLD, the Z80's signal timing, bus
contention or the flash cache. Its upper limits (LATEST) come from one board,
so a pass is no substitute for a test on a board.
"""
import re
import struct
import sys
from pathlib import Path

WORD = 0xffffffff
SIO = 0xd0000000
PSEL, IOPORT, RDPORT = 1 << 10, 1 << 12, 1 << 13
DATA_SHIFT, PAGE_SHIFT = 15, 27
DATA_PINS = 0xff << DATA_SHIFT
SELECT, DATA = 0xbf3b, 0xff3b  # register select and data ports
PDDIR = 1 << 26
NS_PER_CYCLE = 4  # at 250 MHz

# The shortest gaps in the V4 image that shipped on production boards, in CPU
# cycles, as measured by bus_timing() on its original ELF (picoPgmV4.zip in
# the git history). 4.2's are the same. Don't lower one without a board test.
V4_TIMING = {
    'select filter': 5,         # between the two polls of PSEL
    'address low settle': 16,   # PSEL seen to the last read of A7-A0
    'address high settle': 11,  # buffer switch to the last read of A15-A8
    'data settle': 23,          # buffer switch to the last read of D7-D0
    'data after select': 72,    # PSEL seen to the last read of D7-D0
    'turnaround out': 6,        # buffer turned to the Z80 to the Pico driving
    'turnaround in': 6,         # Pico released to the buffer turned back
}

# The longest the loop may take, in CPU cycles. 4.5 worked out register reads
# while the Z80 waited: on a board, answers that took 548-604 ns were often
# wrong, while the version read at 516 ns never was. Every answer must now be on
# the bus by then. After a screen write, the loop must be polling again before
# the next one: a PUSH writes twice, about 850 ns apart.
LATEST = {
    'answer': 129,              # PSEL seen to the answer driven, any register
    'screen write': 190,        # PSEL seen to polling for the next bus cycle
}


def signed(value, bits):
    return value - (1 << bits) if value & (1 << (bits - 1)) else value


class Idle(Exception):
    """The bus loop polled with no bus cycle left to serve."""


class Image:
    """The loadable bytes and the symbols of a 32-bit little-endian ELF."""

    def __init__(self, path):
        elf = Path(path).read_bytes()
        if elf[:6] != b'\x7fELF\x01\x01':
            raise SystemExit(f'{path} is not a 32-bit little-endian ELF')
        phoff, shoff = struct.unpack_from('<II', elf, 28)
        phsize, phnum, shsize, shnum = struct.unpack_from('<4H', elf, 42)

        self.memory = {}
        for i in range(phnum):
            kind, offset, vaddr, _, size = struct.unpack_from('<5I', elf, phoff + i * phsize)
            if kind == 1:  # PT_LOAD, at its run address
                for k in range(size):
                    self.memory[vaddr + k] = elf[offset + k]

        sections = [struct.unpack_from('<10I', elf, shoff + i * shsize) for i in range(shnum)]
        self.symbols = {}
        for section in sections:
            if section[1] != 2:  # SHT_SYMTAB
                continue
            names = sections[section[6]][4]
            for at in range(section[4], section[4] + section[5], 16):
                name, value = struct.unpack_from('<II', elf, at)
                if name:
                    end = elf.index(b'\0', names + name)
                    self.symbols[elf[names + name:end].decode()] = value


class Bus:
    """The pins as busMain sees them, for a list of bus cycles."""

    def __init__(self, cycles):
        self.waiting = list(cycles)
        self.cycle = None
        self.out = self.enable = 0
        self.replies = []
        self.clock = 0
        self.started = 0
        self.events = []  # (bus cycle, clock, kind, value) for each pin access

    def pins(self):
        if self.cycle is None:
            if not self.waiting:
                raise Idle
            self.cycle, self.paged = self.waiting.pop(0), False
            self.started += 1
        self.events.append((self.started, self.clock, 'read',
                            (self.out >> PAGE_SHIFT) & 3))
        if self.cycle == 'ending':
            self.cycle = None
            return IOPORT | RDPORT  # PSEL low
        io, reading, address, data = self.cycle
        if self.enable & DATA_PINS:  # the Pico is driving its answer
            self.replies.append((self.out >> DATA_SHIFT) & 0xff)
            self.cycle = 'ending'
            return 0
        page = (self.out >> PAGE_SHIFT) & 3
        if page == 0 and self.paged:  # back to ADRL: the Pico has finished
            self.cycle = 'ending'
            return 0
        self.paged = self.paged or page != 0
        value = (address & 0xff, address >> 8, data, 0)[page] << DATA_SHIFT
        return value | PSEL | (0 if io else IOPORT) | (0 if reading else RDPORT)

    def write(self, register, value):
        before = self.out, self.enable
        if register in (0x10, 0x14, 0x18, 0x1c):
            self.out = self.update(self.out, register - 0x10, value)
        elif register in (0x20, 0x24, 0x28, 0x2c):
            self.enable = self.update(self.enable, register - 0x20, value)
        self.events.append((self.started, self.clock, 'write',
                            before + (self.out, self.enable)))

    @staticmethod
    def update(old, kind, value):
        return {0: value, 4: old | value, 8: old & ~value, 12: old ^ value}[kind] & WORD


class Machine:
    """Enough of a Cortex-M0+ to run busMain as GCC compiles it."""

    def __init__(self, image, cycles):
        self.memory = dict(image.memory)
        self.symbols = image.symbols
        self.bus = Bus(cycles)
        self.sio = False
        self.r = [0] * 16
        self.n = self.z = self.c = self.v = 0

    def byte(self, name, index=0):
        """A byte of a global, or None if the linker dropped it."""
        if name not in self.symbols:
            return None
        return self.memory.get(self.symbols[name] + index, 0)

    def run(self, limit=2000000):
        self.r[13] = 0x20041000
        pc = self.symbols['busMain'] & ~1
        try:
            for _ in range(limit):
                self.sio = False
                op, start = self.load(pc, 2), pc
                pc = self.step(pc)
                self.bus.clock += self.cost(op, pc != start + 2)
        except Idle:
            return self
        raise RuntimeError(f'busMain did not go idle; stopped at {pc:#x}')

    def load(self, address, size):
        address &= WORD
        if SIO <= address < SIO + 0x100:
            self.sio = True
        if address == SIO + 4:
            return self.bus.pins()
        if address == SIO + 0x10:
            return self.bus.out
        if address == SIO + 0x20:
            return self.bus.enable
        return sum(self.memory.get(address + i, 0) << (8 * i) for i in range(size))

    def store(self, address, size, value):
        address &= WORD
        value &= WORD
        if SIO <= address < SIO + 0x100:
            self.sio = True
            self.bus.write(address - SIO, value)
            return
        for i in range(size):
            self.memory[address + i] = (value >> (8 * i)) & 0xff

    def cost(self, op, branched):
        """Cycles the instruction takes on an M0+ running from RAM."""
        top = op >> 12
        if top == 15:  # bl, msr, mrs and barriers
            return 3
        if (top == 4 and op & 0x0800) or top == 9:  # a literal or the stack
            return 2
        if top in (5, 6, 7, 8):  # SIO is on the single-cycle I/O port
            return 1 if self.sio else 2
        if top == 12 or (top == 11 and op & 0x0600 == 0x0400):  # several registers
            count = bin(op & (0xff if top == 12 else 0x1ff)).count('1')
            return count + (3 if branched else 1)
        return 2 if branched else 1

    def flags(self, value):
        value &= WORD
        self.n, self.z = value >> 31, int(value == 0)
        return value

    def add(self, a, b, carry=0):
        total = a + b + carry
        self.c = int(total > WORD)
        self.v = ((~(a ^ b) & (a ^ total)) >> 31) & 1
        return self.flags(total)

    def subtract(self, a, b, carry=1):
        return self.add(a, ~b & WORD, carry)

    def shift(self, kind, value, by):
        if by:
            if kind == 'left':
                self.c = ((value << by) >> 32) & 1 if by <= 32 else 0
                value <<= by
            elif kind == 'right':
                self.c = (value >> (by - 1)) & 1
                value >>= by
            elif kind == 'arith':
                wide = signed(value, 32)
                self.c = (wide >> (by - 1)) & 1
                value = wide >> by
            else:  # rotate
                by &= 31
                value = ((value >> by) | (value << (32 - by))) & WORD
                self.c = value >> 31
        return self.flags(value)

    def alu(self, op, d, m):
        a, b = self.r[d], self.r[m]
        if op == 8:
            self.flags(a & b)
        elif op == 10:
            self.subtract(a, b)
        elif op == 11:
            self.add(a, b)
        elif op in (2, 3, 4, 7):
            kind = {2: 'left', 3: 'right', 4: 'arith', 7: 'rotate'}[op]
            self.r[d] = self.shift(kind, a, b & 0xff)
        elif op == 5:
            self.r[d] = self.add(a, b, self.c)
        elif op == 6:
            self.r[d] = self.subtract(a, b, self.c)
        elif op == 9:
            self.r[d] = self.subtract(0, b)
        else:
            result = {0: a & b, 1: a ^ b, 12: a | b, 13: a * b, 14: a & ~b, 15: ~b}[op]
            self.r[d] = self.flags(result)

    def step(self, pc):
        r, op, following = self.r, self.load(pc, 2), pc + 2
        top, low3, mid3, imm5 = op >> 12, op & 7, (op >> 3) & 7, (op >> 6) & 31
        high3, imm8 = (op >> 8) & 7, op & 0xff

        if op < 0x1800:  # shift by a constant
            kind = ('left', 'right', 'arith')[op >> 11]
            by = imm5 if kind == 'left' else imm5 or 32
            r[low3] = self.shift(kind, r[mid3], by)
        elif op < 0x2000:  # add or subtract, three operands
            other = imm5 & 7 if op & 0x400 else r[imm5 & 7]
            r[low3] = (self.subtract if op & 0x200 else self.add)(r[mid3], other)
        elif op < 0x4000:  # move, compare, add or subtract an 8-bit constant
            kind = (op >> 11) & 3
            if kind == 0:
                r[high3] = self.flags(imm8)
            elif kind == 1:
                self.subtract(r[high3], imm8)
            else:
                r[high3] = (self.add if kind == 2 else self.subtract)(r[high3], imm8)
        elif op < 0x4400:
            self.alu((op >> 6) & 15, low3, mid3)
        elif op < 0x4800:  # high registers and branches to a register
            kind, d, m = (op >> 8) & 3, low3 | ((op >> 4) & 8), (op >> 3) & 15
            b = pc + 4 if m == 15 else r[m]
            a = pc + 4 if d == 15 else r[d]
            if kind == 3:
                if op & 0x80:
                    r[14] = following | 1
                return b & ~1
            if kind == 1:
                self.subtract(a, b)
            else:
                value = (a + b) & WORD if kind == 0 else b
                if d == 15:
                    return value & ~1
                r[d] = value
        elif top == 4:  # load a literal
            r[high3] = self.load(((pc + 4) & ~3) + imm8 * 4, 4)
        elif top == 5:  # load or store, register offset
            kind, address = (op >> 9) & 7, r[mid3] + r[imm5 & 7]
            size = (4, 2, 1, 1, 4, 2, 1, 2)[kind]
            if kind < 3:
                self.store(address, size, r[low3])
            else:
                value = self.load(address, size)
                r[low3] = signed(value, size * 8) & WORD if kind in (3, 7) else value
        elif top in (6, 7, 8):  # load or store, constant offset
            size = {6: 4, 7: 1, 8: 2}[top]
            address = r[mid3] + imm5 * size
            if op & 0x800:
                r[low3] = self.load(address, size)
            else:
                self.store(address, size, r[low3])
        elif top == 9:  # load or store on the stack
            if op & 0x800:
                r[high3] = self.load(r[13] + imm8 * 4, 4)
            else:
                self.store(r[13] + imm8 * 4, 4, r[high3])
        elif top == 10:  # form an address
            r[high3] = ((r[13] if op & 0x800 else (pc + 4) & ~3) + imm8 * 4) & WORD
        elif top == 11:
            kind = (op >> 8) & 15
            registers = [i for i in range(8) if op >> i & 1]
            if kind == 0:
                r[13] += (-4 if op & 0x80 else 4) * (op & 0x7f)
            elif kind == 2:  # extend
                value = r[mid3]
                r[low3] = (signed(value & 0xffff, 16) & WORD, signed(value & 0xff, 8) & WORD,
                           value & 0xffff, value & 0xff)[(op >> 6) & 3]
            elif kind in (4, 5):  # push
                registers += [14] if kind == 5 else []
                r[13] -= 4 * len(registers)
                for i, register in enumerate(registers):
                    self.store(r[13] + 4 * i, 4, r[register])
            elif kind in (12, 13):  # pop
                for register in registers:
                    r[register] = self.load(r[13], 4)
                    r[13] += 4
                if kind == 13:
                    r[13] += 4
                    return self.load(r[13] - 4, 4) & ~1
            elif kind not in (6, 15):  # cpsid/cpsie and hints have no effect here
                raise RuntimeError(f'unsupported instruction {op:04x} at {pc:#x}')
        elif top == 12:  # load or store several
            address, registers = r[high3], [i for i in range(8) if op >> i & 1]
            for register in registers:
                if op & 0x800:
                    r[register] = self.load(address, 4)
                else:
                    self.store(address, 4, r[register])
                address += 4
            if not (op & 0x800 and high3 in registers):
                r[high3] = address
        elif top == 13:  # conditional branch
            condition = (op >> 8) & 15
            if condition >= 14:
                raise RuntimeError(f'unsupported instruction {op:04x} at {pc:#x}')
            n, z, c, v = self.n, self.z, self.c, self.v
            met = (z, c, n, v, c and not z, n == v, not z and n == v)[condition >> 1]
            if bool(met) != bool(condition & 1):
                return pc + 4 + 2 * signed(imm8, 8)
        elif top == 14 and not op & 0x800:
            return pc + 4 + 2 * signed(op & 0x7ff, 11)
        elif top == 15:
            second = self.load(following, 2)
            if op & 0xfff0 in (0xf380, 0xf3b0, 0xf3e0):  # msr, barriers, mrs
                return pc + 4
            if second & 0xd000 != 0xd000:
                raise RuntimeError(f'unsupported instruction {op:04x} {second:04x} at {pc:#x}')
            sign = (op >> 10) & 1
            i1 = 1 - (((second >> 13) & 1) ^ sign)
            i2 = 1 - (((second >> 11) & 1) ^ sign)
            offset = sign << 24 | i1 << 23 | i2 << 22 | (op & 0x3ff) << 12 | (second & 0x7ff) << 1
            r[14] = (pc + 4) | 1
            return (pc + 4 + signed(offset, 25)) & WORD
        else:
            raise RuntimeError(f'unsupported instruction {op:04x} at {pc:#x}')
        return following


def out(port, value):
    return [(True, False, port, value)]


def inp(port):
    return [(True, True, port, 0)]


def poke(address, value):
    return [(False, False, address, value)]


def register(number, value):
    return out(SELECT, number) + out(DATA, value)


def software_version():
    text = (Path(__file__).resolve().parent.parent / 'macros.h').read_text()
    return int(re.search(r'#define\s+SOFTWARE_VER\s+(\w+)', text).group(1), 0)


def bus_timing(image):
    """The shortest gap of each kind in V4_TIMING, in CPU cycles."""
    writes = [poke(0x4005, 0xaa), poke(0x6005, 0x55), poke(0x5823, 0x47),
              out(0xfe, 2), out(0xff, 6), out(SELECT, 5), out(DATA, 0x5a)]
    answered = [inp(SELECT), inp(DATA), inp(0x21)]
    found = {name: [] for name in V4_TIMING}

    for target in writes + answered:
        # between two other cycles, so the loop is in its steady state
        bus = Machine(image, poke(0x5f00, 0) + target + poke(0x5f00, 0)).run().bus
        events = [event[1:] for event in bus.events if event[0] == 2]
        reads = [(at, page) for at, kind, page in events if kind == 'read']
        changes = [(at, value) for at, kind, value in events if kind == 'write']
        switches = {}  # when each buffer was first switched in
        for at, (out0, _, out1, _) in changes:
            old, new = (out0 >> PAGE_SHIFT) & 3, (out1 >> PAGE_SHIFT) & 3
            if old != new:
                switches.setdefault(new, at)

        selected = reads[0][0]
        if len(reads) > 1:
            found['select filter'].append(reads[1][0] - selected)
        if 1 in switches:
            low = [at for at, page in reads if page == 0 and at < switches[1]]
            found['address low settle'].append(low[-1] - selected)
            high = [at for at, page in reads if page == 1]
            if high:
                found['address high settle'].append(high[-1] - switches[1])

        if target in writes:
            # the RDPORT test, the reads of the data, then the IOPORT test
            data = [at for at, page in reads if page == 2]
            if 2 in switches and len(data) >= 3:
                found['data settle'].append(data[-2] - switches[2])
                found['data after select'].append(data[-2] - selected)
        else:
            turned = driving = released = None
            for at, (out0, enable0, out1, enable1) in changes:
                if out0 & PDDIR and not out1 & PDDIR:
                    turned = at
                if not enable0 & DATA_PINS and enable1 & DATA_PINS:
                    driving = at
                if enable0 & DATA_PINS and not enable1 & DATA_PINS:
                    released = at
                if not out0 & PDDIR and out1 & PDDIR and released is not None:
                    found['turnaround in'].append(at - released)
            if turned is not None and driving is not None:
                found['turnaround out'].append(driving - turned)

    return {name: min(gaps) if gaps else None for name, gaps in found.items()}


def latest_timing(image):
    """The longest gap of each kind in LATEST, in CPU cycles."""
    found = {name: [] for name in LATEST}

    def measure(before, target):
        cycles = before + poke(0x5f00, 0) + target + poke(0x5f00, 0)
        n = len(before) + 2
        events = Machine(image, cycles).run().bus.events
        mine = [(at, kind, value) for m, at, kind, value in events if m == n]
        following = [at for m, at, kind, value in events if m == n + 1]
        return mine[0][0], mine, following[0]

    index = register(0x44, 0) + register(0x45, 0)
    for number in (0, 0x3f, 0x40, 0x41, 0x43, 0x46, 0x47, 0x48, 0x7f):
        for target in (inp(DATA), inp(SELECT)):
            start, events, _ = measure(index + out(SELECT, number), target)
            driven = [at for at, kind, value in events
                      if kind == 'write' and value[3] & DATA_PINS and not value[1] & DATA_PINS]
            found['answer'].append(driven[0] - start if driven else None)
    for before in (out(SELECT, 0), out(SELECT, 0x48), register(0x43, 1)):
        for address in (0x4005, 0x6005):
            start, _, following = measure(before, poke(address, 0xaa))
            found['screen write'].append(following - start)

    return {name: None if None in gaps else max(gaps) for name, gaps in found.items()}


def check(image):
    failures = []
    total = 0

    def expect(what, cycles, name, wanted, index=0):
        nonlocal total
        total += 1
        got = Machine(image, cycles).run().byte(name, index)
        if got != wanted:
            failures.append(f'{what}: {name}[{index}] is {got}, expected {wanted}')

    def reads(what, cycles, wanted):
        nonlocal total
        total += 1
        got = Machine(image, cycles).run().bus.replies
        if got != wanted:
            failures.append(f'{what}: read {got}, expected {wanted}')

    # Port writes that the video core must see.
    normal = [image.memory[image.symbols['tsColors'] + i] for i in range(8)]
    for color in range(8):
        expect(f'BORDER {color}', out(0xfe, color), 'gBorder', normal[color])
    for mode in range(1, 8):
        expect(f'OUT 255,{mode}', out(0xff, mode), 'gVidMode', mode)
    expect('return to mode 0', out(0xff, 6) + out(0xff, 0), 'gVidMode', 0)
    expect('64-column colors', out(0xff, 0x3e), 'gTsCol64Ink', 7)

    palette_on = register(0x40, 1)
    expect('ULAplus on', palette_on, 'gUlaEna', 1)
    expect('ULAplus off', palette_on + register(0x40, 0), 'gUlaEna', 0)
    expect('ULAplus color', register(63, 0x5a), 'gUlaColors', 0x5a, 63)
    expect('ULAplus border', register(12, 0x5a) + palette_on + out(0xfe, 4), 'gBorder', 0x5a)
    expect('pinned mode', palette_on + register(0x41, 5), 'gVidMode', 5)
    expect('pinned mode holds', palette_on + register(0x41, 5) + out(0xff, 2), 'gVidMode', 5)
    expect('pinned mode 0', palette_on + out(0xff, 6) + register(0x41, 0x80), 'gVidMode', 0)
    expect('Speccy Black on', register(0x42, 0) + register(0x42, 1), 'speccyBlack', 1)
    expect('Speccy Black off', register(0x42, 0), 'speccyBlack', 0)

    index = register(0x44, 0x02) + register(0x45, 0x01)
    expect('font', index + register(0x46, 0x3c), 'vgaFont', 0x3c, 0x102)
    expect('quad color', register(0x44, 1) + register(0x45, 0) + register(0x47, 0x66),
           'biColor', 0x66, 1)

    # Video memory.
    expect('DF1 write', poke(0x4005, 0xaa), 'gDfRam', 0xaa, 5)
    expect('DF2 write', poke(0x6005, 0x55), 'gDfRam', 0x55, 0x2005)
    expect('DF2 mirror', register(0x43, 1) + poke(0x4009, 0x77), 'gDfRam', 0x77, 0x2009)
    expect('$F4 pages chunk 2', out(0xf4, 4) + poke(0x4007, 0x99), 'gDfRam', 0, 7)
    expect('$F4 pages chunk 3', out(0xf4, 8) + poke(0x6007, 0x99), 'gDfRam', 0, 0x2007)
    expect('Megabank owns chunk 2', out(0xfd, 2) + out(0xfc, 9) + poke(0x4008, 0x99),
           'gDfRam', 0, 8)
    expect('Megabank owns chunk 3', out(0xfd, 3) + out(0xfc, 9) + poke(0x6008, 0x99),
           'gDfRam', 0, 0x2008)
    expect('mode 9 write',
           palette_on + register(0x41, 9) + out(0xe1, 0x10) + out(0xe2, 0x40) + out(0xe0, 0x42),
           'gDfRam', 0x42, 0x4010)

    # What the Z80 reads back.
    version = out(SELECT, 0x7f) + inp(DATA)
    reads('version register', version, [software_version()])
    reads('selected register', out(SELECT, 0x2a) + inp(SELECT), [0x2a])
    reads('ULAplus state', palette_on + inp(DATA), [1])
    reads('mode pin', register(0x41, 5) + inp(DATA), [5])
    reads('mirror state', register(0x43, 1) + inp(DATA), [1])
    reads('video memory', poke(0x4000, 0xc3) + out(SELECT, 0x48) + inp(DATA), [0xc3])
    reads('palette entry', register(20, 0x5a) + out(SELECT, 20) + inp(DATA), [0x5a])
    reads('unused register', out(SELECT, 0x42) + inp(DATA), [0xff])
    reads('read after write, no reselect', register(0x43, 1) + inp(DATA), [1])

    # Registers $46-$48 read through the index, which steps on after each read.
    index = register(0x44, 0) + register(0x45, 0)
    reads('video memory in turn', poke(0x4000, 170) + poke(0x4001, 85) + index
          + out(SELECT, 0x48) + inp(DATA) + inp(DATA), [170, 85])
    reads('screen write after selecting $48', index + out(SELECT, 0x48)
          + poke(0x4000, 0x3c) + inp(DATA), [0x3c])
    reads('mirrored write', register(0x43, 1) + poke(0x4000, 0x96) + register(0x43, 0)
          + register(0x44, 0) + register(0x45, 0x20) + out(SELECT, 0x48) + inp(DATA), [0x96])
    reads('video memory wraps', poke(0x4000, 0x77) + register(0x44, 0xff)
          + register(0x45, 0xbf) + out(SELECT, 0x48) + inp(DATA) + inp(DATA), [0, 0x77])
    reads('quad colors in turn', index + out(SELECT, 0x47) + inp(DATA) * 5,
          [0, 0xff, 0xe0, 0xfc, 0])
    reads('quad color written', index + register(0x47, 0x66) + index
          + out(SELECT, 0x47) + inp(DATA), [0x66])
    reads('font in turn', register(0x44, 0xff) + register(0x45, 0x07)
          + register(0x46, 0x3c) + register(0x44, 0xff) + register(0x45, 0x07)
          + out(SELECT, 0x46) + inp(DATA) + inp(DATA),
          [0x3c, image.memory[image.symbols['vgaFont']]])

    measured = bus_timing(image)
    for name, least in V4_TIMING.items():
        total += 1
        got = measured[name]
        if got is None:
            failures.append(f'timing, {name}: not found, so busMain no longer '
                            'reads the bus the way this check expects')
        elif got < least:
            failures.append(f'timing, {name}: {got * NS_PER_CYCLE} ns, '
                            f'V4 allows {least * NS_PER_CYCLE} ns')

    for name, most in LATEST.items():
        total += 1
        got = latest_timing(image)[name]
        if got is None:
            failures.append(f'timing, {name}: not found, so busMain no longer '
                            'answers the way this check expects')
        elif got > most:
            failures.append(f'timing, {name}: {got * NS_PER_CYCLE} ns, '
                            f'at most {most * NS_PER_CYCLE} ns allowed')

    return total, failures


def main():
    if len(sys.argv) != 2:
        raise SystemExit('usage: check_arm.py build/picoTsVid.elf')
    total, failures = check(Image(sys.argv[1]))
    for failure in failures:
        print('FAIL:', failure)
    if failures:
        raise SystemExit(f'{len(failures)} of {total} checks failed on the linked busMain')
    print(f'PASS: {total} checks on the linked busMain (simulated, not a hardware test)')


if __name__ == '__main__':
    main()
