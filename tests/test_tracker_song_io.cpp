#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../3rdparty/json/tests/thirdparty/doctest/doctest.h"

#include <fstream>
#include <cmath>
#include <sstream>

#define CLAY_IMPLEMENTATION
#include "../eggsfm/xfm_api.h"
#include "../my-ym2612-plugin/build/_deps/ymfm-src/src/ymfm_misc.cpp"
#include "../my-ym2612-plugin/build/_deps/ymfm-src/src/ymfm_adpcm.cpp"
#include "../my-ym2612-plugin/build/_deps/ymfm-src/src/ymfm_ssg.cpp"
#include "../my-ym2612-plugin/build/_deps/ymfm-src/src/ymfm_opn.cpp"
#include "../eggsfm/xfm_impl.cpp"
#include "../sounds/sounds.cpp"
#include "../clayton/keypad.h"
#include "../tracker/tracker.h"
#include "../tracker/tracker_song_io.h"

extern "C" {
SDL_AudioDeviceID SDL_OpenAudioDevice(const char *, int, const SDL_AudioSpec *, SDL_AudioSpec *, int) { return 1; }
void SDL_CloseAudioDevice(SDL_AudioDeviceID) {}
void SDL_PauseAudioDevice(SDL_AudioDeviceID, int) {}
void SDL_LockAudioDevice(SDL_AudioDeviceID) {}
void SDL_UnlockAudioDevice(SDL_AudioDeviceID) {}
const char *SDL_GetError(void) { return "stub"; }
Uint64 SDL_GetTicks64(void) { return 0; }
}

xfm_wav_module* xfm_wav_module_create(int, int) { return nullptr; }
void xfm_wav_module_destroy(xfm_wav_module*) {}
int xfm_wav_load_memory(xfm_wav_module*, xfm_wav_type, int, const void*, int, bool) { return -1; }
void xfm_wav_song_play(xfm_wav_module*, int, bool) {}
void xfm_wav_song_stop(xfm_wav_module*) {}
xfm_wav_voice_id xfm_wav_sfx_play(xfm_wav_module*, int, int) { return -1; }
void xfm_wav_sfx_stop(xfm_wav_module*, xfm_wav_voice_id) {}
void xfm_wav_sfx_stop_all(xfm_wav_module*) {}
bool xfm_wav_song_is_playing(xfm_wav_module*) { return false; }
void xfm_wav_mix_song(xfm_wav_module*, int16_t*, int) {}
void xfm_wav_mix_sfx(xfm_wav_module*, int16_t*, int) {}
void xfm_wav_module_set_volume(xfm_wav_module*, float) {}

static void Test_MixSongFrames(xfm_module *module, int frames)
{
    REQUIRE(module != nullptr);
    int16_t scratch[128 * 2] = {};
    int remaining = frames;
    while (remaining > 0)
    {
        int chunk = std::min(remaining, 128);
        xfm_mix_song(module, scratch, chunk);
        remaining -= chunk;
    }
}

static void Test_AdvanceSongUntilRow(xfm_module *module, int row)
{
    REQUIRE(module != nullptr);
    int guard = 0;
    while (module->active_song.current_row < row && guard++ < 2000)
        Test_MixSongFrames(module, 64);
    REQUIRE(module->active_song.current_row >= row);
}

static void Test_AdvanceSongUntilChannelActive(xfm_module *module, int channel)
{
    REQUIRE(module != nullptr);
    REQUIRE(channel >= 0);
    REQUIRE(channel < 6);
    int guard = 0;
    while (!module->channel_active[channel] && guard++ < 2000)
        Test_MixSongFrames(module, 32);
    REQUIRE(module->channel_active[channel]);
}

static SDL_Event Test_MouseButtonEvent(uint32_t type, uint32_t which)
{
    SDL_Event e {};
    e.type = type;
    e.button.button = SDL_BUTTON_LEFT;
    e.button.which = which;
    return e;
}

static uint8_t Test_EffectCodeAfterReleaseOnlyAdvance(uint8_t startCode, const SDL_Event &e, bool isHover)
{
    int effectIdx = Tracker_EffectDefIndexByCode(startCode);
    if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT && isHover)
        effectIdx = Tracker_NextEffectDefIndex(TRACKER_EFFECT_DEFS[effectIdx].code, 1);
    return TRACKER_EFFECT_DEFS[effectIdx].code;
}

static uint8_t Test_EffectCodeAfterClaytonAdvance(
    uint8_t startCode,
    Clayton_Click *click,
    const SDL_Event &e,
    bool isHover
)
{
    int effectIdx = Tracker_EffectDefIndexByCode(startCode);
    if (isClaytonClickedWithHover(click, e, isHover))
        effectIdx = Tracker_NextEffectDefIndex(TRACKER_EFFECT_DEFS[effectIdx].code, 1);
    return TRACKER_EFFECT_DEFS[effectIdx].code;
}

TEST_CASE("Tracker song names convert between display and filenames")
{
    CHECK(TrackerSongIO_DefaultDateStem(2026, 12, 31) == "SONG_261231");
    CHECK(TrackerSongIO_StemToDisplay("SONG_261231") == "Song 261231");
    CHECK(TrackerSongIO_SaveFilenameForDisplay("Song 261231") == "SONG_261231.h");
    CHECK(TrackerSongIO_StemToDisplay("MY_COOL_SONG") == "My Cool Song");
    CHECK(TrackerSongIO_SaveFilenameForDisplay("My Cool Song") == "MY_COOL_SONG.h");
}

TEST_CASE("Overlapping native and touch mouse click pair collapses to one Clayton click")
{
    Clayton_Click click {};
    SDL_Event nativeDown = Test_MouseButtonEvent(SDL_MOUSEBUTTONDOWN, 1);
    SDL_Event touchDown = Test_MouseButtonEvent(SDL_MOUSEBUTTONDOWN, SDL_TOUCH_MOUSEID);
    SDL_Event nativeUp = Test_MouseButtonEvent(SDL_MOUSEBUTTONUP, 1);
    SDL_Event touchUp = Test_MouseButtonEvent(SDL_MOUSEBUTTONUP, SDL_TOUCH_MOUSEID);

    CHECK_FALSE(isClaytonClickedWithHover(&click, nativeDown, true));
    CHECK_FALSE(isClaytonClickedWithHover(&click, touchDown, true));
    CHECK(isClaytonClickedWithHover(&click, nativeUp, true));
    CHECK_FALSE(isClaytonClickedWithHover(&click, touchUp, true));
}

TEST_CASE("Release-only effect selector reproduces touch plus mouse double advance")
{
    SDL_Event nativeDown = Test_MouseButtonEvent(SDL_MOUSEBUTTONDOWN, 1);
    SDL_Event nativeUp = Test_MouseButtonEvent(SDL_MOUSEBUTTONUP, 1);
    SDL_Event touchUp = Test_MouseButtonEvent(SDL_MOUSEBUTTONUP, SDL_TOUCH_MOUSEID);
    uint8_t effectCode = 0x01;

    effectCode = Test_EffectCodeAfterReleaseOnlyAdvance(effectCode, nativeDown, true);
    CHECK(effectCode == 0x01);
    effectCode = Test_EffectCodeAfterReleaseOnlyAdvance(effectCode, nativeUp, true);
    CHECK(effectCode == 0x02);
    effectCode = Test_EffectCodeAfterReleaseOnlyAdvance(effectCode, touchUp, true);
    CHECK(effectCode == 0x03);
}

TEST_CASE("Shared Clayton click path advances effect selector only once for one touch tap")
{
    Clayton_Click click {};
    SDL_Event nativeDown = Test_MouseButtonEvent(SDL_MOUSEBUTTONDOWN, 1);
    SDL_Event touchDown = Test_MouseButtonEvent(SDL_MOUSEBUTTONDOWN, SDL_TOUCH_MOUSEID);
    SDL_Event nativeUp = Test_MouseButtonEvent(SDL_MOUSEBUTTONUP, 1);
    SDL_Event touchUp = Test_MouseButtonEvent(SDL_MOUSEBUTTONUP, SDL_TOUCH_MOUSEID);
    uint8_t effectCode = 0x01;

    effectCode = Test_EffectCodeAfterClaytonAdvance(effectCode, &click, nativeDown, true);
    CHECK(effectCode == 0x01);
    effectCode = Test_EffectCodeAfterClaytonAdvance(effectCode, &click, touchDown, true);
    CHECK(effectCode == 0x01);
    effectCode = Test_EffectCodeAfterClaytonAdvance(effectCode, &click, nativeUp, true);
    CHECK(effectCode == 0x02);
    effectCode = Test_EffectCodeAfterClaytonAdvance(effectCode, &click, touchUp, true);
    CHECK(effectCode == 0x02);
}

TEST_CASE("Only username keypad sessions may apply username shortcuts")
{
    char buffer[KEYPAD_MAX_CHARS] = "L12";
    int32_t len = 3;
    Keypad keypad {};
    initKeypad(&keypad, buffer, &len);

    CHECK(Keypad_ShouldApplyUsernameCommands(&keypad));

    keypad.persistUsernameToStorage = false;
    CHECK_FALSE(Keypad_ShouldApplyUsernameCommands(&keypad));
}

TEST_CASE("Empty song helper resets tracker state immediately")
{
    Tracker tracker {};
    Tracker_Clear(&tracker);
    tracker.rowCount = 8;
    tracker.partCount = 2;
    tracker.parts[0].startRow = 0;
    tracker.parts[0].rowCount = 4;
    tracker.parts[1].startRow = 4;
    tracker.parts[1].rowCount = 4;
    std::strncpy(tracker.cells[0][0].text, "C-4007F", TRACKER_CELL_CHARS);
    tracker.songRowsPerBeat = 3;

    Tracker_LoadEmptyPatternState(&tracker);

    CHECK(tracker.songIndex == TRACKER_USER_SONG_SLOT);
    CHECK(tracker.rowCount == 32);
    CHECK(tracker.partCount == 1);
    CHECK(std::string(tracker.parts[0].name) == "PART 1");
    CHECK(tracker.parts[0].rowCount == 32);
    CHECK(std::string(tracker.cells[0][0].text) == ".......");
    CHECK(tracker.songRowsPerBeat == 4);
}

TEST_CASE("Rows per beat zebra band resets within each part")
{
    Tracker tracker {};
    Tracker_Clear(&tracker);
    tracker.rowCount = 16;
    tracker.partCount = 2;
    tracker.parts[0].startRow = 0;
    tracker.parts[0].rowCount = 8;
    tracker.parts[0].enabled = true;
    Tracker_SetPartName(&tracker.parts[0], "PART 1");
    tracker.parts[1].startRow = 8;
    tracker.parts[1].rowCount = 8;
    tracker.parts[1].enabled = true;
    Tracker_SetPartName(&tracker.parts[1], "PART 2");
    tracker.songRowsPerBeat = 4;

    CHECK(Tracker_RowIsDarkZebraBand(&tracker, 0, 0));
    CHECK(Tracker_RowIsDarkZebraBand(&tracker, 0, 3));
    CHECK_FALSE(Tracker_RowIsDarkZebraBand(&tracker, 0, 4));
    CHECK_FALSE(Tracker_RowIsDarkZebraBand(&tracker, 0, 7));
    CHECK(Tracker_RowIsDarkZebraBand(&tracker, 1, 0));
    CHECK_FALSE(Tracker_RowIsDarkZebraBand(&tracker, 1, 4));
}

TEST_CASE("Tracker song load rejects reserved and illegal filenames")
{
    std::string err;
    CHECK_FALSE(TrackerSongIO_IsValidUserStem("ALLEY_CAT", &err));
    CHECK(err == "That name is reserved for a built-in song");

    CHECK_FALSE(TrackerSongIO_IsValidUserStem("NO!", &err));
    CHECK(err == "Song name can only use letters, numbers, and underscore");

    CHECK_FALSE(TrackerSongIO_IsValidUserStem("AB", &err));
    CHECK(err == "Song name is too short");
}

TEST_CASE("Tracker song C++ text round-trips name and pattern")
{
    std::string pattern = "2\nC-4007F|.......|.......|.......|.......|.......\nOFF....|.......|.......|.......|.......|.......\n";
    std::string text = TrackerSongIO_BuildFileText("My Jam", pattern, "");
    TrackerSongLoadResult loaded = TrackerSongIO_ParseFile("MY_JAM.txt", text);

    REQUIRE(loaded.ok);
    CHECK(loaded.displayName == "My Jam");
    CHECK(loaded.pattern == pattern);
}

TEST_CASE("Tracker song C++ text uses the readable macro DSL")
{
    std::string pattern = "1\nPART Intro\nC-4007F|.......|.......|.......|.......|.......\n";
    std::string text = TrackerSongIO_BuildFileText(
        "My Jam",
        pattern,
        "INST 00\nPATCH 3 4 0 0\nENDINST\n",
        75,
        5,
        8,
        0,
        TRACKER_SONG_SCALE_CHINESE_PENTATONIC,
        true,
        3
    );

    CHECK(text.find("#include <xfm_song_dsl.h>") != std::string::npos);
    CHECK(text.find("XFM_SONG_BEGIN(R\"xfmname(My Jam)xfmname\")") != std::string::npos);
    CHECK(text.find("XFM_TICK_RATE(75)") != std::string::npos);
    CHECK(text.find("XFM_SCALE_MODE(") != std::string::npos);
    CHECK(text.find("XFM_PATTERN(R\"xfmpattern(") != std::string::npos);
    CHECK(text.find("XFM_INSTRUMENTS(R\"xfminstruments(") != std::string::npos);
    CHECK(text.find("INST 00\n") != std::string::npos);
    CHECK(text.find("PATCH 3 4 0 0\n") != std::string::npos);

    int setting = 0;
    CHECK(TrackerSongIO_ExtractInt(text, "XFM_TRACKER_TICK_RATE", setting));
    CHECK(setting == 75);
    CHECK(TrackerSongIO_ExtractInt(text, "XFM_TRACKER_SPEED", setting));
    CHECK(setting == 5);
    CHECK(TrackerSongIO_ExtractInt(text, "XFM_TRACKER_ROWS_PER_BEAT", setting));
    CHECK(setting == 8);
    CHECK(TrackerSongIO_ExtractInt(text, "XFM_TRACKER_SCALE_ROOT", setting));
    CHECK(setting == 0);
    CHECK(TrackerSongIO_ExtractInt(text, "XFM_TRACKER_SCALE_MODE", setting));
    CHECK(setting == TRACKER_SONG_SCALE_CHINESE_PENTATONIC);
    CHECK(TrackerSongIO_ExtractInt(text, "XFM_TRACKER_LFO_ENABLED", setting));
    CHECK(setting == 1);
    CHECK(TrackerSongIO_ExtractInt(text, "XFM_TRACKER_LFO_FREQUENCY", setting));
    CHECK(setting == 3);

    TrackerSongLoadResult loaded = TrackerSongIO_ParseFile("MY_JAM.h", text);
    REQUIRE(loaded.ok);
    CHECK(loaded.displayName == "My Jam");
    CHECK(loaded.pattern == pattern);
    CHECK(loaded.songTickRate == 75);
    CHECK(loaded.songSpeed == 5);
    CHECK(loaded.songRowsPerBeat == 8);
    CHECK(loaded.songScaleRoot == 0);
    CHECK(loaded.songScaleMode == TRACKER_SONG_SCALE_CHINESE_PENTATONIC);
    CHECK(loaded.songLfoEnabled);
    CHECK(loaded.songLfoFrequency == 3);

    std::string loadedInstruments;
    REQUIRE(TrackerSongIO_ExtractInstrumentText(text, loadedInstruments));
    CHECK(loadedInstruments.find("INST 00\n") != std::string::npos);
    CHECK(loadedInstruments.find("PATCH 3 4 0 0\n") != std::string::npos);
}

