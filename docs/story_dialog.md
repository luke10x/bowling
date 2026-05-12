# Story Dialog System (Bowling)

This project has a lightweight “story dialog” system that renders a ChatGPT‑style typing dialog over the game using Clay UI.

The goals:
- Story content is defined in **C++ data** (no YAML).
- Dialog is **modal**: it blocks gameplay input while active.
- Dialog can branch via **choice groups** and can emit a **single event** at the end (deferred until the dialog closes).

## Files

- `storyline.h`
  - C++ constants (speakers, events, choice groups)
  - `StorylineNode` + `StoryChoiceOption` structs
  - `STORYLINES[]` / `STORY_OPTIONS[]` arrays (the script)
  - `Story_FindNode(id)` lookup helper
- `dialogbox.h`
  - `DialogBox` runtime state (typing, stack of panels, choices)
  - Clay rendering + pointer consumption
  - “Deferred event on close” delivery (`consumeEvent()`)
- `game.cpp`
  - Owns an instance: `UserContext::dialog`
  - Decides **when to open** a story
  - Renders dialog when no other modals are open
  - Consumes dialog events and performs game actions

## Data model (storyline.h)

### Speakers
Speakers are integer constants:

```cpp
#define SPEAKER_DEVIL  1
#define SPEAKER_MYSELF 2
#define SPEAKER_ANGEL  3
```

### Events
Events are integer constants used to notify the game that something should happen **after the dialog ends**:

```cpp
#define EVENT_NONE        0
#define EVENT_GO_TO_SCHOOL 1
```

### Choice groups
Choice groups define which set of options to render:

```cpp
#define CHOICE_NONE 0
#define CHOICE_GO_TO_SCHOOL 1
#define CHOICE_WIN_GO_SCHOOL_OR_NEW_GAME 2
#define CHOICE_WIN_CONTINUE_GAME 3
```

### StorylineNode
Each storyline node is one “message” (one panel).

```cpp
struct StorylineNode {
  int32_t storyline_id;
  int32_t speaker;
  const char* text;
  int32_t choice_group;   // CHOICE_NONE => no options for this node
  int32_t next_storyline; // 0 => none
};
```

**Rule:** `choice_group` and `next_storyline` are mutually exclusive.
- If `choice_group != CHOICE_NONE`, the node ends with a choice panel.
- Else, if `next_storyline != 0`, the next node will auto-append after typing finishes.

### StoryChoiceOption
Each option belongs to a choice group and either:
- jumps to another node (`goto_storyline != 0`), or
- ends the dialog (`goto_storyline == 0`)

```cpp
struct StoryChoiceOption {
  int32_t choice_id;        // which choice group this belongs to
  const char* option;       // button label
  int32_t goto_storyline;   // node id to append next (0 => none)
  int32_t trigger_event;    // stored and emitted when dialog finishes
};
```

## Runtime behavior (dialogbox.h)

### Stacked message panels
The dialog renders a vertical stack:
- Each `StorylineNode` becomes a **message panel** appended top→bottom.
- When a node finishes typing:
  - if it has `choice_group`, the **choice panel** appears below messages
  - else if it has `next_storyline`, the next node auto-appends after a short delay

### Pre-typing delays
There are two delays:
- **Dialog appear delay**: overlay shows immediately, but the window panel appears after `0.5s`.
- **Per-line delay**: each new line waits `1.0s` before typing starts.

### Typewriter SFX
`DialogBox::update()` counts how many **non‑whitespace** characters were typed this frame.
`game.cpp` consumes that count and plays a short tick SFX (`SFX_TYPEWRITER`) per character (clamped per frame).

### Deferred event delivery
When an option is clicked:
- `StoryChoiceOption::trigger_event` is stored as `deferredEventOnClose`.
- Only when the dialog ends (`finalizeClose()`), the dialog exposes that event via `consumeEvent()`.

This ensures “actions” (like jumping to another UI) happen only after the story is done.

## Hooking it up in the game (game.cpp)

### 1) Store dialog state
`UserContext` contains:
- `DialogBox dialog;`
- `bool firstGameStoryShown;` (to show this story only once)

### 2) Start the story at game end
When the game finishes, pick a starting node id (example: branch by score):

```cpp
const int32_t startStoryId = (usr->board.totalScore >= 100) ? 20 : 10;
usr->dialog.open(startStoryId);
```

### 3) Render it only when no other windows are open
Dialog is modal and intended to be exclusive with other windows.
Current integration renders it only when:

- `usr->dialog.active` AND
- `usr->windowStack.count == 0`

### 4) Consume events
Each frame, the game reads:

```cpp
const int32_t storyEvent = usr->dialog.consumeEvent();
```

And acts accordingly. In the current story, `EVENT_GO_TO_SCHOOL` is handled (placeholder) by moving the game into `Phase::MENU`.

## Adding a new story

1) Add new `EVENT_*` and `CHOICE_*` ids in `storyline.h`.
2) Add nodes to `STORYLINES[]`.
3) Add options to `STORY_OPTIONS[]`.
4) Pick a “start node id” and call `usr->dialog.open(startId)` from `game.cpp`.
5) Add handling for any new events in `game.cpp` where `consumeEvent()` is processed.

