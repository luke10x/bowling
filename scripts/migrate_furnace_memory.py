#!/usr/bin/env python3
"""Migrate legacy tick effects and remembered 03xx glides for Furnace timing.

Usage: python3 scripts/migrate_furnace_memory.py old-song.h new-song.h --report changes.txt
Run once, after migrate_portamento.py, on an exported legacy song. It preserves
at most two effects per cell and reports inserted and remapped commands.
"""
from __future__ import annotations
import argparse
import re
from pathlib import Path

START = 'XFM_PATTERN(R"xfmpattern('
END = ')xfmpattern")'
NOTES = {'C-', 'C#', 'D-', 'D#', 'E-', 'F-', 'F#', 'G-', 'G#', 'A-', 'A#', 'B-'}


def convert(text: str) -> tuple[str, list[str]]:
    if START not in text or END not in text:
        raise ValueError('Expected an exported song containing XFM_PATTERN')
    tick_match = re.search(r'XFM_TICK_RATE\((\d+)\)', text)
    speed_match = re.search(r'XFM_SPEED\((\d+)\)', text)
    tick_rate = int(tick_match.group(1)) if tick_match else 60
    ticks_per_row = int(speed_match.group(1)) if speed_match else 6
    before, rest = text.split(START, 1)
    pattern, after = rest.split(END, 1)
    lines = pattern.splitlines(keepends=True)
    remembered = [0] * 6
    arpeggio = [None] * 6
    legacy_arp_active = [False] * 6
    arp_clear_targets = []
    report = []
    part, row = '(opening)', 0
    for i, line in enumerate(lines):
        stripped = line.strip()
        if stripped.startswith(('PART ', 'SKIP ')):
            part, row = stripped[5:], 0
            continue
        if '|' not in line:
            continue
        newline = '\r\n' if line.endswith('\r\n') else '\n' if line.endswith('\n') else ''
        cells = line.removesuffix(newline).split('|')
        for ch, cell in enumerate(cells[:6]):
            slots = [cell[p:p + 4] for p in range(7, len(cell) - 3, 4)]
            effects = [s.upper() for s in slots if re.fullmatch(r'[0-9A-Fa-f]{4}', s)]
            explicit_porta = next((int(fx[2:], 16) for fx in effects if fx[:2] == '03'), None)
            has_note = cell[:2] in NOTES and len(cell) >= 7 and cell[2].isdigit()
            old_arp = next((fx for fx in effects if fx[:2] == '00'), None)
            if has_note and legacy_arp_active[ch] and old_arp is None:
                arp_clear_targets.append((i, ch, part, row))
                legacy_arp_active[ch] = False
            if old_arp is not None:
                legacy_arp_active[ch] = old_arp != '0000'
            if has_note and remembered[ch] and explicit_porta is None:
                inserted = f'03{remembered[ch]:02X}'
                empty = next((j for j, slot in enumerate(slots) if slot == '....'), None)
                duplicate_arp = next((j for j, slot in enumerate(slots)
                                      if slot.upper() == arpeggio[ch] and slot.upper().startswith('00')), None)
                if empty is not None:
                    slots[empty] = inserted
                elif len(slots) < 2:
                    slots.append(inserted)
                elif duplicate_arp is not None:
                    slots[duplicate_arp] = inserted
                else:
                    raise ValueError(f'{part} row {row} channel {ch + 1}: no effect slot for {inserted}')
                cells[ch] = cell[:7] + ''.join(slots)
                report.append(f'{part} row {row}, channel {ch + 1}: inserted {inserted}')
            for fx in effects:
                if fx[:2] == '00':
                    arpeggio[ch] = fx if fx != '0000' else None
            if explicit_porta is not None:
                remembered[ch] = explicit_porta
            updated = cells[ch]
            slots = [updated[p:p + 4] for p in range(7, len(updated) - 3, 4)]
            for j, slot in enumerate(slots):
                if not re.fullmatch(r'[0-9A-Fa-f]{4}', slot):
                    continue
                code, value = slot[:2].upper(), int(slot[2:], 16)
                new = value
                if code in ('01', '02') and value:
                    new = max(1, min(255, round(value * 8 / tick_rate)))
                elif code == '04' and value:
                    rate, depth = value >> 4, value & 15
                    rate = max(0, min(15, round((0.8 + 0.75 * rate) * 64 / tick_rate)))
                    new = (rate << 4) | depth
                elif code == '07' and value:
                    rate, depth = value >> 4, value & 15
                    rate = max(0, min(15, round((0.8 + 0.75 * rate) * 128 / tick_rate)))
                    depth = min(15, depth * 4)
                    new = (rate << 4) | depth
                elif code == '0A' and value:
                    up, down = value >> 4, value & 15
                    if down:
                        down = max(1, min(15, round(down * 4 / ticks_per_row)))
                        new = down
                    else:
                        up = max(1, min(15, round(up * 4 / ticks_per_row)))
                        new = up << 4
                if new != value:
                    slots[j] = f'{code}{new:02X}'
                    report.append(f'{part} row {row}, channel {ch + 1}: {slot} -> {slots[j]}')
            cells[ch] = updated[:7] + ''.join(slots)
        lines[i] = '|'.join(cells) + newline
        row += 1
    # Legacy arpeggio ended on the next plain note. Furnace keeps it until
    # 0000, so insert a stop at that note or the nearest preceding empty row.
    for line_index, ch, clear_part, clear_row in arp_clear_targets:
        inserted = False
        for target_index in range(line_index, -1, -1):
            target_line = lines[target_index]
            if target_index != line_index and target_line.strip().startswith(('PART ', 'SKIP ')):
                break
            if '|' not in target_line:
                continue
            ending = '\r\n' if target_line.endswith('\r\n') else '\n' if target_line.endswith('\n') else ''
            target_cells = target_line.removesuffix(ending).split('|')
            if ch >= len(target_cells):
                continue
            target_cell = target_cells[ch]
            if target_index != line_index and target_cell[:2] in NOTES:
                break
            target_slots = [target_cell[p:p + 4] for p in range(7, len(target_cell) - 3, 4)]
            empty = next((j for j, slot in enumerate(target_slots) if slot == '....'), None)
            if empty is not None:
                target_slots[empty] = '0000'
            elif len(target_slots) < 2:
                target_slots.append('0000')
            else:
                continue
            target_cells[ch] = target_cell[:7] + ''.join(target_slots)
            lines[target_index] = '|'.join(target_cells) + ending
            position = 'same row' if target_index == line_index else 'preceding empty row'
            report.append(f'{clear_part} row {clear_row}, channel {ch + 1}: inserted 0000 ({position})')
            inserted = True
            break
        if not inserted:
            raise ValueError(f'{clear_part} row {clear_row} channel {ch + 1}: no effect slot for 0000')
    return before + START + ''.join(lines) + END + after, report


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    result, changes = convert(args.source.read_text())
    args.output.write_text(result)
    summary = '\n'.join(changes) + ('\n' if changes else '')
    if args.report:
        args.report.write_text(summary)
    print(summary, end='')
    print(f'Made {len(changes)} effect changes.')


if __name__ == '__main__':
    main()
