#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include <SDL.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>

EM_JS(void, js_storage_set, (const char *key, const char *val), {
    let k = UTF8ToString(key);
    let v = UTF8ToString(val);

    let data = localStorage.getItem("YourApp.settings");
    let obj = data ? JSON.parse(data) : {};
    obj[k] = v;
    localStorage.setItem("YourApp.settings", JSON.stringify(obj));
});

EM_JS(int, js_storage_get, (const char *key, char *out, int maxLen), {
    let k = UTF8ToString(key);
    let data = localStorage.getItem("YourApp.settings");
    if (!data)
        return 0;

    let obj = JSON.parse(data);
    if (!(k in obj))
        return 0;

    let v = obj[k];
    let len = lengthBytesUTF8(v);
    if (len >= maxLen)
        len = maxLen - 1;

    stringToUTF8(v, out, maxLen);
    return len;
});

EM_JS(int, js_storage_get_len, (const char *key), {
    let k = UTF8ToString(key);
    let data = localStorage.getItem("YourApp.settings");
    if (!data)
        return 0;

    let obj = JSON.parse(data);
    if (!(k in obj))
        return 0;

    return lengthBytesUTF8(obj[k]);
});

EM_JS(void, js_storage_remove, (const char *key), {
    let k = UTF8ToString(key);
    let data = localStorage.getItem("YourApp.settings");
    if (!data)
        return;
    let obj = JSON.parse(data);
    delete obj[k];
    localStorage.setItem("YourApp.settings", JSON.stringify(obj));
});
#endif
#ifdef __EMSCRIPTEN__
void js_storage_set(const char *key, const char *val);
int js_storage_get(const char *key, char *out, int maxLen);
int js_storage_get_len(const char *key);
void js_storage_remove(const char *key);
#endif

struct Storage
{
    enum KEYS_ENUM
    {
        USERNAME,
        TOKEN,
        LAST_LEVEL,
        SCHOOL_DONE,
        GREETINGS_SEEN,
        LANGUAGE,
        BANK,
        GAMEPLAY_TIME,
        UNLOCKED_BALLS,
        UNLOCKED_HOUSES,
        UNLOCKED_BOTS,
        EQUIPPED_BALL,
        RUNES,
        SELECTED_SONG,
        CAMPAIGN_COMPLETED,
        CAMPAIGN_CLEAR_TIME,
        CAMPAIGN_LEVEL_ATTEMPTS,
        CAMPAIGN_POSTGAME_FREEPLAY,
        CROWD_CONTROL_BONUS_CLAIMS,
        CROWD_CONTROL_BALL_WON,
        CROWD_CONTROL_PRIZE_INDEX,
        CROWD_CONTROL_PRIZE_WON_MASK,
        CROWD_CONTROL_CAMPAIGN_RESULT,
        KEY_COUNT
    };

    static constexpr const char *keyNames[KEY_COUNT] = {
        "username",
        "token",
        "last_level",
        "school_done",
        "greetings_seen",
        "language",
        "bank",
        "gameplay_time",
        "unlocked_balls",
        "unlocked_houses",
        "unlocked_bots",
        "equipped_ball",
        "runes",
        "selected_song",
        "campaign_completed",
        "campaign_clear_time",
        "campaign_level_attempts",
        "campaign_postgame_freeplay",
        "crowd_control_bonus_claims",
        "crowd_control_ball_won",
        "crowd_control_prize_index",
        "crowd_control_prize_won_mask",
        "crowd_control_campaign_result"
    };

    static constexpr const char *defaultValues[KEY_COUNT] = {
        "guest", // USERNAME
        "",      // TOKEN
        "1",     // LAST_LEVEL
        "0",     // SCHOOL_DONE
        "0",     // GREETINGS_SEEN
        "en_us", // LANGUAGE
        "20",    // BANK
        "0",     // GAMEPLAY_TIME
        "0",     // UNLOCKED_BALLS
        "0",     // UNLOCKED_HOUSES
        "0",     // UNLOCKED_BOTS
        "0",     // EQUIPPED_BALL
        "0,0,0", // RUNES
        "1",     // SELECTED_SONG
        "0",     // CAMPAIGN_COMPLETED
        "0",     // CAMPAIGN_CLEAR_TIME
        "0,0,0,0,0,0,0,0,0,0,0,0,0", // CAMPAIGN_LEVEL_ATTEMPTS
        "0",     // CAMPAIGN_POSTGAME_FREEPLAY
        "0",     // CROWD_CONTROL_BONUS_CLAIMS
        "0",     // CROWD_CONTROL_BALL_WON
        "0",     // CROWD_CONTROL_PRIZE_INDEX
        "0",     // CROWD_CONTROL_PRIZE_WON_MASK
        "0"      // CROWD_CONTROL_CAMPAIGN_RESULT
    };
    char filePath[512];

    static inline int hexValue(char c)
    {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    }

    static inline std::string hexEncode(const char *val, size_t length)
    {
        static constexpr char digits[] = "0123456789ABCDEF";
        std::string out;
        out.reserve(length * 2);
        for (size_t i = 0; i < length; i++)
        {
            unsigned char c = (unsigned char)val[i];
            out.push_back(digits[c >> 4]);
            out.push_back(digits[c & 15]);
        }
        return out;
    }

