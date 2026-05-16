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

#define CHOICE_NONE 0
#define CHOICE_GO_TO_SCHOOL 1
#define CHOICE_WIN_GO_SCHOOL_OR_NEW_GAME 2
#define CHOICE_WIN_CONTINUE_GAME 3
#define CHOICE_SCHOOL_OK 10
// School choice groups
#define CHOICE_SCHOOL_MASS_TEST_DONE 11
#define CHOICE_SCHOOL_SPIN_TEST_DONE 12
#define CHOICE_SCHOOL_EXIT_OK 13
#define CHOICE_SCHOOL_AIM_TEST_DONE 14
#define CHOICE_SCHOOL_OIL_TEST_DONE 15
#define CHOICE_SCHOOL_STRIKE_TEST_DONE 16
#define CHOICE_SCHOOL_STRIKE_HELP 17

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

    // School: Lesson 1 intro
    {
        /*storyline_id=*/1000,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"School :: Lesson 2. Ball Mass.\n"
                 "This is the school and this is a lesson about mass.\n"
                 "Every ball has its mass. Based on mass the balls feel and roll differently.\n"
                 "Your first test is to throw several LIGHT balls and hit pins.\n"
                 "To graduate you also need to hit pins with a HEAVY ball.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },

    // School: Lesson 2 completion (Mass)
    {
        /*storyline_id=*/1010,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"Nice! You passed the Mass test.\n",
        /*choice_group=*/CHOICE_SCHOOL_MASS_TEST_DONE,
        /*next_storyline=*/0,
    },
    // School: Lesson 1 hint when player uses mid-range mass (no progress)
    {
        /*storyline_id=*/1012,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"To pass this test you must set the mass slider to the LIGHT or HEAVY end.\n"
                 "Throwing in the middle does not count toward passing.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    // School: Lesson 1 partial completion hints
    {
        /*storyline_id=*/1013,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"Good. Now switch the slider to the HEAVY end and hit pins.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/1014,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"Good. Now switch the slider to the LIGHT end and hit pins.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },

    // School: leaving hint (shown when leaving early and school isn't finished)
    {
        /*storyline_id=*/1030,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"You can come back to the school anytime.\n",
        /*choice_group=*/CHOICE_SCHOOL_EXIT_OK,
        /*next_storyline=*/0,
    },

    // School: Lesson 2 completion
    {
        /*storyline_id=*/1020,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"Congrats. You passed the Spin test.\n"
                 "Every ball has its intrinsic reaction to spin.\n"
                 "Other params like bite affect how well the ball reacts.\n",
        /*choice_group=*/CHOICE_SCHOOL_SPIN_TEST_DONE,
        /*next_storyline=*/0,
    },
    // School: Lesson 2 intro
    {
        /*storyline_id=*/1022,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"School :: Lesson 3. Spin ball.\n"
                 "Right after ball launch, spin the ball by spin movements on screen.\n"
                 "Then the ball will start to drive to a particular direction.\n"
                 "Please catch all coins to pass.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    // School: Lesson 4 intro (Oil / skid)
    {
        /*storyline_id=*/1052,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"School :: Lesson 4. Oil and skid.\n"
                 "This lane was just oiled. It is covered in max oil for about half to two-thirds of the track.\n"
                 "In this lesson the oil wears out very fast, so after a few shots it will feel different.\n"
                 "Some houses have intrinsic slipperiness, and balls have a skid parameter.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    // School: Lesson 4 completion
    {
        /*storyline_id=*/1060,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"Nice! You passed the Oil test.\n",
        /*choice_group=*/CHOICE_SCHOOL_OIL_TEST_DONE,
        /*next_storyline=*/0,
    },
    // School: Lesson 1 intro (Aim lesson)
    {
        /*storyline_id=*/1032,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"School :: Lesson 1. Aim lesson.\n"
                 "Now we will learn to throw.\n"
                 "Pull the ball all the way back, keep it centered, then let it go.\n"
                 "If you hit any pins, you get a point.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    // School: Lesson 1 completion
    {
        /*storyline_id=*/1040,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"Nice! You passed the Aim test.\n",
        /*choice_group=*/CHOICE_SCHOOL_AIM_TEST_DONE,
        /*next_storyline=*/0,
    },
    // School: Lesson 5 intro (Strike line)
    {
        /*storyline_id=*/1070,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"School :: Lesson 5. Strike line.\n"
                 "Follow the coins. The line bends away from the middle and returns into the pocket.\n"
                 "Your objective is to score a STRIKE.\n"
                 "You can press SWAP LINE to practice the other pocket.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    // School: Lesson 5 completion (graduation)
    {
        /*storyline_id=*/1072,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"Boom! STRIKE.\n"
                 "You graduated the School.\n"
                 "You can come back anytime.\n",
        /*choice_group=*/CHOICE_SCHOOL_STRIKE_TEST_DONE,
        /*next_storyline=*/0,
    },
    // School: Lesson 5 help offer (every few failed attempts)
    {
        /*storyline_id=*/1080,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"I see you struggling.\n"
                 "Try this ball instead?\n",
        /*choice_group=*/CHOICE_SCHOOL_STRIKE_HELP,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/1021,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"You can practice spin more on the zig-zag coins.\n"
                 "Lesson 3 is now unlocked.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
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
    {
        /*choice_id=*/CHOICE_SCHOOL_OK,
        /*option=*/"OK",
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_NONE,
    },
    {
        /*choice_id=*/CHOICE_SCHOOL_EXIT_OK,
        /*option=*/"OK",
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_SCHOOL_EXIT,
    },
    {
        /*choice_id=*/CHOICE_SCHOOL_MASS_TEST_DONE,
        /*option=*/"Yes, take me to the next lesson",
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_SCHOOL_SELECT_LESSON3,
    },
    {
        /*choice_id=*/CHOICE_SCHOOL_MASS_TEST_DONE,
        /*option=*/"No, I want to leave school",
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_SCHOOL_EXIT,
    },
    {
        /*choice_id=*/CHOICE_SCHOOL_SPIN_TEST_DONE,
        /*option=*/"Yes, take me to the next lesson",
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_SCHOOL_SELECT_LESSON4,
    },
    {
        /*choice_id=*/CHOICE_SCHOOL_SPIN_TEST_DONE,
        /*option=*/"No, I want to leave school",
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_SCHOOL_EXIT,
    },
    {
        /*choice_id=*/CHOICE_SCHOOL_AIM_TEST_DONE,
        /*option=*/"Yes, take me to the next lesson",
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_SCHOOL_SELECT_LESSON2,
    },
    {
        /*choice_id=*/CHOICE_SCHOOL_AIM_TEST_DONE,
        /*option=*/"No, I want to leave school",
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_SCHOOL_EXIT,
    },
    {
        /*choice_id=*/CHOICE_SCHOOL_OIL_TEST_DONE,
        /*option=*/"Yes, take me to the next lesson",
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_SCHOOL_SELECT_LESSON5,
    },
    {
        /*choice_id=*/CHOICE_SCHOOL_OIL_TEST_DONE,
        /*option=*/"No, I want to leave school",
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_SCHOOL_EXIT,
    },
    {
        /*choice_id=*/CHOICE_SCHOOL_STRIKE_TEST_DONE,
        /*option=*/"Practice more",
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_SCHOOL_SELECT_LESSON5,
    },
    {
        /*choice_id=*/CHOICE_SCHOOL_STRIKE_TEST_DONE,
        /*option=*/"Back to game",
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_SCHOOL_EXIT,
    },
    {
        /*choice_id=*/CHOICE_SCHOOL_STRIKE_HELP,
        /*option=*/"Ok",
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_SCHOOL_STRIKE_HELP_ACCEPT,
    },
    {
        /*choice_id=*/CHOICE_SCHOOL_STRIKE_HELP,
        /*option=*/"Decline",
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_SCHOOL_STRIKE_HELP_DECLINE,
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
