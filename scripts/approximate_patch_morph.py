#!/usr/bin/env python3
"""Replace EExx patch morphs with sparse, ordinary OPN tracker effects.

Usage: approximate_morph.py old-song.h output-song.h [--report report.txt]

The planner preserves existing effects, uses no more than two per cell, and
approximates the old tick-by-tick patch path with weighted parameter writes.
It deliberately leaves the morph target instrument out of the start cell:
EExx used that instrument as a target while the old instrument kept playing.
"""

from __future__ import annotations

import argparse
import copy
import re
from dataclasses import dataclass
from pathlib import Path

FIELDS = ("TL", "AR", "DR", "SL", "SR", "RR", "SSG", "MUL", "DT", "RS", "AM")
RANGES = {"TL": 127, "AR": 31, "DR": 31, "SL": 15, "SR": 31,
          "RR": 15, "SSG": 8, "MUL": 15, "DT": 7, "RS": 3, "AM": 1}
WEIGHTS = {"ALG": 110, "FB": 15, "AMS": 5, "FMS": 5,
           "TL": 2.8, "AR": 1.5, "DR": 0.8, "SL": 1.2,
           "SR": 0.8, "RR": 0.9, "SSG": 7, "MUL": 11,
           "DT": 6, "RS": 2, "AM": 7}
DT_ENCODE = {-3: 2, -2: 1, -1: 0, 0: 3, 1: 4, 2: 5, 3: 6, 4: 7}
DT_DECODE = {code: value for value, code in DT_ENCODE.items()}
PATTERN_START = 'XFM_PATTERN(R"xfmpattern('
PATTERN_END = ')xfmpattern")'


@dataclass(frozen=True)
class Morph:
    row: int
    channel: int
    source: int
    target: int
    speed: int
    part: str
    local_row: int


def parse_instruments(text: str) -> dict[int, dict]:
    instruments = {}
    current = None
    for line in text.splitlines():
        line = line.strip()
        match = re.fullmatch(r"INST ([0-9A-Fa-f]{2})", line)
        if match:
            current = {"ops": [None] * 4}
            instruments[int(match.group(1), 16)] = current
        elif line == "ENDINST":
            current = None
        elif current is not None and line.startswith("PATCH "):
            values = [int(value) for value in line.split()[1:5]]
            current.update(zip(("ALG", "FB", "AMS", "FMS"), values))
        elif current is not None and re.match(r"FM [1-4] +[-0-9]", line):
            values = [int(value) for value in line.split()[1:]]
            op = values[0] - 1
            current["ops"][op] = dict(zip(FIELDS, values[1:]))
    return {key: value for key, value in instruments.items()
            if all(op is not None for op in value["ops"])}


def split_effects(cell: str) -> tuple[str, list[tuple[int, int]]]:
    prefix = cell[:7].ljust(7, ".")
    effects = []
    for pos in range(7, len(cell) - 3, 4):
        chunk = cell[pos:pos + 4]
        if not re.fullmatch(r"[0-9A-Fa-f]{4}", chunk):
            break
        effects.append((int(chunk[:2], 16), int(chunk[2:], 16)))
    return prefix, effects


def join_cell(prefix: str, effects: list[tuple[int, int]], original_width: int) -> str:
    result = prefix + "".join(f"{code:02X}{value:02X}" for code, value in effects)
    return result.ljust(max(7, original_width), ".")


def old_state(source: dict, target: dict, speed: int, ticks: int) -> dict:
    state = copy.deepcopy(source)
    state["ALG"] = target["ALG"]
    for op in range(4):
        state["ops"][op]["AM"] = target["ops"][op]["AM"]
        state["ops"][op]["SSG"] = target["ops"][op]["SSG"]
    steps = ticks * speed // 255
    for field in ("FB", "AMS", "FMS"):
        start, end = source[field], target[field]
        state[field] = start + max(-steps, min(steps, end - start))
    for op in range(4):
        for field in FIELDS:
            if field in ("AM", "SSG"):
                continue
            start, end = source["ops"][op][field], target["ops"][op][field]
            state["ops"][op][field] = start + max(-steps, min(steps, end - start))
    return state


