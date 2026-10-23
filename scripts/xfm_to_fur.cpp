// Small command line entry point for the reusable C++ Furnace encoder.
#include "../tracker/xfm_furnace_export.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: xfm_to_fur input.h output.fur\n";
        return 2;
    }
    try {
        std::ifstream input(argv[1]);
        if (!input) throw std::runtime_error("cannot open input");
        std::string source((std::istreambuf_iterator<char>(input)), {});
        auto result = xfm_export_furnace_from_header(source);
        std::ofstream output(argv[2], std::ios::binary);
        if (!output) throw std::runtime_error("cannot open output");
        output.write(reinterpret_cast<const char*>(result.data.data()), result.data.size());
        if (!output) throw std::runtime_error("failed writing output");
        std::cout << argv[1] << " -> " << argv[2] << " ("
                  << result.source_rows << " rows, " << result.instrument_count
                  << " instruments, " << result.data.size() << " bytes)\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << argv[1] << ": " << error.what() << "\n";
        return 1;
    }
}
