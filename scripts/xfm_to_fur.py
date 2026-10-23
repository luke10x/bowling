#!/usr/bin/env python3
"""Build and run the standalone C++ XFM-to-Furnace converter.

Usage: python3 scripts/xfm_to_fur.py song.h song.fur
       python3 scripts/xfm_to_fur.py --all sounds/builtin_songs output_dir
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
    parser.add_argument('input', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
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
    for path in paths:
        destination = args.output / (path.stem + '.fur') if args.all else args.output
        destination.parent.mkdir(parents=True, exist_ok=True)
        if subprocess.run([str(binary), str(path), str(destination)]).returncode:
            failed += 1
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
