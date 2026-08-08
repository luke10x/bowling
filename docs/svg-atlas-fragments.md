SVG Atlas Fragments
===================

The Clay renderer has a small fixed set of texture slots, so UI images that live
in `assets/files/everything_tex.png` should usually be sampled from the main SVG
atlas instead of loaded as separate textures.

Use `assman/svg_atlas_fragments.py` to inspect an SVG node and get its atlas
pixel bounds, UVs, and optional `Gles3_ImageConfig` C++ initializer.

Typical usage:

```sh
assman/svg_atlas_fragments.py coin-with-cir diamond --texture-slot 0
assman/svg_atlas_fragments.py coin-with-cir diamond --format cpp --texture-slot 0
assman/svg_atlas_fragments.py coin-with-cir diamond --format json
```

The node argument can be either a real SVG `id` or an Inkscape label. This is
important because many hand-named objects in `everything_tex.svg` are labels,
not ids. For example:

```text
coin-with-cir -> g13
diamond       -> g14
```

Inkscape
--------

The helper uses the same Inkscape CLI path as the asset export by default:

```sh
/Applications/Inkscape.app/Contents/MacOS/inkscape
```

Override it with the environment or a flag:

```sh
INKSCAPE=/path/to/inkscape assman/svg_atlas_fragments.py diamond
assman/svg_atlas_fragments.py diamond --inkscape /path/to/inkscape
```

Coordinate Systems
------------------

`make assets` exports the atlas like this:

```sh
inkscape assets/artwork/everything_tex.svg \
    --export-id=exportroot \
    --export-id-only \
    --export-area-page \
    --export-type=png \
    --export-filename=assets/files/everything_tex.png
```

The SVG has a `2048x2048` page export. Inkscape's query output for this file
comes back in exported PNG pixel coordinates with the origin at the top-left.

For Clay images using `Gles3_ImageConfig`, use the reported `clay uv` values.
Those match the renderer's vertex shader path:

```cpp
vUV = mix(aUV.xy, aUV.zw, aPos);
```

For world decals or other OpenGL-style UV code that expects bottom-left UV
space, use the reported `gl/decal uv` values instead. They keep U the same and
flip the V interval:

```text
gl_v0 = 1 - clay_v1
gl_v1 = 1 - clay_v0
```

Verified Fragments
------------------

These were queried from `assets/artwork/everything_tex.svg` against
`assets/files/everything_tex.png` at `2048x2048`.

`coin-with-cir`:

```text
id: g13
px: x=1536, y=1538.55, w=122.534, h=125.45
clay uv: u0=0.75, v0=0.751245117, u1=0.809831055, v1=0.8125
gl/decal uv: u0=0.75, v0=0.1875, u1=0.809831055, v1=0.248754883
```

```cpp
static constexpr Gles3_ImageConfig kCoinWithCirImage{.textureToUse = 0, .u0 = 0.75f, .v0 = 0.751245117f, .u1 = 0.809831055f, .v1 = 0.8125f};
```

`diamond`:

```text
id: g14
px: x=1680.97, y=1541.45, w=94.2054, h=124.014
clay uv: u0=0.820786133, v0=0.752661133, u1=0.866784863, v1=0.813214844
gl/decal uv: u0=0.820786133, v0=0.186785156, u1=0.866784863, v1=0.247338867
```

```cpp
static constexpr Gles3_ImageConfig kDiamondImage{.textureToUse = 0, .u0 = 0.820786133f, .v0 = 0.752661133f, .u1 = 0.866784863f, .v1 = 0.813214844f};
```

Useful flags:

```text
--format text|json|cpp
--texture-slot N
--svg assets/artwork/everything_tex.svg
--png assets/files/everything_tex.png
```
