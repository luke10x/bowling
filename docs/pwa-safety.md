# PWA Safety Notes

This note documents the PWA breakage we hit in June 2026, what was restored, and what to be careful about in future.

## Current known-good direction

The current working combination is:

- keep the simpler offline-focused PWA shell/service-worker behavior
- keep the ball render/shop fixes
- keep LT/JP language support and embedded fonts
- do **not** reintroduce the newer manual PWA update UI/bridge without re-testing install/relaunch very carefully

Relevant files:

- `wasm/sw.js`
- `wasm/shell_itch_io.html`
- `wasm/manifest.json`
- `game.cpp`
- `ball_render.h`
- `clayton/clayton.h`
- `clayton/win_stack.h`
- `tegel/txl_runtime.h`
- `tegel/txl_font_assets.h`
- `Makefile.emscripten`

## What likely broke the installed PWA

The app audio kept running while the installed PWA showed only the shell background. That means the game loop was alive, but the web app startup/render path was broken.

The most likely causes were:

1. Service-worker behavior became too clever.
   - We introduced version stamping, `version.json`, manual update checks, and more aggressive update flows.
   - Installed PWAs are very sensitive to mixed old/new shell assets.

2. Asset-vs-navigation handling became risky.
   - During debugging, there was a period where a bad fetch path could return HTML when JS/WASM/data was expected.
   - That kind of cache poisoning can leave an installed PWA in a broken state even after later source fixes.

3. Shell startup/layout was over-tuned during recovery.
   - We experimented with manual canvas sizing/focus/update handoff logic.
   - In standalone display mode that can fight Emscripten/SDL startup in subtle ways.

4. Installed state can remain poisoned.
   - Once a bad service worker or bad cached startup asset is installed, later source fixes may not help until the site data / PWA install is reset.

## What was restored after the rollback

After the broad rollback commit, these non-PWA regressions were restored:

### Ball behavior

- `ball_render.h` was restored.
- Shop purchase now unlocks/equips the selected ball instead of swapping catalog items.
- Player/enemy rolling balls use the correct atlas selection again.
- Enemy ball selection uses explicit `enemyBallId` instead of fake offset math.
- `tests/test_ball_render.cpp` was restored.

### Language support

- Lithuanian and Japanese runtime support were restored.
- Embedded fonts for LT/JP were restored.
- Language picker buttons for LT/JP were restored.
- Story speaker/option text support for LT/JP was restored.

## Things to be careful about in future

### 1. Treat `wasm/sw.js` as high risk

Do not change service-worker fetch behavior casually.

Be especially careful with:

- navigation fallback behavior
- cache key normalization
- update flows using `skipWaiting`
- deleting old caches on activate
- serving shell HTML in any asset path

Rule:

- `index.js`, `index.wasm`, `index.data`, fonts, and images must never fall back to `index.html`

### 2. Be conservative with `wasm/shell_itch_io.html`

Avoid large startup experiments in the shell unless absolutely necessary.

High-risk changes include:

- manually forcing canvas width/height/backing size
- focus hacks on load
- update-triggered redirects/reloads
- standalone-only launch overlays
- DOM/layout logic that tries to outsmart SDL/Emscripten

Rule:

- prefer minimal shell behavior over “smart” startup behavior

### 3. Don’t mix multiple PWA changes in one step

Bad pattern:

- service-worker change
- manifest change
- shell layout change
- update UI change

all at once

Good pattern:

- change one layer
- publish
- test browser load
- test installed PWA cold start
- test installed PWA relaunch
- test offline relaunch

### 4. Keep manifest changes minimal

Standalone launch can be sensitive to:

- `start_url`
- `scope`

If a prior combination is known-good, prefer staying with it unless there is a strong reason to change it.

### 5. Embedded fonts are safer than extra runtime font assets

For this project, the intended direction is:

- compile language fonts into the binary / generated headers
- avoid depending on separately shipped font assets for UI correctness

That keeps PWA asset loading simpler.

### 6. Installed PWA testing needs a clean-state workflow

When debugging PWA startup:

1. test in normal browser tab
2. test installed PWA cold launch
3. test installed PWA relaunch
4. test offline relaunch

If behavior seems impossible or inconsistent, assume cached state may be poisoned and retest after:

1. uninstalling the PWA
2. clearing site data / service worker
3. reopening the site fresh
4. reinstalling

## Safe future approach for PWA work

If we revisit update checking later, do it in this order:

1. add a visible build label only
2. verify installed build identity manually
3. add passive version fetch with no auto-apply behavior
4. only then consider a manual update button

And when doing that:

- keep offline startup working first
- keep shell logic simple
- never combine it with unrelated gameplay/render changes in the same commit
