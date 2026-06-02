#pragma once

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

static constexpr int TRACKER_BUILTIN_SONG_COUNT = 4;
static constexpr int TRACKER_USER_SONG_SLOT = 5;
static constexpr int TRACKER_MAX_SONG_COUNT = 5;
static constexpr int TRACKER_USER_SONG_MAX_ROWS = 1024;
static constexpr int TRACKER_USER_SONG_PATTERN_CAPACITY = 1024 * (6 * (7 + 4 * 4 + 1) + 1) + 32;
static constexpr int TRACKER_SONG_NAME_CAPACITY = 32;

struct TrackerSongLoadResult
{
    bool ok = false;
    std::string displayName;
    std::string pattern;
    std::string error;
};

inline void TrackerSongIO_MarkReferencedInstruments(const std::string &pattern, bool referenced[256])
{
    if (!referenced) return;
    const char *p = pattern.c_str();
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    while (*p >= '0' && *p <= '9') p++;
    while (*p && *p != '\n') p++;
    if (*p == '\n') p++;

    while (*p)
    {
        int columnPos = 0;
        while (*p && *p != '\n')
        {
            if (columnPos == 3)
            {
                auto hex = [](char c) -> int {
                    if (c >= '0' && c <= '9') return c - '0';
                    if (c >= 'A' && c <= 'F') return 10 + c - 'A';
                    if (c >= 'a' && c <= 'f') return 10 + c - 'a';
                    return -1;
                };
                int hi = hex(p[0]);
                int lo = hex(p[1]);
                if (hi >= 0 && lo >= 0)
                    referenced[(hi << 4) | lo] = true;
            }
            columnPos++;
            if (*p == '|')
                columnPos = 0;
            p++;
        }
        if (*p == '\n') p++;
    }
}

inline bool TrackerSongIO_IsNameChar(char c)
{
    unsigned char uc = (unsigned char)c;
    return std::isalnum(uc) || c == '_';
}

inline std::string TrackerSongIO_StripExtension(const std::string &filename)
{
    size_t slash = filename.find_last_of("/\\");
    size_t begin = slash == std::string::npos ? 0 : slash + 1;
    size_t dot = filename.find_last_of('.');
    if (dot == std::string::npos || dot < begin)
        dot = filename.size();
    return filename.substr(begin, dot - begin);
}

inline std::string TrackerSongIO_ToUpperStem(std::string stem)
{
    for (char &c : stem)
    {
        if (c == ' ') c = '_';
        else c = (char)std::toupper((unsigned char)c);
    }
    return stem;
}

inline std::string TrackerSongIO_DisplayToStem(const std::string &displayName)
{
    std::string out;
    out.reserve(displayName.size());
    bool prevUnderscore = false;
    for (char c : displayName)
    {
        unsigned char uc = (unsigned char)c;
        if (std::isalnum(uc))
        {
            out.push_back((char)std::toupper(uc));
            prevUnderscore = false;
        }
        else if ((c == ' ' || c == '_') && !prevUnderscore && !out.empty())
        {
            out.push_back('_');
            prevUnderscore = true;
        }
    }
    while (!out.empty() && out.back() == '_') out.pop_back();
    return out;
}

inline std::string TrackerSongIO_StemToDisplay(const std::string &stem)
{
    std::string upper = TrackerSongIO_ToUpperStem(stem);
    std::string out;
    out.reserve(upper.size());
    bool atWordStart = true;
    for (char c : upper)
    {
        if (c == '_')
        {
            if (!out.empty() && out.back() != ' ')
                out.push_back(' ');
            atWordStart = true;
            continue;
        }
        if (std::isalpha((unsigned char)c))
            out.push_back(atWordStart ? c : (char)std::tolower((unsigned char)c));
        else
            out.push_back(c);
        atWordStart = false;
    }
    return out;
}

inline bool TrackerSongIO_IsBuiltinStem(const std::string &stem)
{
    std::string s = TrackerSongIO_ToUpperStem(stem);
    return s == "BOWLING_STRIKE" || s == "GUTTER_GROOVE" ||
           s == "PIN_CRUSHER" || s == "ALLEY_CAT" ||
           s == "SONG_01" || s == "SONG_02" || s == "SONG_03" || s == "SONG_04";
}

inline bool TrackerSongIO_IsValidUserStem(const std::string &stem, std::string *error = nullptr)
{
    if (stem.size() < 3)
    {
        if (error) *error = "Song name is too short";
        return false;
    }
    if (stem.size() > 24)
    {
        if (error) *error = "Song name is too long";
        return false;
    }
    for (char c : stem)
    {
        if (!TrackerSongIO_IsNameChar(c))
        {
            if (error) *error = "Song name can only use letters, numbers, and underscore";
            return false;
        }
    }
    if (TrackerSongIO_IsBuiltinStem(stem))
    {
        if (error) *error = "That name is reserved for a built-in song";
        return false;
    }
    return true;
}

inline std::string TrackerSongIO_DefaultDateStem(int fullYear, int month, int day)
{
    char buf[16];
    std::snprintf(buf, sizeof(buf), "SONG_%02d%02d%02d", fullYear % 100, month, day);
    return buf;
}