    static inline bool hexDecode(const char *val, std::string &out)
    {
        out.clear();
        if (!val)
            return false;
        size_t len = strlen(val);
        if ((len & 1u) != 0)
            return false;
        out.reserve(len / 2);
        for (size_t i = 0; i < len; i += 2)
        {
            int hi = hexValue(val[i]);
            int lo = hexValue(val[i + 1]);
            if (hi < 0 || lo < 0)
            {
                out.clear();
                return false;
            }
            out.push_back((char)((hi << 4) | lo));
        }
        return true;
    }

    static inline std::string storageLineKey(const char *key)
    {
        return std::string(key ? key : "") + "=";
    }

    static inline bool lineMatchesKey(const std::string &line, const std::string &keyPrefix)
    {
        return line.size() >= keyPrefix.size() && line.compare(0, keyPrefix.size(), keyPrefix) == 0;
    }

    /**
     * Initialize storage
     */
    void storageInit(const char *company, const char *app)
    {
#ifdef __EMSCRIPTEN__
        filePath[0] = 0;
        // ensure defaults exist
        for (int i = 0; i < KEY_COUNT; ++i)
        {
            char tmp[256];
            if (getChar((KEYS_ENUM)i, tmp, sizeof(tmp)) == 0)
            {
                js_storage_set(keyNames[i], defaultValues[i]);
            }
        }
#else
        char *base = SDL_GetPrefPath(company, app);
        if (!base)
        {
            filePath[0] = 0;
            return;
        }
        snprintf(filePath, sizeof(filePath), "%ssettings.ini", base);
        SDL_free(base);
        fprintf(stdout, "Usinf settings file: %s\n", filePath);

        FILE *f = fopen(filePath, "r");
        if (!f)
        {
            f = fopen(filePath, "w");
            if (f)
            {
                for (int i = 0; i < KEY_COUNT; ++i)
                    fprintf(f, "%s=%s\n", keyNames[i], defaultValues[i]);
                fclose(f);
            }
        }
#endif
    }

    size_t setCharKey(const char *key, const char *val, size_t length)
    {
        if (!key || !key[0] || !val)
            return 0;
#ifdef __EMSCRIPTEN__
        std::string tmp(val, length);
        js_storage_set(key, tmp.c_str());
        return length;
#else
        if (!filePath[0])
            return 0;
        std::string existing;
        FILE *f = fopen(filePath, "rb");
        if (f)
        {
            fseek(f, 0, SEEK_END);
            long size = ftell(f);
            rewind(f);
            if (size > 0)
            {
                existing.resize((size_t)size);
                fread(existing.data(), 1, existing.size(), f);
            }
            fclose(f);
        }

        std::string keyPrefix = storageLineKey(key);
        std::string encoded = hexEncode(val, length);
        std::string out;
        out.reserve(existing.size() + keyPrefix.size() + encoded.size() + 2);
        bool replaced = false;
        size_t start = 0;
        while (start <= existing.size())
        {
            size_t end = existing.find('\n', start);
            bool hadNewline = end != std::string::npos;
            if (!hadNewline)
                end = existing.size();
            std::string line = existing.substr(start, end - start);
            if (!lineMatchesKey(line, keyPrefix))
            {
                if (!line.empty())
                {
                    out += line;
                    out.push_back('\n');
                }
            }
            else if (!replaced)
            {
                out += keyPrefix;
                out += encoded;
                out.push_back('\n');
                replaced = true;
            }
            start = end + 1;
            if (!hadNewline)
                break;
        }
        if (!replaced)
        {
            out += keyPrefix;
            out += encoded;
            out.push_back('\n');
        }

        f = fopen(filePath, "wb");
        if (!f)
            return 0;
        fwrite(out.data(), 1, out.size(), f);
        fclose(f);
        return length;
#endif
    }

    size_t getCharKey(const char *key, std::string &outVal)
    {
        outVal.clear();
        if (!key || !key[0])
            return 0;
#ifdef __EMSCRIPTEN__
        int len = js_storage_get_len(key);
        if (len <= 0)
            return 0;
        std::string tmp;
        tmp.resize((size_t)len + 1);
        int read = js_storage_get(key, tmp.data(), len + 1);
        if (read <= 0)
        {
            tmp.clear();
            return 0;
        }
        tmp.resize((size_t)read);
        outVal = tmp;
        return outVal.size();
#else
        if (!filePath[0])
            return 0;
        FILE *f = fopen(filePath, "rb");
        if (!f)
            return 0;
        std::string existing;
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        rewind(f);
        if (size > 0)
        {
            existing.resize((size_t)size);
            fread(existing.data(), 1, existing.size(), f);
        }
        fclose(f);

        std::string keyPrefix = storageLineKey(key);
        size_t start = 0;
        while (start <= existing.size())
        {
            size_t end = existing.find('\n', start);
            bool hadNewline = end != std::string::npos;
            if (!hadNewline)
                end = existing.size();
            std::string line = existing.substr(start, end - start);
            while (!line.empty() && line.back() == '\r')
                line.pop_back();
            if (lineMatchesKey(line, keyPrefix))
            {
                std::string decoded;
                if (!hexDecode(line.c_str() + keyPrefix.size(), decoded))
                    return 0;
                outVal = decoded;
                return outVal.size();
            }
            start = end + 1;
            if (!hadNewline)
                break;
        }
        return 0;
#endif
    }

