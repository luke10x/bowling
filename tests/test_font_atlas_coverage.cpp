// Audit the real translation strings, font files, and atlas dimensions used by Clayton.
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "../storyline.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

static bool IsSharedSymbol(unsigned cp)
{
    return cp == 0x25C0 || cp == 0x25B6 || cp == 0x25BC ||
           cp == 0x25B2 || cp == 0x2713;
}

static unsigned NextUtf8(const char *&p)
{
    const unsigned char a = (unsigned char)*p++;
    if (a < 0x80) return a;
    const unsigned char b = (unsigned char)*p++;
    if (a < 0xE0) return ((a & 31) << 6) | (b & 63);
    const unsigned char c = (unsigned char)*p++;
    if (a < 0xF0) return ((a & 15) << 12) | ((b & 63) << 6) | (c & 63);
    const unsigned char d = (unsigned char)*p++;
    return ((a & 7) << 18) | ((b & 63) << 12) | ((c & 63) << 6) | (d & 63);
}

static bool CheckAtlas(const char *name, const char *path, const std::string &chars,
                       int side, bool allowSymbolFallback, float pixelHeight = 32.0f)
{
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::fprintf(stderr, "%s: cannot open %s\n", name, path);
        return false;
    }
    std::vector<unsigned char> fontBytes(std::istreambuf_iterator<char>{file}, {});
    stbtt_fontinfo font = {};
    if (!stbtt_InitFont(&font, fontBytes.data(),
                        stbtt_GetFontOffsetForIndex(fontBytes.data(), 0))) {
        std::fprintf(stderr, "%s: invalid font %s\n", name, path);
        return false;
    }

    std::vector<int> codepoints;
    const char *p = chars.c_str();
    while (*p) {
        const unsigned cp = NextUtf8(p);
        if (cp < 0x20) continue; // line breaks and other layout controls
        if (!stbtt_FindGlyphIndex(&font, cp)) {
            if (allowSymbolFallback && IsSharedSymbol(cp)) continue;
            std::fprintf(stderr, "%s: font lacks U+%04X\n", name, cp);
            return false;
        }
        codepoints.push_back((int)cp);
    }

    std::vector<unsigned char> atlas(side * side);
    std::vector<stbtt_packedchar> packed(codepoints.size());
    stbtt_pack_context context = {};
    if (!stbtt_PackBegin(&context, atlas.data(), side, side, 0, 1, nullptr)) {
        std::fprintf(stderr, "%s: cannot initialize atlas\n", name);
        return false;
    }
    stbtt_PackSetOversampling(&context, 1, 1);
    stbtt_PackSetSkipMissingCodepoints(&context, true);
    stbtt_pack_range range = {};
    range.font_size = pixelHeight;
    range.first_unicode_codepoint_in_range = -1;
    range.array_of_unicode_codepoints = codepoints.data();
    range.num_chars = (int)codepoints.size();
    range.chardata_for_range = packed.data();
    const bool fits = stbtt_PackFontRanges(&context, fontBytes.data(), 0, &range, 1);
    stbtt_PackEnd(&context);
    if (!fits) {
        std::fprintf(stderr, "%s: %zu glyphs do not fit %dx%d atlas\n",
                     name, codepoints.size(), side, side);
    } else {
        int packedPixels = 0;
        int inkPixels = 0;
        for (const stbtt_packedchar &glyph : packed)
            packedPixels += (glyph.x1 - glyph.x0 + 1) * (glyph.y1 - glyph.y0 + 1);
        for (unsigned char pixel : atlas)
            inkPixels += pixel != 0;
        std::printf("[font atlas] %s: %dx%d, %zu glyphs, packed %.1f%%, ink %.1f%%\n",
                    name, side, side, codepoints.size(),
                    100.0 * packedPixels / (side * side),
                    100.0 * inkPixels / (side * side));
    }
    return fits;
}

int main()
{
    const char *paths[TXL_LANG_COUNT] = {
        "assets/files/Roboto-Regular.ttf",
        "assets/files/NotoSansSC-Regular.ttf",
        "assets/files/NotoSans-Regular.ttf",
        "assets/files/NotoSansJP-Regular.ttf",
    };
    const char *names[TXL_LANG_COUNT] = {"English", "Chinese", "Lithuanian", "Japanese"};
    bool okay = true;
    for (int i = 0; i < TXL_LANG_COUNT; ++i) {
        const TxlLanguage language = (TxlLanguage)i;
        const std::string chars = std::string(k_txl_shared_chars) +
                                  Txl_CharsForLanguage(language) +
                                  Story_AllCharsForLanguage(language);
        if (chars.size() >= 8192) {
            std::fprintf(stderr, "%s: %zu bytes exceed Clayton's character buffer\n",
                         names[i], chars.size());
            okay = false;
            continue;
        }
        const int side = (language == TXL_LANG_ZH_CN || language == TXL_LANG_JP_JP) ? 1024 : 512;
        okay = CheckAtlas(names[i], paths[i], chars, side, true) && okay;
        const std::string monoName = std::string(names[i]) + " mono";
        okay = CheckAtlas(monoName.c_str(), "assets/files/RobotoMono-Regular.ttf",
                          k_txl_shared_chars, side, true, 48.0f) && okay;
    }
    for (int side : {512, 1024}) {
        const std::string name = std::string("Shared symbols ") + std::to_string(side);
        okay = CheckAtlas(name.c_str(), "assets/files/NotoSansJP-Regular.ttf",
                          "◀▶▼▲✓+-%÷", side, false) && okay;
    }
    if (okay) std::puts("All requested translation glyphs fit their font atlases.");
    return okay ? 0 : 1;
}
