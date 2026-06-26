#pragma once

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

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
    int songTickRate = 60;
    int songSpeed = 6;
    int songRowsPerBeat = 4;
    int songScaleRoot = 0;
    int songScaleMode = 0;
    bool songLfoEnabled = false;
    int songLfoFrequency = 0;
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
    return TrackerSongIO_DisplayToStem(displayName) + ".h";
}

inline bool TrackerSongIO_ExtractRawString(const std::string &text, const char *symbol, std::string &out)
{
    size_t sym = text.find(symbol);
    if (sym == std::string::npos)
    {
        if (std::strcmp(symbol, "XFM_TRACKER_SONG_NAME") == 0)
        {
            sym = text.find("XFM_SONG_BEGIN");
            if (sym == std::string::npos)
                sym = text.find("XFM_SONG_NAME");
        }
        else if (std::strcmp(symbol, "XFM_TRACKER_SONG_PATTERN") == 0) sym = text.find("XFM_PATTERN");
        else if (std::strcmp(symbol, "XFM_TRACKER_CUSTOM_INSTRUMENTS") == 0) sym = text.find("XFM_INSTRUMENTS");
        else return false;
        if (sym == std::string::npos)
            return false;
    }
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

inline bool TrackerSongIO_ContainsSymbol(const std::string &text, const char *symbol)
{
    if (!symbol) return false;
    if (text.find(symbol) != std::string::npos) return true;
    if (std::strcmp(symbol, "XFM_TRACKER_SONG_NAME") == 0)
        return text.find("XFM_SONG_NAME") != std::string::npos || text.find("XFM_SONG_BEGIN") != std::string::npos;
    if (std::strcmp(symbol, "XFM_TRACKER_SONG_PATTERN") == 0) return text.find("XFM_PATTERN") != std::string::npos;
    if (std::strcmp(symbol, "XFM_TRACKER_CUSTOM_INSTRUMENTS") == 0) return text.find("XFM_INSTRUMENTS") != std::string::npos;
    return false;
}

inline std::string TrackerSongIO_JoinMessages(const std::vector<std::string> &messages)
{
    std::string out;
    for (size_t i = 0; i < messages.size(); i++)
    {
        if (i > 0) out += "\n";
        out += messages[i];
    }
    return out;
}

inline int TrackerSongIO_CountMessages(const std::string &messages)
{
    int count = 0;
    bool inMessage = false;
    for (char c : messages)
    {
        if (c == '\n' || c == '\r')
        {
            if (inMessage)
            {
                count++;
                inMessage = false;
            }
        }
        else if (c != ' ' && c != '\t')
            inMessage = true;
    }
    if (inMessage)
        count++;
    return count;
}

inline std::string TrackerSongIO_LoadErrorSummary(const std::string &messages)
{
    int count = TrackerSongIO_CountMessages(messages);
    if (count <= 0)
        return "LOAD FAILED: invalid tracker file";
    if (count == 1)
        return "LOAD FAILED: 1 parser error";
    return "LOAD FAILED: " + std::to_string(count) + " parser errors";
}

inline bool TrackerSongIO_IsBlankLine(const char *begin, const char *end)
{
    while (begin < end)
    {
        if (*begin != ' ' && *begin != '\t' && *begin != '\r')
            return false;
        begin++;
    }
    return true;
}

inline std::string TrackerSongIO_Trim(std::string s)
{
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\r' || s.front() == '\n'))
        s.erase(s.begin());
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r' || s.back() == '\n'))
        s.pop_back();
    return s;
}

inline bool TrackerSongIO_StartsWith(const std::string &s, const char *prefix)
{
    size_t len = std::strlen(prefix);
    return s.size() >= len && s.compare(0, len, prefix) == 0;
}

inline std::string TrackerSongIO_EscapeString(const std::string &s)
{
    std::string out;
    out.reserve(s.size());
    for (char c : s)
    {
        if (c == '\\' || c == '"') out.push_back('\\');
        out.push_back(c);
    }
    return out;
}

inline std::string TrackerSongIO_ExtractQuotedArg(const std::string &line)
{
    size_t q = line.find('"');
    if (q == std::string::npos) return "";
    std::string out;
    bool escaped = false;
    for (size_t i = q + 1; i < line.size(); i++)
    {
        char c = line[i];
        if (escaped)
        {
            out.push_back(c);
            escaped = false;
            continue;
        }
        if (c == '\\')
        {
            escaped = true;
            continue;
        }
        if (c == '"') break;
        out.push_back(c);
    }
    return out;
}

inline int TrackerSongIO_ParseIntToken(std::string token, int fallback = 0)
{
    token = TrackerSongIO_Trim(token);
    if (!token.empty() && token.back() == ')') token.pop_back();
    if (!token.empty() && token.back() == ',') token.pop_back();
    char *end = nullptr;
    long value = std::strtol(token.c_str(), &end, 0);
    return end == token.c_str() ? fallback : (int)value;
}

inline bool TrackerSongIO_ParseIntStrict(std::string token, int &out)
{
    token = TrackerSongIO_Trim(token);
    if (!token.empty() && token.back() == ')') token.pop_back();
    if (!token.empty() && token.back() == ',') token.pop_back();
    token = TrackerSongIO_Trim(token);
    if (token.empty()) return false;
    char *end = nullptr;
    long value = std::strtol(token.c_str(), &end, 0);
    if (end == token.c_str()) return false;
    while (*end == ' ' || *end == '\t') end++;
    if (*end != '\0') return false;
    out = (int)value;
    return true;
}

inline bool TrackerSongIO_IsNameBoundary(char c)
{
    return !(std::isalnum((unsigned char)c) || c == '_');
}

inline const char *TrackerSongIO_FindNamedArgValue(const std::string &line, const char *name)
{
    size_t nameLen = std::strlen(name);
    size_t pos = 0;
    while ((pos = line.find(name, pos)) != std::string::npos)
    {
        bool beforeOk = pos == 0 || TrackerSongIO_IsNameBoundary(line[pos - 1]);
        size_t after = pos + nameLen;
        bool afterOk = after >= line.size() || TrackerSongIO_IsNameBoundary(line[after]) || line[after] == ' ' || line[after] == '\t';
        if (beforeOk && afterOk)
        {
            while (after < line.size() && (line[after] == ' ' || line[after] == '\t')) after++;
            if (after < line.size() && line[after] == '=')
            {
                after++;
                while (after < line.size() && (line[after] == ' ' || line[after] == '\t')) after++;
                return line.c_str() + after;
            }
        }
        pos += nameLen;
    }
    return nullptr;
}

inline bool TrackerSongIO_NamedIntArgStrict(const std::string &line, const char *name, int &out)
{
    const char *p = TrackerSongIO_FindNamedArgValue(line, name);
    if (!p) return false;
    char *end = nullptr;
    long value = std::strtol(p, &end, 0);
    if (end == p) return false;
    out = (int)value;
    return true;
}

inline int TrackerSongIO_NamedIntArg(const std::string &line, const char *name, int fallback)
{
    int value = fallback;
    return TrackerSongIO_NamedIntArgStrict(line, name, value) ? value : fallback;
}

inline std::string TrackerSongIO_FirstArg(const std::string &line)
{
    size_t open = line.find('(');
    if (open == std::string::npos) return "";
    size_t comma = line.find(',', open + 1);
    size_t close = line.find(')', open + 1);
    size_t end = comma == std::string::npos ? close : comma;
    if (end == std::string::npos) end = line.size();
    return TrackerSongIO_Trim(line.substr(open + 1, end - open - 1));
}

inline const char *TrackerSongIO_MacroTargetName(int target)
{
    static constexpr const char *names[] = {
        "NONE", "TL1", "TL2", "TL3", "TL4", "MUL1", "MUL2", "MUL3", "MUL4",
        "DT1", "DT2", "DT3", "DT4", "FB", "ARP", "AR1", "AR2", "AR3", "AR4",
        "DR1", "DR2", "DR3", "DR4", "SR1", "SR2", "SR3", "SR4", "SL1", "SL2",
        "SL3", "SL4", "RR1", "RR2", "RR3", "RR4", "SSG1", "SSG2", "SSG3", "SSG4"
    };
    return target >= 0 && target < (int)(sizeof(names) / sizeof(names[0])) ? names[target] : "NONE";
}

inline int TrackerSongIO_MacroTargetFromArg(std::string arg)
{
    arg = TrackerSongIO_Trim(arg);
    if (!arg.empty() && arg[0] >= '0' && arg[0] <= '9')
        return TrackerSongIO_ParseIntToken(arg, 0);
    for (char &c : arg) c = (char)std::toupper((unsigned char)c);
    for (int i = 0; i < 39; i++)
        if (arg == TrackerSongIO_MacroTargetName(i))
            return i;
    return 0;
}

inline bool TrackerSongIO_MacroTargetFromArgStrict(std::string arg, int &out)
{
    arg = TrackerSongIO_Trim(arg);
    if (arg.empty()) return false;
    if (arg[0] >= '0' && arg[0] <= '9')
    {
        if (!TrackerSongIO_ParseIntStrict(arg, out)) return false;
        return out > 0 && out < 39;
    }
    for (char &c : arg) c = (char)std::toupper((unsigned char)c);
    for (int i = 1; i < 39; i++)
    {
        if (arg == TrackerSongIO_MacroTargetName(i))
        {
            out = i;
            return true;
        }
    }
    return false;
}

inline int TrackerSongIO_CountMacroValues(const std::string &values)
{
    std::istringstream in(values);
    int count = 0;
    int ignored = 0;
    while (in >> ignored)
        count++;
    return count;
}

inline std::string TrackerSongIO_LegacyInstrumentsToDsl(const std::string &legacy)
{
    if (legacy.empty()) return "";
    std::istringstream in(legacy);
    std::string out;
    std::string tag;
    bool openInstrument = false;
    while (in >> tag)
    {
        if (tag == "INST")
        {
            if (openInstrument)
                out += "XFM_END_INSTRUMENT()\n\n";
            std::string hex;
            in >> hex;
            int inst = (int)std::strtol(hex.c_str(), nullptr, 16);
            char line[96];
            std::snprintf(line, sizeof(line), "XFM_INSTRUMENT(0x%02X)\n", std::max(0, std::min(255, inst)));
            out += line;
            openInstrument = true;
        }
        else if (tag == "PATCH")
        {
            int alg, fb, ams, fms;
            in >> alg >> fb >> ams >> fms;
            char line[128];
            std::snprintf(line, sizeof(line), "XFM_PATCH(ALG = %d, FB = %d, AMS = %d, FMS = %d)\n", alg, fb, ams, fms);
            out += line;
        }
        else if (tag == "NAME")
        {
            std::string name;
            std::getline(in, name);
            name = TrackerSongIO_Trim(name);
            out += "XFM_INSTRUMENT_NAME(\"";
            out += TrackerSongIO_EscapeString(name);
            out += "\")\n";
        }
        else if (tag == "COLOR")
        {
            std::string hex;
            in >> hex;
            unsigned int rgb = (unsigned int)std::strtoul(hex.c_str(), nullptr, 16);
            char line[96];
            std::snprintf(line, sizeof(line), "XFM_INSTRUMENT_COLOR(0x%06X)\n", rgb & 0xFFFFFFu);
            out += line;
        }
        else if (tag == "OP")
        {
            int op, dt, mul, tl, rs, ar, am, dr, sr, sl, rr, ssg;
            in >> op >> dt >> mul >> tl >> rs >> ar >> am >> dr >> sr >> sl >> rr >> ssg;
            char line[256];
            std::snprintf(
                line,
                sizeof(line),
                "XFM_OP(%d, DT = %d, MUL = %d, TL = %d, RS = %d, AR = %d, AM = %d, DR = %d, SR = %d, SL = %d, RR = %d, SSG = %d)\n",
                op, dt, mul, tl, rs, ar, am, dr, sr, sl, rr, ssg
            );
            out += line;
        }
        else if (tag == "MACRO")
        {
            int target, length, loopStart, releaseStart;
            in >> target >> length >> loopStart >> releaseStart;
            std::string values;
            for (int i = 0; i < length; i++)
            {
                int v = 0;
                in >> v;
                if (i > 0) values += ' ';
                values += std::to_string(v);
            }
            char line[256];
            std::snprintf(
                line,
                sizeof(line),
                "XFM_TRACKER_MACRO(%s, LENGTH = %d, LOOP = %d, RELEASE = %d, VALUES = \"",
                TrackerSongIO_MacroTargetName(target),
                length,
                loopStart,
                releaseStart
            );
            out += line;
            out += TrackerSongIO_EscapeString(values);
            out += "\")\n";
        }
        else if (tag == "ENDINST")
        {
            if (openInstrument)
            {
                out += "XFM_END_INSTRUMENT()\n\n";
                openInstrument = false;
            }
        }
    }
    if (openInstrument)
        out += "XFM_END_INSTRUMENT()\n";
    return out;
}

inline std::string TrackerSongIO_DslInstrumentsToLegacy(const std::string &text)
{
    std::istringstream lines(text);
    std::string out;
    std::string line;
    int currentInst = -1;
    while (std::getline(lines, line))
    {
        line = TrackerSongIO_Trim(line);
        if (line.empty() || TrackerSongIO_StartsWith(line, "//"))
            continue;
        char buf[512];
        if (TrackerSongIO_StartsWith(line, "XFM_INSTRUMENT("))
        {
            currentInst = std::max(0, std::min(255, TrackerSongIO_ParseIntToken(TrackerSongIO_FirstArg(line), 0)));
            std::snprintf(buf, sizeof(buf), "INST %02X\n", currentInst);
            out += buf;
        }
        else if (TrackerSongIO_StartsWith(line, "XFM_INSTRUMENT_NAME(") && currentInst >= 0)
        {
            out += "NAME ";
            out += TrackerSongIO_ExtractQuotedArg(line);
            out += '\n';
        }
        else if (TrackerSongIO_StartsWith(line, "XFM_INSTRUMENT_COLOR(") && currentInst >= 0)
        {
            int rgb = TrackerSongIO_ParseIntToken(TrackerSongIO_FirstArg(line), 0);
            std::snprintf(buf, sizeof(buf), "COLOR %06X\n", (unsigned int)(rgb & 0xFFFFFF));
            out += buf;
        }
        else if (TrackerSongIO_StartsWith(line, "XFM_PATCH(") && currentInst >= 0)
        {
            std::snprintf(
                buf,
                sizeof(buf),
                "PATCH %d %d %d %d\n",
                TrackerSongIO_NamedIntArg(line, "ALG", 0),
                TrackerSongIO_NamedIntArg(line, "FB", 0),
                TrackerSongIO_NamedIntArg(line, "AMS", 0),
                TrackerSongIO_NamedIntArg(line, "FMS", 0)
            );
            out += buf;
        }
        else if (TrackerSongIO_StartsWith(line, "XFM_OP(") && currentInst >= 0)
        {
            std::snprintf(
                buf,
                sizeof(buf),
                "OP %d %d %d %d %d %d %d %d %d %d %d %d\n",
                TrackerSongIO_ParseIntToken(TrackerSongIO_FirstArg(line), 1),
                TrackerSongIO_NamedIntArg(line, "DT", 0),
                TrackerSongIO_NamedIntArg(line, "MUL", 0),
                TrackerSongIO_NamedIntArg(line, "TL", 0),
                TrackerSongIO_NamedIntArg(line, "RS", 0),
                TrackerSongIO_NamedIntArg(line, "AR", 0),
                TrackerSongIO_NamedIntArg(line, "AM", 0),
                TrackerSongIO_NamedIntArg(line, "DR", 0),
                TrackerSongIO_NamedIntArg(line, "SR", 0),
                TrackerSongIO_NamedIntArg(line, "SL", 0),
                TrackerSongIO_NamedIntArg(line, "RR", 0),
                TrackerSongIO_NamedIntArg(line, "SSG", 0)
            );
            out += buf;
        }
        else if (TrackerSongIO_StartsWith(line, "XFM_TRACKER_MACRO(") && currentInst >= 0)
        {
            int target = TrackerSongIO_MacroTargetFromArg(TrackerSongIO_FirstArg(line));
            int length = TrackerSongIO_NamedIntArg(line, "LENGTH", 0);
            int loopStart = TrackerSongIO_NamedIntArg(line, "LOOP", 255);
            int releaseStart = TrackerSongIO_NamedIntArg(line, "RELEASE", 255);
            std::string values = TrackerSongIO_ExtractQuotedArg(line);
            std::snprintf(buf, sizeof(buf), "MACRO %d %d %d %d", target, length, loopStart, releaseStart);
            out += buf;
            if (!values.empty())
            {
                std::istringstream valueStream(values);
                int v = 0;
                while (valueStream >> v)
                {
                    std::snprintf(buf, sizeof(buf), " %d", v);
                    out += buf;
                }
            }
            out += '\n';
        }
        else if (TrackerSongIO_StartsWith(line, "XFM_END_INSTRUMENT(") && currentInst >= 0)
        {
            out += "ENDINST\n";
            currentInst = -1;
        }
    }
    if (currentInst >= 0)
        out += "ENDINST\n";
    return out;
}

inline bool TrackerSongIO_ExtractInstrumentText(const std::string &text, std::string &out)
{
    if (TrackerSongIO_ExtractRawString(text, "XFM_TRACKER_CUSTOM_INSTRUMENTS", out))
        return true;
    if (!TrackerSongIO_ContainsSymbol(text, "XFM_INSTRUMENT"))
        return false;
    out = TrackerSongIO_DslInstrumentsToLegacy(text);
    return !out.empty();
}

inline void TrackerSongIO_AddLineMessage(std::vector<std::string> &messages, int lineNumber, const char *message)
{
    char buf[192];
    std::snprintf(buf, sizeof(buf), "line %d: %s", lineNumber, message);
    messages.emplace_back(buf);
}

inline std::vector<std::string> TrackerSongIO_ValidateInstrumentDsl(const std::string &text)
{
    std::vector<std::string> messages;
    if (!TrackerSongIO_ContainsSymbol(text, "XFM_INSTRUMENT") &&
        !TrackerSongIO_ContainsSymbol(text, "XFM_PATCH") &&
        !TrackerSongIO_ContainsSymbol(text, "XFM_OP") &&
        !TrackerSongIO_ContainsSymbol(text, "XFM_TRACKER_MACRO"))
        return messages;

    std::istringstream lines(text);
    std::string line;
    bool inInstrument = false;
    int instrumentStartLine = 0;
    int lineNumber = 0;
    while (std::getline(lines, line))
    {
        lineNumber++;
        line = TrackerSongIO_Trim(line);
        if (line.empty() || TrackerSongIO_StartsWith(line, "//"))
            continue;

        if (TrackerSongIO_StartsWith(line, "XFM_INSTRUMENT_NAME(") ||
            TrackerSongIO_StartsWith(line, "XFM_INSTRUMENT_COLOR("))
        {
            if (!inInstrument)
                TrackerSongIO_AddLineMessage(messages, lineNumber, "instrument attribute appears outside XFM_INSTRUMENT block");
            continue;
        }

        if (TrackerSongIO_StartsWith(line, "XFM_INSTRUMENT("))
        {
            int inst = 0;
            std::string firstArg = TrackerSongIO_FirstArg(line);
            bool validInstrument = !firstArg.empty() && TrackerSongIO_ParseIntStrict(firstArg, inst) && inst >= 0 && inst <= 255;
            if (!validInstrument)
                TrackerSongIO_AddLineMessage(messages, lineNumber, "XFM_INSTRUMENT id is invalid; expected 0x00..0xFF");
            if (inInstrument)
                TrackerSongIO_AddLineMessage(messages, lineNumber, "XFM_INSTRUMENT starts before previous instrument was closed");
            inInstrument = validInstrument;
            if (inInstrument)
                instrumentStartLine = lineNumber;
            continue;
        }

        if (TrackerSongIO_StartsWith(line, "XFM_PATCH("))
        {
            if (!inInstrument)
            {
                TrackerSongIO_AddLineMessage(messages, lineNumber, "XFM_PATCH appears outside XFM_INSTRUMENT block");
                continue;
            }
            static constexpr const char *required[] = {"ALG", "FB", "AMS", "FMS"};
            for (const char *name : required)
            {
                int ignored = 0;
                if (!TrackerSongIO_NamedIntArgStrict(line, name, ignored))
                {
                    char buf[128];
                    std::snprintf(buf, sizeof(buf), "XFM_PATCH missing required field %s", name);
                    TrackerSongIO_AddLineMessage(messages, lineNumber, buf);
                }
            }
            continue;
        }

        if (TrackerSongIO_StartsWith(line, "XFM_OP("))
        {
            if (!inInstrument)
            {
                TrackerSongIO_AddLineMessage(messages, lineNumber, "XFM_OP appears outside XFM_INSTRUMENT block");
                continue;
            }
            int op = 0;
            if (!TrackerSongIO_ParseIntStrict(TrackerSongIO_FirstArg(line), op) || op < 1 || op > 4)
                TrackerSongIO_AddLineMessage(messages, lineNumber, "XFM_OP operator index is invalid; expected 1..4");
            continue;
        }

        if (TrackerSongIO_StartsWith(line, "XFM_TRACKER_MACRO("))
        {
            if (!inInstrument)
            {
                TrackerSongIO_AddLineMessage(messages, lineNumber, "XFM_TRACKER_MACRO appears outside XFM_INSTRUMENT block");
                continue;
            }
            int target = 0;
            if (!TrackerSongIO_MacroTargetFromArgStrict(TrackerSongIO_FirstArg(line), target))
                TrackerSongIO_AddLineMessage(messages, lineNumber, "XFM_TRACKER_MACRO target is unknown");
            int length = 0;
            if (!TrackerSongIO_NamedIntArgStrict(line, "LENGTH", length) || length < 1 || length > 64)
                TrackerSongIO_AddLineMessage(messages, lineNumber, "XFM_TRACKER_MACRO LENGTH is invalid; expected 1..64");
            std::string values = TrackerSongIO_ExtractQuotedArg(line);
            int valueCount = TrackerSongIO_CountMacroValues(values);
            if (length > 0 && valueCount != length)
            {
                char buf[144];
                std::snprintf(buf, sizeof(buf), "XFM_TRACKER_MACRO LENGTH says %d but VALUES contains %d values", length, valueCount);
                TrackerSongIO_AddLineMessage(messages, lineNumber, buf);
            }
            continue;
        }

        if (TrackerSongIO_StartsWith(line, "XFM_END_INSTRUMENT("))
        {
            if (!inInstrument)
                TrackerSongIO_AddLineMessage(messages, lineNumber, "XFM_END_INSTRUMENT appears without an open instrument");
            inInstrument = false;
            instrumentStartLine = 0;
            continue;
        }
    }

    if (inInstrument)
    {
        char buf[144];
        std::snprintf(buf, sizeof(buf), "XFM_INSTRUMENT block opened on line %d is missing XFM_END_INSTRUMENT()", instrumentStartLine);
        messages.emplace_back(buf);
    }
    return messages;
}

inline bool TrackerSongIO_IsPartMarkerLine(const char *begin, const char *end)
{
    return end - begin >= 5 && begin[4] == ' ' &&
        ((begin[0] == 'P' && begin[1] == 'A' && begin[2] == 'R' && begin[3] == 'T') ||
         (begin[0] == 'S' && begin[1] == 'K' && begin[2] == 'I' && begin[3] == 'P'));
}

inline int TrackerSongIO_CountChannels(const char *begin, const char *end)
{
    if (begin >= end) return 0;
    int channels = 1;
    for (const char *p = begin; p < end; p++)
        if (*p == '|')
            channels++;
    return channels;
}

inline bool TrackerSongIO_ReadLeadingRowCount(const std::string &pattern, int &rows, size_t &bodyOffset)
{
    rows = 0;
    const char *base = pattern.c_str();
    const char *p = base;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    const char *digits = p;
    while (*p >= '0' && *p <= '9')
    {
        rows = rows * 10 + (*p - '0');
        p++;
    }
    if (p == digits)
        return false;
    while (*p == ' ' || *p == '\t' || *p == '\r') p++;
    if (*p == '\n') p++;
    bodyOffset = (size_t)(p - base);
    return true;
}

inline std::vector<std::string> TrackerSongIO_ValidatePattern(const std::string &pattern, int declaredRows)
{
    std::vector<std::string> messages;
    size_t bodyOffset = 0;
    int ignoredRows = 0;
    (void)TrackerSongIO_ReadLeadingRowCount(pattern, ignoredRows, bodyOffset);
    const char *base = pattern.c_str();
    const char *p = base + std::min(bodyOffset, pattern.size());
    int trackerRows = 0;
    int lineNumber = 1;
    for (const char *scan = base; scan < p && *scan; scan++)
        if (*scan == '\n')
            lineNumber++;

    while (*p)
    {
        const char *lineStart = p;
        const char *lineEnd = p;
        while (*lineEnd && *lineEnd != '\n' && *lineEnd != '\r') lineEnd++;
        if (TrackerSongIO_IsBlankLine(lineStart, lineEnd))
        {
            // Readable song headers may use blank separators; they do not count as tracker rows.
        }
        else if (TrackerSongIO_IsPartMarkerLine(lineStart, lineEnd))
        {
            const char *name = lineStart + 5;
            if (TrackerSongIO_IsBlankLine(name, lineEnd))
            {
                char buf[96];
                std::snprintf(buf, sizeof(buf), "line %d: PART/SKIP name is empty", lineNumber);
                messages.emplace_back(buf);
            }
        }
        else
        {
            trackerRows++;
            int channels = TrackerSongIO_CountChannels(lineStart, lineEnd);
            if (channels > 6)
            {
                char buf[112];
                std::snprintf(buf, sizeof(buf), "line %d: row has %d channels, maximum is 6", lineNumber, channels);
                messages.emplace_back(buf);
            }
            if (lineEnd - lineStart < 7)
            {
                char buf[112];
                std::snprintf(buf, sizeof(buf), "line %d: row is too short for a tracker cell", lineNumber);
                messages.emplace_back(buf);
            }
        }
        p = lineEnd;
        while (*p == '\r') p++;
        if (*p == '\n') p++;
        lineNumber++;
    }

    if (trackerRows != declaredRows)
    {
        char buf[128];
        std::snprintf(buf, sizeof(buf), "row count mismatch: header says %d, file contains %d tracker rows", declaredRows, trackerRows);
        messages.emplace_back(buf);
    }
    return messages;
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
    const char *p = nullptr;
    if (sym != std::string::npos)
    {
        size_t eq = text.find('=', sym);
        if (eq == std::string::npos) return false;
        p = text.c_str() + eq + 1;
    }
    else
    {
        const char *alias = nullptr;
        if (std::strcmp(symbol, "XFM_TRACKER_TICK_RATE") == 0) alias = "XFM_TICK_RATE";
        else if (std::strcmp(symbol, "XFM_TRACKER_SPEED") == 0) alias = "XFM_SPEED";
        else if (std::strcmp(symbol, "XFM_TRACKER_ROWS_PER_BEAT") == 0) alias = "XFM_ROWS_PER_BEAT";
        else if (std::strcmp(symbol, "XFM_TRACKER_SCALE_ROOT") == 0) alias = "XFM_SCALE_ROOT";
        else if (std::strcmp(symbol, "XFM_TRACKER_SCALE_MODE") == 0) alias = "XFM_SCALE_MODE";
        else if (std::strcmp(symbol, "XFM_TRACKER_LFO_ENABLED") == 0) alias = "XFM_LFO_ENABLED";
        else if (std::strcmp(symbol, "XFM_TRACKER_LFO_FREQUENCY") == 0) alias = "XFM_LFO_FREQUENCY";
        if (!alias)
            return false;
        sym = text.find(alias);
        if (sym == std::string::npos)
            return false;
        size_t open = text.find('(', sym);
        if (open == std::string::npos)
            return false;
        p = text.c_str() + open + 1;
    }
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
    std::vector<std::string> instrumentMessages = TrackerSongIO_ValidateInstrumentDsl(text);

    std::string pattern;
    if (!TrackerSongIO_ExtractRawString(text, "XFM_TRACKER_SONG_PATTERN", pattern))
    {
        if (TrackerSongIO_ContainsSymbol(text, "XFM_TRACKER_SONG_PATTERN"))
        {
            std::vector<std::string> messages;
            messages.emplace_back("XFM_TRACKER_SONG_PATTERN raw string is malformed");
            messages.insert(messages.end(), instrumentMessages.begin(), instrumentMessages.end());
            result.error = TrackerSongIO_JoinMessages(messages);
            return result;
        }
        pattern = text;
    }
    int rows = 0;
    size_t bodyOffset = 0;
    bool hasRowCount = TrackerSongIO_ReadLeadingRowCount(pattern, rows, bodyOffset);
    (void)bodyOffset;
    if (!hasRowCount)
    {
        std::vector<std::string> messages;
        messages.emplace_back("missing tracker row count at start of pattern");
        messages.insert(messages.end(), instrumentMessages.begin(), instrumentMessages.end());
        result.error = TrackerSongIO_JoinMessages(messages);
        return result;
    }
    if (rows <= 0 || rows > TRACKER_USER_SONG_MAX_ROWS)
    {
        char buf[128];
        std::snprintf(buf, sizeof(buf), "song row count %d is invalid; expected 1..%d", rows, TRACKER_USER_SONG_MAX_ROWS);
        std::vector<std::string> messages;
        messages.emplace_back(buf);
        messages.insert(messages.end(), instrumentMessages.begin(), instrumentMessages.end());
        result.error = TrackerSongIO_JoinMessages(messages);
        return result;
    }
    std::vector<std::string> messages = TrackerSongIO_ValidatePattern(pattern, rows);
    messages.insert(messages.end(), instrumentMessages.begin(), instrumentMessages.end());
    if (!messages.empty())
    {
        result.error = TrackerSongIO_JoinMessages(messages);
        return result;
    }

    result.ok = true;
    std::string ignoredError;
    if (!stem.empty() && !TrackerSongIO_IsBuiltinStem(stem) && TrackerSongIO_IsValidUserStem(stem, &ignoredError))
        result.displayName = TrackerSongIO_ExtractDisplayName(text, stem);
    else
        result.displayName = TrackerSongIO_ExtractDisplayName(text, "LOADED_SONG");
    result.pattern = pattern;
    int setting = 0;
    if (TrackerSongIO_ExtractInt(text, "XFM_TRACKER_TICK_RATE", setting))
        result.songTickRate = std::max(1, std::min(300, setting));
    if (TrackerSongIO_ExtractInt(text, "XFM_TRACKER_SPEED", setting))
        result.songSpeed = std::max(1, std::min(32, setting));
    if (TrackerSongIO_ExtractInt(text, "XFM_TRACKER_ROWS_PER_BEAT", setting))
        result.songRowsPerBeat = std::max(1, std::min(32, setting));
    if (TrackerSongIO_ExtractInt(text, "XFM_TRACKER_SCALE_ROOT", setting))
        result.songScaleRoot = ((setting % 12) + 12) % 12;
    if (TrackerSongIO_ExtractInt(text, "XFM_TRACKER_SCALE_MODE", setting))
        result.songScaleMode = std::max(0, setting);
    if (TrackerSongIO_ExtractInt(text, "XFM_TRACKER_LFO_ENABLED", setting))
        result.songLfoEnabled = setting != 0;
    if (TrackerSongIO_ExtractInt(text, "XFM_TRACKER_LFO_FREQUENCY", setting))
        result.songLfoFrequency = std::max(0, std::min(7, setting));
    return result;
}

inline std::string TrackerSongIO_BuildFileText(
    const std::string &displayName,
    const std::string &pattern,
    const std::string &customInstrumentsText,
    int tickRate = 60,
    int speed = 6,
    int rowsPerBeat = 4,
    int scaleRoot = 0,
    int scaleMode = 0,
    bool lfoEnabled = false,
    int lfoFrequency = 0
)
{
    std::string out;
    out.reserve(pattern.size() + customInstrumentsText.size() + 512);
    out += "#pragma once\n";
    out += "#include \"tracker/xfm_song_dsl.h\"\n\n";
    out += "// XFM tracker song file. This is valid C++ and can be pasted into built-in songs.\n";
    out += "XFM_SONG_BEGIN(R\"xfmname(";
    out += displayName;
    out += ")xfmname\")\n";
    out += "XFM_TICK_RATE(";
    out += std::to_string(tickRate);
    out += ")\n";
    out += "XFM_SPEED(";
    out += std::to_string(speed);
    out += ")\n";
    out += "XFM_ROWS_PER_BEAT(";
    out += std::to_string(rowsPerBeat);
    out += ")\n";
    out += "XFM_SCALE_ROOT(";
    out += std::to_string(scaleRoot);
    out += ")\n";
    out += "XFM_SCALE_MODE(";
    out += std::to_string(scaleMode);
    out += ")\n";
    out += "XFM_LFO_ENABLED(";
    out += lfoEnabled ? "1" : "0";
    out += ")\n";
    out += "XFM_LFO_FREQUENCY(";
    out += std::to_string(lfoFrequency);
    out += ")\n\n";
    out += "XFM_PATTERN(R\"xfmpattern(";
    out += pattern;
    if (!pattern.empty() && pattern.back() != '\n') out += '\n';
    out += ")xfmpattern\")\n\n";
    std::string instrumentDsl = TrackerSongIO_LegacyInstrumentsToDsl(customInstrumentsText);
    if (!instrumentDsl.empty())
    {
        out += instrumentDsl;
        if (out.back() != '\n') out += '\n';
        out += '\n';
    }
    out += "XFM_SONG_END()\n";
    return out;
}
