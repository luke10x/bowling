# Clay UI notes

## Hover state for sibling controls

Do **not** use `CLAY_THEME_BTN_PRIMARY`, `CLAY_THEME_BTN_DANGER`, or another
theme that calls `CLAY_THEME_HOVER_COLOR(...)` unchanged for a group of
sibling interactive controls. Those themes evaluate contextual
`Clay_Hovered()` while the layout tree is being built. In an open hovered row
or panel, that contextual state can apply to every sibling, causing several
buttons or cards to light up together.

Instead, copy the desired theme declaration and set its background from the
specific control's persistent click ID:

```cpp
Clay_ElementDeclaration button = CLAY_THEME_BTN_PRIMARY;
button.backgroundColor = Clay_PointerOver(clayton->continueClick.clayId)
    ? Clay_Color{104, 84, 244, 255}
    : CLAY_COLOR_BTN_PRIMARY;
CLAY(clayton->continueClick.clayId, button) { /* content */ }
```

Use the equivalent danger color for destructive actions. This is required for
campaign card rows, side-by-side confirmation actions, and any other sibling
button group.

## Repeated generated elements

Every generated `CLAY(...)` element needs a unique ID. Do not reuse a fixed
`CLAY_ID(...)` inside a loop or helper invoked multiple times. Use
`CLAY_IDI("Name", index)` or a distinct named ID for each fixed element.

## Emscripten: designated-property ordering

Emscripten's C++ compiler rejects designated initializers when properties are
not written in the declaration order of their struct. This is especially easy
to hit in `Clay_ElementDeclaration` and nested `Clay_LayoutConfig` literals.

Keep designated properties in their struct's declared order. For example,
write layout properties in the order defined by `Clay_LayoutConfig`; do not
place `.layoutDirection` before an earlier property such as `.childGap` when
both are present. Likewise, keep top-level `Clay_ElementDeclaration` fields
in their declared order (for example `.layout`, then visual fields such as
`.backgroundColor`, `.border`, and `.cornerRadius`).

When the order is unclear or a declaration needs conditional changes, prefer
the compiler-safe two-step form instead of a large designated literal:

```cpp
Clay_ElementDeclaration card = CLAY_THEME_BTN_PRIMARY;
card.layout.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(60)};
card.layout.childGap = 8;
card.layout.layoutDirection = CLAY_LEFT_TO_RIGHT;
card.backgroundColor = CLAY_COLOR_BTN_PRIMARY;
```

This avoids property-order diagnostics on desktop and Emscripten builds.
