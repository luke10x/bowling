# Native (macOS) Hot-Reload Setup Notes

These notes describe the current tmux-based native (macOS) workflow for fast iteration using a swappable
runtime `.so`.

## tmux layout

In tmux session `4`, window `native`:
- Left pane: full build + run (hot-reload runner).
- Right pane: incremental rebuild of the swappable runtime `.so`.

## Left pane (full build + run)

From the repo root:

```sh
make -f Makefile.mac test
```

Notes:
- This runs `make -f Makefile.mac main HOT_RELOAD=1` and then executes `build/macos/bin/bowling`.
- The build produces `build/macos/bin/hot_runtime.so` and links the executable to load it.
- When `hot_runtime.so` updates, the running binary detects it and performs a hot reload (you’ll see
  lines like `Reloading plugin...` / `Hot reload successful`).

## Right pane (incremental runtime rebuild)

From the repo root:

```sh
make -f Makefile.mac runtime HOT_RELOAD=1
```

Notes:
- This is the fast path: it rebuilds `build/macos/bin/hot_runtime.so` only, to trigger hot reload in the
  already-running process in the left pane.

