# Memory Model + Hot Reload Contract

This project is built around a **stable, long-lived `UserContext`** that survives code hot-reloads. This document describes the contract so modules can be extracted safely without breaking hot reload.

## Architecture overview

### Core types

- `vtx::VertexContext` (`/Users/lape/workspace/bowling/framework/boot.h`)
  - Owned by the framework (static `g_ctx` in `boot.cpp`).
  - Contains:
    - SDL window + GL context
    - screen dimensions
    - `void *usrptr` → pointer to gameplay state.

- `UserContext` (`/Users/lape/workspace/bowling/game.cpp`)
  - Owned by the game layer.
  - Stored behind `ctx->usrptr`.
  - Must remain **layout-stable** across hot reload.

### Lifecycle entry points

The framework calls into the game through 4 exported functions (`init`, `loop`, `hang`, `load`) via the shim:

- `/Users/lape/workspace/bowling/framework/api_shim.cpp`:
  - `extern "C" init(void*)` → `vtx::init`
  - `extern "C" loop(void*)` → `vtx::loop`
  - `extern "C" hang(void*)` → `vtx::hang`
  - `extern "C" load(void*)` → `vtx::load`

### HOT_RUNTIME behavior (the important part)

In HOT_RUNTIME builds, the framework loads a swappable shared library (`hot_runtime.so`) and:

- calls `init()` exactly once on startup,
- then calls `loop()` every frame,
- and on a detected code change it:
  - dlopens the new `.so`,
  - resolves the function pointers again,
  - calls `load(&g_ctx)` **without reinitializing** the `VertexContext`,
  - continues calling the new `loop()`.

This means:

- `ctx->usrptr` is expected to keep pointing at the same persistent `UserContext`.
- Hot-reload is “code-only”; **data memory is preserved**.

## Allocation model (what actually exists today)

### VertexContext

- `VertexContext g_ctx` is static in `/Users/lape/workspace/bowling/framework/boot.cpp`.
- It is never reallocated during hot reload.

### UserContext

In the current code, `UserContext` is allocated once by game code:

- `/Users/lape/workspace/bowling/game.cpp` does:
  - `ctx->usrptr = new UserContext;`

After that:

- hot reload never changes `ctx->usrptr`,
- `load()` is responsible for reloading GPU resources / shader programs / UI contexts as needed.

If you later decide to move `UserContext` allocation into the framework (preallocated arena / static storage),
that’s compatible with the model **as long as**:

- `ctx->usrptr` points to the same address for the whole session, and
- game code never replaces it.

## The hot-reload invariants (hard rules)

### 1) `UserContext` memory layout must not change

Because hot reload keeps the same `UserContext*` but swaps code, you must avoid changes that alter:

- field ordering
- field types/sizes
- padding/alignment
- `sizeof(UserContext)`

Otherwise, the new code will interpret old memory incorrectly → crashes, NaNs, corrupted state.

#### What counts as “layout change”

- Adding/removing/reordering fields in `UserContext`
- Changing `struct` members from `int` to `float`, etc.
- Changing array sizes
- Replacing embedded structs with pointers (or vice versa)

#### Safe-ish changes (still use caution)

- Changing logic in `loop()` / `load()` without touching the data layout.
- Changing code inside modules that operate on existing fields.

### 2) Modules must not depend on `UserContext` type

Project convention:

- Modules live in headers (often `./<mod>/<mod>.h`).
- Modules define their own state struct (e.g., `Shop`, `Houses`, `Oil`, …).
- `UserContext` *owns* instances of module structs as members.
- Modules receive pointers to their own state (and maybe `Clayton*`, `Physics*`, etc).
- **Modules should not include or reference `UserContext`.**

This avoids circular include dependencies and keeps modules reusable.

### Passing UI and story objects into modules (Clayton / DialogBox)

It’s OK (and recommended) to pass “service objects” into modules the same way we pass `Clayton*`:

- `Clayton*` is passed to modules that need to build UI.
- `DialogBox*` (story/dialog controller) can be passed to modules that need to:
  - open a dialog/storyline node,
  - check whether a modal dialog is active,
  - (optionally) emit/consume story events via a narrow API.

**Rule of thumb:** keep ownership in `UserContext`, but pass a pointer/reference through a small
explicit “services” struct (e.g. `SchoolServices { DialogBox* dialog; ... }`), so:

- modules do **not** include or know `UserContext`,
- only the required capabilities are exposed (avoids tight coupling),
- hot reload stays safe (no hidden global state or cross-module singletons).

If you want even stricter separation, define a tiny `StoryApi`/`StoryServices` interface struct
(function pointers or methods) instead of passing `DialogBox*` directly.

### 3) GPU / UI contexts often need explicit re-binding on `load()`

Hot reload replaces the `.so` and resets TU-local statics. Some libraries also keep “current context”
in static storage.

Important example: **Clay**.

Clay stores a “current context” in static storage. After hot reload, the new `.so` can lose that value,
so calling Clay layout functions can crash.

**Fix pattern:**

- Store the Clay context pointer in a long-lived object (`Clayton`).
- In `loop()` before any Clay work, call:
  - `Clay_SetCurrentContext(usr->clayton.clayCtx);`

And in `load()` (after reinitializing Clayton):

- rewire the framebuffer-backed Clay textures (ball preview / oil map textures), because `Clayton::initClayton`
  reloads / resets the renderer texture slots.

## Recommended module pattern (the project “house style”)

For a “big module” like School:

- `./school/school.h`
  - defines `struct School`
  - defines pure logic functions operating on `School*` + required dependencies
    - e.g. `School_Enter(School*, ...)`, `School_SelectLesson(School*, ...)`, `School_OnThrowComplete(...)`
- `./school/school_clay.h`
  - contains Clay builders that only depend on:
    - `School*`
    - `Clayton*`
    - and direct widget types (e.g. `Clayton_Click`, `Clayton_Slider`)
  - example functions:
    - `School_ClayInit(School*, Clayton*)`
    - `School_ClayHandleEvent(School*, Clayton*, SDL_Event const&)`
    - `School_ClayBuildPanel(School*, Clayton*)`
    - `School_ClayBuildHud(School*, Clayton*)`

Then in `game.cpp`:

- `UserContext` contains `School school;`
- `init()` calls `School_Init(&usr->school, ...)`
- `load()` calls `School_Load(&usr->school, ...)` if needed (usually UI/widget re-init, texture rebind)
- `loop()` calls:
  - `School_HandleEvent(&usr->school, ...)` for input
  - `School_Update(&usr->school, ...)` for per-frame state changes
  - `School_ClayBuild...` from inside the Clay layout tree

This preserves:

- stable memory layout,
- minimal `game.cpp` surface area,
- and no circular includes.

## What went wrong in the failed School refactor (root cause)

The “bad failure mode” is almost always one of:

- changing `UserContext` layout (breaks hot reload),
- using Clay without restoring its current context after reload,
- introducing module headers that depend on `UserContext` (circular / ordering problems),
- or reinitializing Clay/GL resources without rebinding textures and pointers.

The safest approach is:

- keep `UserContext` layout stable,
- keep module structs as embedded members of `UserContext`,
- and have `load()` rebind/recreate **external** resources (Clay context, shader programs, GL textures),
  while leaving the “gameplay state” untouched.
