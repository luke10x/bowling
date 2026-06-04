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
    CHECK(TrackerSongIO_SaveFilenameForDisplay("Song 261231") == "SONG_261231.h");
    CHECK(TrackerSongIO_StemToDisplay("MY_COOL_SONG") == "My Cool Song");
    CHECK(TrackerSongIO_SaveFilenameForDisplay("My Cool Song") == "MY_COOL_SONG.h");
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
    std::string text = TrackerSongIO_BuildFileText("My Jam", pattern, "INST 00\nPATCH 3 4 0 0\nENDINST\n", 75, 5, 8, true, 3);

    CHECK(text.find("#include \"tracker/xfm_song_dsl.h\"") != std::string::npos);
    CHECK(text.find("XFM_SONG_BEGIN(R\"xfmname(My Jam)xfmname\")") != std::string::npos);
    CHECK(text.find("XFM_TICK_RATE(75)") != std::string::npos);
    CHECK(text.find("XFM_PATTERN(R\"xfmpattern(") != std::string::npos);
    CHECK(text.find("XFM_INSTRUMENT(0x00)") != std::string::npos);
    CHECK(text.find("XFM_PATCH(ALG = 3, FB = 4, AMS = 0, FMS = 0)") != std::string::npos);

    int setting = 0;
    CHECK(TrackerSongIO_ExtractInt(text, "XFM_TRACKER_TICK_RATE", setting));
    CHECK(setting == 75);
    CHECK(TrackerSongIO_ExtractInt(text, "XFM_TRACKER_SPEED", setting));
    CHECK(setting == 5);
    CHECK(TrackerSongIO_ExtractInt(text, "XFM_TRACKER_ROWS_PER_BEAT", setting));
    CHECK(setting == 8);
    CHECK(TrackerSongIO_ExtractInt(text, "XFM_TRACKER_LFO_ENABLED", setting));
    CHECK(setting == 1);
    CHECK(TrackerSongIO_ExtractInt(text, "XFM_TRACKER_LFO_FREQUENCY", setting));
    CHECK(setting == 3);

    TrackerSongLoadResult loaded = TrackerSongIO_ParseFile("MY_JAM.h", text);
    REQUIRE(loaded.ok);
    CHECK(loaded.displayName == "My Jam");
    CHECK(loaded.pattern == pattern);

    std::string loadedInstruments;
    REQUIRE(TrackerSongIO_ExtractInstrumentText(text, loadedInstruments));
    CHECK(loadedInstruments.find("INST 00\n") != std::string::npos);
    CHECK(loadedInstruments.find("PATCH 3 4 0 0\n") != std::string::npos);
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
    REQUIRE(TrackerSongIO_ExtractInstrumentText(fileText, loadedInstruments));
    CHECK(loadedInstruments.find("INST 00\n") != std::string::npos);
    CHECK(loadedInstruments.find("NAME ALBINAS\n") != std::string::npos);

    bool referenced[256] = {};
    TrackerSongIO_MarkReferencedInstruments(pattern, referenced);
    CHECK(referenced[0x00]);
}

TEST_CASE("Readable instrument DSL round-trips macros to loadable instrument text")
{
    std::string legacy =
        "INST 02\n"
        "PATCH 4 5 1 2\n"
        "NAME Lead\n"
        "COLOR A0B0C0\n"
        "OP 1 0 1 20 0 31 1 12 8 3 7 0\n"
        "MACRO 1 4 1 255 20 21 22 23\n"
        "ENDINST\n";

    std::string text = TrackerSongIO_BuildFileText("Dsl Macro", "1\nC-4027F|.......|.......|.......|.......|.......\n", legacy);

    CHECK(text.find("XFM_INSTRUMENT(0x02)") != std::string::npos);
    CHECK(text.find("XFM_INSTRUMENT_NAME(\"Lead\")") != std::string::npos);
    CHECK(text.find("XFM_INSTRUMENT_COLOR(0xA0B0C0)") != std::string::npos);
    CHECK(text.find("XFM_TRACKER_MACRO(TL1, LENGTH = 4, LOOP = 1, RELEASE = 255, VALUES = \"20 21 22 23\")") != std::string::npos);

    std::string loaded;
    REQUIRE(TrackerSongIO_ExtractInstrumentText(text, loaded));
    CHECK(loaded.find("INST 02\n") != std::string::npos);
    CHECK(loaded.find("PATCH 4 5 1 2\n") != std::string::npos);
    CHECK(loaded.find("NAME Lead\n") != std::string::npos);
    CHECK(loaded.find("COLOR A0B0C0\n") != std::string::npos);
    CHECK(loaded.find("OP 1 0 1 20 0 31 1 12 8 3 7 0\n") != std::string::npos);
    CHECK(loaded.find("MACRO 1 4 1 255 20 21 22 23\n") != std::string::npos);
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
