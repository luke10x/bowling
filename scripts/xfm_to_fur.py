#!/usr/bin/env python3
"""Build and run the standalone C++ XFM-to-Furnace converter.

Usage: python3 scripts/xfm_to_fur.py song.h song.fur
       python3 scripts/xfm_to_fur.py --all sounds/builtin_songs output_dir
       python3 scripts/xfm_to_fur.py --all --all-levels sounds/builtin_songs output_dir
"""
from __future__ import annotations

import argparse
import os
from pathlib import Path
import subprocess
import sys


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--all', action='store_true', help='convert all *.h files in input directory')
    parser.add_argument('--player-match', action='store_true',
                        help='alias for --master-volume 0.25 (game default gain)')
    parser.add_argument('--master-volume', choices=('1.0', '0.5', '0.25'), default=None,
                        help='Furnace master volume; YM3438 is always selected (default: 1.0)')
    parser.add_argument('--all-levels', action='store_true',
                        help='with --all, export 1.0, 0.5, and 0.25 master volumes')
    parser.add_argument('input', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    if args.all_levels and (not args.all or args.master_volume or args.player_match):
        parser.error('--all-levels requires --all and cannot be combined with a volume choice')
    if args.player_match and args.master_volume:
        parser.error('--player-match and --master-volume cannot be combined')
    candidates = [Path(__file__).resolve().parents[1], *args.input.resolve().parents]
    root = next((path for path in candidates
                 if (path / 'Makefile.mac').is_file()
                 and (path / 'scripts/xfm_to_fur.cpp').is_file()
                 and (path / 'tracker/xfm_furnace_export.cpp').is_file()
                 and (path.parent / 'eggsfm/xfm_api.h').is_file()), None)
    if root is None:
        parser.error('cannot find the bowling repository and its Makefile.mac')
    binary = root / 'build/macos/bin/xfm_to_fur'
    subprocess.run(['make', '-f', 'Makefile.mac', 'xfm-to-fur'], cwd=root,
                   env={**os.environ, 'PWD': str(root)}, check=True)
    paths = sorted(args.input.glob('*.h')) if args.all else [args.input]
    if not paths:
        parser.error(f'no .h songs found in {args.input}')
    if args.all:
        args.output.mkdir(parents=True, exist_ok=True)
    failed = 0
    levels = ('1.0', '0.5', '0.25') if args.all_levels else (
        '0.25' if args.player_match else args.master_volume or '1.0',)
    for path in paths:
        for level in levels:
            suffix = '-ym3438-master-' + level.replace('.', '_')
            destination = args.output / (path.stem + suffix + '.fur') if args.all else args.output
            destination.parent.mkdir(parents=True, exist_ok=True)
            command = [str(binary), '--master-volume', level, str(path), str(destination)]
            if subprocess.run(command).returncode:
                failed += 1
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