inline std::string TrackerSongIO_SaveFilenameForDisplay(const std::string &displayName)
{
    return TrackerSongIO_DisplayToStem(displayName) + ".txt";
}

inline bool TrackerSongIO_ExtractRawString(const std::string &text, const char *symbol, std::string &out)
{
    size_t sym = text.find(symbol);
    if (sym == std::string::npos) return false;
    size_t raw = text.find("R\"", sym);
    if (raw == std::string::npos) return false;
    size_t open = text.find('(', raw + 2);
    if (open == std::string::npos) return false;
    std::string delimiter = text.substr(raw + 2, open - (raw + 2));
    std::string closeToken = ")" + delimiter + "\"";
    size_t close = text.find(closeToken, open + 1);
    if (close == std::string::npos) return false;
    out = text.substr(open + 1, close - open - 1);
    return true;
}

inline std::string TrackerSongIO_ExtractDisplayName(const std::string &text, const std::string &fallbackStem)
{
    std::string raw;
    if (TrackerSongIO_ExtractRawString(text, "XFM_TRACKER_SONG_NAME", raw))
    {
        while (!raw.empty() && (raw.back() == '\n' || raw.back() == '\r')) raw.pop_back();
        if (!raw.empty()) return raw;
    }
    return TrackerSongIO_StemToDisplay(fallbackStem);
}

inline bool TrackerSongIO_ExtractInt(const std::string &text, const char *symbol, int &out)
{
    size_t sym = text.find(symbol);
    if (sym == std::string::npos) return false;
    size_t eq = text.find('=', sym);
    if (eq == std::string::npos) return false;
    const char *p = text.c_str() + eq + 1;
    while (*p == ' ' || *p == '\t') p++;
    char *end = nullptr;
    long v = std::strtol(p, &end, 10);
    if (end == p) return false;
    out = (int)v;
    return true;
}

inline TrackerSongLoadResult TrackerSongIO_ParseFile(const std::string &filename, const std::string &text)
{
    TrackerSongLoadResult result;
    std::string stem = TrackerSongIO_ToUpperStem(TrackerSongIO_StripExtension(filename));

    std::string pattern;
    if (!TrackerSongIO_ExtractRawString(text, "XFM_TRACKER_SONG_PATTERN", pattern))
    {
        pattern = text;
    }
    int rows = 0;
    const char *p = pattern.c_str();
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    while (*p >= '0' && *p <= '9')
    {
        rows = rows * 10 + (*p - '0');
        p++;
    }
    if (rows <= 0 || rows > TRACKER_USER_SONG_MAX_ROWS)
    {
        result.error = "Song row count is invalid";
        return result;
    }

    result.ok = true;
    std::string ignoredError;
    if (!stem.empty() && !TrackerSongIO_IsBuiltinStem(stem) && TrackerSongIO_IsValidUserStem(stem, &ignoredError))
        result.displayName = TrackerSongIO_ExtractDisplayName(text, stem);
    else
        result.displayName = TrackerSongIO_ExtractDisplayName(text, "LOADED_SONG");
    result.pattern = pattern;
    return result;
}

inline std::string TrackerSongIO_BuildFileText(
    const std::string &displayName,
    const std::string &pattern,
    const std::string &customInstrumentsText,
    int tickRate = 60,
    int speed = 6,
    int rowsPerBeat = 4,
    bool lfoEnabled = false,
    int lfoFrequency = 0
)
{
    std::string out;
    out.reserve(pattern.size() + customInstrumentsText.size() + 512);
    out += "#pragma once\n";
    out += "#include \"sounds/songs_data.h\"\n\n";
    out += "// XFM tracker song file. This is valid C++ and can be pasted into built-in songs later.\n";
    out += "static constexpr const char *XFM_TRACKER_SONG_NAME = R\"xfmname(";
    out += displayName;
    out += ")xfmname\";\n\n";
    out += "static constexpr int XFM_TRACKER_TICK_RATE = ";
    out += std::to_string(tickRate);
    out += ";\n";
    out += "static constexpr int XFM_TRACKER_SPEED = ";
    out += std::to_string(speed);
    out += ";\n";
    out += "static constexpr int XFM_TRACKER_ROWS_PER_BEAT = ";
    out += std::to_string(rowsPerBeat);
    out += ";\n";
    out += "static constexpr int XFM_TRACKER_LFO_ENABLED = ";
    out += lfoEnabled ? "1" : "0";
    out += ";\n";
    out += "static constexpr int XFM_TRACKER_LFO_FREQUENCY = ";
    out += std::to_string(lfoFrequency);
    out += ";\n\n";
    out += "static constexpr const char *XFM_TRACKER_SONG_PATTERN = R\"xfmsong(";
    out += pattern;
    if (!pattern.empty() && pattern.back() != '\n') out += '\n';
    out += ")xfmsong\";\n\n";
    out += "static constexpr const char *XFM_TRACKER_CUSTOM_INSTRUMENTS = R\"xfmins(";
    out += customInstrumentsText;
    if (!customInstrumentsText.empty() && customInstrumentsText.back() != '\n') out += '\n';
    out += ")xfmins\";\n";
    return out;
}