    bool removeCharKey(const char *key)
    {
        if (!key || !key[0])
            return false;
#ifdef __EMSCRIPTEN__
        js_storage_remove(key);
        return true;
#else
        if (!filePath[0])
            return false;
        FILE *f = fopen(filePath, "rb");
        if (!f)
            return false;
        std::string existing;
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        rewind(f);
        if (size > 0)
        {
            existing.resize((size_t)size);
            fread(existing.data(), 1, existing.size(), f);
        }
        fclose(f);

        std::string keyPrefix = storageLineKey(key);
        std::string out;
        out.reserve(existing.size());
        bool removed = false;
        size_t start = 0;
        while (start <= existing.size())
        {
            size_t end = existing.find('\n', start);
            bool hadNewline = end != std::string::npos;
            if (!hadNewline)
                end = existing.size();
            std::string line = existing.substr(start, end - start);
            if (lineMatchesKey(line, keyPrefix))
            {
                removed = true;
            }
            else if (!line.empty())
            {
                out += line;
                out.push_back('\n');
            }
            start = end + 1;
            if (!hadNewline)
                break;
        }

        f = fopen(filePath, "wb");
        if (!f)
            return false;
        fwrite(out.data(), 1, out.size(), f);
        fclose(f);
        return removed;
#endif
    }

    /**
     * Set entry
     */
    size_t setChar(KEYS_ENUM key, const char *val, size_t length)
    {
#ifdef __EMSCRIPTEN__
        char tmp[512];
        memcpy(tmp, val, length);
        tmp[length] = 0;
        js_storage_set(keyNames[key], tmp);
        return length;
#else
        std::string existing;
        FILE *f = fopen(filePath, "rb");
        if (f)
        {
            fseek(f, 0, SEEK_END);
            long size = ftell(f);
            rewind(f);
            if (size > 0)
            {
                existing.resize((size_t)size);
                fread(existing.data(), 1, existing.size(), f);
            }
            fclose(f);
        }

        std::string keyPrefix = storageLineKey(keyNames[key]);
        std::string newFile;
        newFile.reserve(existing.size() + length + strlen(keyNames[key]) + 2);
        bool replaced = false;
        size_t start = 0;
        while (start <= existing.size())
        {
            size_t end = existing.find('\n', start);
            bool hadNewline = end != std::string::npos;
            if (!hadNewline)
                end = existing.size();
            std::string line = existing.substr(start, end - start);
            if (lineMatchesKey(line, keyPrefix))
            {
                newFile += keyPrefix;
                newFile.append(val, length);
                newFile.push_back('\n');
                replaced = true;
            }
            else if (!line.empty())
            {
                newFile += line;
                newFile.push_back('\n');
            }
            start = end + 1;
            if (!hadNewline)
                break;
        }

        if (!replaced)
        {
            newFile += keyPrefix;
            newFile.append(val, length);
            newFile.push_back('\n');
        }

        f = fopen(filePath, "wb");
        if (!f)
            return 0;

        fwrite(newFile.data(), 1, newFile.size(), f);
        fclose(f);
        return length;
#endif
    }

    /**
     * Get entry
     */
    size_t getChar(KEYS_ENUM key, char *outVal, size_t maxLen)
    {
#ifdef __EMSCRIPTEN__
        return js_storage_get(keyNames[key], outVal, (int)maxLen);
#else
        FILE *f = fopen(filePath, "r");
        if (!f)
            return 0;

        char line[512];

        while (fgets(line, sizeof(line), f))
        {
            if (strncmp(line, keyNames[key], strlen(keyNames[key])) == 0 &&
                line[strlen(keyNames[key])] == '=')
            {
                const char *val = line + strlen(keyNames[key]) + 1;
                size_t len = strlen(val);

                if (len && val[len - 1] == '\n')
                    len--;

                if (len > maxLen)
                    len = maxLen;

                memcpy(outVal, val, len);
                outVal[len] = 0;

                fclose(f);
                return len;
            }
        }

        fclose(f);
        return 0;
#endif
    }

    size_t reset()
    {
#ifdef __EMSCRIPTEN__
        EM_ASM({ localStorage.removeItem("YourApp.settings"); });

        // reapply defaults
        for (int i = 0; i < KEY_COUNT; ++i)
            js_storage_set(keyNames[i], defaultValues[i]);

        return KEY_COUNT;
#else
        FILE *f = fopen(filePath, "w");
        if (!f)
            return 0;

        for (int i = 0; i < KEY_COUNT; ++i)
            fprintf(f, "%s=%s\n", keyNames[i], defaultValues[i]);

        fclose(f);
        return KEY_COUNT;
#endif
    }
};
