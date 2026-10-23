#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// Encodes one XFM song header as an uncompressed Furnace 244 YM2612 module.
// Volume inheritance is measured by playing the song through eggsfm's C++
// song engine. Throws std::exception for unsupported or malformed input.
struct XfmFurnaceExportResult {
    std::vector<uint8_t> data;
    std::size_t source_rows = 0;
    std::size_t instrument_count = 0;
};

XfmFurnaceExportResult xfm_export_furnace_from_header(const std::string& source);
