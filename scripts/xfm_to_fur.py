#!/usr/bin/env python3
"""Convert an XFM built-in song header to a Furnace YM2612 .fur module.

Usage: python3 xfm_to_fur.py song.h song.fur
       python3 xfm_to_fur.py --all sounds/builtin_songs output_dir

Requires only Python 3.9+; writes the uncompressed Furnace 244 format. Unsupported
features fail loudly instead of being silently approximated.
"""
from __future__ import annotations

import argparse
from dataclasses import dataclass, field
from pathlib import Path
import re
import struct
import sys

VERSION = 244
NOTES = {n: i for i, n in enumerate(('C-', 'C#', 'D-', 'D#', 'E-', 'F-', 'F#', 'G-', 'G#', 'A-', 'A#', 'B-'))}
# XFM's signed DT is converted to Furnace's DT index. Furnace's YM2612
# driver maps indices 0..7 to hardware values 7,6,5,0,1,2,3,4.
DT_TO_FUR = {-3: 2, -2: 1, -1: 0, 0: 3, 1: 4, 2: 5, 3: 6}


class ConversionError(ValueError):
    pass


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ConversionError(message)


def u8(value: int) -> bytes:
    require(0 <= value <= 255, f'byte out of range: {value}')
    return struct.pack('<B', value)


def u16(value: int) -> bytes:
    require(0 <= value <= 65535, f'word out of range: {value}')
    return struct.pack('<H', value)


def u32(value: int) -> bytes:
    return struct.pack('<I', value)


def f32(value: float) -> bytes:
    return struct.pack('<f', value)


def cstr(value: str) -> bytes:
    return value.encode('utf-8') + b'\0'


def block(code: bytes, body: bytes) -> bytes:
    return code + u32(len(body)) + body


def feature(code: bytes, body: bytes) -> bytes:
    return code + u16(len(body)) + body


@dataclass
class Macro:
    target: int
    values: list[int]
    loop: int
    release: int


@dataclass
class Instrument:
    number: int
    name: str = ''
    patch: tuple[int, int, int, int] | None = None
    ops: dict[int, tuple[int, ...]] = field(default_factory=dict)
    macros: list[Macro] = field(default_factory=list)


@dataclass
class Cell:
    note: int | None = None
    instrument: int | None = None
    volume: int | None = None
    effects: list[tuple[int, int]] = field(default_factory=list)


@dataclass
class Song:
    name: str
    tick_rate: int
    speed: int
    rows_per_beat: int
    tuning_mode: int
    lfo_enabled: int
    lfo_frequency: int
    rows: list[list[Cell]]
    section_names: dict[int, str]
    instruments: list[Instrument]


@dataclass
class PatternSlice:
    start: int
    rows: list[list[Cell]]
    name: str


def extract_raw(source: str, macro: str, delimiter: str) -> str:
    match = re.search(r'\b' + re.escape(macro) + r'\s*\(\s*R"' + delimiter +
                      r'\((.*?)\)' + delimiter + r'"\s*\)', source, re.S)
    require(match is not None, f'missing {macro} raw string')
    return match.group(1)


def extract_int(source: str, macro: str, default: int | None = None) -> int:
    match = re.search(r'\b' + re.escape(macro) + r'\s*\(\s*(-?\d+)\s*\)', source)
    if match:
        return int(match.group(1))
    require(default is not None, f'missing {macro}')
    return default