TEST_CASE("Tracker song C++ text preserves readable legacy instrument block order")
{
    std::string pattern = "1\nC-4007F|.......|.......|.......|.......|.......\n";
    std::string legacy =
        "INST 03\n"
        "PATCH 3 4 0 0\n"
        "NAME Guitar\n"
        "COLOR FFCF66\n"
        "FM OP  TL AR DR SL SR RR SSG MUL DT RS AM\n"
        "FM 1  61 11 0 10 0 0 0 15 3 0 0\n"
        "FM 2  0 21 18 2 0 4 0 1 3 0 0\n"
        "FM 3  19 31 31 15 0 9 1 7 -2 0 0\n"
        "FM 4  6 21 5 1 0 5 0 2 0 0 0\n"
        "ENDINST\n";

    std::string text = TrackerSongIO_BuildFileText("Guitar Song", pattern, legacy);

    const size_t instrumentsPos = text.find("XFM_INSTRUMENTS(R\"xfminstruments(");
    const size_t instPos = text.find("INST 03\n");
    const size_t namePos = text.find("NAME Guitar\n");
    const size_t colorPos = text.find("COLOR FFCF66\n");
    const size_t patchPos = text.find("PATCH 3 4 0 0\n");
    const size_t fmHeaderPos = text.find("FM OP  TL AR DR SL SR RR SSG MUL DT RS AM\n");
    const size_t op1Pos = text.find("FM 1  61 11 0 10 0 0 0 15 3 0 0\n");
    const size_t op2Pos = text.find("FM 2  0 21 18 2 0 4 0 1 3 0 0\n");
    const size_t op3Pos = text.find("FM 3  19 31 31 15 0 9 1 7 -2 0 0\n");
    const size_t op4Pos = text.find("FM 4  6 21 5 1 0 5 0 2 0 0 0\n");
    const size_t endPos = text.find("ENDINST\n");

    REQUIRE(instrumentsPos != std::string::npos);
    REQUIRE(instPos != std::string::npos);
    REQUIRE(namePos != std::string::npos);
    REQUIRE(colorPos != std::string::npos);
    REQUIRE(patchPos != std::string::npos);
    REQUIRE(fmHeaderPos != std::string::npos);
    REQUIRE(op1Pos != std::string::npos);
    REQUIRE(op2Pos != std::string::npos);
    REQUIRE(op3Pos != std::string::npos);
    REQUIRE(op4Pos != std::string::npos);
    REQUIRE(endPos != std::string::npos);

    CHECK(instrumentsPos < instPos);
    CHECK(instPos < patchPos);
    CHECK(patchPos < namePos);
    CHECK(namePos < colorPos);
    CHECK(patchPos < op1Pos);
    CHECK(op1Pos < op2Pos);
    CHECK(op2Pos < op3Pos);
    CHECK(op3Pos < op4Pos);
    CHECK(op4Pos < endPos);
}

TEST_CASE("Tracker song scale helper covers church modes and enabled asian pentatonics")
{
    CHECK(Tracker_SongScaleIncludesNote(TRACKER_SONG_SCALE_CHROMATIC, 0, 1));
    CHECK(std::string(Tracker_SongScaleRootName(0)) == "C");
    CHECK(std::string(Tracker_SongScaleRootName(2)) == "D");
    CHECK(std::string(Tracker_SongScaleModeName(TRACKER_SONG_SCALE_MAJOR)) == "Major");
    CHECK(std::string(Tracker_SongScaleModeName(TRACKER_SONG_SCALE_NATURAL_MINOR)) == "Minor");
    CHECK(Tracker_SongScaleIncludesNote(TRACKER_SONG_SCALE_MAJOR, 0, 0));
    CHECK(Tracker_SongScaleIncludesNote(TRACKER_SONG_SCALE_MAJOR, 0, 7));
    CHECK_FALSE(Tracker_SongScaleIncludesNote(TRACKER_SONG_SCALE_MAJOR, 0, 1));
    CHECK(Tracker_SongScaleIncludesNote(TRACKER_SONG_SCALE_MAJOR, 2, 2));
    CHECK(Tracker_SongScaleIncludesNote(TRACKER_SONG_SCALE_MAJOR, 2, 9));
    CHECK_FALSE(Tracker_SongScaleIncludesNote(TRACKER_SONG_SCALE_MAJOR, 2, 3));
    CHECK(Tracker_SongScaleIncludesNote(TRACKER_SONG_SCALE_MAJOR_PENTATONIC, 0, 0));
    CHECK_FALSE(Tracker_SongScaleIncludesNote(TRACKER_SONG_SCALE_MAJOR_PENTATONIC, 0, 1));
    CHECK(Tracker_SongScaleIncludesNote(TRACKER_SONG_SCALE_CHINESE_PENTATONIC, 0, 0));
    CHECK(Tracker_SongScaleIncludesNote(TRACKER_SONG_SCALE_CHINESE_PENTATONIC, 0, 4));
    CHECK(Tracker_SongScaleIncludesNote(TRACKER_SONG_SCALE_CHINESE_PENTATONIC, 0, 9));
    CHECK_FALSE(Tracker_SongScaleIncludesNote(TRACKER_SONG_SCALE_CHINESE_PENTATONIC, 0, 1));
    CHECK_FALSE(Tracker_SongScaleIncludesNote(TRACKER_SONG_SCALE_CHINESE_PENTATONIC, 0, 6));
    CHECK(Tracker_SongScaleIncludesNote(TRACKER_SONG_SCALE_INSEN, 0, 1));
    CHECK_FALSE(Tracker_SongScaleIncludesNote(TRACKER_SONG_SCALE_INSEN, 0, 4));
    CHECK(Tracker_SongScaleIncludesNote(TRACKER_SONG_SCALE_HIRAJOSHI, 0, 8));
    CHECK(Tracker_SongScaleIncludesNote(TRACKER_SONG_SCALE_YO, 0, 9));
}

TEST_CASE("Tracker reopen preserves song settings when reopening the same song payload")
{
    Tracker tracker {};
    Tracker_Clear(&tracker);
    setTrackerPatternState(&tracker, TRACKER_USER_SONG_SLOT, "32\nPART 1\n", "My Song");
    tracker.songTickRate = 123;
    tracker.songSpeed = 7;
    tracker.songRowsPerBeat = 5;
    tracker.songScaleRoot = 4;
    tracker.songScaleMode = TRACKER_SONG_SCALE_MAJOR;
    tracker.songLfoEnabled = true;
    tracker.songLfoFrequency = 6;

    const std::string currentPattern = Tracker_BuildPartPatternText(&tracker);
    CHECK(Tracker_ShouldReuseCurrentSongStateOnOpen(
        &tracker,
        TRACKER_USER_SONG_SLOT,
        currentPattern.c_str(),
        "My Song"));

    CHECK_FALSE(Tracker_ShouldReuseCurrentSongStateOnOpen(
        &tracker,
        1,
        currentPattern.c_str(),
        "My Song"));

    CHECK_FALSE(Tracker_ShouldReuseCurrentSongStateOnOpen(
        &tracker,
        TRACKER_USER_SONG_SLOT,
        "64\nPART 1\n",
        "My Song"));

    CHECK_FALSE(Tracker_ShouldReuseCurrentSongStateOnOpen(
        &tracker,
        TRACKER_USER_SONG_SLOT,
        currentPattern.c_str(),
        "Other Song"));
}

TEST_CASE("Tracker song files can use their song name as the download filename")
{
    std::string pattern = "1\nC-4007F|.......|.......|.......|.......|.......\n";
    std::string text = TrackerSongIO_BuildFileText("Alley Cat", pattern, "");

    CHECK(TrackerSongIO_SaveFilenameForDisplay("Alley Cat") == "ALLEY_CAT.h");

    TrackerSongLoadResult loaded = TrackerSongIO_ParseFile("ALLEY_CAT.h", text);
    REQUIRE(loaded.ok);
    CHECK(loaded.displayName == "Alley Cat");
    CHECK(loaded.pattern == pattern);
}

TEST_CASE("Built-in song DSL exposes metadata and pattern constants")
{
    CHECK(TRACKER_BUILTIN_SONG_COUNT == BUILTIN_SONG_REGISTRY_COUNT);
    CHECK(TRACKER_USER_SONG_SLOT == TRACKER_BUILTIN_SONG_COUNT + 1);
    CHECK(TRACKER_MAX_SONG_COUNT == TRACKER_USER_SONG_SLOT);
    CHECK(std::string(SONG_01_NAME) == "Bowling Strike");
    CHECK(std::string(SONG_02_NAME) == "Gutter Groove");
    CHECK(SONG_01_TICK_RATE == 60);
    CHECK(SONG_02_SPEED == 8);
    CHECK(SONG_03_ROWS_PER_BEAT == 4);
    CHECK(SONG_04_LFO_ENABLED == 0);
    CHECK(Tracker_SongName(4) == std::string("Alley Cat"));
    CHECK(Tracker_DefaultSongSpeed(2) == SONG_02_SPEED);
    CHECK(std::string(Tracker_SongPattern(1)).find("288\n") != std::string::npos);
}

TEST_CASE("Built-in song registry drives reserved user song filenames")
{
    REQUIRE(TRACKER_BUILTIN_SONG_COUNT >= 4);
    CHECK(TrackerSongIO_IsBuiltinStem("SONG_01"));
    CHECK(TrackerSongIO_IsBuiltinStem("Bowling_Strike"));
    CHECK(TrackerSongIO_IsBuiltinStem("gutter_groove"));
    CHECK(TrackerSongIO_IsBuiltinStem("PIN_CRUSHER"));
    CHECK(TrackerSongIO_IsBuiltinStem("alley_cat"));
    CHECK_FALSE(TrackerSongIO_IsBuiltinStem("MY_CUSTOM_TRACK"));
}

TEST_CASE("Built-in song files are self-contained and carry only their used instruments")
{
    auto readText = [](const char *path) {
        std::ifstream in(path);
        std::ostringstream out;
        out << in.rdbuf();
        return out.str();
    };
    auto assertSelfContained = [&](const char *path, const char *pattern) {
        bool referenced[256] = {};
        TrackerSongIO_MarkReferencedInstruments(pattern, referenced);

        std::string instruments;
        std::string text = readText(path);
        CHECK(text.find("XFM_BUILTIN_") == std::string::npos);
        CHECK(text.find("XFM_INSTRUMENTS(R\"xfminstruments(") != std::string::npos);
        CHECK(text.find("XFM_SONG_BEGIN(") != std::string::npos);
        CHECK(text.find("XFM_PATTERN(") != std::string::npos);
        REQUIRE(TrackerSongIO_ExtractInstrumentText(text, instruments));
        for (int inst = 0; inst < 256; inst++)
        {
            char marker[16];
            std::snprintf(marker, sizeof(marker), "INST %02X\n", inst);
            bool declared = instruments.find(marker) != std::string::npos;
            CHECK(declared == referenced[inst]);
        }
    };

    assertSelfContained("sounds/builtin_songs/song_01.h", SONG_01);
    assertSelfContained("sounds/builtin_songs/song_02.h", SONG_02);
    assertSelfContained("sounds/builtin_songs/song_03.h", SONG_03);
    assertSelfContained("sounds/builtin_songs/song_04.h", SONG_04);

    Tracker tracker {};
    Tracker_Clear(&tracker);
    setTrackerSongState(&tracker, 1);
    CHECK(Tracker_InstrumentAvailable(&tracker, 0x00));
    CHECK(Tracker_InstrumentAvailable(&tracker, 0x01));
    CHECK(Tracker_InstrumentAvailable(&tracker, 0x02));
    CHECK_FALSE(Tracker_InstrumentAvailable(&tracker, 0x03));
}

TEST_CASE("Built-in song DSL files parse cleanly")
{
    auto readText = [](const char *path) {
        std::ifstream in(path);
        std::ostringstream out;
        out << in.rdbuf();
        return out.str();
    };

    for (const BuiltinSongDefinition &song : BUILTIN_SONG_REGISTRY)
    {
        CAPTURE(song.sourcePath);
        TrackerSongLoadResult loaded = TrackerSongIO_ParseFile(song.sourcePath, readText(song.sourcePath));
        CHECK(loaded.ok);
        if (!loaded.ok)
            CHECK(loaded.error.empty());
    }
}

TEST_CASE("Built-in SFX DSL files parse cleanly")
{
    auto readText = [](const char *path) {
        std::ifstream in(path);
        std::ostringstream out;
        out << in.rdbuf();
        return out.str();
    };

    for (const BuiltinSfxDefinition &sfx : BUILTIN_SFX_REGISTRY)
    {
        CAPTURE(sfx.sourcePath);
        TrackerSongLoadResult loaded = TrackerSongIO_ParseFile(sfx.sourcePath, readText(sfx.sourcePath));
        CHECK(loaded.ok);
        if (loaded.ok)
        {
            CHECK(loaded.songTickRate == sfx.tickRate);
            CHECK(loaded.songSpeed == sfx.speed);
        }
    }
}

TEST_CASE("Built-in SFX local instruments remap into a shared global bank")
{
    REQUIRE(BuiltinSfx_GlobalInstrumentCount() >= 6);

    const BuiltinSfxPrepared *lane = BuiltinSfx_PreparedById(GameSoundSystem::SFX_BALL_HIT_LANE);
    const BuiltinSfxPrepared *buy = BuiltinSfx_PreparedById(GameSoundSystem::SFX_BUY);
    const BuiltinSfxPrepared *glass = BuiltinSfx_PreparedById(GameSoundSystem::SFX_GLASS_CRACK);
    REQUIRE(lane != nullptr);
    REQUIRE(buy != nullptr);
    REQUIRE(glass != nullptr);

    CHECK(lane->localToGlobal[0x00] >= 0);
    CHECK(buy->localToGlobal[0x00] >= 0);
    CHECK(glass->localToGlobal[0x00] >= 0);
    CHECK(lane->localToGlobal[0x00] != buy->localToGlobal[0x00]);
    CHECK(buy->localToGlobal[0x00] != glass->localToGlobal[0x00]);

    char laneHex[3] = {};
    char buyHex[3] = {};
    std::memcpy(laneHex, lane->remappedPattern.c_str() + 5, 2);
    std::memcpy(buyHex, buy->remappedPattern.c_str() + 5, 2);
    CHECK(std::string(laneHex) != "00");
    CHECK(std::string(buyHex) != "00");
    CHECK(std::string(laneHex) != std::string(buyHex));
}

