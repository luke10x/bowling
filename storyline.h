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
#define EVENT_CAMPAIGN_POSTGAME_CONTINUE 2012
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
        /*text=*/"Lost soul, thou hast entered the Unseen Realm, the world of spirits.\n"
                 "Learn its runes and score 100, lest thy path close before thee.\n",
        /*choice_group=*/CHOICE_NONE,
        /*next_storyline=*/2,
    },
    {
        /*storyline_id=*/2,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"Wilt thou enter training?\n",
        /*choice_group=*/CHOICE_TUTORIAL_YES_NO,
        /*next_storyline=*/0,
    },

    // Lose path (< 100)
    {
        /*storyline_id=*/10,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"Thou didst not reach 100.\n"
                 "Perchance training will serve thee better than pride.\n",
        /*choice_group=*/CHOICE_NONE,
        /*next_storyline=*/11,
    },
    {
        /*storyline_id=*/11,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"Wilt thou enter training now, or try the Unseen Realm once more?\n",
        /*choice_group=*/CHOICE_LEVEL1_SCHOOL_OFFER,
        /*next_storyline=*/0,
    },

    // Win path (>= 100)
    {
        /*storyline_id=*/20,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"Thou hast passed the first sign.\n"
                 "I am Ezekiel. I shall train thee and test thee, that thou mayest be saved.\n",
        /*choice_group=*/CHOICE_NONE,
        /*next_storyline=*/21,
    },
    {
        /*storyline_id=*/21,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"Wilt thou enter training first, or face Ezekiel now?\n",
        /*choice_group=*/CHOICE_FIRST_WIN_NEXT,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/22,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"Thou hast passed the first sign.\n"
                 "I am Ezekiel. I shall train thee and test thee, that thou mayest be saved.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    // School exit blocked message (when school is mandatory)
    {
        /*storyline_id=*/30,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"Thou canst not leave training yet.\n"
                 "Complete the trials first.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/31,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"Are you sure?\n"
                 "This will cancel the current lesson attempt and start the selected lesson instead.\n",
        /*choice_group=*/CHOICE_SCHOOL_LESSON_SWITCH_CONFIRM,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/32,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"Restart the current lesson?\n"
                 "Your current attempt in this lesson will be reset.\n",
        /*choice_group=*/CHOICE_SCHOOL_LESSON_SWITCH_CONFIRM,
        /*next_storyline=*/0,
    },

    // School: Lesson 1 intro
    {
        /*storyline_id=*/1000,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"Lesson 2. Ball Mass.\n"
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
        /*text=*/"Lesson 3. Spin ball.\n"
                 "Right after ball launch, spin the ball by spin movements on screen.\n"
                 "Then the ball will start to drive to a particular direction.\n"
                 "Knock down all lightweight target pins to pass (2 levels).\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    // School: Lesson 4 intro (Oil / skid)
    {
        /*storyline_id=*/1052,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"Lesson 4. Oil and skid.\n"
                 "This lane was just oiled. It is covered in max oil for about half to two-thirds of the track.\n"
                 "In this lesson the oil wears out very fast, so after a few shots it will feel different.\n"
                 "Some houses have intrinsic slipperiness, and balls have a skid parameter.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    // School: Lesson 4 reminder when the player keeps throwing on worn oil
    {
        /*storyline_id=*/1054,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"The lane is ready for fresh oil now.\n"
                 "Open the Oil window and re-oil before throwing again.\n",
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
        /*text=*/"Lesson 1. Aim lesson.\n"
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
        /*text=*/"Lesson 5. Strike line.\n"
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
        /*storyline_id=*/40,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"I am Ezekiel, thy angel saviour.\n"
                 "The Unseen Realm is a world of spirits and runes. Score 100 to pass its first sign.\n"
                 "If thou needest guidance, training is open.\n",
        /*choice_group=*/CHOICE_LEVEL1_SCHOOL_OFFER,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/41,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"I am Ezekiel, thy angel saviour.\n"
                 "The Unseen Realm is a world of spirits and runes. Score 100 to pass its first sign.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/30020,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"Thou hast passed the first sign.\n"
                 "I am Ezekiel. I shall train and test thee; at the end await Thrones.\n"
                 "Wilt thou enter training first, or continue now?\n",
        /*choice_group=*/CHOICE_FIRST_WIN_NEXT,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3002,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"I am Ezekiel.\n"
                 "Thou seekest salvation; I shall measure thy hand and thy resolve.\n"
                 "Come, and let the first trial begin.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3102,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"Well done.\n"
                 "Leave the gentle lane and follow me into the Desert of Sins.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3003,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"This desert is dry from corruption.\n"
                 "Anoint the lane with oil and frankincense; re-oiling cleans corruption as fire doth.\n",
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
        /*storyline_id=*/3042,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"I noticed you don't throw back glass just like I am throwing at you.\n"
                 "You can also throw glass at me!\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3050,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"Now you have magic to use against me?\n"
                 "Put it into the ball's spin circle to cast it.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3051,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"Oh wow, you can cause an explosion?\n"
                 "It is up to you whether destroying your ball is worth a little damage.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3052,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"I see you got some serious magic.\n"
                 "You can destroy my balls, but I will try to evade what you throw at me.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3053,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"Now you have magic to use against me?\n"
                 "Put it into the ball's spin circle to cast it.\n",
        /*choice_group=*/CHOICE_NONE,
        /*next_storyline=*/3051,
    },
    {
        /*storyline_id=*/3054,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"Now you have magic to use against me?\n"
                 "Put it into the ball's spin circle to cast it.\n",
        /*choice_group=*/CHOICE_NONE,
        /*next_storyline=*/3052,
    },
    {
        /*storyline_id=*/3103,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"Thou hast endured the desert.\n"
                 "Now comes coldness, where the path is eager to make thee slip.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3004,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"Coldness makes all things slip.\n"
                 "So may a soul slip from the path of salvation, or from the womb that bore it. Walk surely.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3104,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"Thou hast not slipped away.\n"
                 "Now comes the broken window: a city's ruin begins with a small breach.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3040,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"This is as far as I may go toward Gomorah.\n"
                 "Beyond the broken glass is Cherubel's realm; I cannot follow thee there.\n",
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
        /*text=*/"Ezekiel's training is ended.\n"
                 "Cherubel waits beyond the broken window.\n",
        /*choice_group=*/CHOICE_NONE,
        /*next_storyline=*/3141,
    },
    {
        /*storyline_id=*/3141,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"Before you answer Cherubel, visit the shop.\n"
                 "Different balls have different characteristics, and you should learn what speaks for your game.\n"
                 "Do you want me to open the shop now?\n",
        /*choice_group=*/CHOICE_MALACH_SHOP_OFFER,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3005,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"I am Cherubel. Who is so bold as to enter Gomorah, where every sin doth flourish?\n"
                 "Through glass, the energy of sin crystallizes into Minerals. Gather them well.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3105,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"Thou hast entered Gomorah.\n"
                 "Now we go to the Desert of Power, where Minerals may be turned into strength.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3006,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"Through glass came gold; through gold came Minerals.\n"
                 "Turn Minerals into energy with Nitro, and take thy portion of this world's drained power.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/30061,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"You have NOS now, but you still will not use it.\n"
                 "I filled your energy. Hold the NOS pedal while your ball is already moving.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3106,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"Carry that power onward.\n"
                 "The last road before the mines is choked with wood.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3007,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"Cut the wood; this world is being made a desert.\n"
                 "Yet use the timber to stop the balls. Thou wilt need it against Seraphel.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3107,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"Thou hast passed my last trial.\n"
                 "I enter not Seraphel's Mineral Mines. Go, and fare thee well.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3008,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"The plague is upon Gomorah, yet I cannot enter the city.\n"
                 "From here we reduce its sinful population. This is my last task with thee.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3108,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"Thou hast done what I required.\n"
                 "Go now to Thrones, who waits within Gomorah.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3009,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"I am Seraphel. These are the Mineral Mines of King Solomon.\n"
                 "Be fierce in thy work, for the buried power is not given freely.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3109,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"The mines have yielded.\n"
                 "Come to the Powerplant: the energy must be extracted.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3010,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"Here Minerals are made into power.\n"
                 "Extract their strength, for these are sinners and deserve not to keep it.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3110,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"The engines have taken their due.\n"
                 "I sent a plague upon Gomorah; come to the cemetery beyond the city.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3011,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"I am Thrones. Gomorah came to lawlessness, and for its deeds it cannot continue to exist.\n"
                 "I know my weight and the impact I make; help me blaze the city to ashes.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3111,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"Gomorah burns, and its smoke rises.\n"
                 "One final work remains beyond its ashes.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3012,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"This is Judgment Day.\n"
                 "The heavens are rolled away, the earth is weighed, and the old world is given to fire.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/3112,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"It came to pass: the earth was made ash.\n"
                 "Thou hast helped destroy evil. Thy trial is ended.\n",
        /*choice_group=*/CHOICE_SCHOOL_OK,
        /*next_storyline=*/0,
    },
    {
        /*storyline_id=*/32000,
        /*speaker=*/SPEAKER_ANGEL,
        /*text=*/"I am Ezekiel, and now it may be spoken plainly:\n"
                 "thou hast passed the trials and helped bring evil to its end.\n"
                 "What wilt thou do next?\n",
        /*choice_group=*/CHOICE_CAMPAIGN_ENDGAME,
        /*next_storyline=*/0,
    },
};

static constexpr StoryChoiceOption STORY_OPTIONS[] = {
    {
        /*choice_id=*/CHOICE_GO_TO_SCHOOL,
        /*option=*/"Enter training",
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
        /*option=*/"Enter training",
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_GO_TO_SCHOOL,
    },
    {
        /*choice_id=*/CHOICE_LEVEL1_SCHOOL_OFFER,
        /*option=*/"Enter training",
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
        /*option=*/"Enter training",
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_GO_TO_SCHOOL,
    },
    {
        /*choice_id=*/CHOICE_FIRST_WIN_NEXT,
        /*option=*/"Compete vs Ezekiel",
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
        /*choice_id=*/CHOICE_SCHOOL_LESSON_SWITCH_CONFIRM,
        /*option=*/"Yes",
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_SCHOOL_CONFIRM_LESSON_SWITCH,
    },
    {
        /*choice_id=*/CHOICE_SCHOOL_LESSON_SWITCH_CONFIRM,
        /*option=*/"No",
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_SCHOOL_CANCEL_LESSON_SWITCH,
    },
    {
        /*choice_id=*/CHOICE_SCHOOL_EXIT_OK,
        /*option=*/"OK",
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_SCHOOL_EXIT,
    },
    {
        /*choice_id=*/CHOICE_SCHOOL_MASS_TEST_DONE,
        /*option=*/"Next",
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_SCHOOL_SELECT_LESSON3,
    },
    {
        /*choice_id=*/CHOICE_SCHOOL_SPIN_TEST_DONE,
        /*option=*/"Next",
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_SCHOOL_SELECT_LESSON4,
    },
    {
        /*choice_id=*/CHOICE_SCHOOL_AIM_TEST_DONE,
        /*option=*/"Next",
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_SCHOOL_SELECT_LESSON2,
    },
    {
        /*choice_id=*/CHOICE_SCHOOL_OIL_TEST_DONE,
        /*option=*/"Next",
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_SCHOOL_SELECT_LESSON5,
    },
    {
        /*choice_id=*/CHOICE_SCHOOL_STRIKE_TEST_DONE,
        /*option=*/"Next",
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
        /*trigger_event=*/EVENT_CONTINUE_CAMPAIGN_AFTER_SHOP_OFFER,
    },
    {
        /*choice_id=*/CHOICE_CAMPAIGN_ENDGAME,
        /*option=*/"Reset Campaign",
        /*goto_storyline=*/0,
        /*trigger_event=*/EVENT_OPEN_RESET_PROGRESS_CONFIRM,
    },
    {
        /*choice_id=*/CHOICE_CAMPAIGN_ENDGAME,
        /*option=*/"Keep Playing",
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

static inline const char *Story_AngelNameForStoryId(TxlLanguage language, int32_t storylineId)
{
    auto localized = [&](const char *en, const char *lt, const char *jp, const char *zh) -> const char *
    {
        if (language == TXL_LANG_LT_LT) return lt;
        if (language == TXL_LANG_JP_JP) return jp;
        if (language == TXL_LANG_ZH_CN) return zh;
        return en;
    };

    switch (storylineId)
    {
        case 3005:
        case 3105:
        case 3006:
        case 30061:
        case 3106:
        case 3007:
        case 3107:
            return localized("Cherubel", "Kerubas", "ケルビム", "基路伯");
        case 3008:
        case 3108:
        case 3009:
        case 3109:
        case 3010:
        case 3110:
            return localized("Seraphel", "Serafelė", "セラフィム", "撒拉弗");
        case 3011:
        case 3111:
        case 3012:
        case 3112:
            return localized("Thrones", "Ofanija", "座天使", "座天使");
        default:
            return localized("Ezekiel", "Ezekielis", "エゼキエル", "以西结");
    }
}

static inline const char *Story_Text(TxlLanguage language, int32_t storylineId, const char *fallback)
{
    if (language == TXL_LANG_LT_LT)
    {
        switch (storylineId)
        {
            case 1: return "Paklydusioji siela, įžengei į Dausas, dvasių pasaulį.\nPažink jo runas ir surink 100, idant kelias tau neužsivertų.\n";
            case 2: return "Ar stosi į pratybas?\n";
            case 10: return "Šimto nesurinkai.\nGal pratybos tau labiau pasitarnaus negu puikybė.\n";
            case 11: return "Ar dabar eisi į pratybas, ar dar sykį mėginsi Dausas?\n";
            case 20: return "Pirmąjį ženklą įveikei.\nAš Ezekielis. Mokysiu ir bandysiu tave, idant būtum išgelbėtas.\n";
            case 21: return "Ar pirma stosi į pratybas, ar dabar stosi prieš Ezekielį?\n";
            case 22: return "Pirmąjį ženklą įveikei.\nAš Ezekielis. Mokysiu ir bandysiu tave, idant būtum išgelbėtas.\n";
            case 30: return "Dar negali palikti mokyklos.\nPirma baik pamoką.\n";
            case 31: return "Ar tikrai?\nTai nutrauks dabartini pamokos bandyma ir prades pasirinkta pamoka.\n";
            case 32: return "Pradeti dabartine pamoka is naujo?\nDabartinis sios pamokos bandymas bus atstatytas.\n";
            case 40: return "Aš Ezekielis, tavo angelas gelbėtojas.\nDausos yra dvasių ir runų pasaulis. Surink 100, idant pereitum pirmąjį ženklą.\nJei geidi vedimo, pratybos atvertos.\n";
            case 41: return "Aš Ezekielis, tavo angelas gelbėtojas.\nDausos yra dvasių ir runų pasaulis. Surink 100, idant pereitum pirmąjį ženklą.\n";
            case 1000: return "2 pamoka. Kamuolio mase.\nCia mokysimes mases.\nKiekvienas kamuolys turi savo mase, todel kitaip jauciasi ir rieda.\nPirmas testas - lengvais kamuoliais pataikyti i keglius.\nBaigimui reikes pataikyti ir sunkiu kamuoliu.\n";
            case 1010: return "Puiku! Mases testa islaikei.\n";
            case 1012: return "Kad islaikytum testa, mases slankikli nustatyk i lengva arba sunku gala.\nMetimas per viduri i pazanga neiskaiciuojamas.\n";
            case 1013: return "Gerai. Dabar perjunk slankikli i sunku gala ir pataikyk i keglius.\n";
            case 1014: return "Gerai. Dabar perjunk slankikli i lengva gala ir pataikyk i keglius.\n";
            case 1030: return "I mokykla gali grizti bet kada.\n";
            case 1020: return "Sveikinu. Sukimosi testa islaikei.\nKiekvienas kamuolys savaip reaguoja i sukima.\nKiti parametrai, pvz. sukibimas, keicia reakcijos stipruma.\n";
            case 1022: return "3 pamoka. Kamuolio sukimas.\nIskart po metimo suk kamuoli judesiais ekrane.\nTada kamuolys ims krypti pasirinkta puse.\nNumusk visus lengvus taikinius, kad islaikytum (2 lygiai).\n";
            case 1052: return "4 pamoka. Alyva ir slydimas.\nTakas ka tik alyvuotas. Maždaug puse ar du trecdaliai tako padengta pilna alyva.\nSioje pamokoje alyva dyla labai greitai, todel po keliu metimu jausmas pasikeis.\nKai kurios sales pacios slidesnes, o kamuoliai turi slydimo parametra.\n";
            case 1054: return "Takui jau reikia naujos alyvos.\nAtidaryk alyvos langa ir peralyvuok pries kita metima.\n";
            case 1060: return "Puiku! Alyvos testa islaikei.\n";
            case 1032: return "1 pamoka. Taikymas.\nDabar mokysimes mesti.\nPatrauk kamuoli iki galo atgal, laikyk per viduri ir paleisk.\nJei pataikysi i bet kuri kegli, gausi taska.\n";
            case 1040: return "Puiku! Taikymo testa islaikei.\n";
            case 1070: return "5 pamoka. Straiko linija.\nSek monetas. Linija nukrypsta nuo vidurio ir grizta i kisene.\nTikslas - ismesti straika.\nGali spausti keisti linija ir treniruotis kitoje puseje.\n";
            case 1072: return "Puiku! Straikas.\nBaigei mokykla.\nGali sugrizti bet kada.\n";
            case 1080: return "Matau, kad sunku.\nGal pabandyk si kamuoli?\n";
            case 1021: return "Sukima gali dar treniruoti zigzago monetu pamokoje.\n3 pamoka dabar atrakinta.\n";
            case 30020: return "Pirmąjį ženklą įveikei.\nAš Ezekielis. Mokysiu ir bandysiu tave; gale laukia Ofanija.\nAr pirma stosi į pratybas, ar tęsi dabar?\n";
            case 3002: return "Aš Ezekielis.\nIeškai išganymo; matuosiu tavo ranką ir valią.\nEikš, teprasideda pirmas bandymas.\n";
            case 3102: return "Gerai padarei.\nPalik ramų taką ir sek mane į Nuodėmių dykumą.\n";
            case 3003: return "Ši dykuma sausa nuo sugedimo.\nPatepk taką alyva ir smilkalais; pertepimas apvalo sugedimą, kaip ugnis apvalo.\n";
            case 30031: return "Pastebejau daug splitu.\nGal bandai pataikyti i keglius is centro.\nVerčiau ivaziuok i juos truputi is sono.\nTai daznai padeda isvengti splitu.\n";
            case 30032: return "Tu dykumoje, o as vis dar nemaciau, kad naudotum alyva.\nGeriau panaudok ja, kol takas nenubaude tavo isdidumo.\nAr atidaryti alyvos langa dabar?\n";
            case 3042: return "Pastebėjau, kad nemeti stiklo atgal, nors aš metu jį į tave.\nTu irgi gali mesti stiklą į mane!\n";
            case 3050:
            case 3053:
            case 3054: return "Dabar turi magijos prieš mane?\nĮdėk ją į kamuolio sukimosi ratą, kad ją panaudotum.\n";
            case 3051: return "Oho, gali sukelti sprogimą?\nTik tu sprendi, ar verta sunaikinti savo kamuolį dėl nedidelės žalos.\n";
            case 3052: return "Matau, gavai rimtos magijos.\nGali sunaikinti mano kamuolius, bet mėginsiu išsisukti nuo to, ką mesi į mane.\n";
            case 3103: return "Dykumą atlaikei.\nDabar šaltis, kur kelias nori tave išslydinti.\n";
            case 3004: return "Šaltis viską daro slidu.\nTaip siela gali nuslysti nuo išganymo kelio ar iš ją nešiojusios įsčios. Eik tvirtai.\n";
            case 3104: return "Nenuszlydai.\nDabar laukia išdužęs langas: miesto pražūtis prasideda nuo mažos properšos.\n";
            case 3040: return "Čia man leista eiti tik iki Gomoros.\nAnapus išdužusio stiklo — Kerubo valdos; ten tavęs lydėti negaliu.\n";
            case 3041: return "Štai taip. Palietei stiklą.\nMano ėjimų metu galėsi mesti stiklą atgal.\nStebėk ėjimo mygtuką.\n";
            case 3140: return "Ezekielio pratybos baigtos.\nKerubas laukia anapus išdužusio lango.\n";
            case 3141: return "Prieš atsakydamas Kerubui, užeik į parduotuvę.\nSkirtingi kamuoliai turi skirtingas savybes, ir verta rasti tai, kas tinka tavo žaidimui.\nAr atidaryti parduotuvę dabar?\n";
            case 3005: return "Aš Kerubas. Kas toks drąsus žengia į Gomorą, kur visos nuodėmės žydi?\nPer stiklą nuodėmių jėga kristalėja į Mineralus. Rink juos rūpestingai.\n";
            case 3105: return "Įžengei į Gomorą.\nDabar keliausime į Galios dykumą, kur Mineralai virsta jėga.\n";
            case 3006: return "Per stiklą atėjo auksas, per auksą — Mineralai.\nPaversk Mineralus energija per Nitro ir pasiimk dalį išsiurbtos pasaulio galios.\n";
            case 30061: return "Dabar turi NOS, bet vis dar jo nenaudoji.\nPripildžiau tavo energiją. Kamuoliui jau judant laikyk NOS pedalą.\n";
            case 3106: return "Nešk šią galią pirmyn.\nPaskutinis kelias prie kasyklų užgultas medžiais.\n";
            case 3007: return "Kirski medžius; šis pasaulis paverčiamas dykuma.\nBet medžiu stabdyk kamuolius. Jo reikės prieš Serafelę.\n";
            case 3107: return "Paskutinį mano bandymą įveikei.\nĮ Serafelės Mineralų kasyklas neinu. Eik, ir tebūna tau gerai.\n";
            case 3008: return "Maras apėmė Gomorą, bet į miestą įeiti negaliu.\nIš čia mažiname jos nuodėmingą gyventojų skaičių. Tai mano paskutinė užduotis su tavimi.\n";
            case 3108: return "Padarei, ko reikalavau.\nEik pas Ofaniją, kuri laukia Gomoroje.\n";
            case 3009: return "Aš Serafelė. Tai Karaliaus Saliamono Mineralų kasyklos.\nBūk nuožmus darbe, nes palaidota galia lengvai neduodama.\n";
            case 3109: return "Kasyklos davė derlių.\nAteik į Jėgainę: energija turi būti išgauta.\n";
            case 3010: return "Čia Mineralai paverčiami galia.\nIštrauk jų stiprybę, nes tai nusidėjėliai ir neverti jos laikyti.\n";
            case 3110: return "Varikliai pasiėmė savo dalį.\nPasiunčiau marą į Gomorą; ateik į kapines už miesto.\n";
            case 3011: return "Aš Ofanija. Gomora pasidavė nedorybei, ir dėl savo darbų nebegali toliau gyvuoti.\nŽinau savo svorį ir smūgį; padėk man miestą paversti pelenais.\n";
            case 3111: return "Gomora liepsnoja, ir jos dūmai kyla.\nAnapus jos pelenų laukia paskutinis darbas.\n";
            case 3012: return "Tai Teismo diena.\nDangus susisuka, žemė pasveriama, o senasis pasaulis atiduodamas ugniai.\n";
            case 3112: return "Ir taip nutiko: žemė virto pelenais.\nPadėjai sunaikinti blogį. Tavo išbandymas baigtas.\n";
            case 32000: return "Aš Ezekielis, ir dabar galima tarti aiškiai:\ntu perėjai bandymus ir padėjai blogį sunaikinti.\nKą darysi toliau?\n";
            default: return fallback;
        }
    }

    if (language == TXL_LANG_JP_JP)
    {
        switch (storylineId)
        {
            case 1: return "ボウリングレーンに立っている。\n最初の目標は1ゲームで100点。\n達成すれば魔法のお守りが手に入る。\n";
            case 2: return "チュートリアルを受ける？\n";
            case 10: return "100点に届かなかった。\n今は意地より学校が役に立つかもしれない。\n";
            case 11: return "今すぐ学校へ行く？ それともレベル1をもう一度？\n";
            case 20: return "最初の試験を突破した。\n正体を明かそう。私はエゼキエル。君と投げ合う。\n";
            case 21: return "先に学校へ行く？ それともレベル2へ進む？\n";
            case 22: return "最初の試験を突破した。\n正体を明かそう。私はエゼキエル。君と投げ合う。\n";
            case 30: return "まだ学校を出られない。\n先にレッスンを終えよう。\n";
            case 31: return "本当に？\n今のレッスン挑戦を中止し、選んだレッスンを始めます。\n";
            case 32: return "今のレッスンをやり直す？\nこのレッスンの挑戦はリセットされます。\n";
            case 40: return "私はエゼキエル。このレーンの君の天使だ。\nレベル1は100点で突破。遠くから見守る。\n先に助けが欲しければ、学校は開いている。\n";
            case 41: return "私はエゼキエル。このレーンの君の天使だ。\nレベル1は100点で突破。遠くから見守る。\n";
            case 1000: return "レッスン2：球の質量。\nここでは質量を学ぶ。\n球ごとに質量があり、感触と転がりが変わる。\nまず軽い球でピンに当てよう。\n卒業には重い球でも当てる必要がある。\n";
            case 1010: return "よし！ 質量テスト合格。\n";
            case 1012: return "合格には質量スライダーを軽い端か重い端に合わせて。\n中間で投げても進行には入らない。\n";
            case 1013: return "いいね。次は重い端に切り替えてピンに当てよう。\n";
            case 1014: return "いいね。次は軽い端に切り替えてピンに当てよう。\n";
            case 1030: return "学校にはいつでも戻れる。\n";
            case 1020: return "おめでとう。回転テスト合格。\n球ごとに回転への反応がある。\n噛みつきなどの値も反応の強さを変える。\n";
            case 1022: return "レッスン3：球の回転。\n投げた直後、画面上の動きで球に回転をかける。\nすると球は特定の方向へ曲がり始める。\n軽い標的ピンを全部倒せば合格（2レベル）。\n";
            case 1052: return "レッスン4：オイルと滑り。\nこのレーンは塗りたて。半分から三分の二ほどが最大オイルだ。\nこのレッスンではオイルがすぐ減るので、数投で感触が変わる。\n場ごとの滑りや、球のスキッド値も影響する。\n";
            case 1054: return "レーンに新しいオイルが必要だ。\nオイル画面を開き、塗り直してから投げよう。\n";
            case 1060: return "よし！ オイルテスト合格。\n";
            case 1032: return "レッスン1：狙い。\n投げ方を学ぼう。\n球をしっかり後ろへ引き、中央に保って離す。\nどのピンでも当てれば1点だ。\n";
            case 1040: return "よし！ 狙いのテスト合格。\n";
            case 1070: return "レッスン5：ストライク線。\nコインを追え。線は中央から外れ、ポケットへ戻る。\n目標はストライク。\n反対側を練習したい時はライン切替を押せる。\n";
            case 1072: return "見事！ ストライク。\n学校を卒業した。\nいつでも戻っていい。\n";
            case 1080: return "苦戦しているね。\nこの球を試してみる？\n";
            case 1021: return "ジグザグのコインで回転をもっと練習できる。\nレッスン3が解放された。\n";
            case 30020: return "最初の試験を突破した。\n正体を明かそう。私はエゼキエル。レベル2は私が相手だ。\n先に学校へ行く？ それとも今すぐ続ける？\n";
            case 3002: return "私はエゼキエル。\n君の初勝利を遠くから見ていた。\n感覚はある。圧力の中で保てるか見せて。\n";
            case 3102: return "悪くない。\n普通のレーンの安心を離れ、砂漠へ来い。\n";
            case 3003: return "砂漠のレーンはオイルが早く減る。\n手前を見ろ。君の番では自尊心より先にオイルを考えろ。\n";
            case 30031: return "スプリットが多いね。\n中央からピンを打とうとしているのかもしれない。\n少し横から入るといい。\nたいていスプリットを防ぎやすい。\n";
            case 30032: return "砂漠にいるのに、まだオイルを使っていないね。\nレーンに罰される前に使った方がいい。\n今オイル画面を開こうか？\n";
            case 3042: return "僕が君にガラスを投げているのに、君は投げ返していないね。\n君も僕にガラスを投げられるよ！\n";
            case 3050:
            case 3053:
            case 3054: return "これで私に使える魔法を手に入れたね。\n球の回転サークルに入れて、魔法を使うんだ。\n";
            case 3051: return "おや、爆発を起こせるのか？\n少しのダメージのために自分の球を壊す価値があるかは、君次第だ。\n";
            case 3052: return "本格的な魔法を手に入れたようだね。\n私の球を壊せるが、君が投げるものは避けようとするよ。\n";
            case 3103: return "順応したね。\n次は氷だ。あのレーンは笑いながら嘘をつく。\n";
            case 3004: return "氷は長く、滑り、忍耐強い。\n信じすぎず、もっと滑らせろ。必要なら店で合う球を探せ。\n";
            case 3104: return "耐えたね。\nネオンへ行こう。まずガラスで教え、それから別の者に渡す。\n";
            case 3040: return "今のネオンは私たちの教室だ。\n君が投げる時、ときどきレーンにガラスを入れる。\n慌てず、まず何をするか学べ。\n";
            case 3041: return "そうだ。ガラスに触れたね。\n私の番では君もガラスを返せる。\nターンボタンを見て。\n";
            case 3140: return "この授業は終わりだ。\nケルビムはネオンの下を長く歩いていた。今はこのレーンを欲しがっている。\n";
            case 3141: return "ケルビムに応じる前に、店へ行こう。\n球ごとに性格が違う。君の投げ方に合うものを知るべきだ。\n今、店を開く？\n";
            case 3005: return "私はケルビム。\n噛みごたえのある勝負と、反撃するプレイヤーが好きだ。\n退くのか、返すのか見せて。\n";
            case 3105: return "悪くない。\n次は普通のレーンへ戻る。今回はNOSを許可する。\n";
            case 3006: return "投球中にNOSを使えるようになった。\nおもちゃのように押すな。球に速度が乗ってから押し続け、力をレーンへ通せ。\n";
            case 30061: return "NOSが使えるのに、まだ使おうとしないね。\nエネルギーは満たしておいた。球が動き出してからNOSペダルを押し続けろ。\n";
            case 3106: return "その力を砂漠へ持って行け。\n最後の勝負の前に、木を私の道へ置くことも許そう。\n";
            case 3007: return "また砂漠だ。\n私が投げる時、君は木を置ける。\n飾りではなく、返答として使え。\n";
            case 3107: return "私を耐え抜いたね。\nセラフィムが黙って見ていた。たいてい、それはもっと悪い。\n";
            case 3008: return "私はセラフィム。\n砂漠は形を保てるものだけを残す。\n私は吠えない。待ち、そして決める。\n";
            case 3108: return "興味深い。\n氷上へ来い。君が均衡を保つ間、私は秘密を保つ。\n";
            case 3009: return "氷は冷静な手に報いる。\n抑制を弱さと間違えるな。\n";
            case 3109: return "ネオンに最後の章がある。\n最後の私のレベルの前に、レンガを渡そう。\n";
            case 3010: return "ネオンは偽りを剥がす。\n今から私のターンでレンガを使える。無駄にするな。\n";
            case 3110: return "取れるなら、その勝利を持っていけ。\nもっと大きく、騒がしく、忍耐のない者が向かっている。\n";
            case 3011: return "私は座天使。\n自分の重さも価値も知っている。君を楽にするために来たのではない。\n君の腕がここまでの上昇ほど勇敢か見よう。\n";
            case 3111: return "君は街の光を耐えた。\n残るレベルは一つ。そこでコンクリートを許可する。\n";
            case 3012: return "私はまだ座天使。そしてこれが最後の授業だ。\n私が投げる時、今度はコンクリートを置ける。\n突破すればキャンペーン完了だ。\n";
            case 3112: return "君は私を倒した。\nコンクリートは持ちこたえ、行進は終わり、全レベルが片付いた。\nこれでキャンペーンは終わりだ。\n";
            case 32000: return "私はエゼキエル。今ならはっきり言える。\n君は私たち全員を倒し、キャンペーンを終えた。\n次はどうする？\n";
            default: return fallback;
        }
    }

    if (language != TXL_LANG_ZH_CN)
        return fallback;

    switch (storylineId)
    {
        case 1: return "你站在一条保龄球道上。\n你的第一个里程碑，是在单局里拿到100分。\n如果做到，你会得到一枚魔法护符。\n";
        case 2: return "你想要一个教学吗？\n";
        case 10: return "你没有达到100分。\n也许学校比逞强更适合现在的你。\n";
        case 11: return "你现在想去学校，还是先再试一次第一关？\n";
        case 20: return "你达到了100分。\n你证明了自己已经有资格挑战我。\n";
        case 21: return "你想先去学校（教学），还是现在就和天使对战？\n";
        case 22: return "你通过了最初的测试。\n现在我可以现身了：我是以西结，我会和你对战。\n";
        case 30: return "你现在还不能离开学校。\n先把课程完成。\n";
        case 31: return "确定吗？\n这会取消当前课程尝试，并开始你选择的课程。\n";
        case 32: return "重新开始当前课程？\n本课程的当前尝试会被重置。\n";
        case 40: return "我是以西结，是这条球道上守护你的天使。\n第1关拿到100分即可通过。我会在远处看着。\n如果你想先练习，学校已经开放。\n";
        case 41: return "我是以西结，是这条球道上守护你的天使。\n第1关拿到100分即可通过。我会在远处看着。\n";
        case 1000: return "第2课：球的质量。\n这里讲的是质量。\n每个球都有自己的质量，质量会改变它的手感和滚动方式。\n你的第一个测试，是用几颗轻球击中球瓶。\n而想毕业，你也必须用重球击中球瓶。\n";
        case 1010: return "很好！你通过了质量测试。\n";
        case 1012: return "要通过这个测试，你必须把质量滑块调到最轻或最重的一端。\n停在中间不会算进通过进度。\n";
        case 1013: return "很好。现在把滑块切到重球一端，再去击中球瓶。\n";
        case 1014: return "很好。现在把滑块切到轻球一端，再去击中球瓶。\n";
        case 1030: return "你随时都可以回到学校。\n";
        case 1020: return "恭喜。你通过了旋转测试。\n每颗球对旋转都有自己天生的反应。\n像咬道这样的参数，也会影响它对旋转的响应程度。\n";
        case 1022: return "第3课：给球上旋。\n在球出手后，立刻在屏幕上做旋转动作给球加旋。\n这样球就会开始朝某个方向发力。\n击倒所有轻量目标球瓶才能通过（共2关）。\n";
        case 1052: return "第4课：油与滑行。\n这条球道刚刚上过油，大约有半条到三分之二的长度都覆盖着满油。\n这一课里油会消耗得很快，所以打几球之后手感会明显改变。\n有些球馆本身就更滑，而球也有自己的滑行参数。\n";
        case 1054: return "现在球道需要重新上油了。\n打开油道窗口，先重新上油，再继续投球。\n";
        case 1060: return "很好！你通过了油道测试。\n";
        case 1032: return "第1课：瞄准课。\n现在我们来学习如何出手。\n把球尽量往后拉，保持在中间，然后放手。\n只要击中任何球瓶，你就能得1分。\n";
        case 1040: return "很好！你通过了瞄准测试。\n";
        case 1070: return "第5课：全中线路。\n跟着金币走。那条线会先从中间弯开，再回到口袋位。\n你的目标，是打出一次全中。\n你可以切换线路，练习另一侧口袋。\n";
        case 1072: return "漂亮！全中。\n你从学校毕业了。\n你随时都可以回来。\n";
        case 1080: return "我看得出你有点吃力。\n要不要试试这颗球？\n";
        case 1021: return "你可以继续在之字形金币那一课练更多旋转。\n第3课已经解锁。\n";
        case 30020: return "你通过了最初的测试。\n现在我可以现身了：我是以西结，第2关会由我来对战。\n你想先去学校，还是现在继续？\n";
        case 3002: return "我是以西结。\n我在远处看见了你第一次通关。\n你有手感，而我想看看你能不能在压力下守住它。\n";
        case 3102: return "不错。\n现在离开普通球道的舒适区，跟我去沙漠。\n";
        case 3003: return "这条沙漠球道的油耗得很快。\n注意前段，等轮到你时，先想到油，再想到自尊。\n";
        case 30031: return "我注意到你打出了很多分瓶。\n也许你正从中间去撞球瓶。\n试着从侧面切进去。\n这样通常更不容易分瓶。\n";
        case 30032: return "你在沙漠里，我却还没看见你用油。\n最好在球道惩罚你的自尊之前用上它。\n要我现在帮你打开油道窗口吗？\n";
        case 3042: return "我一直向你丢玻璃，你却还没有丢回来。\n你也可以向我丢玻璃！\n";
        case 3050:
        case 3053:
        case 3054: return "现在你有能对我使用的魔法了？\n把它放进球的旋转圆环里，就能施放魔法。\n";
        case 3051: return "哦，你能引发爆炸？\n要不要为了这一点伤害毁掉自己的球，由你决定。\n";
        case 3052: return "看来你得到了很厉害的魔法。\n你可以毁掉我的球，但我会尽力躲开你朝我丢来的东西。\n";
        case 3103: return "你适应过来了。\n接下来是冰面，那条球道会一边微笑，一边说谎。\n";
        case 3004: return "冰面很长，很滑，也很有耐心。\n少一点相信，多一点滑行；如果需要一颗会说这种语言的球，就去商店。\n";
        case 3104: return "你挺过去了。\n现在跟我去霓虹。我想先用玻璃给你上一课，然后再把你交给别人。\n";
        case 3040: return "现在霓虹就是我们的教室。\n当你出手时，我有时会把玻璃丢进你的球道。\n别慌，先学它会做什么。\n";
        case 3041: return "就是这样。你已经碰到玻璃了。\n等轮到我出手时，你也可以用玻璃回敬我。\n留意回合按钮。\n";
        case 3140: return "这节课结束了。\n基路伯已经在霓虹灯下踱步很久，现在他想要这条球道。\n";
        case 3141: return "在回应基路伯之前，先去商店看看。\n不同的球有不同特性，你应该了解哪一种适合你的打法。\n要我现在打开商店吗？\n";
        case 3005: return "我是基路伯。\n我喜欢有咬劲的比赛，也喜欢会反击的玩家。\n让我看看你是会缩，还是会回。\n";
        case 3105: return "不赖。\n下一关我们回到普通球道，而且这次我允许你使用 NOS。\n";
        case 3006: return "现在你可以在出手时使用 NOS。\n别把它当玩具乱点。等球已经有速度时按住它，把力量送穿整条球道。\n";
        case 30061: return "你现在有 NOS 了，却还是不用。\n我已经把能量充满了。球开始移动后，按住 NOS 踏板。\n";
        case 3106: return "把这股力量带去沙漠。\n在我和你的最后一关之前，我还会让你把木块丢到我的路线上。\n";
        case 3007: return "又是沙漠。\n这次当我出手时，你可以放木块。\n把它当成一种回嘴，而不是装饰。\n";
        case 3107: return "你挺过我了。\n撒拉弗一直在沉默地看着，而这通常更糟。\n";
        case 3008: return "我是撒拉弗。\n沙漠只留下能保持形状的东西。\n我不吠。我等着，然后由我来决定。\n";
        case 3108: return "你让我感兴趣。\n来冰面上，在你保持平衡的时候，让我继续藏着秘密。\n";
        case 3009: return "冰面奖励冷静的手。\n不要把克制误认为软弱。\n";
        case 3109: return "在霓虹里还有最后一章。\n在我最后一关之前，我会把砖块也交给你。\n";
        case 3010: return "霓虹会剥掉伪装。\n现在你也可以在我出手时用砖块了。别浪费它们。\n";
        case 3110: return "如果你拿得到，就把这场胜利带走。\n一个更大声、更夸张、也更没耐心的家伙已经在路上了。\n";
        case 3011: return "我是座天使。\n我知道自己的重量，也知道自己的价值，而且我来这里不是为了让你轻松。\n让我们看看，你的球技是不是和你的攀升一样勇敢。\n";
        case 3111: return "你已经扛住了城市的灯光。\n还有最后一关在等你，而那一关里我会允许你使用混凝土。\n";
        case 3012: return "我还是座天使，而这就是最后一课。\n现在当我出手时，你可以放混凝土。\n通过这一关，整段战役就完成了。\n";
        case 3112: return "你击败了我。\n混凝土撑住了，游行结束了，所有关卡都已清完。\n这就是战役的终点。\n";
        case 32000: return "我是以西结，现在可以直说了：\n你击败了我们所有人，也完成了整段战役。\n接下来想做什么？\n";
        default: return fallback;
    }
}

static inline const char *Story_OptionText(TxlLanguage language, const StoryChoiceOption &opt)
{
    if (language == TXL_LANG_LT_LT)
    {
        if (opt.choice_id == CHOICE_TUTORIAL_YES_NO && strcmp(opt.option, "Yes") == 0) return "Taip";
        if (opt.choice_id == CHOICE_TUTORIAL_YES_NO && strcmp(opt.option, "No") == 0) return "Ne";
        if (strcmp(opt.option, "Enter training") == 0) return "Eiti į pratybas";
        if (strcmp(opt.option, "Not now") == 0) return "Ne dabar";
        if (strcmp(opt.option, "Compete vs Ezekiel") == 0) return "Varžytis su Ezekieliu";
        if (strcmp(opt.option, "Compete vs Angel") == 0) return "Varžytis su angelu";
        if (strcmp(opt.option, "OK") == 0 || strcmp(opt.option, "Ok") == 0) return "Gerai";
        if (strcmp(opt.option, "Next") == 0) return "Toliau";
        if (strcmp(opt.option, "Open oil") == 0) return "Atidaryti alyvą";
        if (strcmp(opt.option, "Open shop") == 0) return "Atidaryti parduotuvę";
        if (strcmp(opt.option, "No thanks") == 0) return "Ne, ačiū";
        if (strcmp(opt.option, "Reset Campaign") == 0) return "Pradėti kampaniją iš naujo";
        if (strcmp(opt.option, "Yes, take me to the next lesson") == 0) return "Taip, veskite mane į kitą pamoką";
        if (strcmp(opt.option, "No, I want to leave school") == 0) return "Ne, noriu išeiti iš mokyklos";
        if (strcmp(opt.option, "Practice more") == 0) return "Praktikuotis dar";
        if (strcmp(opt.option, "Back to game") == 0) return "Atgal į žaidimą";
        if (strcmp(opt.option, "Keep Playing") == 0) return "Zaisti toliau";
        if (strcmp(opt.option, "Decline") == 0) return "Atsisakyti";
        return opt.option;
    }
    if (language == TXL_LANG_JP_JP)
    {
        if (opt.choice_id == CHOICE_TUTORIAL_YES_NO && strcmp(opt.option, "Yes") == 0) return "はい";
        if (opt.choice_id == CHOICE_TUTORIAL_YES_NO && strcmp(opt.option, "No") == 0) return "いいえ";
        if (strcmp(opt.option, "Enter training") == 0) return "学校へ行く";
        if (strcmp(opt.option, "Not now") == 0) return "今はやめる";
        if (strcmp(opt.option, "Compete vs Ezekiel") == 0) return "エゼキエルと対戦";
        if (strcmp(opt.option, "Compete vs Angel") == 0) return "天使と対戦";
        if (strcmp(opt.option, "OK") == 0 || strcmp(opt.option, "Ok") == 0) return "確認";
        if (strcmp(opt.option, "Next") == 0) return "次へ";
        if (strcmp(opt.option, "Open oil") == 0) return "オイルを開く";
        if (strcmp(opt.option, "Open shop") == 0) return "ショップを開く";
        if (strcmp(opt.option, "No thanks") == 0) return "やめておく";
        if (strcmp(opt.option, "Reset Campaign") == 0) return "キャンペーンをやり直す";
        if (strcmp(opt.option, "Yes, take me to the next lesson") == 0) return "はい、次のレッスンへ";
        if (strcmp(opt.option, "No, I want to leave school") == 0) return "いいえ、学校を出たい";
        if (strcmp(opt.option, "Practice more") == 0) return "もっと練習する";
        if (strcmp(opt.option, "Back to game") == 0) return "ゲームに戻る";
        if (strcmp(opt.option, "Keep Playing") == 0) return "プレイを続ける";
        if (strcmp(opt.option, "Decline") == 0) return "断る";
        return opt.option;
    }
    if (language != TXL_LANG_ZH_CN)
        return opt.option;

    if (opt.choice_id == CHOICE_TUTORIAL_YES_NO && strcmp(opt.option, "Yes") == 0) return "是";
    if (opt.choice_id == CHOICE_TUTORIAL_YES_NO && strcmp(opt.option, "No") == 0) return "否";
    if (strcmp(opt.option, "Enter training") == 0) return "去学校";
    if (strcmp(opt.option, "Not now") == 0) return "现在先不去";
    if (strcmp(opt.option, "Compete vs Ezekiel") == 0) return "和以西结对战";
    if (strcmp(opt.option, "Compete vs Angel") == 0) return "和天使对战";
    if (strcmp(opt.option, "OK") == 0) return "确定";
    if (strcmp(opt.option, "Ok") == 0) return "确定";
    if (strcmp(opt.option, "Next") == 0) return "下一步";
    if (strcmp(opt.option, "Open oil") == 0) return "打开油道";
    if (strcmp(opt.option, "Open shop") == 0) return "打开商店";
    if (strcmp(opt.option, "No thanks") == 0) return "不用了";
    if (strcmp(opt.option, "Reset Campaign") == 0) return "重开战役";
    if (strcmp(opt.option, "Yes, take me to the next lesson") == 0) return "好，带我去下一课";
    if (strcmp(opt.option, "No, I want to leave school") == 0) return "不，我想离开学校";
    if (strcmp(opt.option, "Practice more") == 0) return "继续练习";
    if (strcmp(opt.option, "Back to game") == 0) return "回到游戏";
    if (strcmp(opt.option, "Keep Playing") == 0) return "继续玩";
    if (strcmp(opt.option, "Decline") == 0) return "拒绝";
    return opt.option;
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