def distance(a: dict, b: dict) -> float:
    total = sum(WEIGHTS[field] * abs(a[field] - b[field])
                for field in ("ALG", "FB", "AMS", "FMS"))
    for op in range(4):
        for field in FIELDS:
            weight = WEIGHTS[field]
            if field == "TL" and a["ALG"] == 7:
                weight *= 1.5  # all operators are audible carriers
            total += weight * abs(a["ops"][op][field] - b["ops"][op][field])
    return total


def candidate_effects(state: dict, desired: dict) -> list[tuple[int, int]]:
    effects = []
    for field, code in (("ALG", 0x61), ("FB", 0x11), ("AMS", 0x63), ("FMS", 0x62)):
        if state[field] != desired[field]:
            effects.append((code, desired[field]))
    for op in range(4):
        left, right = state["ops"][op], desired["ops"][op]
        for field in FIELDS:
            if left[field] == right[field]:
                continue
            value = right[field]
            if field == "TL": effects.append((0x12 + op, value))
            elif field == "AR": effects.append((0x1A + op, value))
            elif field == "DR": effects.append((0x57 + op, value))
            elif field == "SR": effects.append((0x5C + op, value))
            elif field == "MUL": effects.append((0x16, ((op + 1) << 4) | value))
            elif field == "DT" and value in DT_ENCODE:
                effects.append((0x53, ((op + 1) << 4) | DT_ENCODE[value]))
            elif field == "SSG":
                effects.append((0x55, ((op + 1) << 4) | (value - 1 if value else 8)))
            elif field in ("AM", "SL", "RR", "RS"):
                code = {"AM": 0x50, "SL": 0x51, "RR": 0x52, "RS": 0x54}[field]
                effects.append((code, ((op + 1) << 4) | value))
    return effects


def apply_effect(state: dict, effect: tuple[int, int]) -> None:
    code, value = effect
    for name, field_code in (("ALG", 0x61), ("FB", 0x11), ("AMS", 0x63), ("FMS", 0x62)):
        if code == field_code:
            state[name] = value
            return
    op = (value >> 4) - 1
    if 0x12 <= code <= 0x15:
        state["ops"][code - 0x12]["TL"] = value
    elif 0x1A <= code <= 0x1D:
        state["ops"][code - 0x1A]["AR"] = value
    elif 0x57 <= code <= 0x5A:
        state["ops"][code - 0x57]["DR"] = value
    elif 0x5C <= code <= 0x5F:
        state["ops"][code - 0x5C]["SR"] = value
    elif code == 0x16 and 0 <= op < 4:
        state["ops"][op]["MUL"] = value & 15
    elif code in (0x50, 0x51, 0x52, 0x53, 0x54, 0x55) and 0 <= op < 4:
        field = {0x50: "AM", 0x51: "SL", 0x52: "RR", 0x53: "DT",
                 0x54: "RS", 0x55: "SSG"}[code]
        nibble = value & 15
        state["ops"][op][field] = (DT_DECODE[nibble] if code == 0x53 else
                                    (nibble + 1 if nibble <= 7 else 0) if code == 0x55 else
                                    nibble)


def parse_rows(pattern: str) -> tuple[list[str], list[tuple[str, int, int]]]:
    lines = pattern.splitlines(keepends=True)
    rows = []
    part = "(opening)"
    local = 0
    for index, line in enumerate(lines):
        stripped = line.strip()
        if stripped.startswith("PART "):
            part, local = stripped[5:], 0
        elif "|" in line and not stripped.startswith(("SKIP ", "PART_COLOR")):
            rows.append((part, local, index))
            local += 1
    return lines, rows