def parse_cell(raw: str, row: int, channel: int) -> Cell:
    require(len(raw) in (7, 11, 15), f'row {row} channel {channel}: invalid cell {raw!r}')
    note_text = raw[:3]
    if note_text == '...':
        note = None
    elif note_text in ('OFF', 'REL', '==='):
        note = {'OFF': 180, 'REL': 181, '===': 182}[note_text]
    else:
        require(note_text[:2] in NOTES and note_text[2].isdigit(),
                f'row {row} channel {channel}: invalid note {note_text!r}')
        # Furnace's PATN note byte 60 is C-0; both trackers now label
        # MIDI C4 as C-4 and sound it at the same base pitch.
        note = 60 + int(note_text[2]) * 12 + NOTES[note_text[:2]]
        require(note < 180, f'row {row}: note is outside Furnace range')
    ins = None if raw[3:5] == '..' else int(raw[3:5], 16)
    vol = None if raw[5:7] == '..' else int(raw[5:7], 16)
    fx = []
    for offset in (7, 11):
        if len(raw) <= offset:
            break
        token = raw[offset:offset + 4]
        if token == '....':
            continue
        require(re.fullmatch(r'[0-9A-Fa-f]{4}', token) is not None,
                f'row {row} channel {channel}: invalid effect {token!r}')
        fx.append((int(token[:2], 16), int(token[2:], 16)))
    return Cell(note, ins, vol, fx)


def parse_song(path: Path) -> Song:
    source = path.read_text(encoding='utf-8')
    name = extract_raw(source, 'XFM_SONG_BEGIN', 'xfmname').strip()
    pattern = extract_raw(source, 'XFM_PATTERN', 'xfmpattern')
    lines = pattern.strip().splitlines()
    require(lines and lines[0].strip().isdigit(), 'pattern must begin with row count')
    declared_rows = int(lines[0])
    rows = []
    sections = {}
    for line in lines[1:]:
        line = line.strip()
        if not line or line == 'COLLAPSED' or line.startswith('PART_COLOR '):
            continue
        if line.startswith(('PART ', 'SKIP ')):
            sections[len(rows)] = line
            continue
        parts = line.split('|')
        require(len(parts) == 6, f'row {len(rows)}: expected six channels, got {len(parts)}')
        rows.append([parse_cell(part, len(rows), c) for c, part in enumerate(parts)])
    require(len(rows) == declared_rows,
            f'pattern says {declared_rows} rows but contains {len(rows)}')
    instruments = []
    ins_raw = extract_raw(source, 'XFM_INSTRUMENTS', 'xfminstruments')
    current = None
    for line in ins_raw.splitlines():
        line = line.strip()
        if not line or line.startswith('//'):
            continue
        parts = line.split()
        tag = parts[0]
        if tag == 'INST':
            require(current is None, 'nested INST')
            current = Instrument(int(parts[1], 16))
            instruments.append(current)
        elif tag == 'ENDINST':
            require(current is not None, 'ENDINST without INST')
            require(current.patch is not None and len(current.ops) == 4,
                    f'instrument {current.number:02X} lacks patch or four operators')
            current = None
        elif tag in ('NAME', 'COLOR', 'PATCH', 'FM', 'OP', 'MACRO'):
            require(current is not None, f'{tag} outside instrument')
            if tag == 'NAME':
                current.name = line[5:].strip()
            elif tag == 'PATCH':
                require(len(parts) == 5, 'PATCH requires ALG FB AMS FMS')
                current.patch = tuple(map(int, parts[1:]))
            elif tag == 'FM':
                if parts[1] == 'OP':
                    continue
                require(len(parts) == 13, f'instrument {current.number:02X}: invalid FM line')
                op = int(parts[1])
                require(1 <= op <= 4, 'FM operator must be 1..4')
                current.ops[op] = tuple(map(int, parts[2:]))
            elif tag == 'OP':
                require(len(parts) == 13, f'instrument {current.number:02X}: invalid OP line')
                op = int(parts[1])
                require(1 <= op <= 4, 'OP operator must be 1..4')
                dt, mul, tl, rs, ar, am, dr, sr, sl, rr, ssg = map(int, parts[2:])
                current.ops[op] = (tl, ar, dr, sl, sr, rr, ssg, mul, dt, rs, am)
            elif tag == 'MACRO':
                require(len(parts) >= 6, 'MACRO requires target length loop release values')
                target, length, loop, release = map(int, parts[1:5])
                values = list(map(int, parts[5:]))
                require(len(values) == length and 1 <= length <= 64,
                        f'instrument {current.number:02X}: invalid macro length')
                require(loop == 255 or 0 <= loop < length, 'invalid loop point')
                require(release == 255 or 0 <= release < length, 'invalid release point')
                current.macros.append(Macro(target, values, loop, release))
        elif tag != 'ALG':
            raise ConversionError(f'unknown instrument line: {line}')
    require(current is None, 'missing ENDINST')
    numbers = [ins.number for ins in instruments]
    require(len(set(numbers)) == len(numbers), 'duplicate instrument number')
    return Song(name, extract_int(source, 'XFM_TICK_RATE'),
                extract_int(source, 'XFM_SPEED'), extract_int(source, 'XFM_ROWS_PER_BEAT', 4),
                extract_int(source, 'XFM_TUNING_MODE', 0),
                extract_int(source, 'XFM_LFO_ENABLED', 1),
                extract_int(source, 'XFM_LFO_FREQUENCY', 0), rows, sections, instruments)


