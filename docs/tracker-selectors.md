# Tracker Selector Guidelines

Tracker prev/next selectors and similar small buttons should follow one input rule:

- Route taps through `isClaytonClicked(...)` in `clayton/clayton_click.h`.
- Do not trigger selector actions from `Clay_PointerOver(...)` on `SDL_MOUSEBUTTONUP`.
- Keep selector behavior consistent with the existing instrument, macro, and algo selectors.

## Why

On touch-capable web builds, the app has a central touch bridge in `game.cpp` that converts
`SDL_FINGER*` events into synthetic `SDL_MOUSE*` events with `SDL_TOUCH_MOUSEID`.

Some devices can still produce overlapping native mouse events and synthetic touch-driven mouse
events for one physical tap. A selector implemented as "if hovered on mouse-up, advance" can
therefore process both releases and move twice:

- tap once
- first mouse-up advances `01 -> 02`
- second overlapping mouse-up advances `02 -> 03`

The shared Clayton click state machine is the safe path because it requires an armed press before
accepting the release, which collapses the overlapping native-plus-synthetic sequence into one
logical click.

## Regression Pattern To Watch For

If a selector:

- works on desktop mouse,
- skips every other entry on some touch devices,
- or moves `01 -> 03` on one tap,

then first check whether that selector bypassed `isClaytonClicked(...)` and is acting directly on
hovered mouse-up.

## Tests

The focused regression coverage lives in `tests/test_tracker_song_io.cpp`:

- `Release-only effect selector reproduces touch plus mouse double advance`
- `Shared Clayton click path advances effect selector only once for one touch tap`
