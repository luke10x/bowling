# Emscripten Setup Notes

This repo uses Emscripten via a locally checked out emsdk sibling directory.

## tmux layout

In tmux session `4`, window `emscripten`:
- Right pane: `emsdk` environment + running Emscripten builds.
- Left pane: currently unused.

## Environment setup (right pane)

From the repo root:

```sh
cd ../emsdk/
source ./emsdk_env.sh
cd -
```

## Build commands

Run the Emscripten build/test target via the repo Makefile:

```sh
make -f Makefile.emscripten test
```

Notes:
- `test` runs `main` and then starts a web server from `build/emscripten/www` on port `8000`.
- If you see `OSError: [Errno 48] Address already in use`, something else is already listening on `8000`.
- The build also produces `build/emscripten/bowling.zip` (zipped contents of `build/emscripten/www`).

