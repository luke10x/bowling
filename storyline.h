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
#include <string.h>
#include "tegel/txl_runtime.h"

#define SPEAKER_DEVIL 1
#define SPEAKER_MYSELF 2
#define SPEAKER_ANGEL 3

#define EVENT_NONE 0
#define EVENT_GO_TO_SCHOOL 1
// Progression
#define EVENT_GO_TO_BOT 2
// School events
#define EVENT_SCHOOL_SELECT_LESSON2 2001
#define EVENT_SCHOOL_PRACTICE_MASS_MORE 2002
#define EVENT_SCHOOL_SELECT_LESSON3 2003
#define EVENT_SCHOOL_PRACTICE_SPIN_MORE 2004
#define EVENT_SCHOOL_EXIT 2005
#define EVENT_SCHOOL_SELECT_LESSON4 2006
#define EVENT_SCHOOL_SELECT_LESSON5 2007
#define EVENT_SCHOOL_STRIKE_HELP_ACCEPT 2008
#define EVENT_SCHOOL_STRIKE_HELP_DECLINE 2009
#define EVENT_OPEN_OIL_WINDOW 2010
#define EVENT_OPEN_SHOP_WINDOW 2011
#define EVENT_CONTINUE_CAMPAIGN_AFTER_SHOP_OFFER 2012
#define EVENT_CAMPAIGN_POSTGAME_CONTINUE 2016
#define EVENT_OPEN_RESET_PROGRESS_CONFIRM 2013
#define EVENT_SCHOOL_CONFIRM_LESSON_SWITCH 2014
#define EVENT_SCHOOL_CANCEL_LESSON_SWITCH 2015

#define CHOICE_NONE 0
#define CHOICE_GO_TO_SCHOOL 1
#define CHOICE_WIN_GO_SCHOOL_OR_NEW_GAME 2
#define CHOICE_WIN_CONTINUE_GAME 3
#define CHOICE_TUTORIAL_YES_NO 4
#define CHOICE_FIRST_FAIL_GO_SCHOOL 5
#define CHOICE_FIRST_WIN_NEXT 6
#define CHOICE_LEVEL1_SCHOOL_OFFER 7
#define CHOICE_SCHOOL_OK 10
// School choice groups
#define CHOICE_SCHOOL_MASS_TEST_DONE 11
#define CHOICE_SCHOOL_SPIN_TEST_DONE 12
#define CHOICE_SCHOOL_EXIT_OK 13
#define CHOICE_SCHOOL_AIM_TEST_DONE 14
#define CHOICE_SCHOOL_OIL_TEST_DONE 15
#define CHOICE_SCHOOL_STRIKE_TEST_DONE 16
#define CHOICE_SCHOOL_STRIKE_HELP 17
#define CHOICE_MALACH_OIL_OFFER 18
#define CHOICE_MALACH_SHOP_OFFER 19
#define CHOICE_CAMPAIGN_ENDGAME 20
#define CHOICE_SCHOOL_LESSON_SWITCH_CONFIRM 21

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
    TxlKey option;            // TXL key for the option label shown to player
    int32_t goto_storyline;   // storyline_id to jump to when chosen (0 = none)
    int32_t trigger_event;    // event emitted on select (0 = none)
};

