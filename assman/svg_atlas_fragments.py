#!/usr/bin/env python3
"""Inspect SVG atlas fragments using Inkscape's geometry query CLI.

The atlas export in `make assets` uses Inkscape to render `exportroot` from
`everything_tex.svg` into `everything_tex.png`. This helper resolves SVG ids or
Inkscape labels, asks Inkscape for rendered bounds, and reports the matching
pixel and UV rectangles.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import struct
import subprocess
import sys
import xml.etree.ElementTree as ET
from dataclasses import asdict, dataclass
from pathlib import Path


DEFAULT_SVG = Path("assets/artwork/everything_tex.svg")
DEFAULT_PNG = Path("assets/files/everything_tex.png")
DEFAULT_INKSCAPE = "/Applications/Inkscape.app/Contents/MacOS/inkscape"
INKSCAPE_LABEL = "{http://www.inkscape.org/namespaces/inkscape}label"


@dataclass
class Fragment:
    name: str
    svg_id: str
    label: str | None
    px: dict[str, float]
    px_rounded: dict[str, int]
    clay_uv: dict[str, float]
    gl_uv: dict[str, float]
    cpp_name: str
    cpp_config: str


def parse_number(value: str) -> float:
    match = re.match(r"\s*([-+]?[0-9]*\.?[0-9]+)", value)
    if not match:
        raise ValueError(f"cannot parse numeric value from {value!r}")
    return float(match.group(1))


def png_size(path: Path) -> tuple[int, int]:
    with path.open("rb") as fh:
        header = fh.read(24)
    if len(header) < 24 or header[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError(f"{path} is not a PNG file")
    width, height = struct.unpack(">II", header[16:24])
    return width, height


def svg_size(path: Path) -> tuple[int, int]:
    root = ET.parse(path).getroot()
    return round(parse_number(root.attrib["width"])), round(parse_number(root.attrib["height"]))


def element_id(element: ET.Element) -> str | None:
    return element.attrib.get("id")


def element_label(element: ET.Element) -> str | None:
    return element.attrib.get(INKSCAPE_LABEL)


def resolve_node(svg: Path, name: str) -> tuple[str, str | None]:
    root = ET.parse(svg).getroot()
    by_id: dict[str, ET.Element] = {}
    by_label: dict[str, list[ET.Element]] = {}

    for element in root.iter():
        node_id = element_id(element)
        if node_id:
            by_id[node_id] = element
        label = element_label(element)
        if label:
            by_label.setdefault(label, []).append(element)

    if name in by_id:
        element = by_id[name]
        return name, element_label(element)

    matches = by_label.get(name, [])
    if not matches:
        raise KeyError(f"no SVG id or Inkscape label found for {name!r}")
    if len(matches) > 1:
        ids = ", ".join(sorted(filter(None, (element_id(e) for e in matches))))
        raise KeyError(f"label {name!r} is ambiguous; matching ids: {ids}")

    node_id = element_id(matches[0])
    if not node_id:
        raise KeyError(f"label {name!r} matched an element without an id")
    return node_id, name


def query_bounds(inkscape: str, svg: Path, node_id: str) -> tuple[float, float, float, float]:
    cmd = [
        inkscape,
        f"--query-id={node_id}",
        "--query-x",
        "--query-y",
        "--query-width",
        "--query-height",
        str(svg),
    ]
    result = subprocess.run(cmd, check=True, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    values = [float(line.strip()) for line in result.stdout.splitlines() if line.strip()]
    if len(values) != 4:
        raise RuntimeError(f"Inkscape returned unexpected query output for {node_id!r}: {result.stdout!r}")
    return values[0], values[1], values[2], values[3]


def cpp_identifier(name: str) -> str:
    parts = re.findall(r"[A-Za-z0-9]+", name)
    if not parts:
        return "kAtlasFragment"
    return "k" + "".join(part[:1].upper() + part[1:] for part in parts) + "Image"


def make_fragment(name: str, svg_id: str, label: str | None, bounds: tuple[float, float, float, float], atlas_size: tuple[int, int], texture_slot: int) -> Fragment:
    atlas_w, atlas_h = atlas_size
    x, y, w, h = bounds
    x1 = x + w
    y1 = y + h

    clay_uv = {
        "u0": x / atlas_w,
        "v0": y / atlas_h,
        "u1": x1 / atlas_w,
        "v1": y1 / atlas_h,
    }
    gl_uv = {
        "u0": x / atlas_w,
        "v0": 1.0 - y1 / atlas_h,
        "u1": x1 / atlas_w,
        "v1": 1.0 - y / atlas_h,
    }
    rounded = {
        "x": round(x),
        "y": round(y),
        "w": round(w),
        "h": round(h),
        "x1": round(x1),
        "y1": round(y1),
    }
    cpp_name = cpp_identifier(name)
    cpp_config = (
        f"static constexpr Gles3_ImageConfig {cpp_name}{{"
        f".textureToUse = {texture_slot}, "
        f".u0 = {clay_uv['u0']:.9g}f, .v0 = {clay_uv['v0']:.9g}f, "
        f".u1 = {clay_uv['u1']:.9g}f, .v1 = {clay_uv['v1']:.9g}f}};"
    )

    return Fragment(
        name=name,
        svg_id=svg_id,
        label=label,
        px={"x": x, "y": y, "w": w, "h": h, "x1": x1, "y1": y1},
        px_rounded=rounded,
        clay_uv=clay_uv,
        gl_uv=gl_uv,
        cpp_name=cpp_name,
        cpp_config=cpp_config,
    )


def print_text(fragments: list[Fragment], atlas_size: tuple[int, int]) -> None:
    print(f"atlas: {atlas_size[0]}x{atlas_size[1]} px")
    print("coordinate note: Inkscape query bounds are top-left pixel coordinates in the exported PNG.")
    print("Clay image UVs keep that top-left V convention; GL/decal UVs flip V for bottom-left UV space.")
    for frag in fragments:
        print()
        label = f", label={frag.label}" if frag.label else ""
        print(f"{frag.name}: id={frag.svg_id}{label}")
        print(
            "  px: "
            f"x={frag.px['x']:.6g}, y={frag.px['y']:.6g}, "
            f"w={frag.px['w']:.6g}, h={frag.px['h']:.6g}, "
            f"x1={frag.px['x1']:.6g}, y1={frag.px['y1']:.6g}"
        )
        print(
            "  px rounded: "
            f"x={frag.px_rounded['x']}, y={frag.px_rounded['y']}, "
            f"w={frag.px_rounded['w']}, h={frag.px_rounded['h']}, "
            f"x1={frag.px_rounded['x1']}, y1={frag.px_rounded['y1']}"
        )
        print(
            "  clay uv: "
            f"u0={frag.clay_uv['u0']:.9g}, v0={frag.clay_uv['v0']:.9g}, "
            f"u1={frag.clay_uv['u1']:.9g}, v1={frag.clay_uv['v1']:.9g}"
        )
        print(
            "  gl/decal uv: "
            f"u0={frag.gl_uv['u0']:.9g}, v0={frag.gl_uv['v0']:.9g}, "
            f"u1={frag.gl_uv['u1']:.9g}, v1={frag.gl_uv['v1']:.9g}"
        )
        print(f"  cpp: {frag.cpp_config}")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Extract pixel/UV atlas fragment data from an Inkscape SVG node id or label."
    )
    parser.add_argument("nodes", nargs="+", help="SVG ids or Inkscape labels to inspect")
    parser.add_argument("--svg", type=Path, default=DEFAULT_SVG, help=f"SVG atlas source, default: {DEFAULT_SVG}")
    parser.add_argument("--png", type=Path, default=DEFAULT_PNG, help=f"exported PNG atlas, default: {DEFAULT_PNG}")
    parser.add_argument(
        "--inkscape",
        default=os.environ.get("INKSCAPE", DEFAULT_INKSCAPE),
        help="Inkscape CLI path; also honors INKSCAPE env var",
    )
    parser.add_argument("--texture-slot", type=int, default=0, help="texture slot for generated Gles3_ImageConfig")
    parser.add_argument("--format", choices=("text", "json", "cpp"), default="text")
    args = parser.parse_args()

    atlas_size = png_size(args.png) if args.png.exists() else svg_size(args.svg)
    fragments: list[Fragment] = []

    for name in args.nodes:
        svg_id, label = resolve_node(args.svg, name)
        bounds = query_bounds(args.inkscape, args.svg, svg_id)
        fragments.append(make_fragment(name, svg_id, label, bounds, atlas_size, args.texture_slot))

    if args.format == "json":
        print(json.dumps({"atlas": {"w": atlas_size[0], "h": atlas_size[1]}, "fragments": [asdict(f) for f in fragments]}, indent=2))
    elif args.format == "cpp":
        for frag in fragments:
            print(frag.cpp_config)
    else:
        print_text(fragments, atlas_size)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