TEST_CASE("Glass SFX DSL keeps legacy glass patch definitions")
{
    const BuiltinSfxDefinition *crack = BuiltinSfx_ById(GameSoundSystem::SFX_GLASS_CRACK);
    const BuiltinSfxDefinition *scrape = BuiltinSfx_ById(GameSoundSystem::SFX_GLASS_SCRAPE);
    const BuiltinSfxDefinition *shards = BuiltinSfx_ById(GameSoundSystem::SFX_GLASS_SHARDS);
    const BuiltinSfxDefinition *tinkle = BuiltinSfx_ById(GameSoundSystem::SFX_GLASS_TINKLE);
    REQUIRE(crack != nullptr);
    REQUIRE(scrape != nullptr);
    REQUIRE(shards != nullptr);
    REQUIRE(tinkle != nullptr);

    CHECK(std::string(crack->instruments).find("PATCH 7 7 0 7") != std::string::npos);
    CHECK(std::string(crack->instruments).find("OP 1 -3 15 0 3 31 1 31 0 15 15 8") != std::string::npos);
    CHECK(std::string(crack->instruments).find("OP 4 3 9 0 3 31 1 31 0 15 15 5") != std::string::npos);

    CHECK(std::string(scrape->instruments).find("PATCH 5 7 0 5") != std::string::npos);
    CHECK(std::string(scrape->instruments).find("OP 1 -3 14 18 3 31 1 12 18 4 11 3") != std::string::npos);
    CHECK(std::string(scrape->instruments).find("OP 4 2 15 0 3 31 1 10 16 5 13 6") != std::string::npos);

    CHECK(std::string(shards->instruments).find("PATCH 7 6 0 6") != std::string::npos);
    CHECK(std::string(shards->instruments).find("OP 1 -2 8 16 3 31 1 22 0 15 12 0") != std::string::npos);
    CHECK(std::string(shards->instruments).find("OP 4 -3 11 2 3 31 1 20 0 15 12 0") != std::string::npos);

    CHECK(std::string(tinkle->displayName) == "Glass Tinkle");
    CHECK(std::string(tinkle->instruments).find("ALG  FB AMS FMS") != std::string::npos);
    CHECK(std::string(tinkle->instruments).find("PATCH  4   5   3   0") != std::string::npos);
    CHECK(std::string(tinkle->pattern).find("D-70061") != std::string::npos);
}

TEST_CASE("Custom song sound path uploads user instrument bank without opening tracker")
{
    GameSoundSystem sound = {};
    sound.musicModule = xfm_module_create(44100, 256, XFM_CHIP_YM3438);
    REQUIRE(sound.musicModule != nullptr);

    const char *uiPattern =
        "2\n"
        "C-4007F\n"
        "REL....\n";
    const char *playbackPattern =
        "2\n"
        "C-4007F\n"
        "REL....\n";
    const char *instruments =
        "INST 00\n"
        "NAME Test Inst\n"
        "PATCH 3 5 1 2\n"
        "OP 1 -1 7 18 2 20 1 9 11 5 6 0\n"
        "OP 2 2 3 30 1 24 0 12 8 6 7 0\n"
        "OP 3 0 9 12 3 31 1 14 7 4 8 2\n"
        "OP 4 -3 1 0 0 31 0 6 0 15 5 0\n"
        "MACRO 15 4 0 255 3 1 2 0\n";

    REQUIRE(sound.setUserSong(
        "Custom Test",
        uiPattern,
        playbackPattern,
        instruments,
        60,
        6,
        4,
        0,
        0,
        true,
        5));
    sound.currentSongIndex = TRACKER_USER_SONG_SLOT;

    soundApplyUserSongInstrumentBankToMusicModule(&sound);

    CHECK(sound.musicModule->patch_present[0x00]);
    CHECK(sound.musicModule->patches[0x00].ALG == 3);
    CHECK(sound.musicModule->patches[0x00].FB == 5);
    CHECK(sound.musicModule->patches[0x00].AMS == 1);
    CHECK(sound.musicModule->patches[0x00].FMS == 2);
    CHECK(sound.musicModule->patches[0x00].op[0].DT == -1);
    CHECK(sound.musicModule->patches[0x00].op[0].MUL == 7);
    CHECK(sound.musicModule->patches[0x00].op[3].TL == 0);

    const int macroId = sound.musicModule->patch_macros[0x00][XFM_MACRO_PAN];
    CHECK(macroId >= 0);
    REQUIRE(macroId < XFM_MAX_MACROS);
    CHECK(sound.musicModule->macro_present[macroId]);
    CHECK(sound.musicModule->macros[macroId].target == XFM_MACRO_PAN);
    CHECK(sound.musicModule->macros[macroId].length == 4);
    CHECK(sound.musicModule->macros[macroId].values[0] == 3);
    CHECK(sound.musicModule->macros[macroId].values[1] == 1);
    CHECK(sound.musicModule->macros[macroId].values[2] == 2);
    CHECK(sound.musicModule->macros[macroId].values[3] == 0);

    xfm_module_destroy(sound.musicModule);
    sound.musicModule = nullptr;
}

TEST_CASE("Tracker song load reports malformed pattern raw string")
{
    std::string text =
        "static constexpr const char *XFM_TRACKER_SONG_PATTERN = R\"xfm(1\n"
        "C-4007F|.......|.......|.......|.......|.......\n";

    TrackerSongLoadResult loaded = TrackerSongIO_ParseFile("BROKEN.h", text);

    CHECK_FALSE(loaded.ok);
    CHECK(loaded.error.find("XFM_TRACKER_SONG_PATTERN raw string is malformed") != std::string::npos);
}

TEST_CASE("Tracker song load reports missing row count")
{
    TrackerSongLoadResult loaded = TrackerSongIO_ParseFile(
        "BROKEN.h",
        "PART Intro\n"
        "C-4007F|.......|.......|.......|.......|.......\n");

    CHECK_FALSE(loaded.ok);
    CHECK(loaded.error == "missing tracker row count at start of pattern");
}

TEST_CASE("Tracker song load reports all pattern validation messages")
{
    TrackerSongLoadResult loaded = TrackerSongIO_ParseFile(
        "BROKEN.h",
        "2\n"
        "PART \n"
        "A\n"
        "C-4007F|.......|.......|.......|.......|.......|.......\n");

    CHECK_FALSE(loaded.ok);
    CHECK(loaded.error.find("line 2: PART/SKIP name is empty") != std::string::npos);
    CHECK(loaded.error.find("line 3: row is too short for a tracker cell") != std::string::npos);
    CHECK(loaded.error.find("line 4: row has 7 channels, maximum is 6") != std::string::npos);
}

TEST_CASE("Tracker song load error summaries count parser messages")
{
    CHECK(TrackerSongIO_CountMessages("") == 0);
    CHECK(TrackerSongIO_LoadErrorSummary("") == "LOAD FAILED: invalid tracker file");
    CHECK(TrackerSongIO_CountMessages("line 2: bad row") == 1);
    CHECK(TrackerSongIO_LoadErrorSummary("line 2: bad row") == "LOAD FAILED: 1 parser error");
    CHECK(TrackerSongIO_CountMessages("line 2: bad row\nline 4: too many channels\n") == 2);
    CHECK(TrackerSongIO_LoadErrorSummary("line 2: bad row\nline 4: too many channels\n") == "LOAD FAILED: 2 parser errors");
}

TEST_CASE("Tracker song load reports instrument DSL validation messages")
{
    std::string text =
        "#include <xfm_song_dsl.h>\n"
        "XFM_SONG_BEGIN(R\"name(Broken)name\")\n"
        "XFM_PATTERN(R\"pat(1\n"
        "A\n"
        ")pat\")\n"
        "XFM_PATCH(ALG = 1, FB = 2, AMS = 0, FMS = 0)\n"
        "XFM_INSTRUMENT(foo)\n"
        "XFM_INSTRUMENT(0x02)\n"
        "XFM_PATCH(ALG = 3, FB = 4)\n"
        "XFM_TRACKER_MACRO(NOPE, LENGTH = 4, LOOP = 255, RELEASE = 255, VALUES = \"1 2\")\n";

    TrackerSongLoadResult loaded = TrackerSongIO_ParseFile("BROKEN.h", text);

    CHECK_FALSE(loaded.ok);
    CHECK(loaded.error.find("line 2: row is too short for a tracker cell") != std::string::npos);
    CHECK(loaded.error.find("line 6: XFM_PATCH appears outside XFM_INSTRUMENT block") != std::string::npos);
    CHECK(loaded.error.find("line 7: XFM_INSTRUMENT id is invalid; expected 0x00..0xFF") != std::string::npos);
    CHECK(loaded.error.find("line 9: XFM_PATCH missing required field AMS") != std::string::npos);
    CHECK(loaded.error.find("line 9: XFM_PATCH missing required field FMS") != std::string::npos);
    CHECK(loaded.error.find("line 10: XFM_TRACKER_MACRO target is unknown") != std::string::npos);
    CHECK(loaded.error.find("line 10: XFM_TRACKER_MACRO LENGTH says 4 but VALUES contains 2 values") != std::string::npos);
    CHECK(loaded.error.find("XFM_INSTRUMENT block opened on line 8 is missing XFM_END_INSTRUMENT()") != std::string::npos);
}

TEST_CASE("Cloned renamed instruments used by the pattern are saved")
{
    Tracker tracker {};
    Tracker_Clear(&tracker);
    tracker.rowCount = 9;

    const int sourceGuitar = 0x03;
    Tracker_SetInstrumentAvailable(&tracker, sourceGuitar);
    tracker.editPatches[sourceGuitar] = PATCH_03_GUITAR;
    tracker.editPatchValid[sourceGuitar] = true;

    REQUIRE(Tracker_CloneInstrument(&tracker, sourceGuitar, 0x00, "ALBINAS", 7));
    std::strncpy(tracker.cells[0][2].text, "A-2007F", TRACKER_CELL_CHARS);
    std::strncpy(tracker.cells[8][2].text, "E-2007F", TRACKER_CELL_CHARS);

    std::string customInstruments = Tracker_BuildCustomInstrumentText(&tracker);
    CHECK(customInstruments.find("INST 00\n") != std::string::npos);
    CHECK(customInstruments.find("NAME ALBINAS\n") != std::string::npos);
    CHECK(customInstruments.find("PATCH 3 4 0 0\n") != std::string::npos);

    std::string pattern =
        "9\n"
        ".......|.......|A-2007F|.......|.......|.......\n"
        ".......|.......|.......|.......|.......|.......\n"
        ".......|.......|.......|.......|.......|.......\n"
        ".......|.......|.......|.......|.......|.......\n"
        ".......|.......|.......|.......|.......|.......\n"
        ".......|.......|.......|.......|.......|.......\n"
        ".......|.......|.......|.......|.......|.......\n"
        ".......|.......|.......|.......|.......|.......\n"
        ".......|.......|E-2007F|.......|.......|.......\n";
    std::string fileText = TrackerSongIO_BuildFileText("Song 260529", pattern, customInstruments);
    std::string loadedInstruments;
    REQUIRE(TrackerSongIO_ExtractInstrumentText(fileText, loadedInstruments));
    CHECK(loadedInstruments.find("INST 00\n") != std::string::npos);
    CHECK(loadedInstruments.find("NAME ALBINAS\n") != std::string::npos);

    bool referenced[256] = {};
    TrackerSongIO_MarkReferencedInstruments(pattern, referenced);
    CHECK(referenced[0x00]);
}

TEST_CASE("Readable tracker save format round-trips loadable instrument text")
{
    std::string legacy =
        "INST 02\n"
        "PATCH 4 5 1 2\n"
        "NAME Lead\n"
        "COLOR A0B0C0\n"
        "FM OP  TL AR DR SL SR RR SSG MUL DT RS AM\n"
        "FM 1  20 31 12 3 8 7 0 1 0 0 1\n"
        "MACRO 1 4 1 255 20 21 22 23\n"
        "ENDINST\n";

    std::string text = TrackerSongIO_BuildFileText("Dsl Macro", "1\nC-4027F|.......|.......|.......|.......|.......\n", legacy);

    CHECK(text.find("XFM_INSTRUMENTS(R\"xfminstruments(") != std::string::npos);
    CHECK(text.find("INST 02\n") != std::string::npos);
    CHECK(text.find("NAME Lead\n") != std::string::npos);
    CHECK(text.find("COLOR A0B0C0\n") != std::string::npos);
    CHECK(text.find("FM OP  TL AR DR SL SR RR SSG MUL DT RS AM\n") != std::string::npos);
    CHECK(text.find("MACRO 1 4 1 255 20 21 22 23\n") != std::string::npos);

    std::string loaded;
    REQUIRE(TrackerSongIO_ExtractInstrumentText(text, loaded));
    CHECK(loaded.find("INST 02\n") != std::string::npos);
    CHECK(loaded.find("PATCH 4 5 1 2\n") != std::string::npos);
    CHECK(loaded.find("NAME Lead\n") != std::string::npos);
    CHECK(loaded.find("COLOR A0B0C0\n") != std::string::npos);
    CHECK(loaded.find("FM OP  TL AR DR SL SR RR SSG MUL DT RS AM\n") != std::string::npos);
    CHECK(loaded.find("FM 1  20 31 12 3 8 7 0 1 0 0 1\n") != std::string::npos);
    CHECK(loaded.find("MACRO 1 4 1 255 20 21 22 23\n") != std::string::npos);
}

TEST_CASE("Legacy instrument guide header lines are exported and ignored on parse")
{
    Tracker tracker {};
    Tracker_Clear(&tracker);
    tracker.rowCount = 1;
    std::strncpy(tracker.cells[0][0].text, "C-4007F", TRACKER_CELL_CHARS);
    Tracker_SetInstrumentAvailable(&tracker, 0x00);
    tracker.editPatches[0x00] = PATCH_00_RUBBER_BASS;
    tracker.editPatchValid[0x00] = true;

    std::string customInstruments = Tracker_BuildCustomInstrumentText(&tracker);
    CHECK(customInstruments.find("     ALG  FB AMS FMS\n") != std::string::npos);
    CHECK(customInstruments.find("FM OP  TL AR DR SL SR RR SSG MUL DT RS AM\n") != std::string::npos);

    std::string text = TrackerSongIO_BuildFileText(
        "Guide Headers",
        "1\nC-4007F|.......|.......|.......|.......|.......\n",
        customInstruments
    );
    std::string loaded;
    REQUIRE(TrackerSongIO_ExtractInstrumentText(text, loaded));
    CHECK(loaded.find("PATCH 2 5 0 0\n") != std::string::npos);
    CHECK(loaded.find("FM 1  38 12 7 4 11 6 0 3 1 0 0\n") != std::string::npos);
}

TEST_CASE("Legacy instrument parser accepts both OP and FM operator row syntax")
{
    std::string mixed =
        "INST 00\n"
        "PATCH 3 4 0 0\n"
        "OP 1 1 3 38 0 12 0 7 11 4 6 0\n"
        "FM OP  TL AR DR SL SR RR SSG MUL DT RS AM\n"
        "FM 2  20 21 8 5 7 3 1 4 -2 2 1\n"
        "ENDINST\n";

    std::string dsl = TrackerSongIO_LegacyInstrumentsToDsl(mixed);
    std::string normalized = TrackerSongIO_DslInstrumentsToLegacy(dsl);
    CHECK(normalized.find("FM OP  TL AR DR SL SR RR SSG MUL DT RS AM\n") != std::string::npos);
    CHECK(normalized.find("FM 1  38 12 7 4 11 6 0 3 1 0 0\n") != std::string::npos);
    CHECK(normalized.find("FM 2  20 21 8 5 7 3 1 4 -2 2 1\n") != std::string::npos);
}