def transform(text: str) -> tuple[str, list[str]]:
    instruments = parse_instruments(text)
    if not instruments:
        raise ValueError("No FM instruments found")
    before, remainder = text.split(PATTERN_START, 1)
    pattern, after = remainder.split(PATTERN_END, 1)
    lines, rows = parse_rows(pattern)
    cells = [lines[index].rstrip("\n").split("|") for _, _, index in rows]
    selected = [None] * 6
    morphs = []
    cancellations = set()
    tick_match = re.search(r"XFM_SPEED\((\d+)\)", text)
    ticks_per_row = int(tick_match.group(1)) if tick_match else 6

    for row, (part, local, _) in enumerate(rows):
        for ch, cell in enumerate(cells[row]):
            prefix, effects = split_effects(cell)
            instrument = int(prefix[3:5], 16) if re.fullmatch(r"[0-9A-Fa-f]{2}", prefix[3:5]) else None
            starts = [value for code, value in effects if code == 0xEE and value]
            if (0xEE, 0) in effects:
                cancellations.add((row, ch))
            if starts:
                if instrument is None:
                    raise ValueError(f"Morph has no target instrument at row {row}, channel {ch + 1}")
                source = selected[ch] if selected[ch] is not None else instrument
                if source not in instruments or instrument not in instruments:
                    raise ValueError(f"Missing FM instrument {source:02X} or {instrument:02X}")
                morphs.append(Morph(row, ch, source, instrument, starts[-1], part, local))
                prefix = prefix[:3] + ".." + prefix[5:]
            elif instrument is not None:
                selected[ch] = instrument
            if any(code == 0xEE for code, _ in effects):
                cells[row][ch] = join_cell(prefix, [(code, value) for code, value in effects if code != 0xEE], len(cell))

    report = []
    for morph in morphs:
        row, ch = morph.row, morph.channel
        source = instruments[morph.source]
        target = instruments[morph.target]
        state = copy.deepcopy(source)
        # Keep filling free slots after the original morph reaches its target:
        # a sparse approximation may still have parameters left to correct.
        stop = min(len(rows), row + 48)
        for later in range(row + 1, stop):
            prefix, _ = split_effects(cells[later][ch])
            # The old morph kept advancing while a channel was released or
            # silent; its live patch could be heard again on a later note.
            if (later, ch) in cancellations:
                stop = later
                break
            if (re.fullmatch(r"[0-9A-Fa-f]{2}", prefix[3:5])
                    and int(prefix[3:5], 16) != morph.source):
                stop = later
                break
        chosen = []
        baseline = 0.0
        optimized = 0.0
        for current_row in range(row, stop):
            desired = old_state(source, target, morph.speed, (current_row - row + 1) * ticks_per_row)
            baseline += distance(source, desired)
            prefix, existing = split_effects(cells[current_row][ch])
            if len(existing) > 2:
                raise ValueError(f"Already over two effects at row {current_row}, channel {ch + 1}")
            for effect in existing:
                apply_effect(state, effect)
            while len(existing) < 2:
                horizon = [old_state(source, target, morph.speed, (r - row + 1) * ticks_per_row)
                           for r in range(current_row, min(stop, current_row + 4))]
                best = None
                best_gain = 0.0
                for effect in candidate_effects(state, desired):
                    trial = copy.deepcopy(state)
                    apply_effect(trial, effect)
                    gain = sum(distance(state, future) - distance(trial, future) for future in horizon)
                    if gain > best_gain + 1e-6:
                        best, best_gain = effect, gain
                if best is None:
                    break
                existing.append(best)
                apply_effect(state, best)
                chosen.append((current_row - row, best))
            cells[current_row][ch] = join_cell(prefix, existing, len(cells[current_row][ch]))
            optimized += distance(state, desired)
        command_text = ", ".join(f"+{offset}:{code:02X}{value:02X}" for offset, (code, value) in chosen)
        report.append(f"{morph.part} row {morph.local_row}, ch {ch + 1}: "
                      f"{morph.source:02X} -> {morph.target:02X}, "
                      f"{stop - row} rows, weighted error {baseline:.0f} -> {optimized:.0f}; "
                      f"{command_text}")

    for row, (_, _, line_index) in enumerate(rows):
        ending = "\n" if lines[line_index].endswith("\n") else ""
        lines[line_index] = "|".join(cells[row]) + ending
    result = before + PATTERN_START + "".join(lines) + PATTERN_END + after
    return result, report


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    result, report = transform(args.source.read_text())
    args.output.write_text(result)
    text = "\n".join(report) + "\n"
    if args.report:
        args.report.write_text(text)
    print(text, end="")


if __name__ == "__main__":
    main()
