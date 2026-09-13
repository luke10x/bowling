#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../3rdparty/json/tests/thirdparty/doctest/doctest.h"

#define CLAY_IMPLEMENTATION
#include "../clayton/numkeypad.h"

extern "C" {
Uint64 SDL_GetTicks64(void) { return 0; }
}

TEST_CASE("NumKeypad decimal range disables prefixes that cannot become valid values")
{
    NumKeypadRules rules {
        .minValue = 0,
        .maxValue = 127,
        .base = NUMKEYPAD_BASE_DECIMAL,
        .allowZeroValue = false,
    };
    const char *empty = "";

    CHECK_FALSE(NumKeypad_CanEnter(rules, empty, 0));
    CHECK_FALSE(NumKeypad_CanAppendDigit(rules, empty, 0, 0));
    for (int32_t digit = 1; digit <= 9; ++digit)
        CHECK(NumKeypad_CanAppendDigit(rules, empty, 0, digit));

    const char *three = "3";
    CHECK(NumKeypad_CanEnter(rules, three, 1));
    for (int32_t digit = 0; digit <= 9; ++digit)
        CHECK(NumKeypad_CanAppendDigit(rules, three, 1, digit));

    const char *twelve = "12";
    CHECK(NumKeypad_CanEnter(rules, twelve, 2));
    for (int32_t digit = 0; digit <= 7; ++digit)
        CHECK(NumKeypad_CanAppendDigit(rules, twelve, 2, digit));
    CHECK_FALSE(NumKeypad_CanAppendDigit(rules, twelve, 2, 8));
    CHECK_FALSE(NumKeypad_CanAppendDigit(rules, twelve, 2, 9));

    const char *oneTwentySeven = "127";
    CHECK(NumKeypad_CanEnter(rules, oneTwentySeven, 3));
    for (int32_t digit = 0; digit <= 9; ++digit)
        CHECK_FALSE(NumKeypad_CanAppendDigit(rules, oneTwentySeven, 3, digit));
}

TEST_CASE("NumKeypad range can require a partial prefix before enter is allowed")
{
    NumKeypadRules rules {
        .minValue = 30,
        .maxValue = 39,
        .base = NUMKEYPAD_BASE_DECIMAL,
        .allowZeroValue = false,
    };
    const char *empty = "";
    const char *three = "3";
    const char *thirty = "30";

    CHECK(NumKeypad_CanAppendDigit(rules, empty, 0, 3));
    CHECK_FALSE(NumKeypad_CanAppendDigit(rules, empty, 0, 2));
    CHECK_FALSE(NumKeypad_CanAppendDigit(rules, empty, 0, 4));

    CHECK_FALSE(NumKeypad_CanEnter(rules, three, 1));
    for (int32_t digit = 0; digit <= 9; ++digit)
        CHECK(NumKeypad_CanAppendDigit(rules, three, 1, digit));

    CHECK(NumKeypad_CanEnter(rules, thirty, 2));
    for (int32_t digit = 0; digit <= 9; ++digit)
        CHECK_FALSE(NumKeypad_CanAppendDigit(rules, thirty, 2, digit));
}

TEST_CASE("NumKeypad zero is opt-in and never becomes a leading zero prefix")
{
    NumKeypadRules rules {
        .minValue = 0,
        .maxValue = 9,
        .base = NUMKEYPAD_BASE_DECIMAL,
        .allowZeroValue = true,
    };
    const char *empty = "";
    const char *zero = "0";

    CHECK(NumKeypad_CanAppendDigit(rules, empty, 0, 0));
    CHECK(NumKeypad_CanEnter(rules, zero, 1));
    for (int32_t digit = 0; digit <= 9; ++digit)
        CHECK_FALSE(NumKeypad_CanAppendDigit(rules, zero, 1, digit));

    rules.allowZeroValue = false;
    CHECK_FALSE(NumKeypad_CanAppendDigit(rules, empty, 0, 0));
    CHECK_FALSE(NumKeypad_CanEnter(rules, zero, 1));
}