def macro_target(target: int) -> tuple[bytes, int]:
    if 1 <= target <= 4:
        return f'O{target}'.encode(), 6   # TL
    if 5 <= target <= 8:
        return f'O{target - 4}'.encode(), 3  # MULT
    if 9 <= target <= 12:
        return f'O{target - 8}'.encode(), 9  # DT
    if target == 13:
        return b'MA', 9  # feedback
    if target == 14:
        return b'MA', 1  # relative arpeggio
    if target == 15:
        # XFM pan uses two-bit 0..3, whereas Furnace pan macros are separate L/R.
        raise ConversionError('PAN macro cannot be represented by one Furnace macro')
    if target in (16, 17, 18):
        raise ConversionError(f'macro target {target} has different pitch/phase semantics in Furnace')
    for start, code in ((19, 1), (23, 2), (27, 10), (31, 5), (35, 4), (39, 11)):
        if start <= target < start + 4:
            return f'O{target - start + 1}'.encode(), code
    raise ConversionError(f'unsupported macro target {target}')


def encode_macro(macro: Macro, code: int, dt: bool = False) -> bytes:
    values = [DT_TO_FUR[v] for v in macro.values] if dt else macro.values
    lo, hi = min(values), max(values)
    if 0 <= lo and hi <= 255:
        wordsize, encoded = 0, bytes(values)
    elif -128 <= lo and hi <= 127:
        wordsize, encoded = 64, struct.pack('<' + 'b' * len(values), *values)
    elif -32768 <= lo and hi <= 32767:
        wordsize, encoded = 128, struct.pack('<' + 'h' * len(values), *values)
    else:
        wordsize, encoded = 192, struct.pack('<' + 'i' * len(values), *values)
    # mode 0 for relative arpeggio, delay 0 and speed 1 match XFM's tick macro.
    return bytes((code, len(values), macro.loop, macro.release, 0, wordsize, 0, 1)) + encoded


def encode_instrument(ins: Instrument) -> bytes:
    alg, fb, ams, fms = ins.patch
    require(0 <= alg <= 7 and 0 <= fb <= 7 and 0 <= ams <= 3 and 0 <= fms <= 7,
            f'instrument {ins.number:02X}: invalid patch setting')
    fm = bytes((0xF4, (alg << 4) | fb, (ams << 3) | fms, 0x20, 0))
    for index in range(1, 5):
        tl, ar, dr, sl, sr, rr, ssg, mul, dt, rs, am = ins.ops[index]
        require(0 <= tl <= 127 and 0 <= ar <= 31 and 0 <= dr <= 31 and
                0 <= sl <= 15 and 0 <= sr <= 31 and 0 <= rr <= 15 and
                0 <= ssg <= 15 and 0 <= mul <= 15 and dt in DT_TO_FUR and
                0 <= rs <= 3 and 0 <= am <= 1,
                f'instrument {ins.number:02X} operator {index}: out-of-range setting')
        fm += bytes(((DT_TO_FUR[dt] << 4) | mul, tl, (rs << 6) | ar,
                     (am << 7) | dr, sr, (sl << 4) | rr, ssg, 0))
    features = feature(b'NA', cstr(ins.name or f'Instrument {ins.number:02X}'))
    features += feature(b'FM', fm)
    grouped: dict[bytes, list[bytes]] = {}
    used = set()
    for macro in ins.macros:
        group, code = macro_target(macro.target)
        require((group, code) not in used,
                f'instrument {ins.number:02X}: duplicate Furnace macro target')
        used.add((group, code))
        if macro.target in range(9, 13):
            require(all(v in DT_TO_FUR for v in macro.values), 'DT macro value must be -3..3')
        grouped.setdefault(group, []).append(encode_macro(macro, code, 9 <= macro.target <= 12))
    for group, macros in grouped.items():
        features += feature(group, u16(8) + b''.join(macros) + b'\xff')
    features += b'EN\0\0'
    return block(b'INS2', u16(VERSION) + b'\x01\x00' + features)


