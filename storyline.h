#pragma once

// storyline.h
// Data-only story scripting in C++ (no YAML).
//
// Concepts:
// - Storyline node: (speaker, text, optional choice group, optional trigger event)
// - Choice group: a set of options that can jump to another storyline node.
// - Trigger event: emitted when node is first shown or when an option is selected.

#include <stdint.h>

#define SPEAKER_DEVIL 1
#define SPEAKER_MYSELF 2
#define SPEAKER_ANGEL 3

#define EVENT_NONE 0
#define EVENT_MYSELF_WAS_TOLD_TO_RUN_AFTER_COINS 1
#define EVENT_MYSELF_AGREE_TO_OIL_NOW 2
#define EVENT_MYSELF_REFUSE_TO_OIL_NOW 3

#define CHOICE_NONE 0
#define CHOICE_GO_OIL_FIRST_TRACK_NOW_OR_LATER 1
#define CHOICE_CONTINUE 2

struct StorylineNode
{
    int32_t storyline_id;
    int32_t speaker;
    const char *text;
    // Mutually exclusive with next_storyline:
    // - if choice_group != CHOICE_NONE, the dialog shows options for that group.
    // - else, the dialog can show a single CONTINUE button if next_storyline != 0.
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

// --- Story content (first game outro) ---
static constexpr StorylineNode STORYLINES[] = {
    {
        /*storyline_id=*/1,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"Welcome to this world where there is nothing else but bowling.\n"
                 "A bag of money has burst and will show you the way.\n"
                 "Your first assignment is to collect enough coins to change the oil on this track.\n",
        /*choice_group=*/CHOICE_NONE,
        /*next_storyline=*/2,
    },
    {
        /*storyline_id=*/2,
        /*speaker=*/SPEAKER_MYSELF,
                 "You sure about that\n",
        /*choice_group=*/CHOICE_NONE,
        /*next_storyline=*/3,
    },
    {
        /*storyline_id=*/3,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"Good job. You now have enough coins to change the oil on this lane.\n"
                 "Your balls will slide faster!\n",
        /*choice_group=*/CHOICE_GO_OIL_FIRST_TRACK_NOW_OR_LATER,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/4,
        /*speaker=*/SPEAKER_MYSELF,
        /*text=*/"Yes. Let's open the oil window now.",
        /*choice_group=*/CHOICE_CONTINUE,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/5,
        /*speaker=*/SPEAKER_MYSELF,
        /*text=*/"No, not now. I want to save more coins first.",
        /*choice_group=*/CHOICE_CONTINUE,
        /*next_storyline=*/0,
    },
};

static constexpr StoryChoiceOption STORY_OPTIONS[] = {
    {
        /*choice_id=*/CHOICE_GO_OIL_FIRST_TRACK_NOW_OR_LATER,
        /*option=*/"Yes, go now",
        /*goto_storyline=*/4,
        /*trigger_event=*/EVENT_MYSELF_AGREE_TO_OIL_NOW,
    },
    {
        /*choice_id=*/CHOICE_GO_OIL_FIRST_TRACK_NOW_OR_LATER,
        /*option=*/"No, maybe later",
        /*goto_storyline=*/5,
        /*trigger_event=*/EVENT_MYSELF_REFUSE_TO_OIL_NOW,
    },
    {
        /*choice_id=*/CHOICE_CONTINUE,
        /*option=*/"Continue",
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