TEST_CASE("NumKeypad hex mode allows A-F only when they can lead to a valid value")
{
    NumKeypadRules rules {
        .minValue = 0xA0,
        .maxValue = 0xAF,
        .base = NUMKEYPAD_BASE_HEX,
        .allowZeroValue = false,
    };
    const char *empty = "";
    const char *a = "A";
    const char *af = "AF";

    CHECK(NumKeypad_CanAppendDigit(rules, empty, 0, 10));
    CHECK_FALSE(NumKeypad_CanAppendDigit(rules, empty, 0, 9));
    CHECK_FALSE(NumKeypad_CanAppendDigit(rules, empty, 0, 11));

    CHECK_FALSE(NumKeypad_CanEnter(rules, a, 1));
    for (int32_t digit = 0; digit < NUMKEYPAD_BASE_HEX; ++digit)
        CHECK(NumKeypad_CanAppendDigit(rules, a, 1, digit));

    CHECK(NumKeypad_CanEnter(rules, af, 2));
    for (int32_t digit = 0; digit < NUMKEYPAD_BASE_HEX; ++digit)
        CHECK_FALSE(NumKeypad_CanAppendDigit(rules, af, 2, digit));
}

TEST_CASE("NumKeypad delete recalculates enabled keys from the shortened prefix")
{
    int32_t value = 0;
    NumKeypad keypad {};
    initNumKeypad(&keypad, &value, 0, 127);
    keypad.currentText[0] = '1';
    keypad.currentText[1] = '2';
    keypad.currentText[2] = '7';
    keypad.currentTextLen = 3;

    CHECK(NumKeypad_CanSubmit(&keypad));
    CHECK_FALSE(NumKeypad_CanAppend(&keypad, 0));

    keypad.currentTextLen -= 1;
    CHECK(NumKeypad_CanSubmit(&keypad));
    CHECK(NumKeypad_CanAppend(&keypad, 0));
    CHECK(NumKeypad_CanAppend(&keypad, 7));
    CHECK_FALSE(NumKeypad_CanAppend(&keypad, 8));
}

TEST_CASE("NumKeypad exposes minus only as an initial negative prefix")
{
    NumKeypadRules rules {
        .minValue = -127,
        .maxValue = 127,
        .base = NUMKEYPAD_BASE_DECIMAL,
        .allowZeroValue = false,
    };
    const char *empty = "";
    const char *minus = "-";
    const char *negativeOne = "-1";
    const char *one = "1";

    CHECK(NumKeypad_CanAppendMinus(rules, empty, 0));
    CHECK_FALSE(NumKeypad_CanEnter(rules, minus, 1));
    CHECK_FALSE(NumKeypad_CanAppendMinus(rules, minus, 1));
    CHECK_FALSE(NumKeypad_CanAppendMinus(rules, one, 1));

    CHECK(NumKeypad_CanEnter(rules, negativeOne, 2));
    for (int32_t digit = 0; digit <= 9; ++digit)
        CHECK(NumKeypad_CanAppendDigit(rules, negativeOne, 2, digit));
}

TEST_CASE("NumKeypad negative range prunes digits by remaining possible values")
{
    NumKeypadRules rules {
        .minValue = -127,
        .maxValue = -120,
        .base = NUMKEYPAD_BASE_DECIMAL,
        .allowZeroValue = false,
    };
    const char *minus = "-";
    const char *negativeOne = "-1";
    const char *negativeTwelve = "-12";
    const char *negativeOneTwentySeven = "-127";

    CHECK_FALSE(NumKeypad_CanEnter(rules, minus, 1));
    CHECK(NumKeypad_CanAppendDigit(rules, minus, 1, 1));
    CHECK_FALSE(NumKeypad_CanAppendDigit(rules, minus, 1, 2));

    CHECK_FALSE(NumKeypad_CanEnter(rules, negativeOne, 2));
    CHECK(NumKeypad_CanAppendDigit(rules, negativeOne, 2, 2));
    CHECK_FALSE(NumKeypad_CanAppendDigit(rules, negativeOne, 2, 1));
    CHECK_FALSE(NumKeypad_CanAppendDigit(rules, negativeOne, 2, 3));

    CHECK_FALSE(NumKeypad_CanEnter(rules, negativeTwelve, 3));
    for (int32_t digit = 0; digit <= 7; ++digit)
        CHECK(NumKeypad_CanAppendDigit(rules, negativeTwelve, 3, digit));
    CHECK_FALSE(NumKeypad_CanAppendDigit(rules, negativeTwelve, 3, 8));
    CHECK_FALSE(NumKeypad_CanAppendDigit(rules, negativeTwelve, 3, 9));

    CHECK(NumKeypad_CanEnter(rules, negativeOneTwentySeven, 4));
    for (int32_t digit = 0; digit <= 9; ++digit)
        CHECK_FALSE(NumKeypad_CanAppendDigit(rules, negativeOneTwentySeven, 4, digit));
}