def encode_cell(cell: Cell, remap: dict[int, int]) -> bytes:
    mask = 0
    payload = b''
    require(len(cell.effects) <= 4, 'this exporter supports at most four effect columns')
    if cell.note is not None:
        mask |= 1
        payload += u8(cell.note)
    if cell.instrument is not None:
        require(cell.instrument in remap, f'pattern references undefined instrument {cell.instrument:02X}')
        mask |= 2
        payload += u8(remap[cell.instrument])
    if cell.volume is not None:
        mask |= 4
        payload += u8(cell.volume)
    effect_mask = 0
    for i, (code, value) in enumerate(cell.effects):
        if i == 0:
            mask |= 0x18
        else:
            mask |= 0x20
            effect_mask |= 3 << (i * 2)
        payload += bytes((code, value))
    if len(cell.effects) > 1:
        return bytes((mask, effect_mask)) + payload
    return u8(mask) + payload


def encode_pattern(channel: int, order: int, rows: list[list[Cell]],
                   remap: dict[int, int], name: str) -> bytes:
    payload = bytearray()
    empty = 0
    for row in rows:
        cell = row[channel]
        if cell.note is None and cell.instrument is None and cell.volume is None and not cell.effects:
            empty += 1
            if empty == 129:
                payload += bytes((0xff - 1,))  # 0xFE skips 128 rows
                empty = 1
            continue
        while empty >= 2:
            count = min(empty, 128)
            payload += bytes((0x80 | (count - 2),))
            empty -= count
        if empty:
            payload += b'\0'
            empty = 0
        payload += encode_cell(cell, remap)
    # trailing blank rows are implicit after the end marker
    payload += b'\xff'
    return block(b'PATN', b'\0' + u8(channel) + u16(order) + cstr(name) + payload)


def encode_sng2(song: Song, pattern_length: int, orders: list[int],
                effect_columns: list[int]) -> bytes:
    require(1 <= song.speed <= 65535 and song.tick_rate > 0, 'invalid song timing')
    require(1 <= len(orders) <= 256, 'Furnace permits at most 256 orders')
    body = f32(float(song.tick_rate)) + bytes((1, 1))
    body += u16(pattern_length) + u16(len(orders)) + bytes((song.rows_per_beat, 16))
    body += u16(150) + u16(150)  # virtual tempo 1:1
    body += b'\x01' + u16(song.speed) + b'\0' * 30
    body += cstr(song.name) + b'\0'
    body += bytes(orders) * 6  # orders, channel-major
    body += bytes(effect_columns)
    body += b'\x03' * 6  # visible channel + oscilloscope
    body += b'\0' * 6
    body += b'\0' * 12  # long and short channel names
    body += b'\0' * 24  # default colors
    return block(b'SNG2', body)


def split_parts(song: Song) -> list[PatternSlice]:
    boundaries = sorted(set((0, len(song.rows), *song.section_names)))
    slices = []
    for start, end in zip(boundaries, boundaries[1:]):
        name = song.section_names.get(start, f'PART {start}')
        if name.startswith('SKIP '):
            continue
        for offset in range(start, end, 256):
            chunk = song.rows[offset:min(offset + 256, end)]
            suffix = '' if offset == start else f' +{offset - start}'
            slices.append(PatternSlice(offset, chunk, name + suffix))
    require(len(slices) <= 256, 'Furnace permits at most 256 patterns per channel')
    return slices