TEST_CASE("Setting special '...' clears only the note token")
{
    Tracker tracker {};
    Tracker_Clear(&tracker);
    tracker.rowCount = 1;
    tracker.editRow = 0;
    tracker.editChannel = 0;

    tracker.editSpecial = 4; // "..."
    tracker.editInstrumentExplicit = true;
    tracker.editInstrument = 0x00;
    tracker.editVolumeExplicit = true;
    tracker.editVolume = 0x7F;
    tracker.editEffectActive[0] = true;
    tracker.editEffectCodes[0] = 0x04;
    tracker.editEffectValues[0] = 0x07;

    Tracker_ApplyEditorToCell(&tracker);
    CHECK(std::string(tracker.cells[0][0].text) == "...007F0407");
}

TEST_CASE("Editor DEL clears note instrument volume and effects")
{
    Tracker tracker {};
    Tracker_Clear(&tracker);
    tracker.rowCount = 1;
    tracker.editRow = 0;
    tracker.editChannel = 0;
    std::strncpy(tracker.cells[0][0].text, "C-4007F0407", TRACKER_CELL_CHARS);
    Tracker_ParseCellForEditor(&tracker);

    Tracker_DeleteEditorCell(&tracker);

    CHECK(std::string(tracker.cells[0][0].text) == ".......");
    CHECK_FALSE(tracker.editInstrumentExplicit);
    CHECK_FALSE(tracker.editVolumeExplicit);
    for (int i = 0; i < TRACKER_MAX_EFFECT_SLOTS; i++)
        CHECK_FALSE(tracker.editEffectActive[i]);
}

TEST_CASE("Editor explicit instrument and volume toggles use inherited channel values")
{
    Tracker tracker {};
    Tracker_Clear(&tracker);
    tracker.rowCount = 2;
    std::strncpy(tracker.cells[0][0].text, "C-40140", TRACKER_CELL_CHARS);
    tracker.editRow = 1;
    tracker.editChannel = 0;
    std::strncpy(tracker.cells[1][0].text, "D-40250", TRACKER_CELL_CHARS);
    Tracker_ParseCellForEditor(&tracker);

    REQUIRE(tracker.editInstrumentExplicit);
    REQUIRE(tracker.editVolumeExplicit);
    Tracker_ToggleEditorInstrumentExplicit(&tracker);
    Tracker_ToggleEditorVolumeExplicit(&tracker);
    Tracker_ApplyEditorToCell(&tracker);

    CHECK_FALSE(tracker.editInstrumentExplicit);
    CHECK_FALSE(tracker.editVolumeExplicit);
    CHECK(tracker.editInstrument == 0x01);
    CHECK(tracker.editVolume == 0x40);
    CHECK(std::string(tracker.cells[1][0].text) == "D-4....");

    tracker.editInstrument = 0x02;
    Tracker_NormalizeExplicitFields(&tracker);
    CHECK(tracker.editInstrumentExplicit);
}

TEST_CASE("Inactive effect slots keep their editor value but are not written")
{
    Tracker tracker {};
    Tracker_Clear(&tracker);
    tracker.rowCount = 1;
    tracker.editRow = 0;
    tracker.editChannel = 0;
    tracker.editSpecial = 0;
    tracker.editNote = 0;
    tracker.editOctave = 4;
    tracker.editInstrumentExplicit = true;
    tracker.editInstrument = 0x00;
    tracker.editVolumeExplicit = true;
    tracker.editVolume = 0x7F;
    tracker.editEffectCodes[0] = 0x04;
    tracker.editEffectValues[0] = 0x07;
    tracker.editEffectActive[0] = false;
    tracker.editEffectCodes[1] = 0x07;
    tracker.editEffectValues[1] = 0x05;
    tracker.editEffectActive[1] = true;

    Tracker_ApplyEditorToCell(&tracker);

    CHECK(std::string(tracker.cells[0][0].text) == "C-4007F0705");
    CHECK(tracker.editEffectCodes[0] == 0x04);
}

TEST_CASE("Editor selected effect toggle respects two active effects limit")
{
    Tracker tracker {};
    Tracker_Clear(&tracker);
    tracker.rowCount = 1;
    tracker.editRow = 0;
    tracker.editChannel = 0;

    tracker.editEffect = Tracker_EffectDefIndexByCode(0x04);
    Tracker_SetSelectedEffectValue(&tracker, 0x07);
    Tracker_ToggleSelectedEffectActive(&tracker);

    tracker.editEffect = Tracker_EffectDefIndexByCode(0x07);
    Tracker_SetSelectedEffectValue(&tracker, 0x05);
    Tracker_ToggleSelectedEffectActive(&tracker);

    REQUIRE(Tracker_ActiveEffectCount(&tracker) == TRACKER_CELL_ACTIVE_EFFECT_LIMIT);

    tracker.editEffect = Tracker_EffectDefIndexByCode(0x0A);
    Tracker_SetSelectedEffectValue(&tracker, 0x0F);
    Tracker_ToggleSelectedEffectActive(&tracker);
    CHECK_FALSE(Tracker_SelectedEffectActive(&tracker));
    CHECK(Tracker_ActiveEffectCount(&tracker) == TRACKER_CELL_ACTIVE_EFFECT_LIMIT);

    tracker.editEffect = Tracker_EffectDefIndexByCode(0x04);
    Tracker_ToggleSelectedEffectActive(&tracker);
    CHECK_FALSE(Tracker_SelectedEffectActive(&tracker));
    CHECK(Tracker_ActiveEffectCount(&tracker) == 1);
}

TEST_CASE("Full patch sync marks available patches and macros dirty")
{
    Tracker tracker {};
    Tracker_Clear(&tracker);

    const int customInst = 0x02;
    Tracker_SetInstrumentAvailable(&tracker, customInst);
    tracker.editPatches[customInst] = Tracker_DefaultPatch();
    tracker.editPatchValid[customInst] = true;
    tracker.editPatchDirty[customInst] = false;

    Tracker_DefaultMacro(&tracker.editMacros[customInst][XFM_MACRO_TL1], XFM_MACRO_TL1);
    tracker.editMacroEnabled[customInst][XFM_MACRO_TL1] = true;
    tracker.editMacroValid[customInst][XFM_MACRO_TL1] = true;
    tracker.editMacroDirty[customInst][XFM_MACRO_TL1] = false;

    const int otherInst = 0x03;
    tracker.editPatchValid[otherInst] = true;
    tracker.editPatchDirty[otherInst] = false;

    Tracker_MarkAllAvailablePatchesAndMacrosDirty(&tracker);

    CHECK(tracker.editPatchDirty[customInst]);
    CHECK(tracker.editMacroDirty[customInst][XFM_MACRO_TL1]);
    CHECK_FALSE(tracker.editPatchDirty[otherInst]);
}

TEST_CASE("Browser resume should not restart user-stopped music")
{
    GameSoundSystem snd {};
    snd.useWavPlayback = false;

    xfm_module module {};
    module.active_song.active = false; // user hit stop in tracker
    snd.musicModule = &module;

    CHECK_FALSE(Sound_MusicActiveForBrowserSuspend(snd));

    module.active_song.active = true; // user was playing
    CHECK(Sound_MusicActiveForBrowserSuspend(snd));
}

TEST_CASE("Audio reopen prefers the last obtained sample rate")
{
    GameSoundSystem snd {};
    snd.sampleRate = 44100;
    snd.obtainedSampleRate = 0;
    CHECK(Sound_PreferredAudioSampleRate(snd) == 44100);

    snd.obtainedSampleRate = 48000;
    CHECK(Sound_PreferredAudioSampleRate(snd) == 48000);
}

TEST_CASE("Live patch refresh updates active voices immediately and invalidates inactive cache")
{
    xfm_module *module = xfm_module_create(44100, 256, XFM_CHIP_YM3438);
    REQUIRE(module != nullptr);

    xfm_patch_opn patchA = Tracker_DefaultPatch();
    patchA.op[0].TL = 12;
    xfm_patch_opn patchB = patchA;
    patchB.op[0].TL = 77;
    xfm_patch_opn patchC = patchA;
    patchC.op[0].TL = 99;

    xfm_patch_set(module, 0x00, &patchA, sizeof(patchA), XFM_CHIP_YM3438);
    xfm_voice_id voice = xfm_note_on(module, 60, 0x00, 127);
    REQUIRE(voice != FM_VOICE_INVALID);
    module->active_song.channels[voice].current_volume = 127;

    xfm_patch_set(module, 0x00, &patchB, sizeof(patchB), XFM_CHIP_YM3438);
    xfm_patch_refresh_live(module, 0x00);

    CHECK(module->current_patch[voice] == 0x00);
    CHECK(module->live_patch_valid[voice]);
    CHECK(module->live_patch_id[voice] == 0x00);
    CHECK(module->live_patches[voice].op[0].TL == patchB.op[0].TL);

    xfm_note_off(module, voice);
    xfm_patch_set(module, 0x00, &patchC, sizeof(patchC), XFM_CHIP_YM3438);
    xfm_patch_refresh_live(module, 0x00);

    CHECK(module->current_patch[voice] == -1);
    CHECK_FALSE(module->live_patch_valid[voice]);
    CHECK(module->live_patch_id[voice] == -1);

    xfm_module_destroy(module);
}

TEST_CASE("Patch morph starts from the live patch and applies instant target fields")
{
    xfm_module *module = xfm_module_create(44100, 256, XFM_CHIP_YM3438);
    REQUIRE(module != nullptr);

    xfm_patch_opn patchA = Tracker_DefaultPatch();
    patchA.ALG = 2;
    patchA.FB = 1;
    patchA.op[0].TL = 12;
    patchA.op[0].AM = 0;
    patchA.op[0].SSG = 0;

    xfm_patch_opn patchB = Tracker_DefaultPatch();
    patchB.ALG = 7;
    patchB.FB = 6;
    patchB.AMS = 3;
    patchB.FMS = 7;
    patchB.op[0].TL = 96;
    patchB.op[0].AM = 1;
    patchB.op[0].SSG = 5;

    xfm_patch_set(module, 0x00, &patchA, sizeof(patchA), XFM_CHIP_YM3438);
    xfm_patch_set(module, 0x01, &patchB, sizeof(patchB), XFM_CHIP_YM3438);

    const char *pattern =
        "2\n"
        "C-4007F\n"
        "...01..EE20\n";
    REQUIRE(xfm_song_declare(module, 1, pattern, 100, 4) == 1);
    xfm_song_play(module, 1, false);

    Test_AdvanceSongUntilChannelActive(module, 0);
    Test_AdvanceSongUntilRow(module, 1);

    const XfmSongChannel &ch = module->active_song.channels[0];
    REQUIRE(ch.patch_morph_active);
    CHECK(module->live_patches[0].ALG == patchB.ALG);
    CHECK(module->live_patches[0].op[0].AM == patchB.op[0].AM);
    CHECK(module->live_patches[0].op[0].SSG == patchB.op[0].SSG);
    CHECK(module->live_patches[0].op[0].TL == patchA.op[0].TL);
    CHECK(ch.patch_morph_target_patch_id == 0x01);

    xfm_module_destroy(module);
}

TEST_CASE("Patch morph advances per tick and EE00 cancels without snapping back")
{
    xfm_module *module = xfm_module_create(44100, 256, XFM_CHIP_YM3438);
    REQUIRE(module != nullptr);

    xfm_patch_opn patchA = Tracker_DefaultPatch();
    patchA.op[0].TL = 8;
    patchA.FB = 0;

    xfm_patch_opn patchB = Tracker_DefaultPatch();
    patchB.op[0].TL = 120;
    patchB.FB = 7;

    xfm_patch_set(module, 0x00, &patchA, sizeof(patchA), XFM_CHIP_YM3438);
    xfm_patch_set(module, 0x01, &patchB, sizeof(patchB), XFM_CHIP_YM3438);

    const char *pattern =
        "3\n"
        "C-4007F\n"
        "...01..EE20\n"
        ".......EE00\n";
    REQUIRE(xfm_song_declare(module, 1, pattern, 100, 4) == 1);
    xfm_song_play(module, 1, false);

    Test_AdvanceSongUntilChannelActive(module, 0);
    Test_AdvanceSongUntilRow(module, 1);
    const int beforeTickTl = module->live_patches[0].op[0].TL;
    for (int i = 0; i < 8; i++) {
        song_advance_patch_morph_tick(module, 0);
    }
    const int afterTicksTl = module->live_patches[0].op[0].TL;
    CHECK(afterTicksTl > beforeTickTl);
    CHECK(afterTicksTl < patchB.op[0].TL);

    Test_AdvanceSongUntilRow(module, 2);
    const int canceledTl = module->live_patches[0].op[0].TL;
    CHECK_FALSE(module->active_song.channels[0].patch_morph_active);
    CHECK(canceledTl == module->live_patches[0].op[0].TL);

    xfm_module_destroy(module);
}

TEST_CASE("Patch morph lowest speed waits across ticks before advancing")
{
    xfm_module *module = xfm_module_create(44100, 256, XFM_CHIP_YM3438);
    REQUIRE(module != nullptr);

    xfm_patch_opn patchA = Tracker_DefaultPatch();
    patchA.op[0].TL = 8;

    xfm_patch_opn patchB = Tracker_DefaultPatch();
    patchB.op[0].TL = 20;

    xfm_patch_set(module, 0x00, &patchA, sizeof(patchA), XFM_CHIP_YM3438);
    xfm_patch_set(module, 0x01, &patchB, sizeof(patchB), XFM_CHIP_YM3438);

    const char *pattern =
        "2\n"
        "C-4007F\n"
        "...01..EE01\n";
    REQUIRE(xfm_song_declare(module, 1, pattern, 100, 4) == 1);
    xfm_song_play(module, 1, false);

    Test_AdvanceSongUntilChannelActive(module, 0);
    Test_AdvanceSongUntilRow(module, 1);

    CHECK(module->live_patches[0].op[0].TL == patchA.op[0].TL);
    song_advance_patch_morph_tick(module, 0);
    CHECK(module->live_patches[0].op[0].TL == patchA.op[0].TL);

    for (int i = 0; i < 254; i++) {
        song_advance_patch_morph_tick(module, 0);
    }
    CHECK(module->live_patches[0].op[0].TL == patchA.op[0].TL + 1);
    CHECK(module->active_song.channels[0].patch_morph_active);

    xfm_module_destroy(module);
}

TEST_CASE("Plain instrument change during morph updates the live state but keeps the target")
{
    xfm_module *module = xfm_module_create(44100, 256, XFM_CHIP_YM3438);
    REQUIRE(module != nullptr);

    xfm_patch_opn patchA = Tracker_DefaultPatch();
    patchA.op[0].TL = 10;

    xfm_patch_opn patchB = Tracker_DefaultPatch();
    patchB.op[0].TL = 100;

    xfm_patch_opn patchC = Tracker_DefaultPatch();
    patchC.op[0].TL = 40;

    xfm_patch_set(module, 0x00, &patchA, sizeof(patchA), XFM_CHIP_YM3438);
    xfm_patch_set(module, 0x01, &patchB, sizeof(patchB), XFM_CHIP_YM3438);
    xfm_patch_set(module, 0x02, &patchC, sizeof(patchC), XFM_CHIP_YM3438);

    const char *pattern =
        "4\n"
        "C-4007F\n"
        "...01..EE20\n"
        "...02..\n"
        ".......\n";
    REQUIRE(xfm_song_declare(module, 1, pattern, 100, 4) == 1);
    xfm_song_play(module, 1, false);

    Test_AdvanceSongUntilChannelActive(module, 0);
    Test_AdvanceSongUntilRow(module, 1);
    song_advance_patch_morph_tick(module, 0);
    REQUIRE(module->active_song.channels[0].patch_morph_active);

    Test_AdvanceSongUntilRow(module, 2);
    CHECK(module->live_patches[0].op[0].TL == patchC.op[0].TL);
    CHECK(module->active_song.channels[0].patch_morph_target_patch_id == 0x01);
    REQUIRE(module->active_song.channels[0].patch_morph_active);

    for (int i = 0; i < 8; i++) {
        song_advance_patch_morph_tick(module, 0);
    }
    CHECK(module->live_patches[0].op[0].TL > patchC.op[0].TL);
    CHECK(module->live_patches[0].op[0].TL <= patchB.op[0].TL);

    xfm_module_destroy(module);
}

