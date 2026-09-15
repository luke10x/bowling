# Translation Guidelines

This document is the single source of truth for translation policy and
translation workflow. Runtime translations live in the four TXL source files
listed below because the generator requires one complete file per language.

## Current Scope

Translate player-facing game UI, menus, settings, dialogs, story text, result
screens, chest screens, tutorial/school copy, campaign copy, and visible HUD
messages.

All translations should live in the four simple TXL translation source files,
one complete file per language. Do not add new translated prose directly inside
gameplay, UI, or story logic files. If a feature needs new player-facing text,
add keys or entries to the TXL source files and have code reference those
entries.

Do not translate the tracker UI for now. The tracker is intentionally outside
the current translation scope, including editor labels, song/instrument tools,
pattern controls, macros, save/load/import/export buttons, and tracker status
text. Tracker strings may remain hardcoded until that feature is explicitly
added to the translation scope.

Symbols and compact controls such as `x`, `X`, `+`, `-`, arrows, checkmarks,
and numeric or musical notation may remain as literals when they are used as
icons or notation rather than prose.

## Files

TXL should use exactly four simple translation source files:

- `tegel/txl_en_us.h`: all English translations.
- `tegel/txl_lt_lt.h`: all Lithuanian translations.
- `tegel/txl_jp_jp.h`: all Japanese translations.
- `tegel/txl_zh_cn.h`: all Simplified Chinese translations.

Each file should provide the full translation set for its language, including
UI text, story/dialog text, choices, HUD messages, result screens, and other
player-facing copy. Do not split translations across gameplay headers or helper
functions.

- `scripts/generate_txl.py` generates `tegel/generated/txl_generated.h`.
- `tegel/txl_runtime.h` chooses the translated string and language-specific UI
  font at runtime.
- `storyline.h` currently contains story/dialog text. This is legacy structure:
  story translations should also move to separate translation source files
  rather than being embedded in code.
- `docs/story_dialog.md` describes the story dialog system.

## TXL Source Format

Each translation entry is one line:

```cpp
TXL_EXAMPLE_KEY = "Text shown to the player";
```

Rules:

- Every locale file must have the exact same `TXL_*` keys in the exact same
  order.
- Add new keys to `tegel/txl_en_us.h` first, then add matching entries to every
  other `tegel/txl_*.h` file.
- Run `scripts/generate_txl.py` after changing translation files.
- Keep C escape sequences intact, especially `\n`, `\"`, and `\\`.
- Keep printf-style placeholders compatible across languages: `%s`, `%d`,
  `%.0f`, `%.1f`, `%%`, and similar tokens must remain present and in a valid
  order for the code that formats them.
- Keys ending in `_FMT` are format strings. Translate the words, but preserve
  the placeholder meaning.

## Story Text

Story text is currently split from the TXL table, but translations should not
remain embedded in `storyline.h` long term. The base story script currently
lives in `STORYLINES[]` and `STORY_OPTIONS[]` in `storyline.h`, while runtime
helper functions such as `Story_Text`, `Story_OptionText`, and
`Story_SpeakerName` provide language-specific story/dialog strings. Treat this
as legacy organization to be migrated into the four TXL language files.

When translating story text:

- Preserve storyline ids, choice ids, event ids, and branch behavior.
- Preserve line breaks when they create deliberate pacing.
- Keep option labels short enough for choice buttons.
- Use the user-facing angel names listed below.

## User-Facing Names

Use these names in all player-facing text:

- Malach: the first angel, with the bunny mask. Do not call this character
  "my first angel" in UI text unless the sentence truly needs that description.
- Cherubel: do not use "Dog" in user-facing text. Code may still refer to this
  character as `dog`, but translated or visible copy must say Cherubel. Do not
  shorten this to "Cherub".
- Seraphel: do not use "Seraph", "Beak", or "bird" in user-facing text.
- Thrones: do not use "Cow" in user-facing text.

Internal code names may remain as-is when they are identifiers, enum names,
asset names, tests, or comments that are not visible to players.

## Length And Layout

Clay UI has limited room in buttons, windows, HUD banners, and mobile layouts.
Translations should favor clarity, but they must also fit.

Guidelines:

- Prefer short labels for buttons, tabs, HUD banners, and modal actions.
- For wide labels, use natural synonyms, shorter phrasing, or abbreviations.
- Japanese strings can become visually wide or long in compact UI. Shorten or
  rephrase them when needed instead of translating word-for-word.
- Avoid adding extra punctuation or explanatory phrases to compact controls.
- Preserve all-caps style only when it is part of the UI treatment. If all-caps
  is awkward in the target language, prefer a compact natural label.
- Avoid emoji in Clay UI. Emoji may not render reliably.
- If a string appears in a fixed-size UI element, test it in the target language
  or keep the translation intentionally short.

## Extraction Checklist

Before considering a feature translated, check for visible literals in:

- `CLAY_TEXT(...)`
- `ClayArena_FormatString(...)`
- result window labels such as `newGameTitle`, `newGameButtonLabel`, and
  `newGameShopButtonLabel`
- modal title/body/button text
- HUD banners and temporary notifications
- chest reward text
- school/tutorial UI
- campaign/result/endgame summaries
- story dialog strings and choice labels

Do not count tracker strings as missing extraction during the current pass.

## Current Known Gaps

Do not count tracker strings as missing extraction during the current pass.
Story translations are still partially organized through `storyline.h`; keep
future story translation work aligned with the four TXL language files.
