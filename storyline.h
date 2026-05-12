#pragma once

// storyline.h
// Data-only story scripting in C++ (no YAML).
//
// Concepts:
// - Storyline node: speaker + text, and either:
//   - `next_storyline` (auto-append after typing), OR
//   - `choice_group` (renders a choice panel below)
// - Choice group: a set of options that can jump to another node or close the dialog.
// - Trigger event: emitted when an option is selected and the dialog finishes.

#include <stdint.h>

#define SPEAKER_DEVIL 1
#define SPEAKER_MYSELF 2
#define SPEAKER_ANGEL 3

#define EVENT_NONE 0
#define EVENT_GO_TO_SCHOOL 1

#define CHOICE_NONE 0
#define CHOICE_GO_TO_SCHOOL 1
#define CHOICE_WIN_GO_SCHOOL_OR_NEW_GAME 2
#define CHOICE_WIN_CONTINUE_GAME 3

struct StorylineNode
{
    int32_t storyline_id;
    int32_t speaker;
    const char *text;
    // Mutually exclusive with next_storyline:
    // - if choice_group != CHOICE_NONE, the dialog shows options for that group.
    // - else, the dialog auto-appends next_storyline (if non-zero).
    int32_t choice_group;   // CHOICE_NONE = no options
    int32_t next_storyline; // 0 = none
};

struct StoryChoiceOption
{
    int32_t choice_id;        // which choice group this belongs to
    const char *option;       // option label shown to player
    int32_t goto_storyline;   // storyline_id to jump to when chosen (0 = none)
    int32_t trigger_event;    // event emitted on select (0 = none)
};

// --- Story content (first game outro; branches by final score) ---
static constexpr StorylineNode STORYLINES[] = {
    // Lose path (< 100)
    {
        /*storyline_id=*/10,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"Hello.\n",
        /*choice_group=*/CHOICE_NONE,
        /*next_storyline=*/11,
    },
    {
        /*storyline_id=*/11,
        /*speaker=*/SPEAKER_MYSELF,
        /*text=*/"Hello, where am I?",
        /*choice_group=*/CHOICE_NONE,
        /*next_storyline=*/12,
    },
    {
        /*storyline_id=*/12,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"You are in this bowling game.\n"
                 "Now you have to go to school.\n",
        /*choice_group=*/CHOICE_GO_TO_SCHOOL,
        /*next_storyline=*/0,
    },

    // Win path (>= 100)
    {
        /*storyline_id=*/20,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"Hi.\n"
                 "Good game. You did really well.\n",
        /*choice_group=*/CHOICE_NONE,
        /*next_storyline=*/21,
    },
    {
        /*storyline_id=*/21,
        /*speaker=*/SPEAKER_MYSELF,
        /*text=*/"Thanks!",
        /*choice_group=*/CHOICE_NONE,
        /*next_storyline=*/22,
    },
    {
        /*storyline_id=*/22,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"Even if you performed well, maybe you want to go to school?\n",
        /*choice_group=*/CHOICE_WIN_GO_SCHOOL_OR_NEW_GAME,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/23,
        /*speaker=*/SPEAKER_MYSELF,
        /*text=*/"Ok, I go to school.",
        /*choice_group=*/CHOICE_GO_TO_SCHOOL,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/24,
        /*speaker=*/SPEAKER_MYSELF,
        /*text=*/"I'd rather skip for now.",
        /*choice_group=*/CHOICE_NONE,
        /*next_storyline=*/25,
    },
    {
        /*storyline_id=*/25,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"You can always come back if you feel like going to school.\n",
        /*choice_group=*/CHOICE_WIN_CONTINUE_GAME,
        /*next_storyline=*/0,
    },
};

static constexpr StoryChoiceOption STORY_OPTIONS[] = {
    {
        /*choice_id=*/CHOICE_GO_TO_SCHOOL,
        /*option=*/"Go to school",
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_GO_TO_SCHOOL,
    },
    {
        /*choice_id=*/CHOICE_WIN_GO_SCHOOL_OR_NEW_GAME,
        /*option=*/"Go to school",
        /*goto_storyline=*/23,
        /*trigger_event=*/EVENT_NONE,
    },
    {
        /*choice_id=*/CHOICE_WIN_GO_SCHOOL_OR_NEW_GAME,
        /*option=*/"Start a new game",
        /*goto_storyline=*/24,
        /*trigger_event=*/EVENT_NONE,
    },
    {
        /*choice_id=*/CHOICE_WIN_CONTINUE_GAME,
        /*option=*/"Continue game",
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_NONE,
    },
};

static constexpr int32_t STORYLINES_COUNT = (int32_t)(sizeof(STORYLINES) / sizeof(STORYLINES[0]));
static constexpr int32_t STORY_OPTIONS_COUNT = (int32_t)(sizeof(STORY_OPTIONS) / sizeof(STORY_OPTIONS[0]));

static inline const StorylineNode *Story_FindNode(int32_t id)
{
    for (int32_t i = 0; i < STORYLINES_COUNT; i++)
        if (STORYLINES[i].storyline_id == id)
            return &STORYLINES[i];
    return nullptr;
}