TEST_CASE("Morph row with a note keeps the old instrument for the retrigger and arms morph afterwards")
{
    xfm_module *module = xfm_module_create(44100, 256, XFM_CHIP_YM3438);
    REQUIRE(module != nullptr);

    xfm_patch_opn patchA = Tracker_DefaultPatch();
    patchA.op[0].TL = 14;

    xfm_patch_opn patchB = Tracker_DefaultPatch();
    patchB.op[0].TL = 90;
    patchB.ALG = 7;

    xfm_patch_set(module, 0x00, &patchA, sizeof(patchA), XFM_CHIP_YM3438);
    xfm_patch_set(module, 0x01, &patchB, sizeof(patchB), XFM_CHIP_YM3438);

    const char *pattern =
        "2\n"
        "C-4007F\n"
        "D-401..EE20\n";
    REQUIRE(xfm_song_declare(module, 1, pattern, 100, 4) == 1);
    xfm_song_play(module, 1, false);

    Test_AdvanceSongUntilChannelActive(module, 0);
    Test_AdvanceSongUntilRow(module, 1);

    const XfmSongChannel &ch = module->active_song.channels[0];
    CHECK(ch.current_patch == 0x00);
    CHECK(ch.pending_patch == 0x00);
    CHECK(ch.patch_morph_pending_start);
    CHECK_FALSE(ch.patch_morph_active);

    xfm_module_destroy(module);
}

TEST_CASE("Repeated morph retriggers continue from the current live patch state")
{
    xfm_module *module = xfm_module_create(44100, 256, XFM_CHIP_YM3438);
    REQUIRE(module != nullptr);

    xfm_patch_opn patchA = Tracker_DefaultPatch();
    patchA.op[0].TL = 16;

    xfm_patch_opn patchB = Tracker_DefaultPatch();
    patchB.op[0].TL = 96;
    patchB.ALG = 7;

    xfm_patch_set(module, 0x00, &patchA, sizeof(patchA), XFM_CHIP_YM3438);
    xfm_patch_set(module, 0x01, &patchB, sizeof(patchB), XFM_CHIP_YM3438);

    const char *pattern =
        "4\n"
        "C-4007F\n"
        "D-401..EE40\n"
        "E-401..EE40\n"
        ".......\n";
    REQUIRE(xfm_song_declare(module, 1, pattern, 100, 4) == 1);
    xfm_song_play(module, 1, false);

    Test_AdvanceSongUntilChannelActive(module, 0);
    Test_AdvanceSongUntilRow(module, 1);
    REQUIRE(module->active_song.channels[0].patch_morph_pending_start);

    Test_AdvanceSongUntilRow(module, 2);
    REQUIRE(module->active_song.channels[0].patch_morph_active);
    for (int i = 0; i < 8; i++) {
        song_advance_patch_morph_tick(module, 0);
    }
    REQUIRE(module->live_patches[0].op[0].TL > patchA.op[0].TL);
    REQUIRE(module->live_patches[0].op[0].TL < patchB.op[0].TL);
    const int tlBeforeRetrigger = module->live_patches[0].op[0].TL;

    Test_AdvanceSongUntilRow(module, 3);
    REQUIRE(module->active_song.channels[0].patch_morph_active);
    CHECK(module->live_patches[0].op[0].TL >= tlBeforeRetrigger);
    CHECK(module->live_patches[0].op[0].TL < patchB.op[0].TL);

    for (int i = 0; i < 8; i++) {
        song_advance_patch_morph_tick(module, 0);
    }

    CHECK(module->live_patches[0].op[0].TL > tlBeforeRetrigger);
    CHECK(module->live_patches[0].op[0].TL <= patchB.op[0].TL);
    CHECK(module->active_song.channels[0].patch_morph_target_patch_id == 0x01);

    xfm_module_destroy(module);
}

TEST_CASE("Retrigger without morph effect loads the target instrument immediately")
{
    xfm_module *module = xfm_module_create(44100, 256, XFM_CHIP_YM3438);
    REQUIRE(module != nullptr);

    xfm_patch_opn patchA = Tracker_DefaultPatch();
    patchA.op[0].TL = 20;

    xfm_patch_opn patchB = Tracker_DefaultPatch();
    patchB.op[0].TL = 104;
    patchB.ALG = 7;

    xfm_patch_set(module, 0x00, &patchA, sizeof(patchA), XFM_CHIP_YM3438);
    xfm_patch_set(module, 0x01, &patchB, sizeof(patchB), XFM_CHIP_YM3438);

    const char *pattern =
        "4\n"
        "C-4007F\n"
        "D-401..EE40\n"
        "E-401..\n"
        ".......\n";
    REQUIRE(xfm_song_declare(module, 1, pattern, 100, 4) == 1);
    xfm_song_play(module, 1, false);

    Test_AdvanceSongUntilChannelActive(module, 0);
    Test_AdvanceSongUntilRow(module, 1);
    REQUIRE(module->active_song.channels[0].patch_morph_pending_start);

    Test_AdvanceSongUntilRow(module, 2);
    Test_AdvanceSongUntilRow(module, 3);
    CHECK(module->live_patches[0].op[0].TL == patchB.op[0].TL);
    CHECK(module->live_patches[0].ALG == patchB.ALG);
    CHECK(module->active_song.channels[0].patch_morph_target_patch_id == 0x01);

    xfm_module_destroy(module);
}

TEST_CASE("Effect values are clamped to effect definition ranges")
{
    Tracker tracker {};
    Tracker_Clear(&tracker);
    tracker.rowCount = 1;
    tracker.editRow = 0;
    tracker.editChannel = 0;

    // Effect 0x10 (OPN LFO) uses nibbles: A=on (0..1), B=freq (0..7).
    // Provide out-of-range A=9, B=F => should clamp to A=1, B=7 => 0x17.
    std::strncpy(tracker.cells[0][0].text, "C-4007F109F", TRACKER_CELL_CHARS);
    Tracker_ParseCellForEditor(&tracker);

    CHECK(tracker.editEffectCodes[0] == 0x10);
    CHECK(tracker.editEffectValues[0] == 0x17);
}

TEST_CASE("Last activated effect is serialized first")
{
    Tracker tracker {};
    Tracker_Clear(&tracker);
    tracker.rowCount = 1;
    tracker.editRow = 0;
    tracker.editChannel = 0;
    tracker.editSpecial = 0;
    tracker.editNote = 0;
    tracker.editOctave = 4;
    tracker.editInstrumentExplicit = true;
    tracker.editInstrument = 0x00;
    tracker.editVolumeExplicit = true;
    tracker.editVolume = 0x7F;

    tracker.editEffect = Tracker_EffectDefIndexByCode(0x04);
    Tracker_SetSelectedEffectValue(&tracker, 0x07);
    Tracker_ToggleSelectedEffectActive(&tracker);

    tracker.editEffect = Tracker_EffectDefIndexByCode(0x07);
    Tracker_SetSelectedEffectValue(&tracker, 0x05);
    Tracker_ToggleSelectedEffectActive(&tracker);
    Tracker_ApplyEditorToCell(&tracker);
    CHECK(std::string(tracker.cells[0][0].text) == "C-4007F07050407");

    tracker.editEffect = Tracker_EffectDefIndexByCode(0x04);
    Tracker_ApplyEditorToCell(&tracker);
    CHECK(std::string(tracker.cells[0][0].text) == "C-4007F07050407");

    Tracker_ToggleSelectedEffectActive(&tracker);
    Tracker_ApplyEditorToCell(&tracker);
    CHECK(std::string(tracker.cells[0][0].text) == "C-4007F0705");

    Tracker_ToggleSelectedEffectActive(&tracker);
    Tracker_ApplyEditorToCell(&tracker);
    CHECK(std::string(tracker.cells[0][0].text) == "C-4007F04070705");
}

TEST_CASE("Macro loop range auto-defines tail and can be cleared")
{
    Tracker tracker {};
    Tracker_Clear(&tracker);
    tracker.editInstrument = 0;
    tracker.editMacroTarget = XFM_MACRO_TL1;

    Tracker_SetMacroLoopRange(&tracker, 2, 5);
    XfmMacro &macro = Tracker_EditableMacro(&tracker);
    CHECK(macro.has_loop);
    CHECK(macro.loop_start == 2);
    CHECK(macro.release_start == 0xFF);

    Tracker_ClearMacroLoopRange(&tracker);
    CHECK_FALSE(macro.has_loop);
    CHECK(macro.loop_start == 0);
    CHECK(macro.release_start == 0xFF);
}

TEST_CASE("Macro enabled length can be cut back and extended again")
{
    Tracker tracker {};
    Tracker_Clear(&tracker);
    tracker.editInstrument = 0;
    tracker.editMacroTarget = XFM_MACRO_TL1;

    XfmMacro &macro = Tracker_EditableMacro(&tracker);
    CHECK(Tracker_MacroEnabledColumns(&macro) == 0);

    Tracker_EnableMacroThrough(&tracker, 5);
    CHECK(Tracker_MacroEnabledColumns(&macro) == 6);

    Tracker_SetMacroLoopRange(&tracker, 2, 4);
    CHECK(Tracker_MacroEnabledColumns(&macro) == 5);
    CHECK(macro.has_loop);
    CHECK(macro.loop_start == 2);
    CHECK(macro.release_start == 0xFF);

    Tracker_DisableMacroFrom(&tracker, 3);
    CHECK(Tracker_MacroEnabledColumns(&macro) == 3);
    CHECK_FALSE(macro.has_loop);
    CHECK(macro.release_start == 0xFF);

    Tracker_EnableMacroThrough(&tracker, 7);
    CHECK(Tracker_MacroEnabledColumns(&macro) == 8);
}

TEST_CASE("Macro release start can be set and cleared independently")
{
    Tracker tracker {};
    Tracker_Clear(&tracker);
    tracker.editInstrument = 0;
    tracker.editMacroTarget = XFM_MACRO_TL1;

    Tracker_SetMacroReleaseStart(&tracker, 5);
    XfmMacro &macro = Tracker_EditableMacro(&tracker);
    CHECK(Tracker_MacroEnabledColumns(&macro) == 6);
    CHECK(macro.release_start == 5);

    Tracker_ClearMacroReleaseStart(&tracker);
    CHECK(macro.release_start == 0xFF);
}

TEST_CASE("Envelope-style macro targets do not support release tails")
{
    Tracker tracker {};
    Tracker_Clear(&tracker);
    tracker.editInstrument = 0;
    tracker.editMacroTarget = XFM_MACRO_AR1;

    XfmMacro &macro = Tracker_EditableMacro(&tracker);
    CHECK_FALSE(Tracker_MacroTargetSupportsRelease(XFM_MACRO_AR1));

    Tracker_SetMacroReleaseStart(&tracker, 5);
    CHECK(macro.release_start == 0xFF);
    CHECK(Tracker_MacroEnabledColumns(&macro) == 0);

    macro.length = 6;
    macro.release_start = 5;
    Tracker_NormalizeMacroUiState(&macro);
    CHECK(macro.release_start == 0xFF);
}

TEST_CASE("Drawing-related macro edits grow used length from zero")
{
    Tracker tracker {};
    Tracker_Clear(&tracker);
    tracker.editInstrument = 0;
    tracker.editMacroTarget = XFM_MACRO_TL1;

    XfmMacro &macro = Tracker_EditableMacro(&tracker);
    CHECK(Tracker_MacroEnabledColumns(&macro) == 0);

    Tracker_SetMacroLoopRange(&tracker, 2, 5);
    CHECK(Tracker_MacroEnabledColumns(&macro) == 6);
    CHECK(macro.has_loop);
    CHECK(macro.loop_start == 2);

    Tracker_DisableMacroFrom(&tracker, 0);
    CHECK(Tracker_MacroEnabledColumns(&macro) == 0);
    CHECK_FALSE(macro.has_loop);
    CHECK(macro.release_start == 0xFF);

    Tracker_SetMacroReleaseStart(&tracker, 4);
    CHECK(Tracker_MacroEnabledColumns(&macro) == 5);
    CHECK(macro.release_start == 4);
}

TEST_CASE("Clearing macro loop keeps the release tail intact")
{
    Tracker tracker {};
    Tracker_Clear(&tracker);
    tracker.editInstrument = 0;
    tracker.editMacroTarget = XFM_MACRO_TL1;

    Tracker_EnableMacroThrough(&tracker, 7);
    Tracker_SetMacroReleaseStart(&tracker, 5);
    Tracker_SetMacroLoopRange(&tracker, 2, 4);
    XfmMacro &macro = Tracker_EditableMacro(&tracker);
    REQUIRE(macro.has_loop);
    REQUIRE(Tracker_MacroEnabledColumns(&macro) == 5);
    REQUIRE(macro.release_start == 0xFF);

    Tracker_ClearMacroLoopRange(&tracker);
    CHECK_FALSE(macro.has_loop);
    CHECK(macro.loop_start == 0);
    CHECK(macro.release_start == 0xFF);
    CHECK(Tracker_MacroEnabledColumns(&macro) == 5);
}

TEST_CASE("Macro release tail waits for REL when no loop is active")
{
    xfm_module *module = xfm_module_create(44100, 256, XFM_CHIP_YM3438);
    REQUIRE(module != nullptr);

    xfm_patch_opn patch = Tracker_DefaultPatch();
    patch.op[0].TL = 40;
    xfm_patch_set(module, 0x00, &patch, sizeof(patch), XFM_CHIP_YM3438);

    XfmMacro macro {};
    macro.target = XFM_MACRO_TL1;
    macro.length = 3;
    macro.values[0] = 50;
    macro.values[1] = 60;
    macro.values[2] = 90;
    macro.release_start = 2;
    REQUIRE(xfm_macro_set(module, 0x00, &macro) == 0x00);
    xfm_patch_macro_set(module, 0x00, XFM_MACRO_TL1, 0x00);

    REQUIRE(xfm_song_declare(module, 1, "1\nC-4007F\n", 100, 4) == 1);
    xfm_song_play(module, 1, false);
    Test_AdvanceSongUntilChannelActive(module, 0);

    CHECK(module->live_patches[0].op[0].TL == 50);

    int samplesPerTick = std::max(1, module->sample_rate / std::max(1, module->song_patterns[1].tick_rate));
    song_advance_macros(module, samplesPerTick);
    CHECK(module->live_patches[0].op[0].TL == 60);

    song_advance_macros(module, samplesPerTick);
    CHECK(module->live_patches[0].op[0].TL == 60);

    song_release_macros(module, 0);
    CHECK(module->live_patches[0].op[0].TL == 90);

    xfm_module_destroy(module);
}

