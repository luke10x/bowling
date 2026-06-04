#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../3rdparty/json/tests/thirdparty/doctest/doctest.h"

#define CLAY_IMPLEMENTATION
#include "../eggsfm/xfm_api.h"
#include "../sounds/sounds.h"
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

TEST_CASE("Builtin instrument catalog reload preserves custom availability")
{
    Tracker tracker {};
    Tracker_Clear(&tracker);

    const int customInst = 0x02;
    Tracker_SetInstrumentAvailable(&tracker, customInst);
    tracker.editPatches[customInst] = Tracker_DefaultPatch();
    tracker.editPatchValid[customInst] = true;
    tracker.editPatchDirty[customInst] = false;

    Tracker_LoadBuiltinInstrumentCatalogPreserveCustom(&tracker);

    CHECK(Tracker_InstrumentAvailable(&tracker, customInst));
    CHECK_FALSE(tracker.builtinInstruments[customInst]);
    CHECK(Tracker_InstrumentAvailable(&tracker, 0xFF));
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

TEST_CASE("OPN LFO frequency table uses YM2612 values")
{
    CHECK(Tracker_OpnLfoFrequencyHz(0) == doctest::Approx(3.98f));
    CHECK(Tracker_OpnLfoFrequencyHz(5) == doctest::Approx(9.63f));
    CHECK(Tracker_OpnLfoFrequencyHz(7) == doctest::Approx(72.2f));
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
