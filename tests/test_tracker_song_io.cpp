#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../3rdparty/json/tests/thirdparty/doctest/doctest.h"

#define CLAY_IMPLEMENTATION
#include "../eggsfm/xfm_api.h"
#include "../tracker/tracker.h"
#include "../tracker/tracker_song_io.h"

TEST_CASE("Tracker song names convert between display and filenames")
{
    CHECK(TrackerSongIO_DefaultDateStem(2026, 12, 31) == "SONG_261231");
    CHECK(TrackerSongIO_StemToDisplay("SONG_261231") == "Song 261231");
    CHECK(TrackerSongIO_SaveFilenameForDisplay("Song 261231") == "SONG_261231.txt");
    CHECK(TrackerSongIO_StemToDisplay("MY_COOL_SONG") == "My Cool Song");
    CHECK(TrackerSongIO_SaveFilenameForDisplay("My Cool Song") == "MY_COOL_SONG.txt");
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

TEST_CASE("Tracker song files can use their song name as the download filename")
{
    std::string pattern = "1\nC-4007F|.......|.......|.......|.......|.......\n";
    std::string text = TrackerSongIO_BuildFileText("Alley Cat", pattern, "");

    CHECK(TrackerSongIO_SaveFilenameForDisplay("Alley Cat") == "ALLEY_CAT.txt");

    TrackerSongLoadResult loaded = TrackerSongIO_ParseFile("ALLEY_CAT.txt", text);
    REQUIRE(loaded.ok);
    CHECK(loaded.displayName == "Alley Cat");
    CHECK(loaded.pattern == pattern);
}

TEST_CASE("Cloned renamed built-in instruments used by the pattern are saved")
{
    Tracker tracker {};
    Tracker_Clear(&tracker);
    tracker.rowCount = 9;

    const int builtinGuitar = 0xFC; // legacy 0x03 mapped to 0xFF - 0x03
    Tracker_SetBuiltinInstrument(&tracker, builtinGuitar);
    tracker.editPatches[builtinGuitar] = PATCH_03_GUITAR;
    tracker.editPatchValid[builtinGuitar] = true;

    // Custom instruments start at 0x00 and fill upwards.
    REQUIRE(Tracker_CloneInstrument(&tracker, builtinGuitar, 0x00, "ALBINAS", 7));
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
    REQUIRE(TrackerSongIO_ExtractRawString(fileText, "XFM_TRACKER_CUSTOM_INSTRUMENTS", loadedInstruments));
    CHECK(loadedInstruments.find("INST 00\n") != std::string::npos);
    CHECK(loadedInstruments.find("NAME ALBINAS\n") != std::string::npos);

    bool referenced[256] = {};
    TrackerSongIO_MarkReferencedInstruments(pattern, referenced);
    CHECK(referenced[0x00]);
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