TEST_CASE("Macro target base value comes from the edited patch")
{
    Tracker tracker {};
    Tracker_Clear(&tracker);
    tracker.editInstrument = 0;
    tracker.editPatchValid[0] = true;
    tracker.editPatches[0] = Tracker_DefaultPatch();
    tracker.editPatches[0].FB = 6;
    tracker.editPatches[0].op[0].TL = 41;
    tracker.editPatches[0].op[1].DT = -2;
    tracker.editPatches[0].op[2].MUL = 9;
    tracker.editPatches[0].op[3].SSG = 5;

    CHECK(Tracker_MacroTargetBaseValue(&tracker, XFM_MACRO_FB) == 6);
    CHECK(Tracker_MacroTargetBaseValue(&tracker, XFM_MACRO_TL1) == 41);
    CHECK(Tracker_MacroTargetBaseValue(&tracker, XFM_MACRO_DT2) == -2);
    CHECK(Tracker_MacroTargetBaseValue(&tracker, XFM_MACRO_MUL3) == 9);
    CHECK(Tracker_MacroTargetBaseValue(&tracker, XFM_MACRO_SSG4) == 5);
    CHECK(Tracker_MacroTargetBaseValue(&tracker, XFM_MACRO_ARP) == 0);
    CHECK(Tracker_MacroTargetBaseValue(&tracker, XFM_MACRO_PAN) == 3);
    CHECK(Tracker_MacroTargetBaseValue(&tracker, XFM_MACRO_PITCH) == 0);
    CHECK(Tracker_MacroTargetBaseValue(&tracker, XFM_MACRO_RELATIVE) == 0);
    CHECK(Tracker_MacroTargetBaseValue(&tracker, XFM_MACRO_PHASE_RESET) == 0);
}

TEST_CASE("Song IO recognizes new Furnace-style macro targets")
{
    CHECK(std::string(TrackerSongIO_MacroTargetName(XFM_MACRO_PAN)) == "PAN");
    CHECK(std::string(TrackerSongIO_MacroTargetName(XFM_MACRO_PITCH)) == "PITCH");
    CHECK(std::string(TrackerSongIO_MacroTargetName(XFM_MACRO_RELATIVE)) == "RELATIVE");
    CHECK(std::string(TrackerSongIO_MacroTargetName(XFM_MACRO_PHASE_RESET)) == "PHASE");

    CHECK(TrackerSongIO_MacroTargetFromArg("PAN") == XFM_MACRO_PAN);
    CHECK(TrackerSongIO_MacroTargetFromArg("PITCH") == XFM_MACRO_PITCH);
    CHECK(TrackerSongIO_MacroTargetFromArg("RELATIVE") == XFM_MACRO_RELATIVE);
    CHECK(TrackerSongIO_MacroTargetFromArg("PHASE") == XFM_MACRO_PHASE_RESET);
    CHECK(TrackerSongIO_MacroTargetFromArg("REL") == XFM_MACRO_RELATIVE);
    CHECK(TrackerSongIO_MacroTargetFromArg("PHASE_RESET") == XFM_MACRO_PHASE_RESET);
}

TEST_CASE("Cut copies selected cells and clears the source")
{
    Tracker tracker {};
    Tracker_Clear(&tracker);
    tracker.rowCount = 2;
    std::strncpy(tracker.cells[0][0].text, "C-4007F", TRACKER_CELL_CHARS);
    std::strncpy(tracker.cells[1][0].text, "D-4007F", TRACKER_CELL_CHARS);
    Tracker_SetLoopRange(&tracker, 0, 1);
    Tracker_SetChannelSelection(&tracker, 0, 0);

    Tracker_CutSelection(&tracker);

    REQUIRE(tracker.clipboard.valid);
    CHECK(tracker.clipboard.rows == 2);
    CHECK(tracker.clipboard.channels == 1);
    CHECK(std::string(tracker.clipboard.cells[0][0].text) == "C-4007F");
    CHECK(std::string(tracker.clipboard.cells[1][0].text) == "D-4007F");
    CHECK(std::string(tracker.cells[0][0].text) == ".......");
    CHECK(std::string(tracker.cells[1][0].text) == ".......");
}

TEST_CASE("Cut cooldown prevents a second cut from replacing the clipboard")
{
    Tracker tracker {};
    Tracker_Clear(&tracker);
    tracker.rowCount = 2;
    std::strncpy(tracker.cells[0][0].text, "C-4007F", TRACKER_CELL_CHARS);
    std::strncpy(tracker.cells[1][0].text, "D-4007F", TRACKER_CELL_CHARS);
    Tracker_SetLoopRange(&tracker, 0, 1);
    Tracker_SetChannelSelection(&tracker, 0, 0);

    Tracker_CutSelection(&tracker);
    REQUIRE(tracker.clipboard.valid);
    CHECK(std::string(tracker.clipboard.cells[0][0].text) == "C-4007F");
    CHECK(std::string(tracker.clipboard.cells[1][0].text) == "D-4007F");
    CHECK(std::string(tracker.clipboardBannerText) == "[2x1] CUT");
    CHECK(tracker.clipboardBannerKind == TRACKER_CLIPBOARD_BANNER_SUCCESS);
    CHECK(tracker.clipboardCutCooldown > 0.0f);

    Tracker_CutSelection(&tracker);

    CHECK(std::string(tracker.clipboard.cells[0][0].text) == "C-4007F");
    CHECK(std::string(tracker.clipboard.cells[1][0].text) == "D-4007F");
    CHECK(std::string(tracker.clipboardBannerText) == "ACCIDENTAL CUT PREVENTED");
    CHECK(tracker.clipboardBannerKind == TRACKER_CLIPBOARD_BANNER_ERROR);
    CHECK(tracker.clipboardCutCooldown > 0.0f);

    Tracker_CopySelection(&tracker);

    CHECK(std::string(tracker.clipboard.cells[0][0].text) == "C-4007F");
    CHECK(std::string(tracker.clipboard.cells[1][0].text) == "D-4007F");
    CHECK(std::string(tracker.clipboardBannerText) == "ACCIDENTAL COPY PREVENTED");
    CHECK(tracker.clipboardBannerKind == TRACKER_CLIPBOARD_BANNER_ERROR);
}

TEST_CASE("Cell move requires a long press before arming")
{
    Tracker tracker {};
    Tracker_Clear(&tracker);
    tracker.rowCount = 2;
    std::strncpy(tracker.cells[0][0].text, "C-4007F", TRACKER_CELL_CHARS);

    Tracker_BeginCellMovePending(&tracker, 0, 0, 100.0f, 120.0f, 1000);
    REQUIRE(tracker.cellMovePending);
    CHECK_FALSE(Tracker_TryArmCellMovePending(&tracker, 1399));
    CHECK_FALSE(tracker.cellMoving);

    CHECK(Tracker_TryArmCellMovePending(&tracker, 1400));
    REQUIRE(tracker.cellMoving);
    CHECK(tracker.cellMoveSourceRow == 0);
    CHECK(tracker.cellMoveSourceChannel == 0);
    CHECK_FALSE(tracker.cellMovePending);
}

TEST_CASE("Cell move pending cancels when the finger moves too far before the hold threshold")
{
    Tracker tracker {};
    Tracker_Clear(&tracker);
    tracker.rowCount = 2;
    std::strncpy(tracker.cells[0][0].text, "C-4007F", TRACKER_CELL_CHARS);

    Tracker_BeginCellMovePending(&tracker, 0, 0, 100.0f, 120.0f, 1000);
    Tracker_UpdateCellMovePendingPointer(&tracker, 112.5f, 135.0f);
    CHECK_FALSE(Tracker_TryArmCellMovePending(&tracker, 1900));
    CHECK_FALSE(tracker.cellMovePending);
    CHECK_FALSE(tracker.cellMoving);
}

TEST_CASE("Leaving an occupied cell before the hold threshold turns the gesture into scroll")
{
    Tracker tracker {};
    Tracker_Clear(&tracker);
    tracker.rowCount = 2;
    std::strncpy(tracker.cells[0][0].text, "C-4007F", TRACKER_CELL_CHARS);

    Tracker_BeginCellMovePending(&tracker, 0, 0, 100.0f, 120.0f, 1000);
    CHECK_FALSE(tracker.dragging);

    Tracker_SuppressCellMovePending(&tracker);
    Tracker_BeginScrollDragFromPendingCellMove(&tracker, 120.0f);

    CHECK_FALSE(tracker.cellMovePending);
    CHECK(tracker.cellMovePendingSuppressed);
    CHECK(tracker.dragging);
    CHECK(tracker.dragMoved);
    CHECK(tracker.dragStartY == doctest::Approx(120.0f));
    CHECK(tracker.dragLastY == doctest::Approx(120.0f));
}

TEST_CASE("Pending occupied-cell press still counts as a tap if released in the same cell")
{
    Tracker tracker {};
    Tracker_Clear(&tracker);
    tracker.rowCount = 2;
    std::strncpy(tracker.cells[0][0].text, "C-4007F", TRACKER_CELL_CHARS);

    Tracker_BeginCellMovePending(&tracker, 0, 0, 100.0f, 120.0f, 1000);
    Tracker_UpdateCellMovePendingPointer(&tracker, 104.0f, 124.0f);

    CHECK(Tracker_IsTapReleaseForPendingCellMove(&tracker, 0, 0));
    CHECK_FALSE(Tracker_IsTapReleaseForPendingCellMove(&tracker, 0, 1));

    Tracker_UpdateCellMovePendingPointer(&tracker, 109.0f, 120.0f);
    CHECK_FALSE(Tracker_IsTapReleaseForPendingCellMove(&tracker, 0, 0));
}

TEST_CASE("Cell move stays suppressed for the rest of the touch after leaving the source cell")
{
    Tracker tracker {};
    Tracker_Clear(&tracker);
    tracker.rowCount = 2;
    std::strncpy(tracker.cells[0][0].text, "C-4007F", TRACKER_CELL_CHARS);

    Tracker_BeginCellMovePending(&tracker, 0, 0, 100.0f, 120.0f, 1000);
    Tracker_SuppressCellMovePending(&tracker);
    CHECK_FALSE(Tracker_TryArmCellMovePending(&tracker, 2000));
    CHECK_FALSE(tracker.cellMovePending);
    CHECK(tracker.cellMovePendingSuppressed);

    Tracker_CancelCellMovePending(&tracker);
    CHECK_FALSE(tracker.cellMovePendingSuppressed);
    Tracker_BeginCellMovePending(&tracker, 0, 0, 100.0f, 120.0f, 3000);
    CHECK(Tracker_TryArmCellMovePending(&tracker, 3800));
    REQUIRE(tracker.cellMoving);
}

TEST_CASE("Edit selection overrides play selection for copy and cut")
{
    Tracker tracker {};
    Tracker_Clear(&tracker);
    tracker.rowCount = 2;
    Tracker_ResetSinglePart(&tracker);
    tracker.editSelectionEnabled = true;
    std::strncpy(tracker.cells[0][0].text, "C-4000F", TRACKER_CELL_CHARS);
    std::strncpy(tracker.cells[0][1].text, "D-4000F", TRACKER_CELL_CHARS);
    std::strncpy(tracker.cells[1][0].text, "E-4000F", TRACKER_CELL_CHARS);
    std::strncpy(tracker.cells[1][1].text, "F-4000F", TRACKER_CELL_CHARS);

    Tracker_SetLoopRange(&tracker, 0, 0);
    Tracker_SetChannelSelection(&tracker, 0, 0);
    Tracker_SetEditSelection(&tracker, 0, 1, 0, 1);

    REQUIRE(Tracker_SelectionUsesEdit(&tracker));
    CHECK(Tracker_SelectedRowCount(&tracker) == 2);
    CHECK(Tracker_SelectedChannelCount(&tracker) == 2);

    Tracker_CopySelection(&tracker);

    REQUIRE(tracker.clipboard.valid);
    CHECK(tracker.clipboard.rows == 2);
    CHECK(tracker.clipboard.channels == 2);
    CHECK(std::string(tracker.clipboard.cells[0][0].text) == "C-4000F");
    CHECK(std::string(tracker.clipboard.cells[0][1].text) == "D-4000F");
    CHECK(std::string(tracker.clipboard.cells[1][0].text) == "E-4000F");
    CHECK(std::string(tracker.clipboard.cells[1][1].text) == "F-4000F");

    Tracker_CutSelection(&tracker);

    CHECK(std::string(tracker.cells[0][0].text) == ".......");
    CHECK(std::string(tracker.cells[0][1].text) == ".......");
    CHECK(std::string(tracker.cells[1][0].text) == ".......");
    CHECK(std::string(tracker.cells[1][1].text) == ".......");
}

TEST_CASE("Clipboard pasteability tracks the active selection shape")
{
    Tracker tracker {};
    Tracker_Clear(&tracker);
    tracker.rowCount = 2;
    Tracker_ResetSinglePart(&tracker);
    tracker.editSelectionEnabled = true;
    std::strncpy(tracker.cells[0][0].text, "C-4000F", TRACKER_CELL_CHARS);
    std::strncpy(tracker.cells[0][1].text, "D-4000F", TRACKER_CELL_CHARS);
    std::strncpy(tracker.cells[1][0].text, "E-4000F", TRACKER_CELL_CHARS);
    std::strncpy(tracker.cells[1][1].text, "F-4000F", TRACKER_CELL_CHARS);

    Tracker_SetEditSelection(&tracker, 0, 1, 0, 1);
    Tracker_CopySelection(&tracker);

    REQUIRE(tracker.clipboard.valid);
    CHECK(Tracker_CanPaste(&tracker));

    Tracker_SetEditSelection(&tracker, 0, 0, 0, 1);
    CHECK(Tracker_SelectedRowCount(&tracker) == 1);
    CHECK(Tracker_SelectedChannelCount(&tracker) == 2);
    CHECK_FALSE(Tracker_CanPaste(&tracker));

    Tracker_SetEditSelection(&tracker, 0, 1, 0, 0);
    CHECK(Tracker_SelectedRowCount(&tracker) == 2);
    CHECK(Tracker_SelectedChannelCount(&tracker) == 1);
    CHECK_FALSE(Tracker_CanPaste(&tracker));
}

TEST_CASE("Operator editor cycling wraps across the four operators")
{
    Tracker tracker {};
    Tracker_Clear(&tracker);

    tracker.editOperator = 0;
    Tracker_CycleEditOperator(&tracker, -1);
    CHECK(tracker.editOperator == 3);

    Tracker_CycleEditOperator(&tracker, +1);
    CHECK(tracker.editOperator == 0);

    Tracker_SetEditOperator(&tracker, 2);
    CHECK(tracker.editOperator == 2);
    Tracker_SetEditOperator(&tracker, 99);
    CHECK(tracker.editOperator == 3);
}

TEST_CASE("Edit selection can move within part bounds")
{
    Tracker tracker {};
    Tracker_Clear(&tracker);
    tracker.rowCount = 8;
    tracker.partCount = 1;
    tracker.parts[0].startRow = 0;
    tracker.parts[0].rowCount = 8;
    tracker.parts[0].enabled = true;
    Tracker_SetPartName(&tracker.parts[0], "PART 1");
    tracker.editSelectionEnabled = true;
    Tracker_SetEditSelection(&tracker, 1, 2, 1, 2);
    tracker.editMoveGrabRowOffset = 0;
    tracker.editMoveGrabChannelOffset = 0;

    Tracker_MoveEditSelectionToGrabbedCell(&tracker, 7, 5);

    CHECK(tracker.editSelectionStartRow == 6);
    CHECK(tracker.editSelectionEndRow == 7);
    CHECK(tracker.editSelectionStartChannel == 4);
    CHECK(tracker.editSelectionEndChannel == 5);
}

