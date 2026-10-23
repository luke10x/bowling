#!/usr/bin/env python3
"""Convert legacy 03xx portamento speeds in an exported tracker song.

Usage:
  python3 scripts/migrate_portamento.py old-song.h converted-song.h --report changes.txt

The old player completed every glide in about 16/xx rows, regardless of its
interval. Furnace's default linear-pitch YM2612 mode instead moves xx*4/128
semitones per tracker tick. For each command, use its current or next note
interval to choose a new speed with approximately the same glide duration.
No effects are added, so existing effect columns remain untouched.

Run this only once on a song saved with the old portamento speed model.
"""

from __future__ import annotations

import argparse
import re
from pathlib import Path

NOTE_INDEX = {"C-": 0, "C#": 1, "D-": 2, "D#": 3, "E-": 4, "F-": 5,
              "F#": 6, "G-": 7, "G#": 8, "A-": 9, "A#": 10, "B-": 11}
PATTERN_START = 'XFM_PATTERN(R"xfmpattern('
PATTERN_END = ')xfmpattern")'


def note_number(cell: str) -> int | None:
    return (int(cell[2]) * 12 + NOTE_INDEX[cell[:2]]
            if len(cell) >= 3 and cell[:2] in NOTE_INDEX and cell[2].isdigit()
            else None)


def convert(text: str) -> tuple[str, list[str]]:
    speed_match = re.search(r"XFM_SPEED\((\d+)\)", text)
    ticks_per_row = int(speed_match.group(1)) if speed_match else 6
    if ticks_per_row <= 0:
        raise ValueError("Song speed must be positive")
    if PATTERN_START not in text or PATTERN_END not in text:
        raise ValueError("Expected an exported song containing XFM_PATTERN")
    before, rest = text.split(PATTERN_START, 1)
    pattern, after = rest.split(PATTERN_END, 1)
    lines = pattern.splitlines(keepends=True)
    rows: list[tuple[int, str, int, list[str]]] = []
    part, local_row = "(opening)", 0
    for line_index, line in enumerate(lines):
        stripped = line.strip()
        if stripped.startswith(("PART ", "SKIP ")):
            part, local_row = stripped[5:], 0
        elif "|" in line:
            rows.append((line_index, part, local_row, line.rstrip("\r\n").split("|")))
            local_row += 1

    previous: list[int | None] = [None] * 6
    report: list[str] = []
    for row_index, (line_index, part, local_row, cells) in enumerate(rows):
        for channel, cell in enumerate(cells):
            pitch = note_number(cell)
            source = previous[channel]
            if pitch is not None:
                previous[channel] = pitch
            effects = [
                pos for pos in range(7, len(cell) - 3, 4)
                if re.fullmatch(r"[0-9A-Fa-f]{4}", cell[pos:pos + 4])
                and cell[pos:pos + 2].upper() == "03"
            ]
            if not effects:
                continue
            for pos in reversed(effects):
                old = int(cell[pos + 2:pos + 4], 16)
                if old == 0:
                    continue
                target = pitch
                if target is None:
                    for _, _, _, future_cells in rows[row_index + 1:]:
                        if channel >= len(future_cells):
                            break
                        target = note_number(future_cells[channel])
                        if target is not None:
                            break
                if source is None or target is None:
                    interval = 1
                    qualifier = "fallback 1 semitone"
                else:
                    interval = max(1, abs(target - source))
                    qualifier = f"{interval} semitones"
                raw = round(old * interval * 2 / ticks_per_row)
                new = max(1, min(255, raw))
                cell = cell[:pos + 2] + f"{new:02X}" + cell[pos + 4:]
                if raw > 255:
                    qualifier += "; capped at 03FF"
                report.append(
                    f"{part} row {local_row}, channel {channel + 1}: "
                    f"03{old:02X} -> 03{new:02X} ({qualifier})"
                )
            cells[channel] = cell
        ending = "\n" if lines[line_index].endswith("\n") else ""
        lines[line_index] = "|".join(cells) + ending
    return before + PATTERN_START + "".join(lines) + PATTERN_END + after, report


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    result, changes = convert(args.source.read_text())
    args.output.write_text(result)
    summary = "\n".join(changes) + ("\n" if changes else "")
    if args.report:
        args.report.write_text(summary)
    print(summary, end="")
    print(f"Converted {len(changes)} portamento commands.")


if __name__ == "__main__":
    main()