TEST_CASE("NumKeypad delete recalculates negative prefix options")
{
    int32_t value = 0;
    NumKeypad keypad {};
    initNumKeypad(&keypad, &value, -50, -10);
    keypad.currentText[0] = '-';
    keypad.currentText[1] = '5';
    keypad.currentText[2] = '0';
    keypad.currentTextLen = 3;

    CHECK(NumKeypad_CanSubmit(&keypad));
    CHECK_FALSE(NumKeypad_CanAppendMinus(&keypad));
    CHECK_FALSE(NumKeypad_CanAppend(&keypad, 0));

    keypad.currentTextLen -= 1;
    CHECK_FALSE(NumKeypad_CanSubmit(&keypad));
    CHECK(NumKeypad_CanAppend(&keypad, 0));
    CHECK_FALSE(NumKeypad_CanAppend(&keypad, 1));

    keypad.currentTextLen = 0;
    CHECK(NumKeypad_CanAppendMinus(&keypad));
    CHECK_FALSE(NumKeypad_CanSubmit(&keypad));
}

TEST_CASE("NumKeypad uploads and parses negative values")
{
    int32_t value = -42;
    NumKeypad keypad {};
    initNumKeypad(&keypad, &value, -127, 127);
    uploadNumKeypadValue(&keypad);

    REQUIRE(keypad.currentTextLen == 3);
    CHECK(keypad.currentText[0] == '-');
    CHECK(keypad.currentText[1] == '4');
    CHECK(keypad.currentText[2] == '2');
    CHECK(NumKeypad_CurrentValue(&keypad) == -42);
    CHECK(NumKeypad_CanSubmit(&keypad));
}

TEST_CASE("NumKeypad sparse allowed values prune prefixes and submit only exact matches")
{
    static const int32_t allowed[] = {0x00, 0x01, 0x0A, 0x10, 0x55, 0xF5, 0xF6};
    NumKeypadRules rules {
        .minValue = 0,
        .maxValue = 255,
        .base = NUMKEYPAD_BASE_HEX,
        .allowZeroValue = true,
        .allowedValues = allowed,
        .allowedValueCount = (int32_t)(sizeof(allowed) / sizeof(allowed[0])),
    };
    const char *empty = "";
    const char *zero = "0";
    const char *f = "F";
    const char *f5 = "F5";
    const char *f7 = "F7";

    CHECK(NumKeypad_CanAppendDigit(rules, empty, 0, 0));
    CHECK(NumKeypad_CanAppendDigit(rules, empty, 0, 1));
    CHECK(NumKeypad_CanAppendDigit(rules, empty, 0, 5));
    CHECK(NumKeypad_CanAppendDigit(rules, empty, 0, 15));
    CHECK_FALSE(NumKeypad_CanAppendDigit(rules, empty, 0, 2));

    CHECK(NumKeypad_CanEnter(rules, zero, 1));
    CHECK(NumKeypad_CanAppendDigit(rules, f, 1, 5));
    CHECK(NumKeypad_CanAppendDigit(rules, f, 1, 6));
    CHECK_FALSE(NumKeypad_CanAppendDigit(rules, f, 1, 7));
    CHECK(NumKeypad_CanEnter(rules, f5, 2));
    CHECK_FALSE(NumKeypad_CanEnter(rules, f7, 2));
}
