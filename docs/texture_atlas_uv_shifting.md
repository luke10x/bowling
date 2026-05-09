# Texture Atlas UV Shifting (Bowling)

This project uses a **single texture atlas** (`assets/files/everything_tex.png`) and a shader-side “atlas shift” mechanism to sample different regions of that atlas **without changing mesh UVs**.

This document defines vocabulary, explains the math used by our shader/uniforms, lists common pitfalls we hit while implementing lane “house” textures, and gives prompt examples that would have avoided the back-and-forth.

## Vocabulary (use these terms to be unambiguous)

- **Texture atlas**: One big texture that contains many smaller images (“tiles/cells”).
- **UVs / texture coordinates**: Per-vertex coordinates (`u`, `v`) that index into the texture.
- **Authored UVs**: The UVs baked into the mesh file (from Blender/assman).
- **Atlas cell / tile**: A fixed-size region in the atlas. In our atlas, we often think in **fractions** like `1/8` or `1/16`.
- **Cell coordinates**:
  - **Atlas grid**: conceptual grid like `8×8` where each cell is `1/8` wide and `1/8` tall.
  - **Cell index (Y)**: an integer 0..7 (we need to agree whether “0 is bottom” or “0 is top”; see below).
- **UV origin convention**:
  - **Texture file pixels** are typically described top→bottom.
  - **UV space** in OpenGL/WebGL uses **`v=0` at the bottom**, **`v=1` at the top**.
- **Wrap / repeat** (`GL_REPEAT`): sampling wraps modulo 1.0. Adding an integer to UV (`+1.0`) usually has no visual effect.
- **Atlas shift**: adding a constant `(Δu, Δv)` to the UVs (or to the atlas base) to select another cell.

## What the shader does (industry-standard view)

We render meshes with `ShaderProgram::DEFAULT_FRAGMENT_SHADER` (in `mesh.h`). The key parts are:

1. Start with the mesh UVs:
   - `v_texCoords` from the mesh vertex buffer.
2. Apply a global atlas scaling:
   - `texCoords = v_texCoords * u_atlasScale;`
3. Choose a base “atlas start”:
   - By default, `atlasStart = v_atlasStart` (per-instance attribute).
   - But **we can override** it with the uniform `u_atlasStart` when the instance value is zero:
     - `if (atlasStart.x == 0.0 && u_atlasStart.x != 0.0) atlasStart.x = u_atlasStart.x;`
     - `if (atlasStart.y == 0.0 && u_atlasStart.y != 0.0) atlasStart.y = u_atlasStart.y;`

4. Convert UVs into the final atlas sample UVs:
   - For our common case `u_tileSize = (1,1)` (one “tile” is the whole atlas):
     - `tileStart ≈ atlasStart + floor(texCoords) * 1`
     - `tileUVs ≈ tileStart + fract(texCoords * textureScale) * 1`
   - Because the atlas texture uses `GL_REPEAT`, any integer part is effectively ignored; the **fractional part** selects where we sample.

### Practical consequence

For `u_tileSize = (1,1)` + `GL_REPEAT`:

- **`u_atlasStart` acts like a constant UV offset**, but:
  - It only takes effect if the shader sees `u_atlasStart.* != 0`.
  - The instance `atlasStart` is `0,0` by default, so we can safely use the uniform override.

This means:

- “Neutral” override is typically `u_atlasStart = (1,1)` (or any integer) because it:
  - is non-zero (activates the override path),
  - doesn’t change sampling (repeat makes `+1` neutral).

## How to shift to a specific atlas cell

### Step 1: choose a grid

In this project we commonly use:

- `cellW = 1/8` and `cellH = 1/8` for square cells.
- Ball decals use a different packing (effectively `1/8` wide × `1/16` tall), so their step is `1/16` in `v`.

### Step 2: define cell indexing (THIS is where we kept talking past each other)

There are two competing ways to describe “row index”:

1. **PNG pixel viewpoint** (top row is “row 0”).
2. **UV viewpoint** (bottom row is “row 0” because `v=0` is bottom).

To avoid ambiguity, always state:

- “Row index is counted **from the bottom in UV space**.”
- Or “Row index is counted **from the top in PNG pixel space**.”

### Step 3: compute the `(Δu, Δv)` shift

If a mesh is already authored to a specific atlas column/row and you only need to swap variants in a vertical stack, then the simplest approach is:

- Keep `Δu = 0` (do not move in X).
- Move in Y by exact cell steps:
  - `Δv = +rowIndex * cellH` (when rowIndex is UV-bottom-based).
  - `Δv = -rowIndex * cellH` (when rowIndex is UV-top-based).