def convert(song: Song) -> bytes:
    require(song.tuning_mode == 0, 'just intonation has no exact Furnace note grid equivalent')
    require(song.lfo_enabled in (0, 1) and 0 <= song.lfo_frequency <= 7,
            'invalid YM2612 LFO settings')
    slices = split_parts(song)
    require(slices, 'all parts are marked SKIP')
    orders = list(range(len(slices)))
    pattern_length = max(len(part.rows) for part in slices)
    # Each source part is a Furnace pattern. A 0D00 on the first padding row
    # advances to the next order, while 0B00 loops after the final order.
    # These navigation effects occupy only new padding rows.
    effect_columns = [2] * 6
    for index, part in enumerate(slices):
        if len(part.rows) < pattern_length:
            code = 0x0B if index == orders[-1] else 0x0D
            part.rows = part.rows + [[Cell(effects=[(code, 0)])] +
                                     [Cell() for _ in range(5)]]
    # Furnace's Genesis chip initializes LFO enabled; XFM stores song-wide
    # enable and frequency. Place 10xy in the first played cell as setup.
    original = slices[orders[0]].rows[0][0]
    first = Cell(original.note, original.instrument, original.volume,
                 original.effects + [(0x10, (song.lfo_enabled << 4) | song.lfo_frequency)])
    slices[orders[0]].rows[0] = [first] + slices[orders[0]].rows[0][1:]
    effect_columns[0] = max(2, len(first.effects))
    remap = {ins.number: index for index, ins in enumerate(song.instruments)}
    require(len(remap) <= 256, 'too many instruments')
    patterns = [encode_pattern(ch, index, part.rows, remap, part.name)
                for ch in range(6) for index, part in enumerate(slices)]
    instruments = [encode_instrument(ins) for ins in song.instruments]
    header = b'-Furnace module-' + u16(VERSION) + b'\0\0' + u32(32) + b'\0' * 8
    body = bytearray()
    body += cstr(song.name) + b'\0' * 7  # 8 metadata strings
    body += f32(440.0) + b'\x01'  # auto system name
    body += f32(1.0) + u16(6) + u16(1)
    body += u16(0x83) + u16(6) + f32(1.0) + f32(0.0) + f32(0.0)
    body += u32(0) + b'\x01'  # auto patchbay
    pointer_groups = []
    def add_group(kind: int, count: int) -> None:
        body.extend(u8(kind) + u32(count))
        positions = []
        for _ in range(count):
            positions.append(len(body))
            body.extend(b'\0' * 4)
        pointer_groups.append(positions)
    add_group(1, 1)
    add_group(4, len(instruments))
    add_group(7, len(patterns))
    body += b'\0'
    inf_size = len(body)
    result = bytearray(header + b'INF2' + u32(inf_size) + body)
    for positions, blocks in zip(pointer_groups,
                                 ([encode_sng2(song, pattern_length, orders, effect_columns)],
                                  instruments, patterns)):
        for pos, item in zip(positions, blocks):
            result[40 + pos:44 + pos] = u32(len(result))
            result += item
    return bytes(result)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--all', action='store_true', help='convert all *.h files in input directory')
    parser.add_argument('input', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    paths = sorted(args.input.glob('*.h')) if args.all else [args.input]
    require(paths, f'no .h songs found in {args.input}')
    if args.all:
        args.output.mkdir(parents=True, exist_ok=True)
    failed = 0
    for path in paths:
        destination = args.output / (path.stem + '.fur') if args.all else args.output
        try:
            song = parse_song(path)
            data = convert(song)
            destination.parent.mkdir(parents=True, exist_ok=True)
            destination.write_bytes(data)
            print(f'{path.name} -> {destination} ({len(song.rows)} rows, '
                  f'{len(song.instruments)} instruments, {len(data)} bytes)')
        except (ConversionError, ValueError) as exc:
            print(f'{path}: {exc}', file=sys.stderr)
            failed += 1
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
