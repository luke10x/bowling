// Small command line entry point for the reusable C++ Furnace encoder.
#include "../tracker/xfm_furnace_export.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>

int main(int argc, char** argv) {
    float master_volume = 1.0f;
    int first_path = 1;
    if (argc >= 2 && std::string(argv[1]) == "--player-match") {
        master_volume = 0.25f;
        first_path = 2;
    } else if (argc >= 3 && std::string(argv[1]) == "--master-volume") {
        const std::string level = argv[2];
        if (level == "1.0") master_volume = 1.0f;
        else if (level == "0.5") master_volume = 0.5f;
        else if (level == "0.25") master_volume = 0.25f;
        else {
            std::cerr << "master volume must be 1.0, 0.5, or 0.25\n";
            return 2;
        }
        first_path = 3;
    }
    if (argc != first_path + 2) {
        std::cerr << "usage: xfm_to_fur [--master-volume 1.0|0.5|0.25] input.h output.fur\n";
        return 2;
    }
    const char* source_path = argv[first_path];
    const char* output_path = argv[first_path + 1];
    try {
        std::ifstream input(source_path);
        if (!input) throw std::runtime_error("cannot open input");
        std::string source((std::istreambuf_iterator<char>(input)), {});
        auto result = xfm_export_furnace_from_header(source, {master_volume});
        std::ofstream output(output_path, std::ios::binary);
        if (!output) throw std::runtime_error("cannot open output");
        output.write(reinterpret_cast<const char*>(result.data.data()), result.data.size());
        if (!output) throw std::runtime_error("failed writing output");
        std::cout << source_path << " -> " << output_path << " ("
                  << result.source_rows << " rows, " << result.instrument_count
                  << " instruments, " << result.data.size() << " bytes)\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << source_path << ": " << error.what() << "\n";
        return 1;
    }
}
