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
#define EVENT_CAMPAIGN_POSTGAME_CONTINUE 2012
#define EVENT_OPEN_RESET_PROGRESS_CONFIRM 2013

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
    // Intro (first frame)
    {
        /*storyline_id=*/1,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"You are in a bowling lane.\n"
                 "Your first milestone is to score 100 points in a single game.\n"
                 "If you do, you will earn a magic amulet.\n",
        /*choice_group=*/CHOICE_NONE,
        /*next_storyline=*/2,
    },
    {
        /*storyline_id=*/2,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"Do you want a tutorial?\n",
        /*choice_group=*/CHOICE_TUTORIAL_YES_NO,
        /*next_storyline=*/0,
    },

    // Lose path (< 100)
    {
        /*storyline_id=*/10,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"You did not reach 100 points.\n"
                 "Perhaps school would help you more than pride will.\n",
        /*choice_group=*/CHOICE_NONE,
        /*next_storyline=*/11,
    },
    {
        /*storyline_id=*/11,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"Do you want to go to school now, or try level 1 again first?\n",
        /*choice_group=*/CHOICE_LEVEL1_SCHOOL_OFFER,
        /*next_storyline=*/0,
    },

    // Win path (>= 100)
    {
        /*storyline_id=*/20,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"You reached 100 points.\n"
                 "You proved you are good enough to challenge me.\n",
        /*choice_group=*/CHOICE_NONE,
        /*next_storyline=*/21,
    },
    {
        /*storyline_id=*/21,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"Do you want to go to school (tutorial) first, or compete vs Angel now?\n",
        /*choice_group=*/CHOICE_FIRST_WIN_NEXT,
        /*next_storyline=*/0,
    },
    // School exit blocked message (when school is mandatory)
    {
        /*storyline_id=*/30,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"You cannot leave school yet.\n"
                 "Complete the lessons first.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
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
                 "Please catch all coins to pass (2 levels).\n",
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
    {
        /*storyline_id=*/3002,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"I am Malach.\n"
                 "I watched your first clear from a distance.\n"
                 "You have touch, and I want to see whether you can hold it under pressure.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3102,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"Good.\n"
                 "Now leave the comfort of a normal lane and follow me into the desert.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3003,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"This desert lane burns its oil fast.\n"
                 "Watch the front, and when your turn is coming, think about oil before pride.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/30031,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"I noticed you have many splits.\n"
                 "Perhaps you are trying to hit pins from the centre.\n"
                 "Instead, try to drive into them a bit from the side.\n"
                 "That usually helps prevent splits.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/30032,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"You are in the desert and I still have not seen you use oil.\n"
                 "You had better use it before the lane punishes your pride.\n"
                 "Do you want me to open the oil window for you now?\n",
        /*choice_group=*/CHOICE_MALACH_OIL_OFFER,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3103,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"You adapted.\n"
                 "Next comes ice, where the lane smiles and lies at the same time.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3004,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"Ice is long, slick, and patient.\n"
                 "Trust less, slide more, and visit the shop if you need a ball that speaks this language.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3104,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"You made it through.\n"
                 "Now come to Neon. I want to teach you with glass before I hand you to anyone else.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3040,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"Neon is our classroom now.\n"
                 "While you throw, I will sometimes drop glass into your lane.\n"
                 "Do not panic. Learn what it does.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3041,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"There. You felt the glass.\n"
                 "When it is my turn, you may answer with glass of your own.\n"
                 "Watch the turn buttons.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3140,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"Class is over.\n"
                 "Dog has been pacing under the Neon lights and wants the lane now.\n",
        /*choice_group=*/CHOICE_NONE,
        /*next_storyline=*/3141,
    },
    {
        /*storyline_id=*/3141,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"Before you answer Dog, visit the shop.\n"
                 "Different balls have different characteristics, and you should learn what speaks for your game.\n"
                 "Do you want me to open the shop now?\n",
        /*choice_group=*/CHOICE_MALACH_SHOP_OFFER,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3005,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"They call me Dog.\n"
                 "I like matches with bite, and I like players who push back.\n"
                 "Show me whether you fold or answer.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3105,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"Not bad.\n"
                 "Next we go back to a normal lane, and this time I let you use NOS.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3006,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"Now you can use NOS while you throw.\n"
                 "Do not tap it like a toy. Hold it when the ball already has speed and drive through the lane.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3106,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"Take that power into the desert.\n"
                 "Before my last round with you, I will also let you throw wood into my path.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3007,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"Desert again.\n"
                 "This time you can place wood when I am the one throwing.\n"
                 "Use it like an argument, not like decoration.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3107,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"You survived me.\n"
                 "Beak has been watching in silence, which is usually worse.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3008,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"I am Beak.\n"
                 "The desert keeps only what can hold its shape.\n"
                 "I do not bark. I wait, and then I decide.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3108,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"You interest me.\n"
                 "Come onto the ice and keep your balance while I keep my secrets.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3009,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"Ice rewards calm hands.\n"
                 "Do not confuse restraint with weakness.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3109,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"One more chapter in Neon.\n"
                 "Before my last level, I am giving you bricks to throw into the argument.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3010,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"Neon strips away disguise.\n"
                 "You can use bricks on my turns now. Make them count.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3110,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"Take the win if you can.\n"
                 "Someone larger, louder, and far less patient is already on her way.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3011,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"I am Cow.\n"
                 "I know my weight, I know my worth, and I am not here to make this easy for you.\n"
                 "Let's see if your game is as brave as your climb.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3111,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"You handled the city lights.\n"
                 "One more level waits, and in it I allow concrete.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3012,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"I am still Cow, and this is the final class.\n"
                 "Now you may place concrete when I throw.\n"
                 "If you want the top prize, build something worthy of it.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3112,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"I am Cow, and you defeated me.\n"
                 "In fact, you defeated all of us.\n"
                 "That was the top of the current ladder.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/32000,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"I am Cow, and now I can say it plainly:\n"
                 "you defeated all of us.\n"
                 "What do you want to do next?\n",
        /*choice_group=*/CHOICE_CAMPAIGN_ENDGAME,
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
        /*choice_id=*/CHOICE_TUTORIAL_YES_NO,
        /*option=*/"Yes",
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_GO_TO_SCHOOL,
    },
    {
        /*choice_id=*/CHOICE_TUTORIAL_YES_NO,
        /*option=*/"No",
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_NONE,
    },
    {
        /*choice_id=*/CHOICE_FIRST_FAIL_GO_SCHOOL,
        /*option=*/"Go to school",
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_GO_TO_SCHOOL,
    },
    {
        /*choice_id=*/CHOICE_LEVEL1_SCHOOL_OFFER,
        /*option=*/"Go to school",
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_GO_TO_SCHOOL,
    },
    {
        /*choice_id=*/CHOICE_LEVEL1_SCHOOL_OFFER,
        /*option=*/"Not now",
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_NONE,
    },
    {
        /*choice_id=*/CHOICE_FIRST_WIN_NEXT,
        /*option=*/"Go to school",
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_GO_TO_SCHOOL,
    },
    {
        /*choice_id=*/CHOICE_FIRST_WIN_NEXT,
        /*option=*/"Compete vs Angel",
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_GO_TO_BOT,
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
    {
        /*choice_id=*/CHOICE_MALACH_OIL_OFFER,
        /*option=*/"Open oil",
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_OPEN_OIL_WINDOW,
    },
    {
        /*choice_id=*/CHOICE_MALACH_OIL_OFFER,
        /*option=*/"No thanks",
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_NONE,
    },
    {
        /*choice_id=*/CHOICE_MALACH_SHOP_OFFER,
        /*option=*/"Open shop",
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_OPEN_SHOP_WINDOW,
    },
    {
        /*choice_id=*/CHOICE_MALACH_SHOP_OFFER,
        /*option=*/"No thanks",
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_NONE,
    },
    {
        /*choice_id=*/CHOICE_CAMPAIGN_ENDGAME,
        /*option=*/"Reset Progress",
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_OPEN_RESET_PROGRESS_CONFIRM,
    },
    {
        /*choice_id=*/CHOICE_CAMPAIGN_ENDGAME,
        /*option=*/"Continue Against Angels",
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
    if (language == TXL_LANG_LT_LT)
    {
        if (speaker == SPEAKER_ANGEL) return "ANGELAS";
        if (speaker == SPEAKER_DEVIL) return "VELNIAS";
        if (speaker == SPEAKER_MYSELF) return "AŠ";
        return "???";
    }
    if (language == TXL_LANG_JP_JP)
    {
        if (speaker == SPEAKER_ANGEL) return "天使";
        if (speaker == SPEAKER_DEVIL) return "悪魔";
        if (speaker == SPEAKER_MYSELF) return "私";
        return "???";
    }
    if (language == TXL_LANG_ZH_CN)
    {
        if (speaker == SPEAKER_ANGEL) return "天使";
        if (speaker == SPEAKER_DEVIL) return "恶魔";
        if (speaker == SPEAKER_MYSELF) return "我";
        return "???";
    }
    if (speaker == SPEAKER_ANGEL) return "ANGEL";
    if (speaker == SPEAKER_DEVIL) return "DEVIL";
    if (speaker == SPEAKER_MYSELF) return "ME";
    return "???";
}

static inline const char *Story_Text(TxlLanguage language, int32_t storylineId, const char *fallback)
{
    if (language != TXL_LANG_ZH_CN)
    {
        if (language == TXL_LANG_LT_LT || language == TXL_LANG_JP_JP)
            return fallback;
        return fallback;
    }

    switch (storylineId)
    {
        case 1: return "你站在一条保龄球道上。\n你的第一个里程碑，是在单局里拿到100分。\n如果做到，你会得到一枚魔法护符。\n";
        case 2: return "你想要一个教学吗？\n";
        case 10: return "你没有达到100分。\n也许学校比逞强更适合现在的你。\n";
        case 11: return "你现在想去学校，还是先再试一次第一关？\n";
        case 20: return "你达到了100分。\n你证明了自己已经有资格挑战我。\n";
        case 21: return "你想先去学校（教学），还是现在就和天使对战？\n";
        case 30: return "你现在还不能离开学校。\n先把课程完成。\n";
        case 1000: return "学校 :: 第2课：球的质量。\n这里是学校，这一课讲的是质量。\n每个球都有自己的质量，质量会改变它的手感和滚动方式。\n你的第一个测试，是用几颗轻球击中球瓶。\n而想毕业，你也必须用重球击中球瓶。\n";
        case 1010: return "很好！你通过了质量测试。\n";
        case 1012: return "要通过这个测试，你必须把质量滑块调到最轻或最重的一端。\n停在中间不会算进通过进度。\n";
        case 1013: return "很好。现在把滑块切到重球一端，再去击中球瓶。\n";
        case 1014: return "很好。现在把滑块切到轻球一端，再去击中球瓶。\n";
        case 1030: return "你随时都可以回到学校。\n";
        case 1020: return "恭喜。你通过了旋转测试。\n每颗球对旋转都有自己天生的反应。\n像 bite 这样的参数，也会影响它对旋转的响应程度。\n";
        case 1022: return "学校 :: 第3课：给球上旋。\n在球出手后，立刻在屏幕上做旋转动作给球加旋。\n这样球就会开始朝某个方向发力。\n请收集所有金币才能通过（共2关）。\n";
        case 1052: return "学校 :: 第4课：油与滑行。\n这条球道刚刚上过油，大约有半条到三分之二的长度都覆盖着满油。\n这一课里油会消耗得很快，所以打几球之后手感会明显改变。\n有些球馆本身就更滑，而球也有自己的 skid 参数。\n";
        case 1060: return "很好！你通过了油道测试。\n";
        case 1032: return "学校 :: 第1课：瞄准课。\n现在我们来学习如何出手。\n把球尽量往后拉，保持在中间，然后放手。\n只要击中任何球瓶，你就能得1分。\n";
        case 1040: return "很好！你通过了瞄准测试。\n";
        case 1070: return "学校 :: 第5课：全中线路。\n跟着金币走。那条线会先从中间弯开，再回到口袋位。\n你的目标，是打出一次STRIKE。\n你可以按下 SWAP LINE 来练习另一侧口袋。\n";
        case 1072: return "漂亮！STRIKE。\n你从学校毕业了。\n你随时都可以回来。\n";
        case 1080: return "我看得出你有点吃力。\n要不要试试这颗球？\n";
        case 1021: return "你可以继续在之字形金币那一课练更多旋转。\n第3课已经解锁。\n";
        case 3002: return "我是玛拉克。\n我在远处看见了你第一次通关。\n你有手感，而我想看看你能不能在压力下守住它。\n";
        case 3102: return "不错。\n现在离开普通球道的舒适区，跟我去沙漠。\n";
        case 3003: return "这条沙漠球道的油耗得很快。\n注意前段，等轮到你时，先想到油，再想到自尊。\n";
        case 3103: return "你适应过来了。\n接下来是冰面，那条球道会一边微笑，一边说谎。\n";
        case 3004: return "冰面很长，很滑，也很有耐心。\n少一点相信，多一点滑行；如果需要一颗会说这种语言的球，就去商店。\n";
        case 3104: return "你挺过去了。\n现在跟我去霓虹。我想先用玻璃给你上一课，然后再把你交给别人。\n";
        case 3040: return "现在霓虹就是我们的教室。\n当你出手时，我有时会把玻璃丢进你的球道。\n别慌，先学它会做什么。\n";
        case 3041: return "就是这样。你已经碰到玻璃了。\n等轮到我出手时，你也可以用玻璃回敬我。\n留意回合按钮。\n";
        case 3140: return "这节课结束了。\n狗已经在霓虹灯下踱步很久，现在他想要这条球道。\n";
        case 3005: return "他们叫我狗。\n我喜欢有咬劲的比赛，也喜欢会反击的玩家。\n让我看看你是会缩，还是会回。\n";
        case 3105: return "不赖。\n下一关我们回到普通球道，而且这次我允许你使用 NOS。\n";
        case 3006: return "现在你可以在出手时使用 NOS。\n别把它当玩具乱点。等球已经有速度时按住它，把力量送穿整条球道。\n";
        case 3106: return "把这股力量带去沙漠。\n在我和你的最后一关之前，我还会让你把木块丢到我的路线上。\n";
        case 3007: return "又是沙漠。\n这次当我出手时，你可以放木块。\n把它当成一种回嘴，而不是装饰。\n";
        case 3107: return "你挺过我了。\n喙一直在沉默地看着，而这通常更糟。\n";
        case 3008: return "我是喙。\n沙漠只留下能保持形状的东西。\n我不吠。我等着，然后由我来决定。\n";
        case 3108: return "你让我感兴趣。\n来冰面上，在你保持平衡的时候，让我继续藏着秘密。\n";
        case 3009: return "冰面奖励冷静的手。\n不要把克制误认为软弱。\n";
        case 3109: return "在霓虹里还有最后一章。\n在我最后一关之前，我会把砖块也交给你。\n";
        case 3010: return "霓虹会剥掉伪装。\n现在你也可以在我出手时用砖块了。别浪费它们。\n";
        case 3110: return "如果你拿得到，就把这场胜利带走。\n一个更大声、更夸张、也更没耐心的家伙已经在路上了。\n";
        case 3011: return "我是牛。\n我知道自己的重量，也知道自己的价值，而且我来这里不是为了让你轻松。\n让我们看看，你的球技是不是和你的攀升一样勇敢。\n";
        case 3111: return "你已经扛住了城市的灯光。\n还有最后一关在等你，而那一关里我会允许你使用混凝土。\n";
        case 3012: return "我还是牛，而这就是最后一课。\n现在当我出手时，你可以放混凝土。\n如果你想拿走顶级奖励，就搭出配得上的东西。\n";
        case 3112: return "这就是当前阶梯的顶端。\n城市已经看见你了，天使们也是。\n";
        default: return fallback;
    }
}

static inline const char *Story_OptionText(TxlLanguage language, const StoryChoiceOption &opt)
{
    if (language == TXL_LANG_LT_LT)
    {
        if (opt.choice_id == CHOICE_TUTORIAL_YES_NO && strcmp(opt.option, "Yes") == 0) return "Taip";
        if (opt.choice_id == CHOICE_TUTORIAL_YES_NO && strcmp(opt.option, "No") == 0) return "Ne";
        if (strcmp(opt.option, "Go to school") == 0) return "Eiti į mokyklą";
        if (strcmp(opt.option, "Not now") == 0) return "Ne dabar";
        if (strcmp(opt.option, "Compete vs Angel") == 0) return "Varžytis su angelu";
        if (strcmp(opt.option, "OK") == 0 || strcmp(opt.option, "Ok") == 0) return "Gerai";
        if (strcmp(opt.option, "Yes, take me to the next lesson") == 0) return "Taip, veskite mane į kitą pamoką";
        if (strcmp(opt.option, "No, I want to leave school") == 0) return "Ne, noriu išeiti iš mokyklos";
        if (strcmp(opt.option, "Practice more") == 0) return "Praktikuotis dar";
        if (strcmp(opt.option, "Back to game") == 0) return "Atgal į žaidimą";
        if (strcmp(opt.option, "Decline") == 0) return "Atsisakyti";
        return opt.option;
    }
    if (language == TXL_LANG_JP_JP)
    {
        if (opt.choice_id == CHOICE_TUTORIAL_YES_NO && strcmp(opt.option, "Yes") == 0) return "はい";
        if (opt.choice_id == CHOICE_TUTORIAL_YES_NO && strcmp(opt.option, "No") == 0) return "いいえ";
        if (strcmp(opt.option, "Go to school") == 0) return "学校へ行く";
        if (strcmp(opt.option, "Not now") == 0) return "今はやめる";
        if (strcmp(opt.option, "Compete vs Angel") == 0) return "天使と対戦";
        if (strcmp(opt.option, "OK") == 0 || strcmp(opt.option, "Ok") == 0) return "OK";
        if (strcmp(opt.option, "Yes, take me to the next lesson") == 0) return "はい、次のレッスンへ";
        if (strcmp(opt.option, "No, I want to leave school") == 0) return "いいえ、学校を出たい";
        if (strcmp(opt.option, "Practice more") == 0) return "もっと練習する";
        if (strcmp(opt.option, "Back to game") == 0) return "ゲームに戻る";
        if (strcmp(opt.option, "Decline") == 0) return "断る";
        return opt.option;
    }
    if (language != TXL_LANG_ZH_CN)
        return opt.option;

    if (opt.choice_id == CHOICE_TUTORIAL_YES_NO && strcmp(opt.option, "Yes") == 0) return "是";
    if (opt.choice_id == CHOICE_TUTORIAL_YES_NO && strcmp(opt.option, "No") == 0) return "否";
    if (strcmp(opt.option, "Go to school") == 0) return "去学校";
    if (strcmp(opt.option, "Not now") == 0) return "现在先不去";
    if (strcmp(opt.option, "Compete vs Angel") == 0) return "和天使对战";
    if (strcmp(opt.option, "OK") == 0) return "好";
    if (strcmp(opt.option, "Ok") == 0) return "好";
    if (strcmp(opt.option, "Yes, take me to the next lesson") == 0) return "好，带我去下一课";
    if (strcmp(opt.option, "No, I want to leave school") == 0) return "不，我想离开学校";
    if (strcmp(opt.option, "Practice more") == 0) return "继续练习";
    if (strcmp(opt.option, "Back to game") == 0) return "回到游戏";
    if (strcmp(opt.option, "Decline") == 0) return "拒绝";
    return opt.option;
}

static inline const char *Story_AllCharsForLanguage(TxlLanguage language)
{
    static char enBuf[16384];
    static bool enInit = false;
    static char zhBuf[32768];
    static bool zhInit = false;

    char *buf = (language == TXL_LANG_ZH_CN) ? zhBuf : enBuf;
    bool *init = (language == TXL_LANG_ZH_CN) ? &zhInit : &enInit;
    const size_t cap = (language == TXL_LANG_ZH_CN) ? sizeof(zhBuf) : sizeof(enBuf);
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