TEST_CASE("Edit selection moves by pointer delta instead of collapsing")
{
    Tracker tracker {};
    Tracker_Clear(&tracker);
    tracker.rowCount = 8;
    tracker.partCount = 1;
    tracker.parts[0].startRow = 0;
    tracker.parts[0].rowCount = 8;
    tracker.parts[0].enabled = true;
    Tracker_SetPartName(&tracker.parts[0], "PART 1");
    tracker.editSelectionEnabled = true;
    Tracker_SetEditSelection(&tracker, 1, 3, 1, 2);
    tracker.editSelectionAnchorPart = 0;
    tracker.editMoveBaseStartRow = tracker.editSelectionStartRow;
    tracker.editMoveBaseStartChannel = tracker.editSelectionStartChannel;
    tracker.editMovePointerStartRow = 1;
    tracker.editMovePointerStartChannel = 1;

    Tracker_MoveEditSelectionByPointer(&tracker, 3, 3);

    CHECK(tracker.editSelectionStartRow == 3);
    CHECK(tracker.editSelectionEndRow == 5);
    CHECK(tracker.editSelectionStartChannel == 3);
    CHECK(tracker.editSelectionEndChannel == 4);
}

TEST_CASE("Disabling edit selection falls back to the play selection")
{
    Tracker tracker {};
    Tracker_Clear(&tracker);
    tracker.rowCount = 2;
    Tracker_ResetSinglePart(&tracker);
    tracker.editSelectionEnabled = true;
    std::strncpy(tracker.cells[0][0].text, "C-4000F", TRACKER_CELL_CHARS);
    std::strncpy(tracker.cells[1][0].text, "D-4000F", TRACKER_CELL_CHARS);
    std::strncpy(tracker.cells[0][1].text, "E-4000F", TRACKER_CELL_CHARS);
    std::strncpy(tracker.cells[1][1].text, "F-4000F", TRACKER_CELL_CHARS);

    Tracker_SetLoopRange(&tracker, 0, 1);
    Tracker_SetChannelSelection(&tracker, 0, 0);
    Tracker_SetEditSelection(&tracker, 1, 1, 1, 1);
    tracker.editSelectionEnabled = false;

    REQUIRE_FALSE(Tracker_SelectionUsesEdit(&tracker));
    CHECK(Tracker_SelectedRowCount(&tracker) == 2);
    CHECK(Tracker_SelectedChannelCount(&tracker) == 1);

    Tracker_CopySelection(&tracker);

    REQUIRE(tracker.clipboard.valid);
    CHECK(tracker.clipboard.rows == 2);
    CHECK(tracker.clipboard.channels == 1);
    CHECK(std::string(tracker.clipboard.cells[0][0].text) == "C-4000F");
    CHECK(std::string(tracker.clipboard.cells[1][0].text) == "D-4000F");
}

TEST_CASE("Paste uses the active edit selection rectangle")
{
    Tracker source {};
    Tracker_Clear(&source);
    source.rowCount = 2;
    Tracker_ResetSinglePart(&source);
    source.editSelectionEnabled = true;
    std::strncpy(source.cells[0][0].text, "C-4000F", TRACKER_CELL_CHARS);
    std::strncpy(source.cells[0][1].text, ".......", TRACKER_CELL_CHARS);
    std::strncpy(source.cells[1][0].text, "D-4000F", TRACKER_CELL_CHARS);
    std::strncpy(source.cells[1][1].text, ".......", TRACKER_CELL_CHARS);

    Tracker_SetEditSelection(&source, 0, 1, 0, 0);
    Tracker_CopySelection(&source);

    Tracker dest {};
    Tracker_Clear(&dest);
    dest.rowCount = 2;
    Tracker_ResetSinglePart(&dest);
    dest.clipboard = source.clipboard;
    dest.editSelectionEnabled = true;
    Tracker_SetLoopRange(&dest, 0, 0);
    Tracker_SetChannelSelection(&dest, 0, 0);
    Tracker_SetEditSelection(&dest, 0, 1, 1, 1);

    Tracker_PasteSelection(&dest);

    CHECK(std::string(dest.cells[0][1].text) == "C-4000F");
    CHECK(std::string(dest.cells[1][1].text) == "D-4000F");
    CHECK(std::string(dest.cells[0][0].text) == ".......");
    CHECK(std::string(dest.cells[1][0].text) == ".......");
}

TEST_CASE("Edit selection clamps to its part bounds")
{
    Tracker tracker {};
    Tracker_Clear(&tracker);
    tracker.rowCount = 4;
    tracker.partCount = 2;
    tracker.parts[0].startRow = 0;
    tracker.parts[0].rowCount = 2;
    tracker.parts[0].enabled = true;
    Tracker_SetPartName(&tracker.parts[0], "PART 1");
    tracker.parts[1].startRow = 2;
    tracker.parts[1].rowCount = 2;
    tracker.parts[1].enabled = true;
    Tracker_SetPartName(&tracker.parts[1], "PART 2");

    tracker.editSelectionEnabled = true;
    Tracker_SetEditSelection(&tracker, 0, 3, 0, 2);

    CHECK(tracker.editSelectionStartRow == 0);
    CHECK(tracker.editSelectionEndRow == 1);
    CHECK(tracker.editSelectionStartChannel == 0);
    CHECK(tracker.editSelectionEndChannel == 2);
    CHECK(Tracker_SelectedRowCount(&tracker) == 2);
}

TEST_CASE("Pasting within the same song keeps matching clipboard instruments")
{
    Tracker tracker {};
    Tracker_Clear(&tracker);
    tracker.rowCount = 2;
    Tracker_ClearAvailableInstruments(&tracker);
    Tracker_SetInstrumentAvailable(&tracker, 0x07);
    Tracker_SetInstrumentName(&tracker, 0x07, "Lead", 4);
    tracker.instrumentColors[0x07] = 0x123456;
    tracker.editPatches[0x07] = Tracker_DefaultPatch();
    tracker.editPatches[0x07].ALG = 5;
    tracker.editPatchValid[0x07] = true;
    Tracker_DefaultMacro(&tracker.editMacros[0x07][XFM_MACRO_TL1], XFM_MACRO_TL1);
    tracker.editMacros[0x07][XFM_MACRO_TL1].length = 4;
    tracker.editMacros[0x07][XFM_MACRO_TL1].values[0] = 11;
    tracker.editMacros[0x07][XFM_MACRO_TL1].values[1] = 12;
    tracker.editMacros[0x07][XFM_MACRO_TL1].values[2] = 13;
    tracker.editMacros[0x07][XFM_MACRO_TL1].values[3] = 14;
    tracker.editMacroEnabled[0x07][XFM_MACRO_TL1] = true;
    tracker.editMacroValid[0x07][XFM_MACRO_TL1] = true;
    std::strncpy(tracker.cells[0][0].text, "C-4077F", TRACKER_CELL_CHARS);

    Tracker_SetLoopRange(&tracker, 0, 0);
    Tracker_SetChannelSelection(&tracker, 0, 0);
    Tracker_CopySelection(&tracker);

    REQUIRE(tracker.clipboard.instrumentCount == 1);
    CHECK(tracker.clipboard.instrumentSourceIds[0] == 0x07);
    CHECK(tracker.clipboard.instrumentPasteIds[0] == 0x07);

    Tracker_SetLoopRange(&tracker, 1, 1);
    Tracker_PasteSelection(&tracker);

    CHECK(std::string(tracker.cells[1][0].text) == "C-4077F");
    CHECK(tracker.availableInstruments[0x07]);
    CHECK_FALSE(tracker.availableInstruments[0x00]);
    CHECK(std::string(Tracker_InstrumentName(&tracker, 0x07)) == "Lead");
}

TEST_CASE("Pasting across songs imports and remaps clipboard instruments")
{
    Tracker source {};
    Tracker_Clear(&source);
    source.rowCount = 1;
    Tracker_ClearAvailableInstruments(&source);
    Tracker_SetInstrumentAvailable(&source, 0x07);
    Tracker_SetInstrumentName(&source, 0x07, "Lead", 4);
    source.instrumentColors[0x07] = 0x123456;
    source.editPatches[0x07] = Tracker_DefaultPatch();
    source.editPatches[0x07].ALG = 5;
    source.editPatches[0x07].FB = 6;
    source.editPatchValid[0x07] = true;
    Tracker_DefaultMacro(&source.editMacros[0x07][XFM_MACRO_TL1], XFM_MACRO_TL1);
    source.editMacros[0x07][XFM_MACRO_TL1].length = 4;
    source.editMacros[0x07][XFM_MACRO_TL1].values[0] = 21;
    source.editMacros[0x07][XFM_MACRO_TL1].values[1] = 22;
    source.editMacros[0x07][XFM_MACRO_TL1].values[2] = 23;
    source.editMacros[0x07][XFM_MACRO_TL1].values[3] = 24;
    source.editMacroEnabled[0x07][XFM_MACRO_TL1] = true;
    source.editMacroValid[0x07][XFM_MACRO_TL1] = true;
    std::strncpy(source.cells[0][0].text, "C-4077F", TRACKER_CELL_CHARS);
    Tracker_SetLoopRange(&source, 0, 0);
    Tracker_SetChannelSelection(&source, 0, 0);
    Tracker_CopySelection(&source);

    Tracker dest {};
    Tracker_Clear(&dest);
    dest.rowCount = 1;
    dest.clipboard = source.clipboard;
    Tracker_ClearAvailableInstruments(&dest);
    Tracker_SetInstrumentAvailable(&dest, 0x07);
    Tracker_SetInstrumentName(&dest, 0x07, "Bass", 4);
    dest.instrumentColors[0x07] = 0x654321;
    dest.editPatches[0x07] = Tracker_DefaultPatch();
    dest.editPatches[0x07].ALG = 1;
    dest.editPatchValid[0x07] = true;
    Tracker_SetLoopRange(&dest, 0, 0);
    Tracker_SetChannelSelection(&dest, 0, 0);

    Tracker_PasteSelection(&dest);

    REQUIRE(dest.clipboard.instrumentCount == 1);
    REQUIRE(dest.clipboard.instrumentPasteMapped[0]);
    CHECK(dest.clipboard.instrumentPasteIds[0] == 0x00);
    CHECK(std::string(dest.cells[0][0].text) == "C-4007F");
    CHECK(dest.availableInstruments[0x00]);
    CHECK(dest.availableInstruments[0x07]);
    CHECK(std::string(Tracker_InstrumentName(&dest, 0x00)) == "Lead");
    CHECK(std::string(Tracker_InstrumentName(&dest, 0x07)) == "Bass");
    CHECK(dest.instrumentColors[0x00] == 0x123456);
    CHECK(dest.instrumentColors[0x07] == 0x654321);
    CHECK(dest.editPatchValid[0x00]);
    CHECK(dest.editPatches[0x00].ALG == 5);
    CHECK(dest.editPatches[0x00].FB == 6);
    CHECK(dest.editPatches[0x07].ALG == 1);
    CHECK(dest.editMacroEnabled[0x00][XFM_MACRO_TL1]);
    CHECK(dest.editMacroValid[0x00][XFM_MACRO_TL1]);
    CHECK(dest.editMacros[0x00][XFM_MACRO_TL1].length == 4);
    CHECK(dest.editMacros[0x00][XFM_MACRO_TL1].values[3] == 24);
}

TEST_CASE("Loading a new song clears stale instrument slots before clipboard remap")
{
    Tracker source {};
    Tracker_Clear(&source);
    source.rowCount = 1;
    Tracker_ClearInstrumentState(&source, false);
    Tracker_SetInstrumentAvailable(&source, 0x00);
    Tracker_SetInstrumentName(&source, 0x00, "A", 1);
    source.editPatches[0x00] = Tracker_DefaultPatch();
    source.editPatches[0x00].ALG = 1;
    source.editPatchValid[0x00] = true;
    Tracker_SetInstrumentAvailable(&source, 0x01);
    Tracker_SetInstrumentName(&source, 0x01, "B", 1);
    source.editPatches[0x01] = Tracker_DefaultPatch();
    source.editPatches[0x01].ALG = 2;
    source.editPatchValid[0x01] = true;
    std::strncpy(source.cells[0][0].text, "C-4007F", TRACKER_CELL_CHARS);
    std::strncpy(source.cells[0][1].text, "D-4017F", TRACKER_CELL_CHARS);
    Tracker_SetLoopRange(&source, 0, 0);
    Tracker_SetChannelSelection(&source, 0, 1);
    Tracker_CopySelection(&source);

    Tracker dest {};
    Tracker_Clear(&dest);
    dest.rowCount = 2;
    dest.clipboard = source.clipboard;
    Tracker_SetInstrumentAvailable(&dest, 0x00);
    Tracker_SetInstrumentName(&dest, 0x00, "A", 1);
    dest.editPatches[0x00] = source.editPatches[0x00];
    dest.editPatchValid[0x00] = true;
    Tracker_SetInstrumentAvailable(&dest, 0x01);
    Tracker_SetInstrumentName(&dest, 0x01, "B", 1);
    dest.editPatches[0x01] = source.editPatches[0x01];
    dest.editPatchValid[0x01] = true;
    std::strncpy(dest.cells[0][0].text, "E-4007F", TRACKER_CELL_CHARS);
    std::strncpy(dest.cells[0][1].text, "F-4017F", TRACKER_CELL_CHARS);

    Tracker_ClearInstrumentState(&dest, true);
    Tracker_LoadCustomInstrumentText(
        &dest,
        "INST 00\nPATCH 3 0 0 0\nNAME C\nENDINST\n"
        "INST 01\nPATCH 4 0 0 0\nNAME D\nENDINST\n"
    );

    CHECK(std::string(dest.cells[0][0].text) == "E-4007F");
    CHECK(std::string(dest.cells[0][1].text) == "F-4017F");
    CHECK(std::string(Tracker_InstrumentName(&dest, 0x00)) == "C");
    CHECK(std::string(Tracker_InstrumentName(&dest, 0x01)) == "D");
    CHECK(dest.editPatches[0x00].ALG == 3);
    CHECK(dest.editPatches[0x01].ALG == 4);

    Tracker_SetLoopRange(&dest, 1, 1);
    Tracker_SetChannelSelection(&dest, 0, 1);
    Tracker_PasteSelection(&dest);

    CHECK(std::string(dest.cells[1][0].text) == "C-4027F");
    CHECK(std::string(dest.cells[1][1].text) == "D-4037F");
    CHECK(std::string(Tracker_InstrumentName(&dest, 0x00)) == "C");
    CHECK(std::string(Tracker_InstrumentName(&dest, 0x01)) == "D");
    CHECK(std::string(Tracker_InstrumentName(&dest, 0x02)) == "A");
    CHECK(std::string(Tracker_InstrumentName(&dest, 0x03)) == "B");
    CHECK(dest.editPatches[0x02].ALG == 1);
    CHECK(dest.editPatches[0x03].ALG == 2);
}