// --- Story content (first game outro; branches by final score) ---
static constexpr StorylineNode STORYLINES[] = {
    // clang-format off
    // Intro (first frame)
    {
        /*storyline_id=*/1,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_NONE,
        /*next_storyline=*/2,
    },
    {
        /*storyline_id=*/2,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_TUTORIAL_YES_NO,
        /*next_storyline=*/0,
    },

    // Lose path (< 100)
    {
        /*storyline_id=*/10,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_NONE,
        /*next_storyline=*/11,
    },
    {
        /*storyline_id=*/11,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_LEVEL1_SCHOOL_OFFER,
        /*next_storyline=*/0,
    },

    // Used after Level 1 is won with 100+ points while school is unfinished.
    // This automatically chains to node 21 for the training-or-Level-2 choice.
    {
        /*storyline_id=*/20,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_NONE,
        /*next_storyline=*/21,
    },
    // Used only as the continuation of node 20's first-win message.
    {
        /*storyline_id=*/21,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_FIRST_WIN_NEXT,
        /*next_storyline=*/0,
    },
    // Used after Level 1 is won with 100+ points when school is already done.
    // This replaces nodes 20 -> 21 so training is not offered a second time.
    {
        /*storyline_id=*/22,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    // School exit blocked message (when school is mandatory)
    {
        /*storyline_id=*/30,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/31,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_SCHOOL_LESSON_SWITCH_CONFIRM,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/32,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_SCHOOL_LESSON_SWITCH_CONFIRM,
        /*next_storyline=*/0,
    },

    // School: Lesson 1 intro
    {
        /*storyline_id=*/1000,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },

    // School: Lesson 2 completion (Mass)
    {
        /*storyline_id=*/1010,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_SCHOOL_MASS_TEST_DONE,
        /*next_storyline=*/0,
    },
    // School: Lesson 1 hint when player uses mid-range mass (no progress)
    {
        /*storyline_id=*/1012,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    // School: Lesson 1 partial completion hints
    {
        /*storyline_id=*/1013,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/1014,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },

    // School: leaving hint (shown when leaving early and school isn't finished)
    {
        /*storyline_id=*/1030,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_SCHOOL_EXIT_OK,
        /*next_storyline=*/0,
    },

    // School: Lesson 2 completion
    {
        /*storyline_id=*/1020,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_SCHOOL_SPIN_TEST_DONE,
        /*next_storyline=*/0,
    },
    // School: Lesson 2 intro
    {
        /*storyline_id=*/1022,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    // School: Lesson 4 intro (Oil / skid)
    {
        /*storyline_id=*/1052,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    // School: Lesson 4 reminder when the player keeps throwing on worn oil
    {
        /*storyline_id=*/1054,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    // School: Lesson 4 completion
    {
        /*storyline_id=*/1060,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_SCHOOL_OIL_TEST_DONE,
        /*next_storyline=*/0,
    },
    // School: Lesson 1 intro (Aim lesson)
    {
        /*storyline_id=*/1032,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    // School: Lesson 1 completion
    {
        /*storyline_id=*/1040,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_SCHOOL_AIM_TEST_DONE,
        /*next_storyline=*/0,
    },
    // School: Lesson 5 intro (Strike line)
    {
        /*storyline_id=*/1070,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    // School: Lesson 5 completion (graduation)
    {
        /*storyline_id=*/1072,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_SCHOOL_STRIKE_TEST_DONE,
        /*next_storyline=*/0,
    },
    // School: Lesson 5 help offer (every few failed attempts)
    {
        /*storyline_id=*/1080,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_SCHOOL_STRIKE_HELP,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/1021,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    // Used when replaying Level 1 after a failed attempt while school remains unfinished.
    {
        /*storyline_id=*/40,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_LEVEL1_SCHOOL_OFFER,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/30020,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_FIRST_WIN_NEXT,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3002,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3102,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3003,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/30031,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/30032,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_MALACH_OIL_OFFER,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3042,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3050,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3051,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3052,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3053,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_NONE,
        /*next_storyline=*/3051,
    },
    {
        /*storyline_id=*/3054,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_NONE,
        /*next_storyline=*/3052,
    },
    {
        /*storyline_id=*/3103,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3004,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3104,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3040,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3041,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3140,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_NONE,
        /*next_storyline=*/3141,
    },
    {
        /*storyline_id=*/3141,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_MALACH_SHOP_OFFER,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3005,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3105,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3006,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/30061,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3106,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3007,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3107,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3008,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3108,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3009,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3109,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3010,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3110,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3011,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3111,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3012,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3112,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/32000,
        /*speaker=*/SPEAKER_ANGEL,
        // Redefined by the locale-specific TXL story key.
        /*text=*/"",
        /*choice_group=*/CHOICE_CAMPAIGN_ENDGAME,
        /*next_storyline=*/0,
    },
    // clang-format on
};

static constexpr StoryChoiceOption STORY_OPTIONS[] = {
    {
        /*choice_id=*/CHOICE_GO_TO_SCHOOL,
        /*option=*/TXL_STORY_OPTION_ENTER_TRAINING,
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_GO_TO_SCHOOL,
    },
    {
        /*choice_id=*/CHOICE_TUTORIAL_YES_NO,
        /*option=*/TXL_STORY_OPTION_YES,
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_GO_TO_SCHOOL,
    },
    {
        /*choice_id=*/CHOICE_TUTORIAL_YES_NO,
        /*option=*/TXL_STORY_OPTION_NO,
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_NONE,
    },
    {
        /*choice_id=*/CHOICE_FIRST_FAIL_GO_SCHOOL,
        /*option=*/TXL_STORY_OPTION_ENTER_TRAINING,
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_GO_TO_SCHOOL,
    },
    {
        /*choice_id=*/CHOICE_LEVEL1_SCHOOL_OFFER,
        /*option=*/TXL_STORY_OPTION_ENTER_TRAINING,
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_GO_TO_SCHOOL,
    },
    {
        /*choice_id=*/CHOICE_LEVEL1_SCHOOL_OFFER,
        /*option=*/TXL_STORY_OPTION_NOT_NOW,
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_NONE,
    },
    {
        /*choice_id=*/CHOICE_FIRST_WIN_NEXT,
        /*option=*/TXL_STORY_OPTION_ENTER_TRAINING,
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_GO_TO_SCHOOL,
    },
    {
        /*choice_id=*/CHOICE_FIRST_WIN_NEXT,
        /*option=*/TXL_STORY_OPTION_COMPETE_VS_EZEKIEL,
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_GO_TO_BOT,
    },
    {
        /*choice_id=*/CHOICE_SCHOOL_OK,
        /*option=*/TXL_STORY_OPTION_OK,
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_NONE,
    },
    {
        /*choice_id=*/CHOICE_SCHOOL_LESSON_SWITCH_CONFIRM,
        /*option=*/TXL_STORY_OPTION_YES,
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_SCHOOL_CONFIRM_LESSON_SWITCH,
    },
    {
        /*choice_id=*/CHOICE_SCHOOL_LESSON_SWITCH_CONFIRM,
        /*option=*/TXL_STORY_OPTION_NO,
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_SCHOOL_CANCEL_LESSON_SWITCH,
    },
    {
        /*choice_id=*/CHOICE_SCHOOL_EXIT_OK,
        /*option=*/TXL_STORY_OPTION_OK,
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_SCHOOL_EXIT,
    },
    {
        /*choice_id=*/CHOICE_SCHOOL_MASS_TEST_DONE,
        /*option=*/TXL_STORY_OPTION_NEXT,
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_SCHOOL_SELECT_LESSON3,
    },
    {
        /*choice_id=*/CHOICE_SCHOOL_SPIN_TEST_DONE,
        /*option=*/TXL_STORY_OPTION_NEXT,
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_SCHOOL_SELECT_LESSON4,
    },
    {
        /*choice_id=*/CHOICE_SCHOOL_AIM_TEST_DONE,
        /*option=*/TXL_STORY_OPTION_NEXT,
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_SCHOOL_SELECT_LESSON2,
    },
    {
        /*choice_id=*/CHOICE_SCHOOL_OIL_TEST_DONE,
        /*option=*/TXL_STORY_OPTION_NEXT,
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_SCHOOL_SELECT_LESSON5,
    },
    {
        /*choice_id=*/CHOICE_SCHOOL_STRIKE_TEST_DONE,
        /*option=*/TXL_STORY_OPTION_NEXT,
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_SCHOOL_EXIT,
    },
    {
        /*choice_id=*/CHOICE_SCHOOL_STRIKE_HELP,
        /*option=*/TXL_STORY_OPTION_OK_TITLE,
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_SCHOOL_STRIKE_HELP_ACCEPT,
    },
    {
        /*choice_id=*/CHOICE_SCHOOL_STRIKE_HELP,
        /*option=*/TXL_STORY_OPTION_DECLINE,
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_SCHOOL_STRIKE_HELP_DECLINE,
    },
    {
        /*choice_id=*/CHOICE_MALACH_OIL_OFFER,
        /*option=*/TXL_STORY_OPTION_OPEN_OIL,
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_OPEN_OIL_WINDOW,
    },
    {
        /*choice_id=*/CHOICE_MALACH_OIL_OFFER,
        /*option=*/TXL_STORY_OPTION_NO_THANKS,
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_NONE,
    },
    {
        /*choice_id=*/CHOICE_MALACH_SHOP_OFFER,
        /*option=*/TXL_STORY_OPTION_OPEN_SHOP,
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_OPEN_SHOP_WINDOW,
    },
    {
        /*choice_id=*/CHOICE_MALACH_SHOP_OFFER,
        /*option=*/TXL_STORY_OPTION_NO_THANKS,
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_CONTINUE_CAMPAIGN_AFTER_SHOP_OFFER,
    },
    {
        /*choice_id=*/CHOICE_CAMPAIGN_ENDGAME,
        /*option=*/TXL_STORY_OPTION_KEEP_PLAYING,
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_CAMPAIGN_POSTGAME_CONTINUE,
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

static inline int32_t Story_FirstSoloOutroIdForScore(int totalScore)
{
    return (totalScore >= 100) ? 20 : 10;
}

static inline const StoryChoiceOption *Story_FindFirstOptionByChoiceId(int32_t choiceId)
{
    for (int32_t i = 0; i < STORY_OPTIONS_COUNT; i++)
        if (STORY_OPTIONS[i].choice_id == choiceId)
            return &STORY_OPTIONS[i];
    return nullptr;
}

static inline const char *Story_SpeakerName(TxlLanguage language, int32_t speaker)
{
    switch (speaker)
    {
        case SPEAKER_ANGEL: return Txl_Get(language, TXL_STORY_SPEAKER_ANGEL);
        case SPEAKER_DEVIL: return Txl_Get(language, TXL_STORY_SPEAKER_DEVIL);
        case SPEAKER_MYSELF: return Txl_Get(language, TXL_STORY_SPEAKER_MYSELF);
        default: return Txl_Get(language, TXL_STORY_SPEAKER_UNKNOWN);
    }
}

static inline const char *Story_AngelNameForStoryId(TxlLanguage language, int32_t storylineId)
{
    switch (storylineId)
    {
        case 3005:
        case 3105:
        case 3006:
        case 30061:
        case 3106:
        case 3007:
        case 3107:
            return Txl_Get(language, TXL_STORY_NAME_CHERUBEL);
        case 3008:
        case 3108:
        case 3009:
        case 3109:
        case 3010:
        case 3110:
            return Txl_Get(language, TXL_STORY_NAME_SERAPHEL);
        case 3011:
        case 3111:
        case 3012:
        case 3112:
            return Txl_Get(language, TXL_STORY_NAME_THRONES);
        default:
            return Txl_Get(language, TXL_STORY_NAME_EZEKIEL);
    }
}

// TXL_KEY_COUNT is the sentinel for an unknown story ID.
static inline TxlKey Story_TextKey(int32_t storylineId)
{
    switch (storylineId)
    {
        case 1: return TXL_STORY_1;
        case 2: return TXL_STORY_2;
        case 10: return TXL_STORY_10;
        case 11: return TXL_STORY_11;
        case 20: return TXL_STORY_20;
        case 21: return TXL_STORY_21;
        case 22: return TXL_STORY_22;
        case 30: return TXL_STORY_30;
        case 31: return TXL_STORY_31;
        case 32: return TXL_STORY_32;
        case 1000: return TXL_STORY_1000;
        case 1010: return TXL_STORY_1010;
        case 1012: return TXL_STORY_1012;
        case 1013: return TXL_STORY_1013;
        case 1014: return TXL_STORY_1014;
        case 1030: return TXL_STORY_1030;
        case 1020: return TXL_STORY_1020;
        case 1022: return TXL_STORY_1022;
        case 1052: return TXL_STORY_1052;
        case 1054: return TXL_STORY_1054;
        case 1060: return TXL_STORY_1060;
        case 1032: return TXL_STORY_1032;
        case 1040: return TXL_STORY_1040;
        case 1070: return TXL_STORY_1070;
        case 1072: return TXL_STORY_1072;
        case 1080: return TXL_STORY_1080;
        case 1021: return TXL_STORY_1021;
        case 40: return TXL_STORY_40;
        case 30020: return TXL_STORY_30020;
        case 3002: return TXL_STORY_3002;
        case 3102: return TXL_STORY_3102;
        case 3003: return TXL_STORY_3003;
        case 30031: return TXL_STORY_30031;
        case 30032: return TXL_STORY_30032;
        case 3042: return TXL_STORY_3042;
        case 3050: return TXL_STORY_3050;
        case 3051: return TXL_STORY_3051;
        case 3052: return TXL_STORY_3052;
        case 3053: return TXL_STORY_3053;
        case 3054: return TXL_STORY_3054;
        case 3103: return TXL_STORY_3103;
        case 3004: return TXL_STORY_3004;
        case 3104: return TXL_STORY_3104;
        case 3040: return TXL_STORY_3040;
        case 3041: return TXL_STORY_3041;
        case 3140: return TXL_STORY_3140;
        case 3141: return TXL_STORY_3141;
        case 3005: return TXL_STORY_3005;
        case 3105: return TXL_STORY_3105;
        case 3006: return TXL_STORY_3006;
        case 30061: return TXL_STORY_30061;
        case 3106: return TXL_STORY_3106;
        case 3007: return TXL_STORY_3007;
        case 3107: return TXL_STORY_3107;
        case 3008: return TXL_STORY_3008;
        case 3108: return TXL_STORY_3108;
        case 3009: return TXL_STORY_3009;
        case 3109: return TXL_STORY_3109;
        case 3010: return TXL_STORY_3010;
        case 3110: return TXL_STORY_3110;
        case 3011: return TXL_STORY_3011;
        case 3111: return TXL_STORY_3111;
        case 3012: return TXL_STORY_3012;
        case 3112: return TXL_STORY_3112;
        case 32000: return TXL_STORY_32000;
        default: return TXL_KEY_COUNT;
    }
}

static inline const char *Story_Text(TxlLanguage language, int32_t storylineId, const char *fallback)
{
    const TxlKey key = Story_TextKey(storylineId);
    return key == TXL_KEY_COUNT ? fallback : Txl_Get(language, key);
}

static inline const char *Story_OptionText(TxlLanguage language, const StoryChoiceOption &opt)
{
    if (opt.choice_id == CHOICE_TUTORIAL_YES_NO)
    {
        if (opt.option == TXL_STORY_OPTION_YES) return Txl_Get(language, TXL_STORY_OPTION_YES_TUTORIAL);
        if (opt.option == TXL_STORY_OPTION_NO) return Txl_Get(language, TXL_STORY_OPTION_NO_TUTORIAL);
    }
    return Txl_Get(language, opt.option);
}

static inline const char *Story_AllCharsForLanguage(TxlLanguage language)
{
    static char enBuf[16384];
    static bool enInit = false;
    static char ltBuf[32768];
    static bool ltInit = false;
    static char jpBuf[32768];
    static bool jpInit = false;
    static char zhBuf[32768];
    static bool zhInit = false;

    char *buf = enBuf;
    bool *init = &enInit;
    size_t cap = sizeof(enBuf);
    if (language == TXL_LANG_LT_LT)
    {
        buf = ltBuf;
        init = &ltInit;
        cap = sizeof(ltBuf);
    }
    else if (language == TXL_LANG_JP_JP)
    {
        buf = jpBuf;
        init = &jpInit;
        cap = sizeof(jpBuf);
    }
    else if (language == TXL_LANG_ZH_CN)
    {
        buf = zhBuf;
        init = &zhInit;
        cap = sizeof(zhBuf);
    }
    if (*init)
        return buf;

    size_t len = 0;
    buf[0] = '\0';
    uint32_t seen[4096];
    size_t seenCount = 0;

    auto decode_utf8 = [](const char *&p) -> uint32_t {
        unsigned char c = (unsigned char)*p++;
        if (c < 0x80)
            return c;
        if ((c & 0xE0) == 0xC0)
        {
            uint32_t cp = ((uint32_t)(c & 0x1F) << 6);
            cp |= (uint32_t)((unsigned char)*p++ & 0x3F);
            return cp;
        }
        if ((c & 0xF0) == 0xE0)
        {
            uint32_t cp = ((uint32_t)(c & 0x0F) << 12);
            cp |= (uint32_t)((unsigned char)*p++ & 0x3F) << 6;
            cp |= (uint32_t)((unsigned char)*p++ & 0x3F);
            return cp;
        }
        if ((c & 0xF8) == 0xF0)
        {
            uint32_t cp = ((uint32_t)(c & 0x07) << 18);
            cp |= (uint32_t)((unsigned char)*p++ & 0x3F) << 12;
            cp |= (uint32_t)((unsigned char)*p++ & 0x3F) << 6;
            cp |= (uint32_t)((unsigned char)*p++ & 0x3F);
            return cp;
        }
        return '?';
    };

    auto append_codepoint_utf8 = [&](uint32_t cp) {
        if (len + 4 >= cap)
            return;
        if (cp < 0x80)
        {
            buf[len++] = (char)cp;
        }
        else if (cp < 0x800)
        {
            buf[len++] = (char)(0xC0 | (cp >> 6));
            buf[len++] = (char)(0x80 | (cp & 0x3F));
        }
        else if (cp < 0x10000)
        {
            buf[len++] = (char)(0xE0 | (cp >> 12));
            buf[len++] = (char)(0x80 | ((cp >> 6) & 0x3F));
            buf[len++] = (char)(0x80 | (cp & 0x3F));
        }
        else
        {
            buf[len++] = (char)(0xF0 | (cp >> 18));
            buf[len++] = (char)(0x80 | ((cp >> 12) & 0x3F));
            buf[len++] = (char)(0x80 | ((cp >> 6) & 0x3F));
            buf[len++] = (char)(0x80 | (cp & 0x3F));
        }
        buf[len] = '\0';
    };

    auto append_unique = [&](const char *s) {
        if (!s)
            return;
        const char *p = s;
        while (*p)
        {
            uint32_t cp = decode_utf8(p);
            bool exists = false;
            for (size_t i = 0; i < seenCount; ++i)
            {
                if (seen[i] == cp)
                {
                    exists = true;
                    break;
                }
            }
            if (!exists)
            {
                if (seenCount < (sizeof(seen) / sizeof(seen[0])))
                    seen[seenCount++] = cp;
                append_codepoint_utf8(cp);
            }
        }
    };

    for (int32_t i = 0; i < STORYLINES_COUNT; ++i)
        append_unique(Story_Text(language, STORYLINES[i].storyline_id, STORYLINES[i].text));
    for (int32_t i = 0; i < STORY_OPTIONS_COUNT; ++i)
        append_unique(Story_OptionText(language, STORY_OPTIONS[i]));
    append_unique(Story_SpeakerName(language, SPEAKER_ANGEL));
    append_unique(Story_SpeakerName(language, SPEAKER_DEVIL));
    append_unique(Story_SpeakerName(language, SPEAKER_MYSELF));

    *init = true;
    return buf;
}
