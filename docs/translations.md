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
- `storyline.h` contains story flow data and references TXL keys for dialogue,
  choices, speaker labels, and angel names.
- `docs/story_dialog.md` describes the story dialog system.

## TXL Source Format

Short translation entries use one line:

```cpp
TXL_EXAMPLE_KEY = "Text shown to the player"; // Used in: UI area or source file
```

For multiline dialogue and poetry, use adjacent string literals with one verse
or dialogue line per source row. Keep four-line stanzas visibly grouped, so
translators can recognize the poetry. Preserve the text and `\n` escapes:

```cpp
TXL_STORY_1 =
    "Lost soul, you have entered The Heavens' gate.\n"
    "Learn runes and score one hundred before it is too late.\n"
    "Let strikes and spares reveal the road you seek.\n"
    "Count every mark, lest fortune make you weak.\n"; // Context: Speaker: Ezekiel. First Level 1 introduction, before the tutorial choice.
```

Rules:

- Every locale file must have the exact same `TXL_*` keys in the exact same
  order.
- Add new keys to `tegel/txl_en_us.h` first, then add matching entries to every
  other `tegel/txl_*.h` file.
- Run `scripts/generate_txl.py` after changing translation files.
- Keep C escape sequences intact, especially `\n`, `\"`, and `\\`.
- Keep a translator context comment on every entry. For story/dialogue keys,
  use `// Context: ...` to identify the speaker and when the text appears:
  before/after which campaign level, which school lesson, the triggering
  situation, and any following dialogue or choice. A source filename alone
  does not provide enough translation context. Keep these notes consistent
  across all four locales and update them when the story flow changes.
  Comments are not included in the generated runtime table.
- Existing UI entries may use `// Used in: ...` to describe the UI area.
- Keep printf-style placeholders compatible across languages: `%s`, `%d`,
  `%.0f`, `%.1f`, `%%`, and similar tokens must remain present and in a valid
  order for the code that formats them.
- Keys ending in `_FMT` are format strings. Translate the words, but preserve
  the placeholder meaning.

## Story Text

Story/dialogue translations belong only in the dedicated `Story and dialogue`
section at the end of each of the four TXL files. Keep matching keys in the same
order in every locale, including `TXL_STORY_<storyline_id>` dialogue keys and
story choice, speaker, and name keys. Do not embed localized prose in
`storyline.h`: `STORYLINES[]` retains flow data and empty fallback text,
`STORY_OPTIONS[]` references choice keys, and `Story_Text()` resolves story IDs
through `Story_TextKey()` and `Txl_Get()`.

When translating story text:

- Preserve storyline ids, choice ids, event ids, and branch behavior.
- Preserve line breaks when they create deliberate pacing.
- Keep option labels short enough for choice buttons.
- Use the user-facing angel names listed below.

## User-Facing Names

Use these names in all player-facing text:

- Ezekiel: formerly called Malach. This is the first angel, with the bunny
  mask. Do not call this character "Malach" or "my first angel" in UI text
  unless the sentence truly needs that description.
- Cherubel: do not use "Dog" in user-facing text. Code may still refer to this
  character as `dog`, but translated or visible copy must say Cherubel. Do not
  shorten this to "Cherub".
- Seraphel: do not use "Seraph", "Beak", or "bird" in user-facing text.
- Thrones: do not use "Cow" in user-facing text.

Use these localized biblical/theological forms when translating angel names:

- Lithuanian:
  - Ezekiel: `Ezekielis`; inflect naturally when grammar requires it, e.g.
    `Ezekielį` in "beat Ezekiel" labels.
  - Cherubel: `Kerubas` - `Kerubą`
  - Seraphel: `Serafelė` - `Serafelę` (Feminine noun)
  - Thrones: `Ofanija` - `Ofaniją`  (Feminine noun)
- Chinese:
  - Ezekiel: `以西结`
  - Cherubel: `基路伯`
  - Seraphel: `撒拉弗`
  - Thrones: `座天使`
- Japanese:
  - Ezekiel: `エゼキエル`
  - Cherubel: `ケルビム`
  - Seraphel: `セラフィム`
  - Thrones: `座天使`

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

## Chinese and Japanese Story Poetry

Use [the campaign lore](../tegel/lore.md) as the authority for the story arc,
then compare both English and Lithuanian for imagery and stanza progression.
The Chinese and Japanese TXL headers document the chosen literary approach:

- Chinese: **古风-inspired four-line verse**, blending literary and readable
  modern phrasing, parallelism, and selective rhyme. This is not strict
  five/seven-character regulated verse; preserve names and gameplay meaning
  rather than claiming fixed metre or tonal rules.
- Japanese: **文語自由詩**, with echoes of **七五調** and the declamatory
  cadence of **漢詩訓読**. Use four-line stanzas and restrained archaic diction,
  without claiming strict metre or requiring end rhyme. Keep clauses short
  enough for the dialogue panel, especially around katakana names.

These are adaptation choices, not replacements for the campaign's setting.
Do not introduce local deities or change the angels' roles. Preserve the gradual
shift from promised salvation to punishment and the ending's cruel irony.
Keep school instructions and choice buttons direct and unambiguous.

## Japanese Line Breaks

Long Japanese text runs, especially katakana loanwords and compounds, can
behave as unbreakable units and overflow narrow panels or buttons. Automatic
word wrapping is not sufficient to guarantee that Japanese text fits.

- Test dialogue and button labels at the narrowest supported/mobile width.
  Prefer short, natural labels. For new or explicitly revised dialogue, use
  a shorter expression or an explicit `\n` at a natural phrase boundary.
- If a katakana compound itself exceeds the available width, break between
  meaningful components rather than treating the whole compound as indivisible.
  Keeping words together is a preference, not a reason to allow overflow.
  Keep character names intact where possible and never split a UTF-8 character.
- Keep deliberate verse lines and pauses. Write each verse line on its own
  source row with a `\n` escape; source formatting alone does not insert a
  displayed newline. Preserve existing stanza lengths rather than forcing
  every Japanese translation into four lines.
- For new or explicitly revised translations, place breaks at natural phrase
  or clause boundaries. Keep words, angel names, numbers with units, and small
  kana with their preceding kana together; avoid isolating particles from
  their phrase.
- Follow Japanese line-breaking conventions (禁則処理): avoid starting a line
  with closing punctuation such as `、。！？」』）`, or ending one with opening
  brackets such as `「『（`. Keep punctuation attached to its phrase.
- Do not add spaces to simulate wrapping or alignment. Check actual dialogue
  wrapping at mobile widths; do not assume the renderer applies these rules
  automatically.
- Formatting-only edits must preserve wording and intentional `\n` escapes.

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

Older Chinese and Japanese translations may be substantially outdated,
including UI text, level titles/subtitles, school copy, and unrevised dialogue.
The newly revised campaign verse does not constitute a complete locale review.
Do not use older localized entries as authoritative terminology or lore:
check current gameplay and context notes, and compare the English/Lithuanian
references with [the campaign lore](../tegel/lore.md).

Do not count tracker strings as missing extraction during the current pass.
Keep future story/dialogue translations in the dedicated `Story and dialogue`
section of the four TXL language files.
