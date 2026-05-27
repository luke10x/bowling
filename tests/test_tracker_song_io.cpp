#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../3rdparty/json/tests/thirdparty/doctest/doctest.h"

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
