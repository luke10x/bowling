#!/usr/bin/env python3
"""Transpose XFM DSL note tokens in-place or to stdout.

This intentionally edits only tracker-style note tokens such as C-4 or F#7.
Control tokens like OFF, ===, REL, hex effects, and macro names are left alone.
"""

from __future__ import annotations

import argparse
from pathlib import Path
import re
import sys


NOTE_RE = re.compile(r"(?<![A-Za-z0-9_])([A-G])([#-])([0-9])")
NOTE_TO_SEMITONE = {
    "C-": 0,
    "C#": 1,
    "D-": 2,
    "D#": 3,
    "E-": 4,
    "F-": 5,
    "F#": 6,
    "G-": 7,
    "G#": 8,
    "A-": 9,
    "A#": 10,
    "B-": 11,
}
SEMITONE_TO_NOTE = ("C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-")


def transpose_text(text: str, semitones: int, path: Path) -> tuple[str, int]:
    changed = 0

    def replace(match: re.Match[str]) -> str:
        nonlocal changed
        note = match.group(1) + match.group(2)
        octave = int(match.group(3))
        absolute = octave * 12 + NOTE_TO_SEMITONE[note] + semitones
        if absolute < 0:
            raise ValueError(f"{path}: transposing {match.group(0)} by {semitones} semitones goes below octave 0")
        new_octave = absolute // 12
        if new_octave > 9:
            raise ValueError(f"{path}: transposing {match.group(0)} by {semitones} semitones goes above octave 9")
        changed += 1
        new_note = SEMITONE_TO_NOTE[absolute % 12]
        return f"{new_note}{new_octave}"

    return NOTE_RE.sub(replace, text), changed


def main() -> int:
    parser = argparse.ArgumentParser(description="Transpose tracker note tokens in XFM DSL files.")
    parser.add_argument("paths", nargs="+", type=Path)
    parser.add_argument("--semitones", type=int, required=True)
    parser.add_argument("--in-place", action="store_true")
    args = parser.parse_args()

    total = 0
    for path in args.paths:
        original = path.read_text()
        updated, count = transpose_text(original, args.semitones, path)
        total += count
        if args.in_place:
            path.write_text(updated)
        else:
            sys.stdout.write(updated)
        print(f"{path}: transposed {count} notes", file=sys.stderr)

    print(f"total: transposed {total} notes", file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