TEST_CASE("Cell move only commits to a different empty cell")
{
    Tracker tracker {};
    Tracker_Clear(&tracker);
    tracker.rowCount = 2;
    std::strncpy(tracker.cells[0][0].text, "C-4007F0407", TRACKER_CELL_CHARS);
    std::strncpy(tracker.cells[1][0].text, "D-4007F", TRACKER_CELL_CHARS);

    Tracker_BeginCellMove(&tracker, 0, 0);
    Tracker_UpdateCellMoveHover(&tracker, 1, 0);
    CHECK_FALSE(tracker.cellMoveValidTarget);
    CHECK_FALSE(Tracker_CommitCellMove(&tracker));
    Tracker_CancelCellMove(&tracker);

    Tracker_BeginCellMove(&tracker, 0, 0);
    Tracker_UpdateCellMoveHover(&tracker, 1, 1);
    REQUIRE(tracker.cellMoveValidTarget);
    CHECK(Tracker_CommitCellMove(&tracker));
    CHECK(std::string(tracker.cells[0][0].text) == ".......");
    CHECK(std::string(tracker.cells[1][1].text) == "C-4007F0407");
}

TEST_CASE("Partless pattern loads into one visible part")
{
    Tracker tracker {};
    setTrackerPatternState(&tracker, TRACKER_USER_SONG_SLOT, "2\nC-4007F|.......|.......|.......|.......|.......\nD-4007F|.......|.......|.......|.......|.......\n", "Unit");

    REQUIRE(tracker.partCount == 1);
    CHECK(std::string(tracker.parts[0].name) == "PART 1");
    CHECK(tracker.parts[0].startRow == 0);
    CHECK(tracker.parts[0].rowCount == 2);
    CHECK(Tracker_VisibleRowCount(&tracker) == 3);
    CHECK(Tracker_MapVisualIndex(&tracker, 0).kind == TRACKER_VISUAL_ROW_PART_TITLE);
    CHECK(Tracker_MapVisualIndex(&tracker, 1).row == 0);
    CHECK(Tracker_MapVisualIndex(&tracker, 2).row == 1);
}

TEST_CASE("Part syntax round trips separately from flat playback pattern")
{
    Tracker tracker {};
    const char *pattern =
        "3\n"
        "PART Verse\n"
        "C-4007F|.......|.......|.......|.......|.......\n"
        "PART Chorus\n"
        "D-4007F|.......|.......|.......|.......|.......\n"
        "E-4007F|.......|.......|.......|.......|.......\n";
    setTrackerPatternState(&tracker, TRACKER_USER_SONG_SLOT, pattern, "Unit");

    REQUIRE(tracker.partCount == 2);
    CHECK(std::string(tracker.parts[0].name) == "Verse");
    CHECK(std::string(tracker.parts[1].name) == "Chorus");
    CHECK(tracker.parts[0].rowCount == 1);
    CHECK(tracker.parts[1].startRow == 1);
    std::string saved = Tracker_BuildPartPatternText(&tracker);
    CHECK(saved.find("PART Verse\n") != std::string::npos);
    CHECK(saved.find("PART Chorus\n") != std::string::npos);
    std::string flat = Tracker_BuildFlatPatternText(&tracker);
    CHECK(flat.find("PART ") == std::string::npos);
}

TEST_CASE("Skipped part round trips and is omitted from flat playback rows")
{
    Tracker tracker {};
    const char *pattern =
        "2\n"
        "PART A\n"
        "C-4007F|.......|.......|.......|.......|.......\n"
        "SKIP B\n"
        "D-4007F|.......|.......|.......|.......|.......\n";
    setTrackerPatternState(&tracker, TRACKER_USER_SONG_SLOT, pattern, "Unit");

    REQUIRE(tracker.partCount == 2);
    CHECK(tracker.parts[0].enabled);
    CHECK_FALSE(tracker.parts[1].enabled);
    CHECK(std::string(tracker.parts[1].name) == "B");
    std::string saved = Tracker_BuildPartPatternText(&tracker);
    CHECK(saved.find("SKIP B\n") != std::string::npos);
    std::string flat = Tracker_BuildFlatPatternText(&tracker);
    CHECK(flat.rfind("1\n", 0) == 0);
    CHECK(flat.find("C-4007F") != std::string::npos);
    CHECK(flat.find("D-4007F") == std::string::npos);
    CHECK(Tracker_PlaybackRowCount(&tracker) == 1);
    CHECK(Tracker_SongRowForPlaybackRow(&tracker, 0) == 0);
    CHECK(Tracker_PlaybackRowForSongRow(&tracker, 1) == 0);
}

TEST_CASE("Skipped song ranges are detectable for explicit selection playback")
{
    Tracker tracker {};
    const char *pattern =
        "3\n"
        "PART A\n"
        "C-4007F|.......|.......|.......|.......|.......\n"
        "SKIP B\n"
        "D-4007F|.......|.......|.......|.......|.......\n"
        "E-4007F|.......|.......|.......|.......|.......\n";
    setTrackerPatternState(&tracker, TRACKER_USER_SONG_SLOT, pattern, "Unit");

    CHECK_FALSE(Tracker_SongRangeTouchesSkippedPart(&tracker, 0, 0));
    CHECK(Tracker_SongRangeTouchesSkippedPart(&tracker, 1, 1));
    CHECK(Tracker_SongRangeTouchesSkippedPart(&tracker, 0, 2));
}

TEST_CASE("Song range playback text includes skipped rows when explicitly requested")
{
    Tracker tracker {};
    const char *pattern =
        "3\n"
        "PART A\n"
        "C-4007F|.......|.......|.......|.......|.......\n"
        "SKIP B\n"
        "D-4007F|E-4007F|.......|.......|.......|.......\n"
        "F-4007F|G-4007F|.......|.......|.......|.......\n";
    setTrackerPatternState(&tracker, TRACKER_USER_SONG_SLOT, pattern, "Unit");

    std::string selectionPlayback = Tracker_BuildSongRangePatternText(&tracker, 1, 2);
    CHECK(selectionPlayback.rfind("2\n", 0) == 0);
    CHECK(selectionPlayback.find("D-4007F|E-4007F") != std::string::npos);
    CHECK(selectionPlayback.find("F-4007F|G-4007F") != std::string::npos);

    std::string soloPlayback = Tracker_BuildSongRangePatternText(&tracker, 1, 2, true, 1, 1);
    CHECK(soloPlayback.find(".......|E-4007F|.......|.......|.......|.......") != std::string::npos);
    CHECK(soloPlayback.find(".......|G-4007F|.......|.......|.......|.......") != std::string::npos);
}

TEST_CASE("OPN LFO frequency table uses YM2612 values")
{
    CHECK(Tracker_OpnLfoFrequencyHz(0) == doctest::Approx(3.98f));
    CHECK(Tracker_OpnLfoFrequencyHz(5) == doctest::Approx(9.63f));
    CHECK(Tracker_OpnLfoFrequencyHz(7) == doctest::Approx(72.2f));
}

TEST_CASE("Oscilloscope OPN frequency and period use fnum block formula")
{
    int fnum = 512;
    int block = 6;
    float hz = TrackerOscilloscope_OpnFrequencyHz(fnum, block);
    CHECK(hz == doctest::Approx((512.0f * 144.0f) / std::pow(2.0f, 14.0f)));
    CHECK(TrackerOscilloscope_PeriodSamples(44100, fnum, block) == doctest::Approx(44100.0f / hz));
    CHECK(TrackerOscilloscope_FastWrap(-1, 8) == 7);
    CHECK(TrackerOscilloscope_FastWrap(9, 8) == 1);
}

TEST_CASE("Oscilloscope renderer repeats one stationary cycle from note start")
{
    int16_t ring[TRACKER_OSC_RING_SIZE] = {};
    for (int i = 0; i < TRACKER_OSC_RING_SIZE; i++)
        ring[i] = (i & 1) ? 12000 : -12000;

    TrackerOscilloscopeSnapshot snapshot = {};
    snapshot.sampleRate = 72;
    snapshot.channels[0].ring = ring;
    snapshot.channels[0].ringSize = TRACKER_OSC_RING_SIZE;
    snapshot.channels[0].sampleCursor = 128;
    snapshot.channels[0].noteStartSample = 0;
    snapshot.channels[0].fnum = 512;
    snapshot.channels[0].block = 7;
    snapshot.channels[0].keyOn = true;

    uint32_t pixels[96 * 60] = {};
    TrackerOscilloscope_DrawAtlas(pixels, 96, 60, snapshot);

    int nonBlack = 0;
    uint32_t black = TrackerOscilloscope_Rgba(0, 0, 0, 255);
    for (uint32_t pixel : pixels)
        if (pixel != black)
            nonBlack++;
    CHECK(nonBlack > 96);
}

TEST_CASE("Collapsed part hides rows but maps playhead to title")
{
    Tracker tracker {};
    setTrackerPatternState(&tracker, TRACKER_USER_SONG_SLOT, "2\nPART A\nC-4007F|.......|.......|.......|.......|.......\nD-4007F|.......|.......|.......|.......|.......\n", "Unit");
    tracker.parts[0].collapsed = true;
    setTrackerCursorState(&tracker, 1, 0, 6);

    CHECK(Tracker_VisibleRowCount(&tracker) == 1);
    CHECK(Tracker_VisualIndexForRow(&tracker, 0) == 0);
    CHECK(Tracker_VisualIndexForRow(&tracker, 1) == 0);
    CHECK(Tracker_PartPlaybackProgress(&tracker, 0) > 0.49f);
}

TEST_CASE("Part title sticks while rows from that part are at the viewport top")
{
    Tracker tracker {};
    setTrackerPatternState(
        &tracker,
        TRACKER_USER_SONG_SLOT,
        "4\n"
        "PART A\n"
        "C-4007F|.......|.......|.......|.......|.......\n"
        "D-4007F|.......|.......|.......|.......|.......\n"
        "PART B\n"
        "E-4007F|.......|.......|.......|.......|.......\n"
        "F-4007F|.......|.......|.......|.......|.......\n",
        "Unit"
    );
    tracker.rowHeight = 44.0f;

    tracker.scrollY = tracker.rowHeight * 0.5f;
    CHECK(Tracker_StickyPartIndexAtScroll(&tracker) == 0);
    CHECK(Tracker_StickyPartTitleTopY(&tracker, 0) == doctest::Approx(0.0f));

    tracker.scrollY = tracker.rowHeight * 2.5f;
    CHECK(Tracker_StickyPartIndexAtScroll(&tracker) == 0);
    CHECK(Tracker_StickyPartTitleTopY(&tracker, 0) == doctest::Approx(0.0f));

    tracker.scrollY = tracker.rowHeight * 3.0f;
    CHECK(Tracker_StickyPartIndexAtScroll(&tracker) == 1);
    CHECK(Tracker_StickyPartTitleTopY(&tracker, 1) == doctest::Approx(0.0f));
}

TEST_CASE("Part boundaries map to following part title visual rows")
{
    Tracker tracker {};
    setTrackerPatternState(
        &tracker,
        TRACKER_USER_SONG_SLOT,
        "4\n"
        "PART A\n"
        "C-4007F|.......|.......|.......|.......|.......\n"
        "D-4007F|.......|.......|.......|.......|.......\n"
        "PART B\n"
        "E-4007F|.......|.......|.......|.......|.......\n"
        "PART C\n"
        "F-4007F|.......|.......|.......|.......|.......\n",
        "Unit"
    );

    CHECK(Tracker_VisualIndexForPartBoundary(&tracker, 0) == Tracker_VisualIndexForPartTitle(&tracker, 1));
    CHECK(Tracker_VisualIndexForPartBoundary(&tracker, 0) == 3);
    CHECK(Tracker_VisualIndexForPartBoundary(&tracker, 1) == 5);
    CHECK(Tracker_VisualIndexForPartBoundary(&tracker, 2) == -1);

    tracker.parts[0].collapsed = true;
    CHECK(Tracker_VisualIndexForPartBoundary(&tracker, 0) == 1);
}

TEST_CASE("Selection is clamped to one part")
{
    Tracker tracker {};
    setTrackerPatternState(&tracker, TRACKER_USER_SONG_SLOT, "4\nPART A\nC-4007F|.......|.......|.......|.......|.......\nD-4007F|.......|.......|.......|.......|.......\nPART B\nE-4007F|.......|.......|.......|.......|.......\nF-4007F|.......|.......|.......|.......|.......\n", "Unit");

    Tracker_SetLoopRange(&tracker, 1, 3);
    CHECK(tracker.loopStart == 1);
    CHECK(tracker.loopEnd == 1);
    Tracker_SetLoopRange(&tracker, 2, 0);
    CHECK(tracker.loopStart == 2);
    CHECK(tracker.loopEnd == 2);
}

TEST_CASE("Adding a row to an earlier part shifts following parts")
{
    Tracker tracker {};
    setTrackerPatternState(&tracker, TRACKER_USER_SONG_SLOT, "2\nPART A\nC-4007F|.......|.......|.......|.......|.......\nPART B\nD-4007F|.......|.......|.......|.......|.......\n", "Unit");

    Tracker_AddRowToPart(&tracker, 0);

    REQUIRE(tracker.rowCount == 3);
    REQUIRE(tracker.partCount == 2);
    CHECK(tracker.parts[0].rowCount == 2);
    CHECK(tracker.parts[1].startRow == 2);
    CHECK(std::string(tracker.cells[2][0].text) == "D-4007F");
}

TEST_CASE("Adding a part appends it at the end")
{
    Tracker tracker {};
    setTrackerPatternState(
        &tracker,
        TRACKER_USER_SONG_SLOT,
        "2\nPART A\nC-4007F|.......|.......|.......|.......|.......\nPART B\nD-4007F|.......|.......|.......|.......|.......\n",
        "Unit"
    );

    Tracker_AddPartToEnd(&tracker);

    REQUIRE(tracker.partCount == 3);
    CHECK(std::string(tracker.parts[0].name) == "A");
    CHECK(std::string(tracker.parts[1].name) == "B");
    CHECK(std::string(tracker.parts[2].name) == "PART 3");
    CHECK(tracker.parts[2].startRow == 2);
    CHECK(tracker.parts[2].rowCount == 1);
}

TEST_CASE("Moving parts swaps row blocks without renaming")
{
    Tracker tracker {};
    setTrackerPatternState(&tracker, TRACKER_USER_SONG_SLOT, "2\nPART A\nC-4007F|.......|.......|.......|.......|.......\nPART B\nD-4007F|.......|.......|.......|.......|.......\n", "Unit");

    Tracker_MovePart(&tracker, 0, 1);

    REQUIRE(tracker.partCount == 2);
    CHECK(std::string(tracker.parts[0].name) == "B");
    CHECK(std::string(tracker.parts[1].name) == "A");
    CHECK(std::string(tracker.cells[0][0].text) == "D-4007F");
    CHECK(std::string(tracker.cells[1][0].text) == "C-4007F");
}

TEST_CASE("Scroll snapping preserves exact bottom when viewport is not row aligned")
{
    Tracker tracker {};
    setTrackerPatternState(&tracker, TRACKER_USER_SONG_SLOT, "4\nPART A\nC-4007F|.......|.......|.......|.......|.......\nD-4007F|.......|.......|.......|.......|.......\nE-4007F|.......|.......|.......|.......|.......\nF-4007F|.......|.......|.......|.......|.......\n", "Unit");
    tracker.rowHeight = 44.0f;
    tracker.viewportHeight = 153.0f;
    float maxScroll = Tracker_MaxScroll(&tracker);
    REQUIRE(maxScroll == doctest::Approx(67.0f));

    tracker.scrollY = maxScroll;
    Tracker_SnapToGrid(&tracker);
    CHECK(tracker.scrollY == doctest::Approx(maxScroll));

    tracker.active = true;
    Tracker_Tick(&tracker, 1.0f / 60.0f);
    CHECK(tracker.scrollY == doctest::Approx(maxScroll));
}
