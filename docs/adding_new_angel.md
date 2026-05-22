# Adding a New Angel (Blend → Mesh/Anim → In-Game)

This project’s animated “angel” characters (Angel/Cherub/Seraph) are integrated via:

1) `*.blend` → exported to `*.glb` by Blender CLI  
2) `assman` extracts a **mesh** and an **animation bundle** (`.mesh` + `.anim`)  
3) `xxd -i` embeds those blobs as C headers  
4) `game.cpp` loads blobs into an `AssetMesh` + `AssmanAnimPlayer`

This document describes the minimum steps to add a new angel when you know:

- `blendFile`: e.g. `assets/artwork/myangel.blend`
- `meshName`: the mesh object name inside Blender/GLB (e.g. `MyAngelMesh`)
- `scaleFactor`: the *only* per-angel scale you want to apply (animations are the same clip names)
- Clip names are the same: `BowlingThrow`, `BowlingArgument`

## 1) Asset pipeline (`Makefile`)

Edit `/Users/lape/workspace/bowling/Makefile` and add:

### 1.1 Export `blend` → `glb`

Add a Blender export line (matching existing ones):

- `blender -b assets/artwork/<blendFile>.blend --python-expr "… export_scene.gltf … ./assets/assman_in/<name>.glb …"`

### 1.2 Extract mesh + animation via `assman`

Add:

- `$(ASSMAN) mesh assets/assman_in/<name>.glb <meshName> -o assets/assman_out/<name>.mesh`

Then write a config that selects the mesh and requested clips, and run:

- `$(ASSMAN) animation assets/assman_in/<name>.glb -cfg assets/assman_<name>.conf -o assets/assman_out/<name>.anim`

For “same animations” angels, the config is always:

- `mesh <meshName>`
- `clip BowlingThrow`
- `clip BowlingArgument`

### 1.3 Embed blobs as headers

Add `xxd -i -n …` rules to generate:

- `assets/xxd_mesh/<name>_mesh.h` (symbol `<name>_mesh_data`)
- `assets/xxd_mesh/<name>_anim.h` (symbol `<name>_anim_data`)

## 2) Include the new blobs (`all_assets.h`)

Edit `/Users/lape/workspace/bowling/all_assets.h`:

- Add `#include "assets/xxd_mesh/<name>_mesh.h"`
- Add `#include "assets/xxd_mesh/<name>_anim.h"`

Tip: until you run `make assets`, you can keep placeholder headers (len=0) so the game still compiles.

## 3) Runtime wiring (`game.cpp`)

All runtime integration for bot angels is in `/Users/lape/workspace/bowling/game.cpp`.

### 3.1 Add mesh + anim globals

Add the same pattern as Angel/Cherub/Seraph:

- `static AssetMesh g<Name>Mesh;`
- `static bool g<Name>MeshReady = false;`
- `static AssmanAnimPlayer g<Name>Anim;`
- `static bool g<Name>AnimReady = false;`

### 3.2 Add per-user cached indices + scale

In `struct UserContext`, add:

- Clip indices: `<name>ClipThrow`, `<name>ClipArgument`
- Hand bone indices (optional but recommended): `<name>RightHandBone`, `<name>RightHandTipBone`
- A scale value: `<name>ModelScale = <scaleFactor>`

### 3.3 Add `<Name>_InitIfNeeded(UserContext*)`

Mirror existing `Angel_InitIfNeeded` / `Cherub_InitIfNeeded` / `Seraph_InitIfNeeded`:

- Load mesh from `<name>_mesh_data`
- Load anim from `<name>_anim_data`
- Find clips by name (`BowlingThrow`, `BowlingArgument`)
- Prime `evaluate()` once (optional but recommended)

### 3.4 Add `<Name>_ComputeModelMatrix(const UserContext*)`

For a “normal” angel that should face the same way as the others:

- `return Bot_ComputeModelMatrix_NoScale(usr) * scale(<scaleFactor>)`

Where `<scaleFactor>` is your per-angel scale.

## 4) Selecting the new angel (optional UI)

If you want it selectable through the in-game UI:

- Add it to the bot catalog / carousel (see `bots/` folder)
- Ensure WindowStack has a window and the menu has an opener

(This is not required for the basic mesh+animation integration.)

## 5) Generate assets

Run:

- `make assets`

This regenerates the `assets/xxd_mesh/<name>_{mesh,anim}.h` files with real blob contents.

## Common pitfalls

- **Wrong `meshName`**: `assman mesh … <meshName>` will fail if Blender object name doesn’t match the exported GLB node name.
- **Clip name mismatch**: if the new file doesn’t contain `BowlingThrow` / `BowlingArgument`, `findClipByName` returns `-1`.
- **Scale confusion**: `scaleFactor` should only be used to match world size; rotation/facing should usually stay consistent via `Bot_ComputeModelMatrix_NoScale`.

