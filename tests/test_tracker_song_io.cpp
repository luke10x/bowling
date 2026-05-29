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

TEST_CASE("Setting special '...' clears instrument and volume")
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

    Tracker_ApplyEditorToCell(&tracker);
    CHECK(std::strncmp(tracker.cells[0][0].text, "...", 3) == 0);
    CHECK(std::strncmp(tracker.cells[0][0].text + 3, "....", 4) == 0);
}
