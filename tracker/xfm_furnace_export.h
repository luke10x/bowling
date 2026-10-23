#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// Encodes one XFM song header as an uncompressed Furnace 244 YM3438 module.
// Volume inheritance is measured by playing the song through eggsfm's C++
// song engine. Throws std::exception for unsupported or malformed input.
struct XfmFurnaceExportResult {
    std::vector<uint8_t> data;
    std::size_t source_rows = 0;
    std::size_t instrument_count = 0;
};

struct XfmFurnaceExportOptions {
    // Furnace master volume. Its YM3438 dispatch also has a 2x post amplifier;
    // 0.25 is the nominal match for the game's default 0.5 music volume.
    float master_volume = 1.0f;
};

XfmFurnaceExportResult xfm_export_furnace_from_header(
    const std::string& source, XfmFurnaceExportOptions options = {});