In code we implemented lane house variants as:

- `u_atlasStart = (1.0, 1.0 + idx * (1/8))`

Why the `1.0 + …`?

- `1.0` keeps it “neutral” under repeat but **non-zero**, so the uniform override is active.
- `+ idx*(1/8)` walks upward in UV space.

## Lane textures (what mattered in this repo)

### Key fact

The **lane mesh UVs are already authored into the atlas column** that contains the lane backgrounds.

So for lane house variants:

- **X is already correct** in the mesh.
- We only needed to adjust **Y (V)** to pick a different square in the same column.

### The mapping that finally matched the user’s “cell numbers”

User described:

- Count atlas cells **from bottom to top** as `0..7`.
- Houses:
  - House0 should use cell 8-equivalent “default” (conceptually “no shift”).
  - House1 should use cell 7
  - House2 should use cell 6
  - House3 should use cell 5

In implementation terms, this became:

- `idx = house->laneTextureIdx` (`0..3`)
- `u_atlasStart.y = 1.0 + idx*(1/8)`

Because the authored lane UVs already land on the correct “base” cell, `idx` just moves up to the next variants.

## Ball decals (how it differs)

Ball decal selection is *also* an atlas selection, but:

- The decal tiles are packed at a **different aspect** (effectively `1/8` wide × `1/16` tall).
- The code uses:
  - `step = 1/16`
  - `stepx = 1.0 + step * 2.0 * (ballId / 16)`  (two columns per “ball column”)
  - `stepy = 1.0 + step * (ballId % 16)`

The important lesson is:

- Always describe the tile packing as **fractions in UV space** (e.g. `1/16 in V`), not as “8×8” unless the tiles are actually an 8×8 grid for that asset group.

## Pitfalls we hit (and what went wrong)

### 1) Mixing “grid talk” with “authored UV talk”

Mistake:

- Treating the lane like a generic “8×8 remap” when the lane mesh UVs were **already inside the target column**.

Better approach:

- First ask: “Are lane UVs authored to the column already?”
- If yes: only apply the minimal V shift.

### 2) Not specifying row indexing convention

Mistake:

- Saying “row 7” without stating whether that’s “from the top” or “from the bottom”.

Fix:

- Always state: “Rows counted from bottom in UV space (v=0 bottom).”

### 3) The shader only applies `u_atlasStart` when it’s non-zero

Mistake:

- Passing `u_atlasStart = (0, something)` and expecting it to override.

Fix:

- Use `1.0` (or any integer) as a neutral-but-non-zero baseline, e.g. `(1.0, 1.0 + Δv)`.

### 4) Texture file (“top-down”) vs UV (“bottom-up”) mental model mismatch

Mistake:

- Selecting the correct “cell in the PNG” but shifting V in the wrong direction.

Fix:

- Convert PNG-top-based row index to UV-bottom-based row index (or vice versa) explicitly.

### 5) Preview rendering program state (Aurora)

When rendering aurora into a preview FBO and then rendering meshes, ensure the correct shader program is bound before setting uniforms for that shader.

## What you could have said to make this one-shot

Here’s a future prompt that’s unambiguous (good for both humans and AI):

> We have an atlas `everything_tex.png`. The lane top surface UVs are already authored into the lane column (`u` is correct). I need to select 4 lane background variants stacked vertically in the same atlas column.  
> Use the existing shader uniforms (`u_atlasStart`, `u_tileSize=(1,1)`, `GL_REPEAT`) to apply a constant **V offset only**.  
> Define row indexing as **UV space (v=0 bottom, v=1 top)** with `cellH=1/8`.  
> House0: no shift, House1: +1 cell, House2: +2 cells, House3: +3 cells.  
> Keep the override active by using a neutral non-zero baseline: `u_atlasStart = (1.0, 1.0 + idx*cellH)`.  
> Apply the same mapping in both the main lane render and the Houses carousel preview render.

## Quick reference: formulas

### Square cells (8×8 conceptual grid)

- `cell = 1.0 / 8.0`
- UV-bottom-based row selection:
  - `atlasStart = (1.0, 1.0 + row * cell)`
- UV-top-based row selection:
  - `atlasStart = (1.0, 1.0 - row * cell)`

### Rectangular cells (balls)

- `cellV = 1.0 / 16.0` (ball decals)
- `atlasStart = (1.0 + somethingX, 1.0 + somethingY)`

## Where this is used in code

- Lane background variants:
  - `game.cpp` (main lane draw + Houses preview pass)
- Ball decals:
  - `game.cpp` (ball draw using `step = 1/16`)

